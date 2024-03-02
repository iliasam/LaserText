//This file contains functions that are used 
// to control polygon mirror and its encoder (used for synchronization)

#include "stm32f10x.h"
#include "poly_mirror.h"
#include "vertical_mirror.h"
#include "laser_controlling.h"
#include "lcd_worker.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


void poly_mirror_init_pwm_timer(void);
void poly_mirror_init_encoder_timer(void);
uint16_t filter_period(uint16_t new_value);
void do_line_scan_switch(void);

volatile uint16_t encoder_period = 0;
volatile uint16_t encoder_period_fast = 0;
volatile uint16_t interrupt_enter = 0;

uint8_t scan_pos = 0;
volatile uint8_t laser_blocked_on = 1;//1 - mean that laser must be turn on all time (no modulation)
uint8_t buffer_switch_request = 0;//Main software set this flag when buffer switching is need

extern volatile uint16_t verical_step;

//Interrupt - "input capture" from timer "ENCODER_TIMER_NAME" + interrupt from time
//syncro timer
void ENCODER_IRQ_Handler(void)
{
  uint16_t interrupt_time = ENCODER_TIMER_NAME->CNT;//Time the MCU entered to IRQ handler
  if (TIM_GetITStatus(ENCODER_TIMER_NAME, TIM_IT_CC1) == SET)//interrupt from encoder (timer input capture)
  {
    encoder_period_fast = TIM_GetCapture1(ENCODER_TIMER_NAME);//Captured (instant) time of photodiode event
    interrupt_enter = interrupt_time - encoder_period_fast;//Time to enter to this IRQ handler
    ENCODER_TIMER_NAME->CNT = interrupt_enter;//RESET timer with compensation of time to enter into this interrupt
    
    encoder_period = filter_period(encoder_period_fast);
    
    TIM_ClearITPendingBit(ENCODER_TIMER_NAME, TIM_IT_CC1);
    scan_pos = 0;
    
    if (encoder_period_fast > (SYSTEM_MIN_PERIOD * (ENCODER_TIMER_FREQ / 1000000)))//Laser must be turned on all time when rotation speed is low
      laser_blocked_on = 1;//disable modulation
    else
      laser_blocked_on = 0;//enable modulation
    
    do_line_scan_switch();
    //LED_PORT->ODR^= LED_PIN;//debugging
  }
  
  if (TIM_GetITStatus(ENCODER_TIMER_NAME, TIM_IT_CC2) == SET)//timer compare interrupt
  {
    TIM_ClearITPendingBit(ENCODER_TIMER_NAME, TIM_IT_CC2);
    do_line_scan_switch();
  }
}


//State machine that is switched by encoder and syncro timer interrupts
//This function update compare register of syncro timer
void do_line_scan_switch(void)
{
  if (laser_blocked_on == 1)
  {
    LASER_PORT->ODR|= LASER_PIN;//always on
    laser_turn_on();
    return;
  }
  
  switch (scan_pos)
  {
    case 0: //encoder event
    {
      uint16_t new_time = (uint16_t)(encoder_period_fast * 2 / 100);
      TIM_SetCompare2(ENCODER_TIMER_NAME, new_time);
      scan_pos++;
      break;
    }
    
    case 1: //time 2% - disable laser
    {
      uint16_t new_time = (uint16_t)(encoder_period_fast * 6 / 100);
      TIM_SetCompare2(ENCODER_TIMER_NAME, new_time);
      scan_pos++;
      laser_turn_off();
      break;
    }
    
    case 2: //time 6% - enable laser - start of visible line
    {
      uint16_t new_time = (uint16_t)(encoder_period_fast * 50 / 100);
      TIM_SetCompare2(ENCODER_TIMER_NAME, new_time);
      scan_pos++;
      laser_dma_start();
      break;
    }
    
    case 3: //time 50% - disable laser end of visible line
    {
      uint16_t new_time = (uint16_t)(encoder_period_fast * 65 / 100);
      TIM_SetCompare2(ENCODER_TIMER_NAME, new_time);
      scan_pos++;
      laser_dma_stop();
      vertical_mirror_new_step();
      prepare_laser_line(verical_step);
      if ((buffer_switch_request != 0) && (verical_step == 0))
      {
        switch_lcd_framebuffers();
        buffer_switch_request = 0;
      }
      
      break;
    }
    
    case 4: //time 65% - enable laser for new sync
    {
      uint16_t new_time = (uint16_t)(encoder_period_fast * 110 / 100);//no other events after this one
      TIM_SetCompare2(ENCODER_TIMER_NAME, new_time);
      scan_pos++;
      laser_turn_on();//laser must be turned on to be detected by photodiode in future
      break;
    }
    
    default: break;
  }
}

