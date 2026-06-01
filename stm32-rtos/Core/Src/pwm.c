#include "pwm.h"
#include "main.h"

void PWM_GPIO_Init(void) {
  RCC->APB2ENR |= (0x1UL << 2U); // IO port A clock enable
  RCC->APB2ENR |= (0x1UL << 3U); // IO port B clock enable
  RCC->APB2ENR |= (0x1UL << 0U); // AFIO clock enable

  // PA1: TIM2_CH2, alternate function push-pull (CNF=10, MODE=11)
  GPIOA->CRL &= ~(0xFUL << 4U);
  GPIOA->CRL |=  (0xBUL << 4U);

  // PB8: TIM4_CH3, PB9: TIM4_CH4, PB10: TIM2_CH3 — AF push-pull (CNF=10, MODE=11)
  GPIOB->CRH &= ~(0xFFFUL << 0U);
  GPIOB->CRH |=  (0xBUL << 0U)   // PB8
              |  (0xBUL << 4U)    // PB9
              |  (0xBUL << 8U);   // PB10

  // TIM2 partial remap 2: CH3=PB10, CH4=PB11 (CH1=PA0, CH2=PA1 unchanged)
  AFIO->MAPR &= ~(0x3UL << 8U);
  AFIO->MAPR |=  (0x2UL << 8U);
}

// TIM2: CH2=PA1, CH3=PB10 — ARR=63999 → 1 kHz at 64 MHz
void TIM2_PWM_Init(void) {
  RCC->APB1ENR |= (0x1UL << 0U); // TIM2 clock enable

  TIM2->SMCR &= ~(0x7UL << 0U);  // internal clock source
  TIM2->CR1  &= ~(0x3UL << 5U) & ~(0x1UL << 4U) & ~(0x1UL << 1U) & ~(0x1UL << 2U);
  TIM2->CR1  |=  (0x1UL << 7U);  // ARR preload

  TIM2->PSC = 0;
  TIM2->ARR = 63999;

  // CH2 (PA1)
  TIM2->CCR2    = 0;
  TIM2->CCMR1  &= ~(0x3UL << 8U);
  TIM2->CCMR1  &= ~(0x7UL << 12U);
  TIM2->CCMR1  |=  (0x6UL << 12U); // PWM Mode 1
  TIM2->CCMR1  |=  (0x1UL << 11U); // OC2PE
  TIM2->CCER   &= ~(0x1UL << 5U);  // CC2P=0 active high
  TIM2->CCER   |=  (0x1UL << 4U);  // CC2E

  // CH3 (PB10)
  TIM2->CCR3    = 0;
  TIM2->CCMR2  &= ~(0x3UL << 0U);
  TIM2->CCMR2  &= ~(0x7UL << 4U);
  TIM2->CCMR2  |=  (0x6UL << 4U);  // PWM Mode 1
  TIM2->CCMR2  |=  (0x1UL << 3U);  // OC3PE
  TIM2->CCER   &= ~(0x1UL << 9U);  // CC3P=0 active high
  TIM2->CCER   |=  (0x1UL << 8U);  // CC3E

  TIM2->EGR |= (0x1UL << 0U);  // load prescaler
  TIM2->SR  &= ~(0x1UL << 0U); // clear update flag
  TIM2->CR1 |=  (0x1UL << 0U); // start timer
}

// TIM4: CH3=PB8, CH4=PB9 — ARR=63999 → 1 kHz at 64 MHz
void TIM4_PWM_Init(void) {
  RCC->APB1ENR |= (0x1UL << 2U); // TIM4 clock enable

  TIM4->SMCR &= ~(0x7UL << 0U);
  TIM4->CR1  &= ~(0x3UL << 5U) & ~(0x1UL << 4U) & ~(0x1UL << 1U) & ~(0x1UL << 2U);
  TIM4->CR1  |=  (0x1UL << 7U);

  TIM4->PSC = 0;
  TIM4->ARR = 63999;

  // CH3 (PB8)
  TIM4->CCR3    = 0;
  TIM4->CCMR2  &= ~(0x3UL << 0U);
  TIM4->CCMR2  &= ~(0x7UL << 4U);
  TIM4->CCMR2  |=  (0x6UL << 4U);  // PWM Mode 1
  TIM4->CCMR2  |=  (0x1UL << 3U);  // OC3PE
  TIM4->CCER   &= ~(0x1UL << 9U);  // CC3P=0
  TIM4->CCER   |=  (0x1UL << 8U);  // CC3E

  // CH4 (PB9)
  TIM4->CCR4    = 0;
  TIM4->CCMR2  &= ~(0x3UL << 8U);
  TIM4->CCMR2  &= ~(0x7UL << 12U);
  TIM4->CCMR2  |=  (0x6UL << 12U); // PWM Mode 1
  TIM4->CCMR2  |=  (0x1UL << 11U); // OC4PE
  TIM4->CCER   &= ~(0x1UL << 13U); // CC4P=0
  TIM4->CCER   |=  (0x1UL << 12U); // CC4E

  TIM4->EGR |= (0x1UL << 0U);
  TIM4->SR  &= ~(0x1UL << 0U);
  TIM4->CR1 |=  (0x1UL << 0U);
}

void Change_Duty_Cycle_M1(uint8_t duty) {
  if (duty > 100) duty = 100;
  TIM2->CCR2 = (TIM2->ARR + 1) * duty / 100;
}

void Change_Duty_Cycle_M2(uint8_t duty) {
  if (duty > 100) duty = 100;
  TIM2->CCR3 = (TIM2->ARR + 1) * duty / 100;
}

void Change_Duty_Cycle_M3(uint8_t duty) {
  if (duty > 100) duty = 100;
  TIM4->CCR3 = (TIM4->ARR + 1) * duty / 100;
}

void Change_Duty_Cycle_M4(uint8_t duty) {
  if (duty > 100) duty = 100;
  TIM4->CCR4 = (TIM4->ARR + 1) * duty / 100;
}
