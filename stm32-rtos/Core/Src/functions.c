#include "functions.h"
#include "main.h"

void EXT_Button_Init(void) {
  RCC->APB2ENR |= (0x1UL << 4U);  // GPIOC clock enable

  // PC0 as input with pull-down (CNF=10, MODE=00)
  GPIOC->CRL &= ~(0xFUL << 0U);
  GPIOC->CRL |=  (0x8UL << 0U);
  GPIOC->ODR &= ~(0x1UL << 0U);   // pull-down via ODR (0 = pull-down)
}

void ADC1_GPIO_Init(void) {
  RCC->APB2ENR |= (0x1UL << 2U);  // GPIOA clock enable
  GPIOA->CRL   &= ~(0xFUL << 0U); // PA0 as analog input (CNF=00, MODE=00)
}

void ADC1_Init(void) {
  RCC->APB2ENR |= (0x1UL << 9U);  // ADC1 clock enable
  RCC->CFGR    |= (0x3UL << 14U); // ADC prescaler /8 (64 MHz → 8 MHz)

  ADC1->CR1 &= ~(0xFUL << 16U);   // independent mode
  ADC1->CR1 &= ~(0x1UL <<  5U);   // EOC interrupt disabled — polled

  ADC1->CR2 &= ~(0x1UL << 11U);   // right alignment
  ADC1->CR2 &= ~(0x1UL <<  1U);   // single conversion mode
  ADC1->CR2 |=  (0x7UL << 17U);   // EXTSEL = 111: SWSTART trigger
  ADC1->CR2 |=  (0x1UL << 20U);   // EXTTRIG enable

  ADC1->SMPR2 &= ~(0x7UL <<  0U); // channel 0 sample time: 1.5 cycles
  ADC1->SQR1  &= ~(0xFUL << 20U); // 1 conversion in regular sequence
  ADC1->SQR3  &= ~(0x1FUL << 0U); // channel 0 first in sequence

  ADC1->CR2 |= (0x1UL << 0U);     // enable ADC1
  ADC1->CR2 |= (0x1UL << 2U);     // start calibration
  while (ADC1->CR2 & (0x1UL << 2U)); // wait for calibration
}
