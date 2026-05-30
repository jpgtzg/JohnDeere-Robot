/* ============================================================
 * main.c — JohnDeere Robot 
 * Arquitectura de Tareas:
 *
 *   T1_Sensores      T=10 ms   P=1  Lee ADC y boton
 *   T2_Controlador   T=20 ms   P=2  Modelo EngTrModel
 *   T3_Motores       T=20 ms   P=3  PWM motores
 *   T4_Comunicacion  T=100 ms  P=4  UART->ESP32
 *   T5_Display       T=500 ms  P=5  LCD
 * ============================================================ */

#include "main.h"
#include "EngTrModel.h"
#include "functions.h"
#include "lcd.h"
#include "motor.h"
#include "ports.h"
#include "pwm.h"
#include "timer.h"
#include "uart.h"
#include "scheduler.h"
#include "tasks.h"

/* -------------------------------------------------------
 * ISR de TIM3 — tick del scheduler
 * Unico punto donde se genera la calendarizacion.
 * ------------------------------------------------------- */
void TIM3_IRQHandler(void) {
    if (TIM3->SR & (0x1UL << 0U)) {
        TIM3->SR &= ~(0x1UL << 0U);
        Scheduler_Tick();
    }
}

int main(void) {
    /* --- Inicializacion del hardware --- */
    SystemClock_Config();
    DWT_Init();               /* medicion de WCET con ciclos de CPU   */

    USART1_Init();            /* UART hacia ESP32 (T4)                */
    ADC1_GPIO_Init();         /* PA0 como entrada analogica (T1)      */
    ADC1_Init();              /* conversion continua con interrupcion  */
    EXT_Button_Init();        /* boton de freno PC0 (T1)              */

    PWM_GPIO_Init();          /* AF pines para TIM2/TIM4              */
    TIM2_PWM_Init();          /* PWM motores M1 (PA1) y M2 (PB10)    */
    TIM4_PWM_Init();          /* PWM motores M3 (PB8)  y M4 (PB9)    */
    Motor_GPIO_Init();        /* pines de direccion PC2-PC9 (T3)      */
    Motor_All_Coast();        /* estado inicial: motores en reposo     */

    LCD_Init();               /* LCD 16x2 via I2C (T5)                */
    LCD_Set_Cursor(1, 1);
    LCD_Put_Str("JohnDeere Robot ");
    LCD_Set_Cursor(2, 1);
    LCD_Put_Str("Iniciando...    ");

    EngTrModel_initialize();  /* modelo Simulink (T2)                 */

    /* --- Arranca el scheduler (debe ser lo ultimo) --- */
    TIM3_Init();
    TIM3_1ms_Interrupt_Config();

    /* --- Loop principal: solo despacha tareas --- */
    for (;;) {
        Scheduler_Run();
    }
}
