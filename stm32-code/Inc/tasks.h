#ifndef TASKS_H_
#define TASKS_H_

#include <stdint.h>

/* =============================================================
 *   ADC ISR   g_adc_raw        T1
 *   T1        g_throttle_pct   T2
 *   T1        g_brake_torque   T2, T3
 *   T2        g_vehicle_speed  T3, T4, T5
 *   T2        g_engine_speed   T4, T5
 *   T2        g_gear           T4, T5
 * ============================================================= */
extern volatile uint16_t g_adc_raw;
extern volatile double   g_throttle_pct;
extern volatile double   g_brake_torque;
extern volatile double   g_vehicle_speed;
extern volatile double   g_engine_speed;
extern volatile double   g_gear;
extern volatile uint8_t  g_motor_duty;

/* Tiempos de ejecucion maximos medidos */
extern volatile uint32_t wcet_sensores;
extern volatile uint32_t wcet_controlador;
extern volatile uint32_t wcet_motores;
extern volatile uint32_t wcet_comunicacion;
extern volatile uint32_t wcet_display;

/* Inicializa el contador de ciclos DWT para medir tiempos */
void DWT_Init(void);

/* Prototipos de Tareas */
void Task_Sensores(void);       /* T1 — periodo 10 ms  — prioridad 1 */
void Task_Controlador(void);    /* T2 — periodo 20 ms  — prioridad 2 */
void Task_Motores(void);        /* T3 — periodo 20 ms  — prioridad 3 */
void Task_Comunicacion(void);   /* T4 — periodo 100 ms — prioridad 4 */
void Task_Display(void);        /* T5 — periodo 500 ms — prioridad 5 */

#endif /* TASKS_H_ */
