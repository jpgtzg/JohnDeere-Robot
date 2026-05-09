#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#define STM32_BUTTON (GPIOC->IDR & (0x1UL << 13U))

void SystemClock_Config(void);
void GPIO_Init(void);
void Delay_10ms_CPU(void);
void Delay_1sec_CPU(void);
void STM32_Button_Init(void);

#endif
