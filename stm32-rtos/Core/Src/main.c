/* **************** START *********************** */
/* Libraries, Definitions and Global Declarations */
#include "main.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "portmacro.h"
#include "task.h"
#include "queue.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lcd.h"
#include "motor.h"
#include "pwm.h"
#include "uart.h"

typedef struct {
  float    xspeed;
  float    yspeed;
} ControlData_t;

static QueueHandle_t hControlQueue;
static QueueHandle_t hUartRxQueue;

void TASK_Sensor(void *pvParameters);
void TASK_ControlMotor(void *pvParameters);

void DifferentialDrive(ControlData_t *control);

TaskHandle_t hSensor, hControlMotor;

void USER_SystemClock_Config(void);

int main(void) {
  HAL_Init();
  USER_SystemClock_Config();

  USART1_Init();
  PWM_GPIO_Init();
  TIM2_PWM_Init();
  TIM4_PWM_Init();
  Motor_GPIO_Init();
  LCD_Init();
  LCD_Clear();

  hControlQueue  = xQueueCreate(1,  sizeof(ControlData_t));
  hUartRxQueue  = xQueueCreate(64, sizeof(uint8_t));

  xTaskCreate(TASK_Sensor,       "Sensor",       256, NULL, 4, &hSensor);
  xTaskCreate(TASK_ControlMotor, "ControlMotor", 256, NULL, 3, &hControlMotor);

  /* Enable USART1 RX interrupt — priority must be >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY */
  NVIC_SetPriority(USART1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
  NVIC_EnableIRQ(USART1_IRQn);
  USART1->CR1 |= (0x1UL << 5U); // RXNEIE: interrupt on RXNE and ORE

  vTaskStartScheduler();

  for (;;)
    ;
}

void USART1_IRQHandler(void) {
  BaseType_t woken = pdFALSE;
  uint32_t sr = USART1->SR;

  if (sr & (0x1UL << 3U)) {  // ORE: read DR to clear, discard the byte
    (void)USART1->DR;
    return;
  }
  if (sr & (0x1UL << 5U)) {  // RXNE: a byte is ready
    uint8_t byte = (uint8_t)USART1->DR;
    xQueueSendFromISR(hUartRxQueue, &byte, &woken);
    portYIELD_FROM_ISR(woken);
  }
}

void USER_SystemClock_Config(void) {
  FLASH->ACR &= ~(0x5UL << 0U);
  FLASH->ACR |= (0x2UL << 0U); // 2 wait states for > 48 MHz
  RCC->CFGR &= ~(0x1UL << 16U) & ~(0x7UL << 11U) & ~(0x3UL << 8U);
  RCC->CFGR |= (0xFUL << 18U)   // PLL x16
               | (0x4UL << 8U); // APB1 /2
  RCC->CR |= (0x1UL << 24U);    // PLL ON
  while (!(RCC->CR & (0x1UL << 25U)))
    ; // wait for PLL lock
  RCC->CFGR &= ~(0x3UL << 0U);
  RCC->CFGR |= (0x2UL << 0U); // PLL as system clock
  while (0x8UL != (RCC->CFGR & 0xCUL))
    ; // wait for switch
  SystemCoreClock = 64000000U;
}

void TASK_Sensor(void *pvParameters) {
  static char    cmd_buf[32];
  static uint8_t cmd_idx = 0;
  uint8_t        rx_char;
  ControlData_t  ctrl = {.xspeed = 0.0f, .yspeed = 0.0f};

  for (;;) {
    /* ---- UART RX: drain the interrupt-fed byte queue, block until data arrives ---- */
    xQueueReceive(hUartRxQueue, &rx_char, portMAX_DELAY);
    do {
      if (rx_char == '\r' || rx_char == '\n') {
        if (cmd_idx > 0) {
          cmd_buf[cmd_idx] = '\0';

          if (strncmp(cmd_buf, "XY:", 3) == 0) {
            char *comma = strchr(cmd_buf + 3, ',');
            if (comma != NULL) {
              *comma = '\0';
              ctrl.xspeed = atof(cmd_buf + 3);
              ctrl.yspeed = atof(comma + 1);
              xQueueOverwrite(hControlQueue, &ctrl);
            }
          }
          cmd_idx = 0;
        }
      } else if (cmd_idx < sizeof(cmd_buf) - 1) {
        cmd_buf[cmd_idx++] = (char)rx_char;
      } else {
        cmd_idx = 0;
      }
    } while (xQueueReceive(hUartRxQueue, &rx_char, 0) == pdTRUE);
  }
}

void TASK_ControlMotor(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    ControlData_t sens;
    xQueuePeek(hControlQueue, &sens, portMAX_DELAY);

    DifferentialDrive(&sens);

    vTaskDelayUntil(&xLastWakeTime, 40);
  }
}


void DifferentialDrive(ControlData_t *control) {
  float left_speed  = control->xspeed - control->yspeed;
  float right_speed = control->xspeed + control->yspeed;


  // TODO: I'm just limiting to 100, but the Duty cycle can be from o to 255

  if (left_speed > 100.0f) left_speed = 100.0f;
  if (left_speed < -100.0f) left_speed = -100.0f;
  if (right_speed > 100.0f) right_speed = 100.0f;
  if (right_speed < -100.0f) right_speed = -100.0f;

  Change_Duty_Cycle_M1((uint8_t)(fabsf(left_speed)));
  Change_Duty_Cycle_M2((uint8_t)(fabsf(left_speed)));
  Change_Duty_Cycle_M3((uint8_t)(fabsf(right_speed)));
  Change_Duty_Cycle_M4((uint8_t)(fabsf(right_speed)));

  if (left_speed >= 0.0f) {
    Motor_Forward(1);
    Motor_Forward(2);
  } else {
    Motor_Reverse(1);
    Motor_Reverse(2);
  }

  if (right_speed >= 0.0f) {
    Motor_Forward(3);
    Motor_Forward(4);
  } else {
    Motor_Reverse(3);
    Motor_Reverse(4);
  }
}
