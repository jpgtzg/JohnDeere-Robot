#include "pwm.h"
#include "ports.h"

void PWM_GPIO_Init(void) {
  RCC->APB2ENR |= (0x1UL << 2U); // IO port A clock enable

  // Configure PA0 as alternate function push-pull (CNF=10, MODE=11)
  GPIOA->CRL &= ~(0x1UL << 2U);                        // clear CNF0[1]
  GPIOA->CRL |= (0x2UL << 2U) | (0x3UL << 0U);         // CNF0=10, MODE0=11 (50 MHz)
}

// ===============================================================================
// TIM2 channel 1 PWM configuration (PWM Mode 1, active high, ARR=63999, 25% DC)
// ===============================================================================

void TIM2_PWM_Init(void) {
  RCC->APB1ENR |= (0x1UL << 0U); // TIM2 clock enable

  TIM2->SMCR &= ~(0x7UL << 0U);  // internal clock source
  TIM2->CR1 &= ~(0x3UL << 5U) & ~(0x1UL << 4U) & ~(0x1UL << 1U) & ~(0x1UL << 2U); // edge-upcounter
  TIM2->CR1 |= (0x1UL << 7U);    // ARR preload (load only on UEV)

  TIM2->PSC  = 0;
  TIM2->ARR  = 63999;
  TIM2->CCR1 = 16000;             // 25% duty cycle (16000/64000)

  TIM2->CCMR1 &= ~(0x3UL << 0U); // CC1S=00 (output)
  TIM2->CCMR1 &= ~(0x1UL << 4U); // clear OC1M[0]
  TIM2->CCMR1 |=  (0x6UL << 4U); // OC1M=110 (PWM Mode 1)
  TIM2->CCMR1 |=  (0x1UL << 3U); // OC1PE: CCR1 preload enable

  TIM2->CCER  &= ~(0x1UL << 1U); // CC1P=0 (active high)
  TIM2->CCER  |=  (0x1UL << 0U); // CC1E: enable capture/compare channel 1

  TIM2->EGR   |=  (0x1UL << 0U); // UG: load prescaler immediately
  TIM2->SR    &= ~(0x1UL << 0U); // clear update flag

  TIM2->CR1   |=  (0x1UL << 0U); // start timer
}

void Change_Duty_Cycle(uint8_t duty) {
  if (duty > 100) duty = 100;
  TIM2->CR1  &= ~(0x1UL << 0U);                        // stop timer
  TIM2->CCR1  = (TIM2->ARR + 1) * duty / 100;
  TIM2->CR1  |=  (0x1UL << 0U);                        // start timer
}
