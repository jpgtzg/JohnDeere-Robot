#include "functions.h"
#include "ports.h"

volatile uint16_t adc_value;

void SystemClock_Config(void) {
  FLASH->ACR &= ~(
      0x5UL << 0U); //		two wait states latency, if SYSCLK > 48MHz
  FLASH->ACR |=
      (0x2UL << 0U);             //		two wait states latency, if SYSCLK > 48MHz
  RCC->CFGR &= ~(0x1UL << 16U)   //			PLL HSI oscillator clock
                                 /// 2 selected as PLL input clock
               & ~(0x7UL << 11U) // 		APB2 prescaler /1
               & ~(0x3UL << 8U); // 		APB1 prescaler /2
  RCC->CFGR |=
      (0xFUL << 18U)         //			PLL input clock x 16 (PLLMUL bits)
      | (0x4UL << 8U);       //		APB1 prescaler /2
  RCC->CR |= (0x1UL << 24U); //		PLL2 ON
  while (!(RCC->CR & ~(0x1UL << 25U)))
    ;                          //	wait until PLL is locked
  RCC->CFGR &= ~(0x1UL << 0U); //		PLL used as system clock (SW
                               // bits)
  RCC->CFGR |= (0x2UL << 0U);  //		PLL used as system clock (SW
                               // bits)
  while (0x8UL != (RCC->CFGR & 0xCUL))
    ; //	wait until PLL is switched
}

void Delay_10ms_CPU(void) {
  __asm(" 		ldr r0, =1249999	"); //	load the value to be
                                                    // used as delay count
  __asm(" again:	sub r0, r0, #1		"); //	decrement the delay
                                                    // count
  __asm("			cmp r0, #0			"); //	check if
                                                                    // the
                                                                    // delay
                                                                    // count has
                                                                    // reached
                                                                    // zero
  __asm("			bne again			"); //	if not,
                                                                    // repeat
                                                                    // the
                                                                    // process
  __asm(
      "			nop					"); //	no
                                                                    // operation
                                                                    //(to
                                                                    // ensure
                                                                    // exact
                                                                    // timing)
}

void Delay_1sec_CPU(void) {
  __asm(" 		ldr r0, =7111111	"); //	load the value to be
                                                    // used as delay count
  __asm(" again1:	sub r0, r0, #1		"); //	decrement the delay
                                                    // count
  __asm("			cmp r0, #0			"); //	check if
                                                                    // the
                                                                    // delay
                                                                    // count has
                                                                    // reached
                                                                    // zero
  __asm("			bne again1			"); //	if not,
                                                                    // repeat
                                                                    // the
                                                                    // process
  __asm(
      "			nop					"); //	no
                                                                    // operation
                                                                    //(to
                                                                    // ensure
                                                                    // exact
                                                                    // timing)
}

void STM32_Button_Init(void) {
  // Enable GPIOC clock
  RCC->APB2ENR |= (0x1UL << 4U); // GPIOC clock enable

  // PC13 as input with pull-up (CNF=10, MODE=00)
  // CRH controls pins 8–15; PC13 is at bits [23:20]
  GPIOC->CRH &= ~(0xFUL << 20U); // clear CNF13 and MODE13
  GPIOC->CRH |=
      (0x8UL << 20U); // CNF13=10 (input pull-up/down), MODE13=00 (input)
  GPIOC->ODR |= (0x1UL << 13U); // pull-up enabled via ODR
}

// This code uses PA0 as the ADC input channel.
void ADC1_GPIO_Init(void) {
  RCC->APB2ENR |= (0x1UL << 2U); // enable GPIOA clock
  GPIOA->CRL &= ~(0xFUL << 0U);  // PA0 as analog input (CNF=00, MODE=00)
}

void ADC1_Init(void) {
  RCC->APB2ENR |= (0x1UL << 9U); // enable ADC1 clock
  RCC->CFGR |= (0x3UL << 14U);   // ADC prescaler /8 (PCLK2 64MHz → 8MHz)

  ADC1->CR1 &= ~(0xFUL << 16U); // independent mode
  ADC1->CR1 |= (0x1UL << 5U);   // EOC interrupt enable

  ADC1->CR2 &= ~(0x1UL << 11U); // right alignment
  ADC1->CR2 |= (0x1UL << 1U);   // continuous conversion mode

  ADC1->SMPR2 &= ~(0x7UL << 0U); // channel 0 sample time: 1.5 cycles
  ADC1->SQR1 &= ~(0xFUL << 20U); // 1 conversion in regular sequence
  ADC1->SQR3 &= ~(0x1FUL << 0U); // channel 0 first in sequence

  NVIC->ISER[0] |= (0x1UL << 18U); // enable ADC1_2 IRQ (18)
  NVIC->IP[18] = (0x10UL << 4U);   // priority 1

  ADC1->CR2 |= (0x1UL << 0U); // enable ADC1
  ADC1->CR2 |= (0x1UL << 2U); // start calibration
  while (ADC1->CR2 & (0x1UL << 2U))
    ;                         // wait for calibration to complete
  ADC1->CR2 |= (0x1UL << 0U); // start conversions
}

void ADC1_2_IRQHandler(void) {
  if (ADC1->SR & (0x1UL << 1U)) {
    adc_value = ADC1->DR & 0xFFFF;
    ADC1->SR &= ~(0x1UL << 1U);
  }
}
