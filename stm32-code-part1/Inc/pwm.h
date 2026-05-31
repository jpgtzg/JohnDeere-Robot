#ifndef PWM_H_
#define PWM_H_

#include <stdint.h>

void PWM_GPIO_Init(void);
void TIM2_PWM_Init(void);
void TIM4_PWM_Init(void);

void Change_Duty_Cycle_M1(uint8_t duty); // PA1  — TIM2 CH2
void Change_Duty_Cycle_M2(uint8_t duty); // PB10 — TIM2 CH3
void Change_Duty_Cycle_M3(uint8_t duty); // PB8  — TIM4 CH3
void Change_Duty_Cycle_M4(uint8_t duty); // PB9  — TIM4 CH4

#endif
