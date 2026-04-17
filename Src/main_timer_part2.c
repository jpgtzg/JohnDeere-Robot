/**
 ******************************************************************************
 * @file           : main.c
 * @author         : rahu7p
 * @board          : NUCLEO-F103RB
 ******************************************************************************
 *
 * C code to transmit the Hello World! string using the serial port (USART1) 
 * in bare metal
 *
 ******************************************************************************
 */

/* Libraries, Definitions and Global Declarations */
#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "user_uart.h"

void USER_SystemClock_Config( void );
void USER_TIM2_Init( void );
void USER_TIM3_Init( void );
void USER_TIM3_Delay_10ms( void );
void USER_Button_Init(void);

#define BUTTON  (GPIOC->IDR & (0x1UL << 13U))

void TIM2_IRQHandler( void ){
    if( TIM2->SR & ( 0x1UL << 0U )){
        TIM2->SR &= ~(0x1UL << 0U);       // clear it   
        GPIOA->ODR ^= ( 0x1UL << 5U );//  toggles the USER LED
    }
}

/* Superloop structure */
int main(void){
    USER_SystemClock_Config();
    USER_Button_Init();       // <-- add this
    USER_TIM2_Init( );
    USER_TIM3_Init( );
    USER_USART1_Init( );
    USER_GPIO_Init();
    
    /* Repetitive block */
    for(;;){
        if( !BUTTON ){//                  if button is pressed
            USER_TIM3_Delay_10ms( );//      10ms
            if( !BUTTON ){//                double checking
                printf("Button pressed!\r\n");
                while( !BUTTON );//           waits until button released
                USER_TIM3_Delay_10ms( );//    10ms
            }
        }
    }
}

void USER_GPIO_Init( void ){
    // USER LED part

	// GPIOx_ODR modified to reset pin 5 of port A (LD2 is connected to PA5)
	GPIOA->ODR		= GPIOA->ODR//				GPIOx_ODR actual value
					&//							to clear
					~( 0x1UL << 5U );//			(mask) ODR5 bit

	// GPIOx_CRL modified to configure pin5 as output
	GPIOA->CRL		=	GPIOA->CRL//			GPIOx_CRL actual value
					&//							to clear
					~( 0x3UL << 22U )//			(mask) CNF5[1:0] bits
					&//							to clear
					~( 0x2UL << 20U );//		(mask) MODE5_1 bit

	// GPIOx_CRL modified to select pin5 max speed of 10MHz
	GPIOA->CRL		=	GPIOA->CRL//			GPIOx_CRL actual value
					|//							to set
					( 0x1UL << 20U );//			(mask) MODE5_0 bit
}

void USER_TIM2_Init( void ){
    RCC->APB1ENR |= ( 0x1UL << 0U );//       TIM2 clock enable
    
    TIM2->SMCR &= ~( 0x7UL << 0U );//                                       select internal CLK source
    TIM2->CR1 &= ~( 0x3UL << 5U ) & ~( 0x1UL << 4U ) & ~( 0x1UL << 1U ); // mode edge-upcounter
    TIM2->SR  &= ~( 0x1UL << 0U );//       clear the overflow flag

    TIM2->PSC  = 1953;
    TIM2->CNT  = 29;
    TIM2->ARR  = 65535;

    TIM2->DIER |=  ( 0x1UL << 0U );//       enable interrupt (UIE bit)
    NVIC->ISER[0U] |= ( 0x1UL << 28U );//     enable TIM2 interrupt in NVIC (position 28 in ISER[0])
    TIM2->CR1 |=  ( 0x1UL << 0U );//       start the timer
}

void USER_TIM3_Init( void ){
    RCC->APB1ENR |= ( 0x1UL << 1U );//       TIM3 clock enable
    TIM3->SMCR &= ~( 0x7UL << 0U );//                                       select internal CLK source
    TIM3->CR1 &= ~( 0x3UL << 5U ) & ~( 0x1UL << 4U ) & ~( 0x1UL << 1U ); // mode edge-upcounter
}

void USER_TIM3_Delay_10ms( void ){
  TIM3->SR  &= ~( 0x1UL << 0U );//       clear the overflow flag
  TIM3->PSC  = 9;
  TIM3->CNT  = 1536;
  TIM3->ARR  = 65535;
  TIM3->CR1 |=  ( 0x1UL << 0U );//       start the timer
  while(!( TIM3->SR & ( 0x1UL << 0U ))); // <--------------------- Polling Method
  TIM3->CR1 &= ~( 0x1UL << 0U );//       stop the timer
}

void USER_Button_Init(void) {
    // Enable GPIOC clock
    RCC->APB2ENR |= (0x1UL << 4U);      // GPIOC clock enable

    // PC13 as input with pull-up (CNF=10, MODE=00)
    // CRH controls pins 8–15; PC13 is at bits [23:20]
    GPIOC->CRH &= ~(0xFUL << 20U);      // clear CNF13 and MODE13
    GPIOC->CRH |=  (0x8UL << 20U);      // CNF13=10 (input pull-up/down), MODE13=00 (input)
    GPIOC->ODR |=  (0x1UL << 13U);      // pull-up enabled via ODR
}

void USER_Delay_10ms( void ){
	__asm(" 		ldr r0, =1249999	");//	load the value to be used as delay count
	__asm(" again:	sub r0, r0, #1		");//	decrement the delay count
	__asm("			cmp r0, #0			");//	check if the delay count has reached zero
	__asm("			bne again			");//	if not, repeat the process
	__asm("			nop					");//	no operation (to ensure exact timing)
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