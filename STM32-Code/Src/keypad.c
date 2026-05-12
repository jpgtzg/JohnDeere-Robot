#include <stdint.h>
#include "keypad.h"
#include "ports.h"
#include "main.h"

void Keypad_Init(void) {
  // Enab(0xFF)le clock for GPIOB (assuming the keypad is connected to GPIOB)
  RCC->APB2ENR |= (0x1UL << 3U);
  RCC->APB2ENR |= (0x1UL << 0U); // enable AFIO clock
  AFIO->MAPR |= (0x2UL << 24U);  // disable JTAG, keep SWD

  // Configure PB0-PB3 as output push-pull
  for (int i = 0; i < 4; i++) {
    GPIOB->CRL &= ~(0x3UL << (2U + (i * 4U))) &
                  ~(0x2UL << (i * 4U)); // Clear CNF0[1:0] and MODE0_1 for PBi
    GPIOB->CRL |= (0x1UL << (i * 4U));  // Set MODE0_1 for PBi
  }

  // Configure PB4 - PB7 as input pull-up
  // CNF: 10
  // MODE: 00
  // ODR: 1
  for (int i = 4; i < 8; i++) {
    GPIOB->CRL &= ~(0x1UL << (2U + (i * 4U))) & ~(0x3UL << (i * 4U));
    GPIOB->CRL |= (0x2UL << (2U + (i * 4U)));
    GPIOB->ODR |= (0x1UL << i);
  }
}

uint8_t Key(void) {
  // output has to be like
  // 0 1 1 1
  // 1 0 1 1
  // 1 1 0 1
  // 1 1 1 0

  // i can do this like
  // for 1 1 1 0
  // ~(0x1UL << i)
  // this generates this:
  // 1111 1110
  // 1111 1101...

  // but i want only the last 4 bits
  // so i can do it like
  // ~(0x1UL << i) & 0x0F;
  // that way:
  // 1111 1110
  // 0000 1111

  // get masked like
  // 0000 1110

  // 1110
  // 0001
  // 0000 -> then weve found the key, we can return it like
  //

  for (int i = 0; i < 4; i++) {
    GPIOB->ODR &= ~(0xFUL << 0U); // Clear the output bits
    GPIOB->ODR |=
        ~(0x1UL << i) & 0xF; // Set the output bits for the current row
    for (int j = 0; j < 4; j++) {
      if ((GPIOB->IDR & (0x1UL << (j + 4U))) ==
          0) {              // check if the input is low
        return (i * 4) + j; // 0 index based key number
      }
    }
  }

  return 0xFF; // no key pressed
}