void poly_mirror_init_hardware(void)
{
  poly_mirror_init_pwm_timer();
  poly_mirror_init_encoder_timer();
}

//Init timer - capture input mode - used to calculate rotation speed and start pulses
void poly_mirror_init_encoder_timer(void)
{
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_ICInitTypeDef  TIM_ICInitStructure;
  TIM_OCInitTypeDef  TIM_OCInitStructure;
  GPIO_InitTypeDef  GPIO_InitStructure;
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
  //RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
  
  GPIO_InitStructure.GPIO_Pin   = ENCODER_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
  GPIO_Init(ENCODER_PORT, &GPIO_InitStructure);
  
  TIM_DeInit(ENCODER_TIMER_NAME);
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
  TIM_TimeBaseStructure.TIM_Period = ENCODER_TIMER_PERIOD;
  TIM_TimeBaseStructure.TIM_Prescaler = ENCODER_TIMER_PRESCALER;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(ENCODER_TIMER_NAME, &TIM_TimeBaseStructure);
  TIM_ARRPreloadConfig(ENCODER_TIMER_NAME, ENABLE);
  
  // channel1 - input capture for encoder
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
  TIM_ICInitStructure.TIM_ICFilter = 2;
  //TIM_ICInitStructure.TIM_ICFilter = 15;
  TIM_ICInit(ENCODER_TIMER_NAME, &TIM_ICInitStructure);
  
  // channel2 - used to generate interrupts during single line scanning
  TIM_OCStructInit(&TIM_OCInitStructure);
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;
  TIM_OCInitStructure.TIM_Pulse = 800;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OC2Init(ENCODER_TIMER_NAME, &TIM_OCInitStructure);
  
  TIM_ITConfig(ENCODER_TIMER_NAME, TIM_IT_CC1, ENABLE);
  NVIC_SetPriority(ENCODER_TIMER_IRQn, 3);
  NVIC_EnableIRQ(ENCODER_TIMER_IRQn);
  
  TIM_ITConfig(ENCODER_TIMER_NAME, TIM_IT_CC2, ENABLE);
  NVIC_SetPriority(ENCODER_TIMER_IRQn, 5);
  NVIC_EnableIRQ(ENCODER_TIMER_IRQn);
  
  TIM_CtrlPWMOutputs(ENCODER_TIMER_NAME, DISABLE);
  
  TIM_Cmd(ENCODER_TIMER_NAME, ENABLE);
}

//This timer is used to generate CLK for polygon mirror motor
void poly_mirror_init_pwm_timer(void)
{
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_OCInitTypeDef  TIM_OCInitStructure;
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
  
  GPIO_InitStructure.GPIO_Pin   = POLY_MOTOR_PWM_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
  GPIO_Init(POLY_MOTOR_PWM_PORT, &GPIO_InitStructure);
  
  TIM_DeInit(POLY_TIMER_NAME);
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
  TIM_TimeBaseStructure.TIM_Period = POLY_TIMER_MIN_PERIOD;
  TIM_TimeBaseStructure.TIM_Prescaler = POLY_TIMER_PRESCALER;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(POLY_TIMER_NAME, &TIM_TimeBaseStructure);
  TIM_ARRPreloadConfig(POLY_TIMER_NAME, ENABLE);
  
  TIM_OCStructInit(&TIM_OCInitStructure);
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = POLY_TIMER_MIN_PERIOD/2;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OC1Init(POLY_TIMER_NAME, &TIM_OCInitStructure);
  
  TIM_OC1PreloadConfig(POLY_TIMER_NAME, TIM_OCPreload_Enable);
  
  TIM_CtrlPWMOutputs(POLY_TIMER_NAME, ENABLE);
  
  TIM_Cmd(POLY_TIMER_NAME, ENABLE);
}

//Set new period of PWM for polygon mirror motor
void poly_mirror_set_pwm_period(uint16_t value)
{
  TIM_SetAutoreload(POLY_TIMER_NAME, value);
  TIM_SetCompare1(POLY_TIMER_NAME, (value / 2));
}

uint16_t filter_period(uint16_t new_value)
{
  static uint16_t filter_values[ENCODER_FILTER_LENGHT];
  static uint8_t filter_cnt = 0;
  uint32_t summ = 0;
  
  filter_cnt++;
  if (filter_cnt >= ENCODER_FILTER_LENGHT)
    filter_cnt = 0;
  
  filter_values[filter_cnt] = new_value;
  
  for (uint8_t i = 0; i < ENCODER_FILTER_LENGHT; i++)
    summ+= filter_values[i];
  
  return (uint16_t)(summ / ENCODER_FILTER_LENGHT);
}

