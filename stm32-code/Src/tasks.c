#include "tasks.h"
#include "functions.h"
#include "uart.h"
#include "pwm.h"
#include "motor.h"
#include "lcd.h"
#include "EngTrModel.h"
#include "ports.h"
#include <stdio.h>

/* -------------------------------------------------------
 * Variables globales
 * ------------------------------------------------------- */
volatile uint16_t g_adc_raw         = 0;
volatile double   g_throttle_pct    = 0.0;
volatile double   g_brake_torque    = 0.0;
volatile double   g_vehicle_speed   = 0.0;
volatile double   g_engine_speed    = 0.0;
volatile double   g_gear            = 0.0;
volatile uint8_t  g_motor_duty      = 0;

/* Peor Tiempo de Ejecucion medido */
volatile uint32_t wcet_sensores     = 0;
volatile uint32_t wcet_controlador  = 0;
volatile uint32_t wcet_motores      = 0;
volatile uint32_t wcet_comunicacion = 0;
volatile uint32_t wcet_display      = 0;

/* -------------------------------------------------------
 * Contador de ciclos 
 * Resolucion: 1 ciclo = 15.6 ns a 64 MHz.
 * CYCLES_TO_US convierte ciclos a microsegundos.
 * ------------------------------------------------------- */
#define DWT_DEMCR  (*(volatile uint32_t*)0xE000EDFC)
#define DWT_CTRL   (*(volatile uint32_t*)0xE0001000)
#define DWT_CYCCNT (*(volatile uint32_t*)0xE0001004)
#define CYCLES_TO_US(c) ((uint32_t)((c) / 64U))

void DWT_Init(void) {
    DWT_DEMCR  |= (1UL << 24);  /* habilita TRCENA */
    DWT_CYCCNT  = 0;
    DWT_CTRL   |= (1UL << 0);   /* habilita CYCCNTENA */
}

static inline uint32_t dwt_now(void) { return DWT_CYCCNT; }

/* =============================================================
 * T1 — Task_Sensores
 * Periodo: 10 ms   |   Prioridad: 1
 * Lee el ADC y el boton externo.
 * ============================================================= */
void Task_Sensores(void) {
    uint32_t t0 = dwt_now();

    g_adc_raw       = adc_value;
    g_throttle_pct  = 1.5 + ((double)g_adc_raw / 4095.0) * 98.5;
    g_brake_torque  = EXT_BUTTON ? 100.0 : 0.0;

    uint32_t elapsed = CYCLES_TO_US(dwt_now() - t0);
    if (elapsed > wcet_sensores) wcet_sensores = elapsed;
}

/* =============================================================
 * T2 — Task_Controlador
 * Periodo: 20 ms   |   Prioridad: 2
 * Ejecuta un paso del modelo Simulink EngTrModel.
 * ============================================================= */
void Task_Controlador(void) {
    uint32_t t0 = dwt_now();

    EngTrModel_U.Throttle    = g_throttle_pct;
    EngTrModel_U.BrakeTorque = g_brake_torque;
    EngTrModel_step();

    g_vehicle_speed = EngTrModel_Y.VehicleSpeed;
    g_engine_speed  = EngTrModel_Y.EngineSpeed;
    g_gear          = EngTrModel_Y.Gear;

    uint32_t elapsed = CYCLES_TO_US(dwt_now() - t0);
    if (elapsed > wcet_controlador) wcet_controlador = elapsed;
}

/* =============================================================
 * T3 — Task_Motores
 * Periodo: 20 ms   |   Prioridad: 3
 * Actualiza la velocidad de los 4 motores segun g_vehicle_speed.
 * Aplica frenado si g_brake_torque > 0.
 * ============================================================= */
void Task_Motores(void) {
    uint32_t t0 = dwt_now();

    if (g_brake_torque > 0.0) {
        Motor_All_Coast();
        g_motor_duty = 0;
    } else {
        Motor_All_Forward();
        double duty_d = (g_vehicle_speed / 140.0) * 100.0;
        if (duty_d > 100.0) duty_d = 100.0;
        g_motor_duty = (uint8_t)duty_d;
        Change_Duty_Cycle_M1(g_motor_duty);
        Change_Duty_Cycle_M2(g_motor_duty);
        Change_Duty_Cycle_M3(g_motor_duty);
        Change_Duty_Cycle_M4(g_motor_duty);
    }

    uint32_t elapsed = CYCLES_TO_US(dwt_now() - t0);
    if (elapsed > wcet_motores) wcet_motores = elapsed;
}

/* =============================================================
 * T4 — Task_Comunicacion
 * Periodo: 100 ms   |   Prioridad: 4
 * Transmite telemetria al ESP32 via USART1 (115200 baud).
 * ============================================================= */
void Task_Comunicacion(void) {
    uint32_t t0 = dwt_now();

    char    buf[32];
    uint16_t len;

    len = (uint16_t)snprintf(buf, sizeof(buf), "VS:%.2f\r\n", g_vehicle_speed);
    USART1_Transmit((uint8_t *)buf, len);

    len = (uint16_t)snprintf(buf, sizeof(buf), "ES:%.2f\r\n", g_engine_speed);
    USART1_Transmit((uint8_t *)buf, len);

    len = (uint16_t)snprintf(buf, sizeof(buf), "GR:%.2f\r\n", g_gear);
    USART1_Transmit((uint8_t *)buf, len);

    uint32_t elapsed = CYCLES_TO_US(dwt_now() - t0);
    if (elapsed > wcet_comunicacion) wcet_comunicacion = elapsed;
}

/* =============================================================
 * T5 — Task_Display
 * Periodo: 500 ms   |   Prioridad: 5
 * Actualiza la pantalla LCD 16x2 con acelerador, marcha y RPM.
 * ============================================================= */
void Task_Display(void) {
    uint32_t t0 = dwt_now();

    char line[17];

    snprintf(line, sizeof(line), "Ac:%4.0f   G:%1u",
             g_throttle_pct, (unsigned)g_gear);
    LCD_Set_Cursor(1, 1);
    LCD_Put_Str(line);

    snprintf(line, sizeof(line), "RPM:%8.1f  ", g_engine_speed);
    LCD_Set_Cursor(2, 1);
    LCD_Put_Str(line);

    uint32_t elapsed = CYCLES_TO_US(dwt_now() - t0);
    if (elapsed > wcet_display) wcet_display = elapsed;
}
