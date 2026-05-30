#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <stdint.h>

/* =========================================================
 * Calendarizacion ciclica con tick de 1 ms 
 *
 * Periodos de las Tareas:
 *   T1_Sensores      10 ms   — muestreo ADC + boton
 *   T2_Controlador   20 ms   — paso del modelo EngTrModel
 *   T3_Motores       20 ms   — actualizacion de PWM / motores
 *   T4_Comunicacion 100 ms   — transmision UART- ESP32
 *   T5_Display      500 ms   — actualizacion LCD
 * ========================================================= */

#define PERIOD_SENSORES       10U
#define PERIOD_CONTROLADOR    20U
#define PERIOD_MOTORES        20U
#define PERIOD_COMUNICACION  100U
#define PERIOD_DISPLAY       500U

extern volatile uint8_t flag_sensores;
extern volatile uint8_t flag_controlador;
extern volatile uint8_t flag_motores;
extern volatile uint8_t flag_comunicacion;
extern volatile uint8_t flag_display;

/* Llamado desde TIM3_IRQHandler cada 1 ms */
void Scheduler_Tick(void);

/*  despacha tareas por prioridad RMS */
void Scheduler_Run(void);

#endif /* SCHEDULER_H_ */
