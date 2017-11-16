//This file contains functions that are used to control vertical mirror

#include "stm32f10x.h"
#include "vertical_mirror.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//#define FORM1

#define BLDC_PWM_D_VAL  (BLDC_PWM_MAX_VALUE - BLDC_PWM_MIN_VALUE)

//Must be initialized at start
uint16_t verical_step_table_a[BLDC_STEP_COUNT];
uint16_t verical_step_table_b[BLDC_STEP_COUNT];

volatile uint16_t verical_step = 0;
uint8_t phase_shift = DEFAULT_PASE_SHIFT;


void init_debug_dac(void);
void vertical_mirror_init_pwm_timer(void);
void vertical_mirror_init_step_tables(void);

void vertical_mirror_init_hardware(void)
{
  init_debug_dac();
  vertical_mirror_init_pwm_timer();
  vertical_mirror_init_step_tables();
}


void vertical_mirror_new_step(void)
{
  verical_step++;
  if (verical_step >= BLDC_STEP_COUNT)
    verical_step = 0;
  
  uint8_t new_pos = verical_step + phase_shift;
  if (new_pos>=BLDC_STEP_COUNT)
    new_pos = new_pos - BLDC_STEP_COUNT;
  
  vertical_mirror_set_pwm_a( verical_step_table_a[new_pos] );
  vertical_mirror_set_pwm_b( verical_step_table_b[new_pos] );
  
  DAC_SetChannel1Data(DAC_Align_12b_R, verical_step_table_a[new_pos]);
}



#ifdef FORM1

void vertical_mirror_init_step_tables(void)
{
  float A = 2.0f * (BLDC_PWM_MIN_VALUE - BLDC_PWM_MAX_VALUE) / (float)BLDC_STEP_COUNT;
  float B1 = (float)BLDC_PWM_MAX_VALUE;
  float B2 = (float)BLDC_PWM_MIN_VALUE + A * (float)BLDC_STEP_COUNT / 2.0f;
  
  uint8_t i;
  for (i=0; i<BLDC_STEP_COUNT; i++)
  {
    if (i < (BLDC_STEP_COUNT /2))
    {
      verical_step_table_a[i] = (uint16_t)(A*(float)i + B1);
      verical_step_table_b[i] = (uint16_t)(BLDC_PWM_MIN_VALUE + BLDC_PWM_MAX_VALUE - verical_step_table_a[i]);
    }
    else
    {
      verical_step_table_a[i] = (uint16_t)(-A*(float)i + B2);
      verical_step_table_b[i] = (uint16_t)(BLDC_PWM_MIN_VALUE + BLDC_PWM_MAX_VALUE - verical_step_table_a[i]);
    }
    
  }
}
#else

void vertical_mirror_init_step_tables(void)
{
  float A = 1.142f * (BLDC_PWM_MIN_VALUE - BLDC_PWM_MAX_VALUE) / (float)BLDC_STEP_COUNT;
  float B1 = (float)BLDC_PWM_MAX_VALUE;
  
  uint8_t i;
  for (i=0; i<BLDC_STEP_COUNT; i++)
  {
    if (i < (7*BLDC_STEP_COUNT / 8))
    {
      verical_step_table_a[i] = (uint16_t)(A*(float)i + B1);
      verical_step_table_b[i] = (uint16_t)(BLDC_PWM_MIN_VALUE + BLDC_PWM_MAX_VALUE - verical_step_table_a[i]);
    }
    else
    {
      verical_step_table_a[i] = (uint16_t)(BLDC_PWM_MAX_VALUE);
      verical_step_table_b[i] = (uint16_t)(BLDC_PWM_MIN_VALUE + BLDC_PWM_MAX_VALUE - verical_step_table_a[i]);
    }
    
  }
}



#endif



//Timer is used to controll vertical mirror
void vertical_mirror_init_pwm_timer(void)
{
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_OCInitTypeDef  TIM_OCInitStructure;
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
  
  GPIO_InitStructure.GPIO_Pin   = (BLDC_PIN_A | BLDC_PIN_B);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
  GPIO_Init(BLDC_PORT, &GPIO_InitStructure);
  
  TIM_DeInit(BLDC_PWM_TIMER_NAME);
  
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
  TIM_TimeBaseStructure.TIM_Period = BLDC_PWM_TIMER_PERIOD;
  TIM_TimeBaseStructure.TIM_Prescaler = BLDC_PWM_TIMER_PRESCALER;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(BLDC_PWM_TIMER_NAME, &TIM_TimeBaseStructure);

  TIM_OCStructInit(&TIM_OCInitStructure);
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = 500;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
   
  TIM_OC3Init(BLDC_PWM_TIMER_NAME, &TIM_OCInitStructure);
  
  TIM_OCInitStructure.TIM_Pulse = 1000;
  TIM_OC4Init(BLDC_PWM_TIMER_NAME, &TIM_OCInitStructure);
  
  TIM_OC3PreloadConfig(BLDC_PWM_TIMER_NAME, TIM_OCPreload_Enable);
  TIM_OC4PreloadConfig(BLDC_PWM_TIMER_NAME, TIM_OCPreload_Enable);
  
  TIM_ARRPreloadConfig(BLDC_PWM_TIMER_NAME, ENABLE);
  TIM_Cmd(BLDC_PWM_TIMER_NAME, ENABLE);
}

void vertical_mirror_set_pwm_a(uint16_t value)
{
  TIM_SetCompare3(BLDC_PWM_TIMER_NAME, value);
}

void vertical_mirror_set_pwm_b(uint16_t value)
{
  TIM_SetCompare4(BLDC_PWM_TIMER_NAME, value);
}

void init_debug_dac(void)
{
  DAC_InitTypeDef  DAC_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  uint16_t tmp=0;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA, ENABLE);
   
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  DAC_InitStructure.DAC_Trigger = DAC_Trigger_None;
  DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
  DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
  DAC_Init(DAC_Channel_1, &DAC_InitStructure);
  DAC_Cmd(DAC_Channel_1, ENABLE);
  DAC_SetChannel1Data(DAC_Align_12b_R, tmp);
  
  DAC_Cmd(DAC_Channel_2, DISABLE);
}
