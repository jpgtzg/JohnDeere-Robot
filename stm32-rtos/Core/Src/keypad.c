#include <stdint.h>
#include "keypad.h"
#include "main.h"

void Keypad_Init(void) {
  RCC->APB2ENR |= (0x1UL << 3U); // GPIOB clock enable
  RCC->APB2ENR |= (0x1UL << 0U); // AFIO clock enable
  AFIO->MAPR   |= (0x2UL << 24U); // disable JTAG, keep SWD

  // PB0-PB3 as output push-pull
  for (int i = 0; i < 4; i++) {
    GPIOB->CRL &= ~(0x3UL << (2U + (i * 4U))) & ~(0x2UL << (i * 4U));
    GPIOB->CRL |=  (0x1UL << (i * 4U));
  }

  // PB4-PB7 as input pull-up (CNF=10, MODE=00, ODR=1)
  for (int i = 4; i < 8; i++) {
    GPIOB->CRL &= ~(0x1UL << (2U + (i * 4U))) & ~(0x3UL << (i * 4U));
    GPIOB->CRL |=  (0x2UL << (2U + (i * 4U)));
    GPIOB->ODR |=  (0x1UL << i);
  }
}

uint8_t Key(void) {
  for (int i = 0; i < 4; i++) {
    GPIOB->ODR &= ~(0xFUL << 0U);
    GPIOB->ODR |= ~(0x1UL << i) & 0xF;
    for (int j = 0; j < 4; j++) {
      if ((GPIOB->IDR & (0x1UL << (j + 4U))) == 0) {
        return (i * 4) + j;
      }
    }
  }
  return 0xFF;
}
