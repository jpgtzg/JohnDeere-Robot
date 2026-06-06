#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <stdint.h>

#define STM32_BUTTON (GPIOC->IDR & (0x1UL << 13U))
#define EXT_BUTTON   (GPIOC->IDR & (0x1UL << 0U))

extern volatile uint16_t adc_value;

void EXT_Button_Init(void);
void ADC1_GPIO_Init(void);
void ADC1_Init(void);

#endif
