#ifndef TIMER_H_
#define TIMER_H_

void TIM2_Init(void);
void TIM3_Init(void);

void TIM2_Delay_10ms_Polling(void);
void TIM2_Delay_2s_Polling(void);
void TIM3_Delay_10ms_Polling(void);
void TIM3_Delay_2s_Polling(void);

void TIM2_Delay_10ms_Interrupt(void);
void TIM2_Delay_2s_Interrupt(void);
void TIM3_Delay_10ms_Interrupt(void);
void TIM3_Delay_2s_Interrupt(void);

#endif
