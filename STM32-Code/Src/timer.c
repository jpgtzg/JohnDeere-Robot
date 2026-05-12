#include "timer.h"
#include "ports.h"

// ===============================================================================
// Functions for configuring TIM2 and TIM3 in basic timer mode (upcounter,
// internal clock source) used in both polling and interrupt methods
// ===============================================================================
void TIM2_Init(void) {
  // TIM2 clock enable
  RCC->APB1ENR |= (0x1UL << 0U);

  // select internal CLK source
  TIM2->SMCR &= ~(0x7UL << 0U);

  // mode edge-upcounter
  TIM2->CR1 &= ~(0x3UL << 5U) & ~(0x1UL << 4U) & ~(0x1UL << 1U);
}

void TIM3_Init(void) {
  RCC->APB1ENR |= (0x1UL << 1U); //       TIM3 clock enable
  TIM3->SMCR &=
      ~(0x7UL << 0U); //                                       select internal
                      //                                       CLK source
  TIM3->CR1 &=
      ~(0x3UL << 5U) & ~(0x1UL << 4U) & ~(0x1UL << 1U); // mode edge-upcounter
}

// ===============================================================================
// Functions for implementing delay functions using polling method
// (assumes TIM2_Init / TIM3_Init already called for clock and mode setup)
// ===============================================================================

void TIM2_Delay_10ms_Polling(void) {
  TIM2->SR &= ~(0x1UL << 0U); //       clear the overflow flag
  TIM2->PSC = 9;
  TIM2->CNT = 1536;
  TIM2->ARR = 65535;
  TIM2->CR1 |= (0x1UL << 0U); //       start the timer
  while (!(TIM2->SR & (0x1UL << 0U)))
    ;                          // <--------------------- Polling Method
  TIM2->CR1 &= ~(0x1UL << 0U); //       stop the timer
}

void TIM2_Delay_2s_Polling(void) {
  TIM2->SR &= ~(0x1UL << 0U); //       clear the overflow flag
  TIM2->PSC = 1953;
  TIM2->CNT = 29;
  TIM2->ARR = 65535;
  TIM2->CR1 |= (0x1UL << 0U); //       start the timer
  while (!(TIM2->SR & (0x1UL << 0U)))
    ;                          // <--------------------- Polling Method
  TIM2->CR1 &= ~(0x1UL << 0U); //       stop the timer
}

void TIM3_Delay_10ms_Polling(void) {
  TIM3->SR &= ~(0x1UL << 0U); //       clear the overflow flag
  TIM3->PSC = 9;
  TIM3->CNT = 1536;
  TIM3->ARR = 65535;
  TIM3->CR1 |= (0x1UL << 0U); //       start the timer
  while (!(TIM3->SR & (0x1UL << 0U)))
    ;                          // <--------------------- Polling Method
  TIM3->CR1 &= ~(0x1UL << 0U); //       stop the timer
}

void TIM3_Delay_2s_Polling(void) {
  TIM3->SR &= ~(0x1UL << 0U); //       clear the overflow flag
  TIM3->PSC = 1953;
  TIM3->CNT = 29;
  TIM3->ARR = 65535;
  TIM3->CR1 |= (0x1UL << 0U); //       start the timer
  while (!(TIM3->SR & (0x1UL << 0U)))
    ;                          // <--------------------- Polling Method
  TIM3->CR1 &= ~(0x1UL << 0U); //       stop the timer
}

// ===============================================================================
// Functions for implementing delay functions using interrupt method
// (assumes TIM2_Init / TIM3_Init already called for clock and mode setup)
// Remember to implement the corresponding interrupt handlers in your main file
// to handle the timer interrupts and clear the overflow flags appropriately
// ===============================================================================

void TIM2_Delay_10ms_Interrupt(void) {
  TIM2->SR &= ~(0x1UL << 0U); // clear overflow flag
  TIM2->PSC = 9;
  TIM2->CNT = 1536;
  TIM2->ARR = 65535;
  TIM2->DIER |= (0x1UL << 0U);      // enable UIE
  NVIC->ISER[0U] |= (0x1UL << 28U); // TIM2 global interrupt at position 28
  TIM2->CR1 |= (0x1UL << 0U);       // start timer
}

void TIM2_Delay_2s_Interrupt(void) {
  TIM2->SR &= ~(0x1UL << 0U); // clear overflow flag
  TIM2->PSC = 1953;
  TIM2->CNT = 29;
  TIM2->ARR = 65535;
  TIM2->DIER |= (0x1UL << 0U);      // enable UIE
  NVIC->ISER[0U] |= (0x1UL << 28U); // TIM2 global interrupt at position 28
  TIM2->CR1 |= (0x1UL << 0U);       // start timer
}

void TIM3_Delay_10ms_Interrupt(void) {
  TIM3->SR &= ~(0x1UL << 0U); // clear overflow flag
  TIM3->PSC = 9;
  TIM3->CNT = 1536;
  TIM3->ARR = 65535;
  TIM3->DIER |= (0x1UL << 0U);      // enable UIE
  NVIC->ISER[0U] |= (0x1UL << 29U); // TIM3 global interrupt at position 29
  TIM3->CR1 |= (0x1UL << 0U);       // start timer
}

void TIM3_Delay_2s_Interrupt(void) {
  TIM3->SR &= ~(0x1UL << 0U); // clear overflow flag
  TIM3->PSC = 1953;
  TIM3->CNT = 29;
  TIM3->ARR = 65535;
  TIM3->DIER |= (0x1UL << 0U);      // enable UIE
  NVIC->ISER[0U] |= (0x1UL << 29U); // TIM3 global interrupt at position 29
  TIM3->CR1 |= (0x1UL << 0U);       // start timer
}
