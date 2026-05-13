#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <stdint.h>

#define STM32_BUTTON (GPIOC->IDR & (0x1UL << 13U))

extern volatile uint16_t adc_value;

void SystemClock_Config(void);
void Delay_10ms_CPU(void);
void Delay_1sec_CPU(void);
void STM32_Button_Init(void);
void ADC1_GPIO_Init(void);
void ADC1_Init(void);
void ADC1_2_IRQHandler(void);

#endif
