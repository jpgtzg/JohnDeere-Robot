#ifndef TIMER_H_
#define TIMER_H_

void TIM2_Init(void);
void TIM3_Init(void);

void TIM2_Delay_10ms_Polling(void);
void TIM2_Delay_2s_Polling(void);
void TIM3_Delay_10ms_Polling(void);
void TIM3_Delay_2s_Polling(void);

void TIM2_Delay_10ms_Interrupt_Config(void);
void TIM2_Delay_2s_Interrupt_Config(void);
void TIM3_Delay_10ms_Interrupt_Config(void);
void TIM3_Delay_2s_Interrupt_Config(void);
void TIM3_40ms_Interrupt_Config(void);

#endif
