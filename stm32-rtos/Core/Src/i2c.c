#include "i2c.h"
#include "main.h"

/*
 * I2C1 on PB6 (SCL) and PB7 (SDA), 100 kHz, APB1 = 32 MHz.
 *
 * STM32F1 I2C issues addressed:
 *   1. BUSY stuck on power-on  → 9-clock GPIO bus recovery before I2C init.
 *   2. STOP not completing     → poll CR1 STOP bit until hardware clears it.
 *   3. AF (NACK) not cleared   → explicitly clear AF before each transaction.
 *
 * I2C_TypeDef and I2C1 pointer are provided by stm32f103xb.h via main.h.
 */

#define I2C_CR1_PE    (0x1UL << 0U)
#define I2C_CR1_START (0x1UL << 8U)
#define I2C_CR1_STOP  (0x1UL << 9U)
#define I2C_CR1_SWRST (0x1UL << 15U)
#define I2C_SR1_SB    (0x1UL << 0U)
#define I2C_SR1_ADDR  (0x1UL << 1U)
#define I2C_SR1_BTF   (0x1UL << 2U)
#define I2C_SR1_TXE   (0x1UL << 7U)
#define I2C_SR1_AF    (0x1UL << 10U)
#define I2C_SR2_BUSY  (0x1UL << 1U)

#define TIMEOUT 100000U

/* ~5 µs busy-wait at 64 MHz */
static void i2c_delay_5us(void) {
    for (volatile uint32_t i = 0; i < 320U; i++);
}

/* Bus recovery: toggle SCL 9 times as GPIO to release any stuck slave. */
static void i2c_bus_recovery(void) {
    RCC->APB2ENR |= (0x1UL << 3U); /* GPIOB clock */

    /* PB6=SCL, PB7=SDA as GP open-drain output, 10 MHz */
    GPIOB->CRL &= ~(0xFFUL << 24U);
    GPIOB->CRL |=  (0x55UL << 24U); /* CNF=01 GP-OD, MODE=01 10 MHz */

    /* Both lines high */
    GPIOB->BSRR = (0x1UL << 6U) | (0x1UL << 7U);
    i2c_delay_5us();

    for (int i = 0; i < 9; i++) {
        GPIOB->BRR  = (0x1UL << 6U); /* SCL low  */
        i2c_delay_5us();
        GPIOB->BSRR = (0x1UL << 6U); /* SCL high */
        i2c_delay_5us();

        if (GPIOB->IDR & (0x1UL << 7U))
            break; /* SDA released */
    }

    /* Manual STOP: SDA low → SCL high → SDA high */
    GPIOB->BRR  = (0x1UL << 7U);
    i2c_delay_5us();
    GPIOB->BSRR = (0x1UL << 6U);
    i2c_delay_5us();
    GPIOB->BSRR = (0x1UL << 7U);
    i2c_delay_5us();
}

void I2C1_Init(void) {
    i2c_bus_recovery();

    /* PB6/PB7 → AF open-drain for I2C1 */
    GPIOB->CRL &= ~(0xFFUL << 24U);
    GPIOB->CRL |=  (0xDDUL << 24U); /* CNF=11 AF-OD, MODE=01 10 MHz */

    RCC->APB1ENR |= (0x1UL << 21U); /* I2C1EN */
    I2C1->CR1    |=  I2C_CR1_SWRST;
    i2c_delay_5us();
    I2C1->CR1    &= ~I2C_CR1_SWRST;
    i2c_delay_5us();

    /* 100 kHz standard mode on 32 MHz APB1 */
    I2C1->CR2   = 32U;
    I2C1->CCR   = 160U;  /* 32 MHz / (2 × 100 kHz) */
    I2C1->TRISE = 33U;   /* (32 µs × 1 MHz) + 1    */

    I2C1->CR1 |= I2C_CR1_PE;
}

void I2C1_Write(uint8_t addr, uint8_t data) {
    uint32_t t;

    t = TIMEOUT;
    while ((I2C1->SR2 & I2C_SR2_BUSY) && --t);
    if (!t) {
        I2C1->CR1 |=  I2C_CR1_SWRST;
        i2c_delay_5us();
        I2C1->CR1 &= ~I2C_CR1_SWRST;
        I2C1->CR2   = 32U;
        I2C1->CCR   = 160U;
        I2C1->TRISE = 33U;
        I2C1->CR1  |= I2C_CR1_PE;
        return;
    }

    I2C1->SR1 &= ~I2C_SR1_AF;

    I2C1->CR1 |= I2C_CR1_START;
    t = TIMEOUT;
    while (!(I2C1->SR1 & I2C_SR1_SB) && --t);
    if (!t) { I2C1->CR1 |= I2C_CR1_STOP; return; }

    I2C1->DR = (uint8_t)(addr << 1U);
    t = TIMEOUT;
    while (!(I2C1->SR1 & (I2C_SR1_ADDR | I2C_SR1_AF)) && --t);
    if (!t || (I2C1->SR1 & I2C_SR1_AF)) {
        I2C1->SR1 &= ~I2C_SR1_AF;
        I2C1->CR1 |= I2C_CR1_STOP;
        t = TIMEOUT;
        while ((I2C1->CR1 & I2C_CR1_STOP) && --t);
        return;
    }
    (void)I2C1->SR1;
    (void)I2C1->SR2;

    t = TIMEOUT;
    while (!(I2C1->SR1 & I2C_SR1_TXE) && --t);
    if (!t) { I2C1->CR1 |= I2C_CR1_STOP; return; }
    I2C1->DR = data;

    t = TIMEOUT;
    while (!(I2C1->SR1 & I2C_SR1_BTF) && --t);

    I2C1->CR1 |= I2C_CR1_STOP;
    t = TIMEOUT;
    while ((I2C1->CR1 & I2C_CR1_STOP) && --t);
}
