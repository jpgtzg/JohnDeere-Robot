#include "uart.h"
#include "main.h"
#include "ports.h"
#include <stdint.h>

void USART1_Init(void) {
  RCC->APB2ENR |= (0x1UL << 14U); //	USART1 clock enable
  USART1->CR1 |= USART_CR1_UE;    //		Step 1 Usart enabled
  USART1->CR1 &= ~USART_CR1_M;    //			Step 2 8 Data bits
  USART1->CR2 &= ~USART_CR2_STOP; //		Step 3 1 Stop bit
  USART1->BRR |= USARTDIV;        //			Step 5 Desired baud rate
  USART1->CR1 |= USART_CR1_TE;    //		Step 6 Transmitter enabled
  USART1->CR1 |= USART_CR1_RE;    //		Step 6 Receiver enabled

  RCC->APB2ENR |= (0x1UL << 2U); //	IO port A clock enable

  // Configuring RX pin (PA10) as input floating -> CNF=01, MODE=00
  GPIOA->CRH &= ~(0xFUL << 8U);
  GPIOA->CRH |= (0x1UL << 10U);

  // pin PA9 (USART1_TX) as alternate function output push-pull, max speed 10MHz
  GPIOA->CRH &= ~(0x1UL << 6U) & ~(0x2UL << 4U);
  GPIOA->CRH |= (0x2UL << 6U) | (0x1UL << 4U);
}

void USART1_Transmit(uint8_t *pData, uint16_t size) {
  for (int i = 0; i < size; i++) {
    USART1_Send_8bit(*pData++);
  }
}

void USART1_Send_8bit(uint8_t Data) {
  while (!(USART1->SR & USART_SR_TXE));
  USART1->DR = Data;
}

uint8_t USART1_Receive_8bit(void) {
  while (!(USART1->SR & USART_SR_RXNE));
  return (uint8_t)USART1->DR;
}

void USART2_Init(void) {
  RCC->APB1ENR |= (0x1UL << 17U); //	USART2 clock enable (APB1)
  USART2->CR1 |= USART_CR1_UE;    //		Step 1 Usart enabled
  USART2->CR1 &= ~USART_CR1_M;    //			Step 2 8 Data bits
  USART2->CR2 &= ~USART_CR2_STOP; //		Step 3 1 Stop bit
  USART2->BRR |= USARTDIV2;       //			Step 5 Desired baud rate (32 MHz APB1)
  USART2->CR1 |= USART_CR1_TE;    //		Step 6 Transmitter enabled
  USART2->CR1 |= USART_CR1_RE;    //		Step 6 Receiver enabled

  RCC->APB2ENR |= (0x1UL << 2U); //	IO port A clock enable

  // Configuring RX pin (PA3) as input floating -> CNF=01, MODE=00
  GPIOA->CRL &= ~(0xFUL << 12U); //		clear bits [15:12]
  GPIOA->CRL |= (0x1UL << 14U);  //	CNF=01 (floating input)

  // pin PA2 (USART2_TX) as alternate function output push-pull, max speed 10MHz
  GPIOA->CRL &= ~(0x1UL << 10U) & ~(0x2UL << 8U);
  GPIOA->CRL |= (0x2UL << 10U) | (0x1UL << 8U);
}

void USART2_Transmit(uint8_t *pData, uint16_t size) {
  for (int i = 0; i < size; i++) {
    USART2_Send_8bit(*pData++);
  }
}

void USART2_Send_8bit(uint8_t Data) {
  // wait until next data can be written
  while (!(USART2->SR & USART_SR_TXE));

  // Step 7 Data to send
  USART2->DR = Data;
}

uint8_t USART2_Receive_8bit(void) {
  while (!(USART2->SR & USART_SR_RXNE))
    ;                         //	wait until data is received
  return (uint8_t)USART2->DR; //			read and return received data
}

int _write(int file, char *ptr, int len) {
  int DataIdx;
  for (DataIdx = 0; DataIdx < len; DataIdx++) {
    while (!(USART2->SR & USART_SR_TXE))
      ;
    USART2->DR = *ptr++;
  }

  return len;
}
