#include "uart.h"
#include "main.h"
#include <stdint.h>

void USART1_Init(void) {
  RCC->APB2ENR |= (0x1UL << 14U); // USART1 clock enable
  USART1->CR1 |=  (0x1UL << 13U); // UE: USART enabled
  USART1->CR1 &= ~(0x1UL << 12U); // M:  8 data bits
  USART1->CR2 &= ~(0x3UL << 12U); // STOP: 1 stop bit
  USART1->BRR  =   USARTDIV;      // 115200 baud at 64 MHz APB2
  USART1->CR1 |=  (0x1UL <<  3U); // TE: transmitter enabled
  USART1->CR1 |=  (0x1UL <<  2U); // RE: receiver enabled

  RCC->APB2ENR |= (0x1UL << 2U);  // IO port A clock enable

  // PA10 (USART1_RX) as input floating — CNF=01, MODE=00
  GPIOA->CRH &= ~(0xFUL << 8U);
  GPIOA->CRH |=  (0x1UL << 10U);

  // PA9 (USART1_TX) as AF output push-pull 10 MHz — CNF=10, MODE=01
  GPIOA->CRH &= ~(0x1UL << 6U) & ~(0x2UL << 4U);
  GPIOA->CRH |=  (0x2UL << 6U) |  (0x1UL << 4U);
}

int USART1_Available(void) {
  return !!(USART1->SR & (0x1UL << 5U));
}

void USART1_Transmit(uint8_t *pData, uint16_t size) {
  for (int i = 0; i < size; i++) {
    USART1_Send_8bit(*pData++);
  }
}

void USART1_Send_8bit(uint8_t Data) {
  while (!(USART1->SR & (0x1UL << 7U))); // wait TXE
  USART1->DR = Data;
}

uint8_t USART1_Receive_8bit(void) {
  while (!(USART1->SR & (0x1UL << 5U))); // wait RXNE
  return (uint8_t)USART1->DR;
}
