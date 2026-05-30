#include "scheduler.h"
#include "tasks.h"

/* Banderas de activacion  */
volatile uint8_t flag_sensores     = 0;
volatile uint8_t flag_controlador  = 0;
volatile uint8_t flag_motores      = 0;
volatile uint8_t flag_comunicacion = 0;
volatile uint8_t flag_display      = 0;

/* Contadores de periodo */
static uint16_t cnt_sensores     = 0;
static uint16_t cnt_controlador  = 0;
static uint16_t cnt_motores      = 0;
static uint16_t cnt_comunicacion = 0;
static uint16_t cnt_display      = 0;

/* -------------------------------------------------------
 * Scheduler_Tick — se llama desde TIM3_IRQHandler
 * Incrementa contadores y activa la bandera al cumplir
 * el periodo de cada tarea.
 * ------------------------------------------------------- */
void Scheduler_Tick(void) {
    if (++cnt_sensores >= PERIOD_SENSORES) {
        cnt_sensores = 0;
        flag_sensores = 1;
    }
    if (++cnt_controlador >= PERIOD_CONTROLADOR) {
        cnt_controlador = 0;
        flag_controlador = 1;
    }
    if (++cnt_motores >= PERIOD_MOTORES) {
        cnt_motores = 0;
        flag_motores = 1;
    }
    if (++cnt_comunicacion >= PERIOD_COMUNICACION) {
        cnt_comunicacion = 0;
        flag_comunicacion = 1;
    }
    if (++cnt_display >= PERIOD_DISPLAY) {
        cnt_display = 0;
        flag_display = 1;
    }
}

/* -------------------------------------------------------
 * Scheduler_Run — se llama en el loop principal.
 * Orden de despacho segun prioridad RMS:
 *   periodo mas corto = prioridad mas alta.
 * ------------------------------------------------------- */
void Scheduler_Run(void) {
    if (flag_sensores)     { flag_sensores = 0;     Task_Sensores(); }
    if (flag_controlador)  { flag_controlador = 0;  Task_Controlador(); }
    if (flag_motores)      { flag_motores = 0;       Task_Motores(); }
    if (flag_comunicacion) { flag_comunicacion = 0;  Task_Comunicacion(); }
    if (flag_display)      { flag_display = 0;       Task_Display(); }
}
