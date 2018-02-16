//Special framebuffer wrapper used for basic operations - text drawing
//This file is taken from my another project that worked with LCD
#include "lcd_worker.h"
#include "main.h"

uint16_t lcd_cursor_text_x = 0;
uint16_t lcd_cursor_text_y = 0;

void lcd_draw_char_size8(uint8_t chr, uint16_t x_start, uint16_t y_start, uint8_t flags);
void lcd_draw_char_size6(uint8_t chr, uint16_t x_start, uint16_t y_start, uint8_t flags);
void lcd_draw_char_size11(uint8_t chr, uint16_t x_start, uint16_t y_start, uint8_t flags);

//Data saved horizontally
//Data storage map - x7-x0 -- x15-x8
uint8_t lcd_framebuffer0[LCD_WIDTH*LCD_HEIGHT / 8];
uint8_t lcd_framebuffer1[LCD_WIDTH*LCD_HEIGHT / 8];

uint8_t* front_framebuffer_p = lcd_framebuffer0; //front framebuffer pinter
uint8_t* shadow_framebuffer_p = lcd_framebuffer1;//shadow framebuffer pinter

uint8_t buffer_switch_flag = 0;//This flag is set after switching buffers

//Set pixel at shadow_framebuffer
void lcd_set_pixel(uint16_t x, uint16_t y)
{
  uint16_t loc_x = x + LCD_LEFT_OFFSET;
  if (loc_x >= LCD_WIDTH)
    return;
  if (loc_x >= LCD_RIGHT_OFFSET)
    return;
  
  uint32_t byte_pos = y * (LCD_WIDTH / 8) + loc_x / 8;//8 - bit width
  uint8_t mask = 1 << (7-(x % 8));//horiz
  shadow_framebuffer_p[byte_pos]|= mask;
}

//Clean pixel at shadow_framebuffer
void lcd_reset_pixel(uint16_t x, uint16_t y)
{
  uint16_t loc_x = x + LCD_LEFT_OFFSET;
  if (loc_x >= LCD_WIDTH)
    return;
  if (loc_x >= LCD_RIGHT_OFFSET)
    return;
  
  uint32_t byte_pos = y * (LCD_WIDTH / 8) + loc_x / 8;//8 - bit width
  uint8_t mask = 1 << (7-(x % 8));//horiz
  shadow_framebuffer_p[byte_pos]&= ~mask;
}

//Switching pointers to framebuffers
void switch_lcd_framebuffers(void)
{
  uint8_t* tmp_buf = shadow_framebuffer_p;
  shadow_framebuffer_p = front_framebuffer_p;
  front_framebuffer_p = tmp_buf;
  buffer_switch_flag = 1;
}


//Clear shadow framebuffer
void lcd_clear_framebuffer(void)
{
  memset(shadow_framebuffer_p, 0, (LCD_WIDTH*LCD_HEIGHT / 8));
  lcd_cursor_text_x = 0;
  lcd_cursor_text_y = 0;
}

void lcd_set_cursor_pos(uint16_t x, uint16_t y)
{
  lcd_cursor_text_x = x;
  lcd_cursor_text_y = y;
}

//x, y - in pixel
//return string width
//String end is 0x00 char
uint16_t lcd_draw_string(char *s, uint16_t x, uint16_t y, uint8_t font_size, uint8_t flags)
{
  uint16_t width = get_font_width(font_size);
  uint8_t chr_pos = 0;
  char chr = *s;
  
  while (chr && (chr_pos < 50)) 
  {
    lcd_draw_char(chr, x + chr_pos*width, y, font_size, flags);
    chr_pos++;
    chr = s[chr_pos];
  }
  lcd_cursor_text_x = x + chr_pos*width;
  lcd_cursor_text_y = y;
  
  return chr_pos*width;
}

