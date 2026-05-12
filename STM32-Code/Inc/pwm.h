#ifndef PWM_H_
#define PWM_H_

#include <stdint.h>

void PWM_GPIO_Init(void);
void TIM2_PWM_Init(void);
void Change_Duty_Cycle(uint8_t duty);

#endif
