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

uint8_t msg[] = "Hello World!\r\n";

void USER_SystemClock_Config( void );
void USER_GPIO_Init( void );
/* Superloop structure */
int main(void)
{
	/* Declarations and Initializations */
	USER_SystemClock_Config( );
	USER_GPIO_Init( );
	USER_USART1_Init( );
    	/* Repetitive block */
    	for(;;){
			GPIOA->ODR	^=	( 0x1UL <<  5U );//		value to toggle pin 5 of Port A (Toggle LD2)
			printf("Hello STM32 World from Equipo 6\r\n");
			GPIOA->ODR	^=	( 0x1UL <<  5U ); //		value to toggle pin 5 of Port A (Toggle LD2)
    	}
}

void USER_GPIO_Init( void ){
	RCC->APB2ENR	|=	 ( 0x1UL <<  2U );//	IO port A clock enable
	//pin PA9 (USART1_TX) as alternate function output push-pull, max speed 10MHz
	GPIOA->CRH		&=	~( 0x1UL <<  6U ) &	~( 0x2UL <<  4U );
	GPIOA->CRH		|=	 ( 0x2UL <<  6U ) |	 ( 0x1UL <<  4U );

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