//Draw text at current cursor position
//String end is 0x00 char
uint16_t lcd_draw_string_cur(char *s, uint8_t font_size, uint8_t flags)
{
  uint16_t length = lcd_draw_string(s, lcd_cursor_text_x, lcd_cursor_text_y, font_size, flags);
  lcd_cursor_text_x+= length;
  if ((flags & LCD_NEW_LINE_FLAG) != 0)
  {
    lcd_cursor_text_y+= font_size;
    lcd_cursor_text_x = 0;
  }
    
  return length;
}

//x - size in char (not in pixel)
//y - in pixel
void lcd_draw_char(uint8_t chr, uint16_t x, uint16_t y, uint8_t font_size, uint8_t flags)
{
  switch (font_size)
  {
    case FONT_SIZE_8:
    {
      lcd_draw_char_size8(chr, x, y, flags);
      break;
    }
    case FONT_SIZE_6:
    {
      lcd_draw_char_size6(chr, x, y, flags);
      break;
    }
    case FONT_SIZE_11:
    {
      lcd_draw_char_size11(chr, x, y, flags);
      break;
    }
  }
}

//x, y - size in pixel
void lcd_draw_char_size8(uint8_t chr, uint16_t x_start, uint16_t y_start, uint8_t flags)
{
  uint16_t x_pos, y_pos;
  
  //decoding symbol
  if (chr >= 32 && chr <= '~')
  {
    chr = chr - 32;
  } 
  else
  {
    if (chr >= 192)
      chr = chr - 97;
    else
    {
        return;
    }
  }
  
  for (x_pos = 0; x_pos < (FONT_SIZE_8_WIDTH); x_pos++)
  {
    for (y_pos = 0; y_pos < FONT_SIZE_8; y_pos++)
    {
      uint8_t pixel = lcd_font_size8[chr][x_pos] & (1<<y_pos);
      if (x_pos == (FONT_SIZE_8_WIDTH-1))
        pixel = 0;
      
      if (flags & LCD_INVERTED_FLAG) 
        if (pixel) pixel = 0; else pixel = 1;
      
      if (pixel) 
        lcd_set_pixel(x_start + x_pos, y_start + y_pos);
      else
        lcd_reset_pixel(x_start + x_pos, y_start + y_pos);
    }
  }
}

void lcd_draw_char_size6(uint8_t chr, uint16_t x_start, uint16_t y_start, uint8_t flags)
{
  uint16_t x_pos, y_pos;
  
  for (x_pos = 0; x_pos < (FONT_SIZE_6_WIDTH); x_pos++)
  {
    for (y_pos = 0; y_pos < FONT_SIZE_6; y_pos++)
    {
      uint8_t pixel = lcd_font_size6[chr][y_pos] & (1<<(3-x_pos));
      
      if (flags & LCD_INVERTED_FLAG) 
        if (pixel) pixel = 0; else pixel = 1;
      
      if (pixel) 
        lcd_set_pixel(x_start + x_pos, y_start + y_pos);
      else
        lcd_reset_pixel(x_start + x_pos, y_start + y_pos);
    }
  }
}

//x, y - size in pixel
void lcd_draw_char_size11(uint8_t chr, uint16_t x_start, uint16_t y_start, uint8_t flags)
{
  uint16_t x_pos, y_pos;
  
  //decoding symbol
  if (chr >= 32 && chr <= 128)
  {
    chr = chr - 32;
  } 
  
  for (x_pos = 0; x_pos < (FONT_SIZE_11_WIDTH - 1); x_pos++)
  {
    for (y_pos = 0; y_pos < (FONT_SIZE_11-1); y_pos++)
    {
      if (lcd_font_size11[chr][y_pos] & (1<<(x_pos))) 
        lcd_set_pixel(x_start + x_pos, y_start + y_pos);
      else
        lcd_reset_pixel(x_start + x_pos, y_start + y_pos);
    }
  }
}

uint16_t get_font_width(uint8_t font)
{
  switch (font)
  {
    case FONT_SIZE_6:  return FONT_SIZE_6_WIDTH;
    case FONT_SIZE_8:  return FONT_SIZE_8_WIDTH;
    case FONT_SIZE_11: return FONT_SIZE_11_WIDTH;
    default: return 5;
  }
}