#ifndef UART_H_
#define UART_H_

#include <stdint.h>

// Baud rate divisors (not defined by CMSIS)
#define USARTDIV  0x22C  // 115200 baud at 64 MHz APB2
#define USARTDIV2 0x116  // 115200 baud at 32 MHz APB1

void USART1_Init(void);
void USART1_Transmit(uint8_t *pData, uint16_t size);
void USART1_Send_8bit(uint8_t Data);
uint8_t USART1_Receive_8bit(void);

#endif
