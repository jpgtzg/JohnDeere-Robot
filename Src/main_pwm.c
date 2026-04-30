#include <stdint.h> // 							standard integer library
#include "main.h"
#include "user_keypad.h"
#include <stdio.h>

void CHANGE_DUTY_CYCLE(uint8_t duty);

int main( void ){
    USER_SystemClock_Config();
    USER_GPIO_Init();
    USER_TIM2_Init( );
    USER_Keypad_Init(); //						initialize the keypad GPIO pins
    /* Repetitive block */
    for(;;){
    }
}

void CHANGE_DUTY_CYCLE(uint8_t duty){
	TIM2->CR1 &~  ( 0x1UL << 0U );//       stop the timer
	
	if(duty > 100) duty = 100; // Cap duty cycle at 100%
	
	TIM2->CCR1 = (TIM2->ARR + 1) * duty / 100; // Update CCR1 based on desired duty cycle
	
	TIM2->CR1 |=  ( 0x1UL << 0U );//       start the timer	
}

void USER_GPIO_Init( void ){
	RCC->APB2ENR	|=	 ( 0x1UL <<  2U );//	IO port A clock enable

    // Configure PA0 as alternate function push-pull (CNF=10, MODE=11)
    GPIOA->CRL &= ~(0x1UL << 2U); // clear CNF0
    GPIOA->CRL |=  (0x2UL << 2U) | (0x3UL << 0U); // CNF0=10 (alternate function push-pull), MODE0=11 (output mode, max speed 50 MHz)
}

void USER_TIM2_Init( void ){
    RCC->APB1ENR |= ( 0x1UL << 0U );//       TIM2 clock enable
    
    TIM2->SMCR &= ~( 0x7UL << 0U );//                                       select internal CLK source
    TIM2->CR1 &= ~( 0x3UL << 5U ) & ~( 0x1UL << 4U ) & ~( 0x1UL << 1U ) & ~(0x1UL << 2U); // mode edge-upcounter
    TIM2->CR1 |= (0x1UL << 7U); // Load ARR only on UEV
    
    TIM2->PSC = 0; // Configure PSC
	TIM2->ARR = 63999; // Configure ARR
	TIM2->CCR1 = 16000; // Configure CCR1 for 25% duty cycle (16000/64000 = 0.25)

	TIM2->CCMR1 &= ~(0x1UL << 4U); // Clear OCM1[0] bit
	TIM2->CCMR1 |=  (0x6UL << 4U); // PWM Mode 1 (OCM1[2:0] = 110)
	TIM2->CCMR1 |=  (0x1UL << 3U); // CCR1 Load UEV in event
    TIM2->CCMR1 &= ~(0x3UL << 0U); // Clear CC1S[1:0] bits -> Configure channel 0

    TIM2->CCER &= ~(0x1UL << 1U);
	TIM2->CCER |=  (0x1UL << 0U);

    TIM2->EGR |= (0x1UL << 0U); // Generate an update event to load the prescaler value immediately
    TIM2->SR &= ~(0x1UL << 0U); // clear the overflow flag

    TIM2->CCER &= ~(0x1UL << 1U); // Ensure CC1P is cleared for active high
    TIM2->CCER |=  (0x1UL << 0U); // Enable capture/compare for channel 1

    TIM2->CR1 |=  ( 0x1UL << 0U );//       start the timer
}


void USER_SystemClock_Config( void ){
	FLASH->ACR	&=	~( 0x5UL <<  0U );//		two wait states latency, if SYSCLK > 48MHz
	FLASH->ACR	|=	 ( 0x2UL <<  0U );//		two wait states latency, if SYSCLK > 48MHz
	RCC->CFGR	&=	~( 0x1UL << 16U )//			PLL HSI oscillator clock /2 selected as PLL input clock
				&	~( 0x7UL << 11U )// 		APB2 prescaler /1
				&	~( 0x3UL <<  8U );// 		APB1 prescaler /2
	RCC->CFGR	|=	 ( 0xFUL << 18U )//			PLL input clock x 16 (PLLMUL bits)
				|	 ( 0x4UL <<  8U );//		APB1 prescaler /2
	RCC->CR		|=	 ( 0x1UL << 24U );//		PLL2 ON
	while( !(RCC->CR & ~( 0x1UL << 25U )));//	wait until PLL is locked
	RCC->CFGR	&=	~( 0x1UL << 0U  );//		PLL used as system clock (SW bits)
	RCC->CFGR	|=	 ( 0x2UL << 0U  );//		PLL used as system clock (SW bits)
	while( 0x8UL != ( RCC->CFGR & 0xCUL ));//	wait until PLL is switched
}


void USER_Delay_10ms( void ){
	__asm(" 		ldr r0, =1249999	");//	load the value to be used as delay count
	__asm(" again:	sub r0, r0, #1		");//	decrement the delay count
	__asm("			cmp r0, #0			");//	check if the delay count has reached zero
	__asm("			bne again			");//	if not, repeat the process
	__asm("			nop					");//	no operation (to ensure exact timing)
}
