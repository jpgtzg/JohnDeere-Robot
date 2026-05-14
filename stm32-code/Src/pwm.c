#include "pwm.h"
#include "ports.h"

void PWM_GPIO_Init(void) {
  RCC->APB2ENR |= (0x1UL << 2U); // IO port A clock enable

  // Configure PA1 as alternate function push-pull (CNF=10, MODE=11)
  GPIOA->CRL &= ~(0x1UL << 6U);                // clear CNF0[1]
  GPIOA->CRL |= (0x2UL << 6U) | (0x3UL << 4U); // CNF0=10, MODE0=11 (50 MHz)
}

// ===============================================================================
// TIM2 channel 1 PWM configuration (PWM Mode 1, active high, ARR=63999, 25% DC)
// ===============================================================================

void TIM2_PWM_Init(void) {
  RCC->APB1ENR |= (0x1UL << 0U); // TIM2 clock enable

  TIM2->SMCR &= ~(0x7UL << 0U); // internal clock source
  TIM2->CR1 &= ~(0x3UL << 5U) & ~(0x1UL << 4U) & ~(0x1UL << 1U) &
               ~(0x1UL << 2U); // edge-upcounter
  TIM2->CR1 |= (0x1UL << 7U);  // ARR preload (load only on UEV)

  TIM2->PSC = 0;
  TIM2->ARR = 63999;
  TIM2->CCR2 = 16000;              // 25% duty cycle (16000/64000)

  TIM2->CCMR1 &= ~(0x3UL << 8U);  // CC2S=00 (output)
  TIM2->CCMR1 &= ~(0x7UL << 12U); // clear OC2M
  TIM2->CCMR1 |=  (0x6UL << 12U); // OC2M=110 (PWM Mode 1)
  TIM2->CCMR1 |=  (0x1UL << 11U); // OC2PE: CCR2 preload enable

  TIM2->CCER &= ~(0x1UL << 5U);   // CC2P=0 (active high)
  TIM2->CCER |=  (0x1UL << 4U);   // CC2E: enable capture/compare channel 2

  TIM2->EGR |= (0x1UL << 0U); // UG: load prescaler immediately
  TIM2->SR &= ~(0x1UL << 0U); // clear update flag

  TIM2->CR1 |= (0x1UL << 0U); // start timer
}

void Change_Duty_Cycle(uint8_t duty) {
  if (duty > 100)
    duty = 100;
  TIM2->CR1 &= ~(0x1UL << 0U); // stop timer
  TIM2->CCR2 = (TIM2->ARR + 1) * duty / 100;
  TIM2->CR1 |= (0x1UL << 0U); // start timer
}
