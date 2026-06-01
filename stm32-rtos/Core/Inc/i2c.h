#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>

void I2C1_Init(void);
void I2C1_Write(uint8_t addr, uint8_t data);

#endif
