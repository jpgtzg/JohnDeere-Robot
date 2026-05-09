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
void USER_TIM2_Delay_2s( void );
void USER_TIM3_Delay_10ms( void );
void USER_Button_Init(void);

#define BUTTON  (GPIOC->IDR & (0x1UL << 13U))

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
        USER_TIM2_Delay_2s( );//          blocking function (2s)
        GPIOA->ODR ^= ( 0x1UL << 5U );//  toggles the USER LED
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

void USER_Button_Init(void) {
    // Enable GPIOC clock
    RCC->APB2ENR |= (0x1UL << 4U);      // GPIOC clock enable

    // PC13 as input with pull-up (CNF=10, MODE=00)
    // CRH controls pins 8–15; PC13 is at bits [23:20]
    GPIOC->CRH &= ~(0xFUL << 20U);      // clear CNF13 and MODE13
    GPIOC->CRH |=  (0x8UL << 20U);      // CNF13=10 (input pull-up/down), MODE13=00 (input)
    GPIOC->ODR |=  (0x1UL << 13U);      // pull-up enabled via ODR
}
