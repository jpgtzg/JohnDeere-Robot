#ifndef MOTOR_H_
#define MOTOR_H_

#include <stdint.h>

// M1: PC2(IN1) PC3(IN2)  M2: PC4(IN1) PC5(IN2)
// M3: PC6(IN1) PC7(IN2)  M4: PC8(IN1) PC9(IN2)

void Motor_GPIO_Init(void);

void Motor_Forward(uint8_t motor); // IN1=1, IN2=0
void Motor_Reverse(uint8_t motor); // IN1=0, IN2=1
void Motor_Brake(uint8_t motor);   // IN1=1, IN2=1
void Motor_Coast(uint8_t motor);   // IN1=0, IN2=0

void Motor_All_Forward(void);
void Motor_All_Coast(void);

#endif
