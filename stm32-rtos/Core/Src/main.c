/* **************** START *********************** */
/* Libraries, Definitions and Global Declarations */
#include "main.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "portmacro.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EngTrModel.h"
#include "functions.h"
#include "lcd.h"
#include "motor.h"
#include "pwm.h"
#include "uart.h"

typedef struct {
  uint16_t adc_value;
  float    remote_throttle;
} SensorData_t;

typedef struct {
  double vehicle_speed;
  double engine_speed;
  double gear;
} ModelOutput_t;

#define EVT_BRAKE         (1 << 0)
#define EVT_REMOTE_MODE   (1 << 1)
#define EVT_REMOTE_BRAKE  (1 << 2)

static QueueHandle_t hSensorQueue;
static QueueHandle_t hModelQueue;
static QueueHandle_t hUartRxQueue;
static EventGroupHandle_t hEvents;

void TASK_Sensor(void *pvParameters);
void TASK_ControlMotor(void *pvParameters);
void TASK_Display(void *pvParameters);
void TASK_Comm(void *pvParameters);

TaskHandle_t hSensor, hControlMotor, hDisplay, hComm;

void USER_SystemClock_Config(void);

int main(void) {
  HAL_Init();
  USER_SystemClock_Config();

  ADC1_GPIO_Init();
  ADC1_Init();
  EXT_Button_Init();
  USART1_Init();
  PWM_GPIO_Init();
  TIM2_PWM_Init();
  TIM4_PWM_Init();
  Motor_GPIO_Init();
  Motor_All_Forward();
  EngTrModel_initialize();
  LCD_Init();
  LCD_Clear();

  hSensorQueue  = xQueueCreate(1,  sizeof(SensorData_t));
  hModelQueue   = xQueueCreate(1,  sizeof(ModelOutput_t));
  hUartRxQueue  = xQueueCreate(64, sizeof(uint8_t));
  hEvents       = xEventGroupCreate();

  xTaskCreate(TASK_Sensor,       "Sensor",       256, NULL, 4, &hSensor);
  xTaskCreate(TASK_ControlMotor, "ControlMotor", 256, NULL, 3, &hControlMotor);
  xTaskCreate(TASK_Comm,         "Comm",         256, NULL, 2, &hComm);
  xTaskCreate(TASK_Display,      "Display",      512, NULL, 1, &hDisplay);

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
  SensorData_t   sens = {.adc_value = 0, .remote_throttle = 1.5f};

  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;) {

    /* ---- UART RX: drain the interrupt-fed byte queue ---- */
    while (xQueueReceive(hUartRxQueue, &rx_char, 0) == pdTRUE) {
      if (rx_char == '\r' || rx_char == '\n') {
        if (cmd_idx > 0) {
          cmd_buf[cmd_idx] = '\0';
          if (cmd_idx > 0 && cmd_buf[cmd_idx - 1] == '\r') cmd_buf[--cmd_idx] = '\0';

          if (strncmp(cmd_buf, "MD:", 3) == 0) {
            if (strncmp(cmd_buf + 3, "REMOTE", 6) == 0) {
              xEventGroupSetBits(hEvents, EVT_REMOTE_MODE);
              xEventGroupClearBits(hEvents, EVT_REMOTE_BRAKE);
            } else {
              xEventGroupClearBits(hEvents, EVT_REMOTE_MODE | EVT_REMOTE_BRAKE);
            }
          } else if (strncmp(cmd_buf, "AC:", 3) == 0) {
            sens.remote_throttle = 1.5f + (atof(cmd_buf + 3) / 100.0f) * 98.5f;
            xQueueOverwrite(hSensorQueue, &sens);
          } else if (strncmp(cmd_buf, "BR:", 3) == 0) {
            if (atof(cmd_buf + 3) > 0.0f)
              xEventGroupSetBits(hEvents, EVT_REMOTE_BRAKE);
            else
              xEventGroupClearBits(hEvents, EVT_REMOTE_BRAKE);
          }
          cmd_idx = 0;
        }
      } else if (cmd_idx < sizeof(cmd_buf) - 1) {
        cmd_buf[cmd_idx++] = (char)rx_char;
      } else {
        cmd_idx = 0;
      }
    }

    /* ---- ADC + brake button ---- */
    if (EXT_BUTTON)
      xEventGroupSetBits(hEvents, EVT_BRAKE);
    else
      xEventGroupClearBits(hEvents, EVT_BRAKE);

    ADC1->CR2 |= (0x1UL << 22U);
    while (!(ADC1->SR & (0x1UL << 1U)));

    sens.adc_value = ADC1->DR & 0xFFFF;
    xQueueOverwrite(hSensorQueue, &sens);

    vTaskDelayUntil(&xLastWakeTime, 5);
  }
}

