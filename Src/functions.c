#include "ports.h"

void USER_SystemClock_Config(void) {
  FLASH->ACR &= ~(
      0x5UL << 0U); //		two wait states latency, if SYSCLK > 48MHz
  FLASH->ACR |=
      (0x2UL << 0U);             //		two wait states latency, if SYSCLK > 48MHz
  RCC->CFGR &= ~(0x1UL << 16U)   //			PLL HSI oscillator clock
                                 ///2 selected as PLL input clock
               & ~(0x7UL << 11U) // 		APB2 prescaler /1
               & ~(0x3UL << 8U); // 		APB1 prescaler /2
  RCC->CFGR |=
      (0xFUL << 18U)         //			PLL input clock x 16 (PLLMUL bits)
      | (0x4UL << 8U);       //		APB1 prescaler /2
  RCC->CR |= (0x1UL << 24U); //		PLL2 ON
  while (!(RCC->CR & ~(0x1UL << 25U)))
    ;                          //	wait until PLL is locked
  RCC->CFGR &= ~(0x1UL << 0U); //		PLL used as system clock (SW
                               //bits)
  RCC->CFGR |= (0x2UL << 0U);  //		PLL used as system clock (SW
                               //bits)
  while (0x8UL != (RCC->CFGR & 0xCUL))
    ; //	wait until PLL is switched
}

void USER_Delay_10ms_CPU(void) {
  __asm(" 		ldr r0, =1249999	"); //	load the value to be
                                                    //used as delay count
  __asm(" again:	sub r0, r0, #1		"); //	decrement the delay
                                                    //count
  __asm(
      "			cmp r0, #0			"); //	check if the
                                                            //delay count has
                                                            //reached zero
  __asm(
      "			bne again			"); //	if not, repeat
                                                            //the process
  __asm(
      "			nop					"); //	no
                                                                    //operation
                                                                    //(to ensure
                                                                    //exact
                                                                    //timing)
}

void USER_Delay_1sec_CPU(void) {
  __asm(" 		ldr r0, =7111111	"); //	load the value to be
                                                    //used as delay count
  __asm(" again1:	sub r0, r0, #1		"); //	decrement the delay
                                                    //count
  __asm(
      "			cmp r0, #0			"); //	check if the
                                                            //delay count has
                                                            //reached zero
  __asm(
      "			bne again1			"); //	if not, repeat
                                                            //the process
  __asm(
      "			nop					"); //	no
                                                                    //operation
                                                                    //(to ensure
                                                                    //exact
                                                                    //timing)
}
