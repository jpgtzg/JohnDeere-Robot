#include "motor.h"
#include "main.h"

// Pin numbers on GPIOC for each motor's IN1/IN2
static const uint8_t IN1_PIN[4] = {2, 4, 6, 8};
static const uint8_t IN2_PIN[4] = {3, 5, 7, 9};

void Motor_GPIO_Init(void) {
    RCC->APB2ENR |= (0x1UL << 4U); // GPIOC clock enable

    // PC2–PC7: output push-pull 2 MHz in CRL
    GPIOC->CRL &= ~(0xFFFFFFUL << 8U);
    GPIOC->CRL |=  (0x222222UL << 8U);

    // PC8–PC9: output push-pull 2 MHz in CRH
    GPIOC->CRH &= ~(0xFFUL << 0U);
    GPIOC->CRH |=  (0x22UL << 0U);

    Motor_All_Coast();
}

static inline void pin_set(uint8_t pin) {
    GPIOC->BSRR = (0x1UL << pin);
}

static inline void pin_clear(uint8_t pin) {
    GPIOC->BRR = (0x1UL << pin);
}

void Motor_Forward(uint8_t motor) {
    if (motor >= 4) return;
    pin_set(IN1_PIN[motor]);
    pin_clear(IN2_PIN[motor]);
}

void Motor_Reverse(uint8_t motor) {
    if (motor >= 4) return;
    pin_clear(IN1_PIN[motor]);
    pin_set(IN2_PIN[motor]);
}

void Motor_Brake(uint8_t motor) {
    if (motor >= 4) return;
    pin_set(IN1_PIN[motor]);
    pin_set(IN2_PIN[motor]);
}

void Motor_Coast(uint8_t motor) {
    if (motor >= 4) return;
    pin_clear(IN1_PIN[motor]);
    pin_clear(IN2_PIN[motor]);
}

void Motor_All_Forward(void) {
    for (uint8_t i = 0; i < 4; i++) Motor_Forward(i);
}

void Motor_All_Coast(void) {
    for (uint8_t i = 0; i < 4; i++) Motor_Coast(i);
}