void TASK_ControlMotor(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    SensorData_t sens;
    xQueuePeek(hSensorQueue, &sens, portMAX_DELAY);

    EventBits_t bits = xEventGroupGetBits(hEvents);
    if (bits & EVT_REMOTE_MODE) {
      float throttle = sens.remote_throttle < 1.5f ? 1.5f : sens.remote_throttle;
      EngTrModel_U.Throttle    = throttle;
      EngTrModel_U.BrakeTorque = (bits & EVT_REMOTE_BRAKE) ? 350.0 : 0.0;
    } else {
      EngTrModel_U.Throttle    = 1.5f + ((float)sens.adc_value / 4095.0f) * 98.5f;
      EngTrModel_U.BrakeTorque = (bits & EVT_BRAKE) ? 350.0 : 0.0;
    }

    /* Reinitialize the model if its state has diverged */
    double spd = EngTrModel_Y.VehicleSpeed;
    if (spd != spd || spd > 1e6 || spd < -1e3) {
      EngTrModel_initialize();
    }

    EngTrModel_step();

    double vehicle_speed = EngTrModel_Y.VehicleSpeed;
    if (vehicle_speed < 0.0)   vehicle_speed = 0.0;
    if (vehicle_speed > 140.0) vehicle_speed = 140.0;

    ModelOutput_t out = {
      .vehicle_speed = vehicle_speed,
      .engine_speed  = EngTrModel_Y.EngineSpeed,
      .gear          = EngTrModel_Y.Gear,
    };
    xQueueOverwrite(hModelQueue, &out);

    uint8_t duty = (uint8_t)((vehicle_speed / 140.0) * 100.0);
    Change_Duty_Cycle_M1(duty);
    Change_Duty_Cycle_M2(duty);
    Change_Duty_Cycle_M3(duty);
    Change_Duty_Cycle_M4(duty);

    vTaskDelayUntil(&xLastWakeTime, 40);
  }
}

void TASK_Comm(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    ModelOutput_t out;
    xQueuePeek(hModelQueue, &out, portMAX_DELAY);

    char buffer[32];
    uint16_t len;

    len = snprintf(buffer, sizeof(buffer), "VS:%.2f\r\n", out.vehicle_speed);
    USART1_Transmit((uint8_t *)buffer, len);

    len = snprintf(buffer, sizeof(buffer), "ES:%.2f\r\n", out.engine_speed);
    USART1_Transmit((uint8_t *)buffer, len);

    len = snprintf(buffer, sizeof(buffer), "GR:%.2f\r\n", out.gear);
    USART1_Transmit((uint8_t *)buffer, len);

    vTaskDelayUntil(&xLastWakeTime, 100);
  }
}

void TASK_Display(void *pvParameters) {
  char     line1[17], line2[18];
  uint16_t rpm_val, spd_val;
  uint8_t  gear_val, duty_pct;

  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    ModelOutput_t out;
    xQueuePeek(hModelQueue, &out, portMAX_DELAY);

    duty_pct = (uint8_t)(out.vehicle_speed / 140.0 * 100.0);
    rpm_val  = (uint16_t)out.engine_speed;
    spd_val  = (uint16_t)out.vehicle_speed;
    gear_val = (uint8_t)out.gear;

    /* Fixed-width lines overwrite each field in one write — no flicker */
    snprintf(line1, sizeof(line1), "ACC:%3u   G:%-3u ", duty_pct, gear_val);
    LCD_Set_Cursor(1, 1);
    LCD_Put_Str(line1);

    snprintf(line2, sizeof(line2), "RPM:%5u V:%-4u", rpm_val, spd_val);
    LCD_Set_Cursor(2, 1);
    LCD_Put_Str(line2);

    vTaskDelayUntil(&xLastWakeTime, 200);
  }
}
