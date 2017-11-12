#include "laser_controlling.h"
#include "stm32f10x.h"
#include "config.h"
#include "string.h"
#include "lcd_worker.h"

//#define TEST_MODE

uint8_t laser_line_data[BYTES_IN_LINE];

//volatile uint8_t line_conv_table[BLDC_STEP_COUNT] = {0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,0,0,0,0,0,0,0,0,0,0};
volatile uint8_t line_conv_table[BLDC_STEP_COUNT] = {0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,0,0,0,0};

extern uint8_t* front_framebuffer_p;//front framwbuffer pinter

void init_laser_spi(void);
void init_laser_dma(void);
void copy_data_to_framebuffer(uint8_t line_number);


void init_laser_controlling(void)
{
  init_laser_spi();
  init_laser_dma();
}

void init_laser_spi(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
  
  GPIO_InitStructure.GPIO_Pin = LASER_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(LASER_PORT, &GPIO_InitStructure);
  
  SPI_InitTypeDef    SPI_InitStructure;
  
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;//clock polarity - idle low
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;//by 2nd edge
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_DIV;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_Init(SPI1, &SPI_InitStructure);
  
  SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
  
  SPI_Cmd(SPI1, ENABLE);
}

//DMA is used to send new data to SPI
void init_laser_dma(void)
{
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  DMA_InitTypeDef           DMA_InitStructure;
  
  DMA_DeInit(LASER_DMA_CHANNEL);
  DMA_InitStructure.DMA_PeripheralBaseAddr      = (uint32_t)&SPI1->DR;
  DMA_InitStructure.DMA_MemoryBaseAddr          = (uint32_t)&laser_line_data;
  DMA_InitStructure.DMA_DIR                     = DMA_DIR_PeripheralDST;
  DMA_InitStructure.DMA_BufferSize              = BYTES_IN_LINE;
  DMA_InitStructure.DMA_PeripheralInc           = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc               = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize      = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize          = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode                    = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority                = DMA_Priority_VeryHigh;
  DMA_InitStructure.DMA_M2M                     = DMA_M2M_Disable;
  
  DMA_Init(LASER_DMA_CHANNEL, &DMA_InitStructure);
  
  //NVIC_SetPriority(DMA1_Channel3_IRQn, 15);
  //NVIC_EnableIRQ(DMA1_Channel3_IRQn);
  
  //LASER_DMA_CHANNEL->CCR|= DMA_CCR1_EN;//START DMA
}

#ifdef TEST_MODE

void prepare_laser_line(uint8_t line_number)
{
  uint8_t i;
  
  if (line_conv_table[line_number] == 0)
    memset(&laser_line_data, 0, sizeof(laser_line_data));
  else
  {
    uint8_t table_line_pos = line_conv_table[line_number];
    
    for (i=0; i<BYTES_IN_LINE; i++)
    {
      if (i<table_line_pos)
        laser_line_data[i] = 255;
      else
        laser_line_data[i] = 0;
    }
  }
}

#else

void prepare_laser_line(uint8_t line_number)
{
  //memset(&laser_line_data, 0, sizeof(laser_line_data));
  if (line_conv_table[line_number] == 0)
    return;
  else
  {
    uint8_t table_line_pos = line_conv_table[line_number] - 1;
    copy_data_to_framebuffer(table_line_pos);
  }
}

#endif

void copy_data_to_framebuffer(uint8_t line_number)
{
  if (line_number >= LCD_HEIGHT)
    return;
  
  uint16_t framebuf_start = line_number * (LCD_WIDTH / 8);
  memcpy(laser_line_data, &front_framebuffer_p[framebuf_start], (LCD_WIDTH / 8));
}

void laser_dma_start(void)
{ 
   DMA_Cmd(LASER_DMA_CHANNEL, DISABLE);
   LASER_DMA_CHANNEL->CMAR = (uint32_t)&laser_line_data;
   LASER_DMA_CHANNEL->CNDTR = BYTES_IN_LINE;
   DMA_ClearITPendingBit(LASER_DMA_TE_FLAG);
   SPI_I2S_ClearITPendingBit(SPI1, SPI_I2S_FLAG_OVR);//overrun
   DMA_Cmd(LASER_DMA_CHANNEL, ENABLE);
}

void  laser_dma_stop(void)
{
  DMA_Cmd(LASER_DMA_CHANNEL, DISABLE);
  laser_turn_off();
}


//Manually turn on laser
void laser_turn_on(void)
{
  SPI1->DR = (uint16_t)0xFF;
}

//Manually turn off laser
void laser_turn_off(void)
{
  SPI1->DR = (uint16_t)0x00;
}