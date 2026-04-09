#include <stdint.h>
#include "main.h"
#include "user_uart.h"

static void USER_USART1_Send_8bit( uint8_t Data );
static void USER_USART1_Send_8bit( uint8_t Data );

void USER_USART1_Init( void ){
	RCC->APB2ENR	|=	 ( 0x1UL << 14U );//	USART 1 clock enable	
	USART1->CR1		|=	 USART_CR1_UE;//		Step 1 Usart enabled
	USART1->CR1		&=	~USART_CR1_M;//			Step 2 8 Data bits
	USART1->CR2		&=	~USART_CR2_STOP;//		Step 3 1 Stop bit
	USART1->BRR	 	|=	 USARTDIV;//			Step 5 Desired baud rate
	USART1->CR1		|= 	 USART_CR1_TE;//		Step 6 Transmitter enabled
	USART1->CR1		|= 	 USART_CR1_RE;//		Step 6 Receiver enabled

	// Configuring RX pin (PA10) as input floating -> CNF=01, MODE=00
	GPIOA->CRH		&=	~( 0xFUL << 8U );//		clear MODE10[1:0] and CNF10[1:0]
	GPIOA->CRH		|=	 ( 0x1UL << 10U );//	CNF10=01 (floating input), MODE10=00
}

void USER_USART1_Transmit( uint8_t *pData, uint16_t size ){
	for( int i = 0; i < size; i++ ){
		USER_USART1_Send_8bit( *pData++ );
	}
}

static void USER_USART1_Send_8bit( uint8_t Data ){
	while(!( USART1->SR & USART_SR_TXE ));//	wait until next data can be written
	USART1->DR = Data;//				Step 7 Data to send
}

uint8_t USER_USART1_Receive_8bit( void ){
	while(!( USART1->SR & USART_SR_RXNE ));//	wait until data is received
	return (uint8_t)USART1->DR;//			read and return received data
}

int _write(int file, char *ptr, int len){
	int DataIdx;
 	for(DataIdx=0; DataIdx<len; DataIdx++){
 		while(!( USART1->SR & USART_SR_TXE ));
		USART1->DR = *ptr++;
	}

 	return len;
}
