#ifndef _CONFIG_H
#define _CONFIG_H

#define M_PI    3.141592f

#define BYTES_IN_LINE   32 //This define length of visble line in bytes (*8 to get pixels in line)
#define DEFAULT_PASE_SHIFT   12

#define SPI_DIV   SPI_BaudRatePrescaler_32

#define LASER_DMA_CHANNEL               DMA1_Channel3 //SPI1_TX
#define LASER_DMA_TE_FLAG               DMA1_IT_TE3 //transfr error

#define BLDC_PIN_A                      GPIO_Pin_0//tim3_ch3
#define BLDC_PIN_B                      GPIO_Pin_1//tim3_ch4
#define BLDC_PORT                       GPIOB

#define POLY_MOTOR_PWM_PIN              GPIO_Pin_6//tim4_ch1
#define POLY_MOTOR_PWM_PORT             GPIOB

#define ENCODER_PIN                     GPIO_Pin_0//tim2_ch1
#define ENCODER_PORT                    GPIOA

#define LASER_PIN                       GPIO_Pin_7 //SPI1_mosi
#define LASER_PORT                      GPIOA

#define BLDC_PWM_TIMER_NAME             TIM3
#define BLDC_PWM_TIMER_PRESCALER        (0)//SystemCoreClock
#define BLDC_PWM_TIMER_FREQ             (20000)//Frequency of PWM - in Hz
#define BLDC_PWM_TIMER_PERIOD           (SystemCoreClock / BLDC_PWM_TIMER_FREQ)//Period of PWM timer

#define BLDC_PWM_OFFSET                 (500)
#define BLDC_PWM_MIN_VALUE              (BLDC_PWM_OFFSET)
#define BLDC_PWM_MAX_VALUE              (BLDC_PWM_OFFSET + 170)

#define BLDC_STEP_COUNT                 32//Number of timer steps in vertical direction

#define BLDC_STEP_TIMER_NAME            TIM6
#define BLDC_STEP_TIMER_PRESCALER       ((SystemCoreClock / 1000000)-1)//1 Mhz timer frequency
#define BLDC_STEP_TIMER_IRQn            TIM6_DAC_IRQn
#define BLDC_STEP_TIMER_IRQ_Handler     TIM6_DAC_IRQHandler

//Polygonal mirror - used to generate CLK for polygon mirror motor
#define POLY_TIMER_NAME                 TIM4
#define POLY_TIMER_PRESCALER            ((SystemCoreClock / 400000)-1)//400 kHz timer frequency
#define POLY_TIMER_MIN_PERIOD           40000

//Used to count encoder pulses time
#define ENCODER_TIMER_NAME              TIM2
#define ENCODER_TIMER_FREQ              24000000
#define ENCODER_TIMER_PRESCALER         ((SystemCoreClock / ENCODER_TIMER_FREQ)-1)
#define ENCODER_TIMER_PERIOD            0xFFFF
#define ENCODER_TIMER_IRQn              TIM2_IRQn
#define ENCODER_IRQ_Handler             TIM2_IRQHandler

#define ENCODER_FILTER_LENGHT           32



#endif