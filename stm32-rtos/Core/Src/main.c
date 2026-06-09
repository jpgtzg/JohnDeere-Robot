/* **************** START *********************** */
/* Libraries, Definitions and Global Declarations */
#include "main.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "semphr.h"
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

static SensorData_t sensor_data = {0};
static ModelOutput_t model_output = {0};
static SemaphoreHandle_t hSensorMutex;
static SemaphoreHandle_t hModelMutex;
static EventGroupHandle_t hEvents;

void TASK_Sensor(void *pvParameters);
void TASK_CommRx(void *pvParameters);
void TASK_ControlMotor(void *pvParameters);
void TASK_Display(void *pvParameters);
void TASK_Comm(void *pvParameters);

TaskHandle_t hSensor, hCommRx, hControlMotor, hDisplay, hComm;

void USER_SystemClock_Config(void);

/* Superloop structure */
int main(void) {
  HAL_Init();
  USER_SystemClock_Config();

  /*
   *  T1 TASK_Sensor:       priority 4, 20 ms
   *  T2 TASK_CommRx:       priority 4,  5 ms (polls UART for incoming commands)
   *  T3 TASK_ControlMotor: priority 3, 40 ms
   *  T4 TASK_Comm:         priority 2, 100 ms
   *  T5 TASK_Display:      priority 1, 500 ms                               */
  hSensorMutex = xSemaphoreCreateMutex();
  hModelMutex  = xSemaphoreCreateMutex();
  hEvents      = xEventGroupCreate();

  USART1_Init();

  xTaskCreate(TASK_Sensor,       "Sensor",       256, NULL, 4, &hSensor);
  xTaskCreate(TASK_CommRx,       "CommRx",       256, NULL, 4, &hCommRx);
  xTaskCreate(TASK_ControlMotor, "ControlMotor", 256, NULL, 3, &hControlMotor);
  xTaskCreate(TASK_Comm,         "Comm",         256, NULL, 2, &hComm);
  xTaskCreate(TASK_Display,      "Display",      512, NULL, 1, &hDisplay);

  vTaskStartScheduler();

  for (;;)
    ;
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
  ADC1_GPIO_Init();
  ADC1_Init();
  EXT_Button_Init();

  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    if (EXT_BUTTON) {
      vTaskDelay(10);
      if (EXT_BUTTON){
        xEventGroupSetBits(hEvents, EVT_BRAKE);
      } else {
        xEventGroupClearBits(hEvents, EVT_BRAKE);
      }
    } else {
      xEventGroupClearBits(hEvents, EVT_BRAKE);
    }

    ADC1->CR2 |= (0x1UL << 22U);
    while (!(ADC1->SR & (0x1UL << 1U)))
    ;
    
    xSemaphoreTake(hSensorMutex, portMAX_DELAY);
    sensor_data.adc_value = ADC1->DR & 0xFFFF;
    xSemaphoreGive(hSensorMutex);

    vTaskDelayUntil(&xLastWakeTime, 20);
  }
}

void TASK_ControlMotor(void *pvParameters) {
  EngTrModel_initialize();
  PWM_GPIO_Init();
  TIM2_PWM_Init();
  TIM4_PWM_Init();
  Motor_GPIO_Init();
  Motor_All_Forward();

  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    SensorData_t sens;
    xSemaphoreTake(hSensorMutex, portMAX_DELAY);
    sens = sensor_data;
    xSemaphoreGive(hSensorMutex);

    EventBits_t bits = xEventGroupGetBits(hEvents);
    if (bits & EVT_REMOTE_MODE) {
      EngTrModel_U.Throttle    = sens.remote_throttle;
      EngTrModel_U.BrakeTorque = (bits & EVT_REMOTE_BRAKE) ? 150.0 : 0.0;
    } else {
      EngTrModel_U.Throttle    = 1.5f + ((float)sens.adc_value / 4095.0f) * 98.5f;
      EngTrModel_U.BrakeTorque = (bits & EVT_BRAKE) ? 150.0 : 0.0;
    }
    EngTrModel_step();

    xSemaphoreTake(hModelMutex, portMAX_DELAY);
    model_output.vehicle_speed = EngTrModel_Y.VehicleSpeed;
    model_output.engine_speed  = EngTrModel_Y.EngineSpeed;
    model_output.gear          = EngTrModel_Y.Gear;
    xSemaphoreGive(hModelMutex);

    uint8_t duty = (uint8_t)((EngTrModel_Y.VehicleSpeed / 140.0) * 100.0);
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
    xSemaphoreTake(hModelMutex, portMAX_DELAY);
    out = model_output;
    xSemaphoreGive(hModelMutex);

    char buffer[32];
    uint16_t len;

    len = snprintf(buffer, sizeof(buffer), "VS:%.2f\r\n",
                   out.vehicle_speed);
    USART1_Transmit((uint8_t *)buffer, len);

    len = snprintf(buffer, sizeof(buffer), "ES:%.2f\r\n",
                   out.engine_speed);
    USART1_Transmit((uint8_t *)buffer, len);

    len = snprintf(buffer, sizeof(buffer), "GR:%.2f\r\n", out.gear);
    USART1_Transmit((uint8_t *)buffer, len);

    vTaskDelayUntil(&xLastWakeTime, 100);
  }
}

void TASK_CommRx(void *pvParameters) {
  static char buf[32];
  uint8_t idx = 0;

  for (;;) {
    while (USART1->SR & (0x1UL << 5U)) {
      char c = (char)(USART1->DR & 0xFF);
      if (c == '\n' || idx >= sizeof(buf) - 1) {
        buf[idx] = '\0';
        if (idx > 0 && buf[idx - 1] == '\r') buf[--idx] = '\0';

        if (strncmp(buf, "MD:", 3) == 0) {
          if (strncmp(buf + 3, "REMOTE", 6) == 0) {
            xEventGroupSetBits(hEvents, EVT_REMOTE_MODE);
            xEventGroupClearBits(hEvents, EVT_REMOTE_BRAKE);
          } else {
            xEventGroupClearBits(hEvents, EVT_REMOTE_MODE | EVT_REMOTE_BRAKE);
          }
        } else if (strncmp(buf, "AC:", 3) == 0) {
          float val = atof(buf + 3);
          float throttle = 1.5f + (val / 100.0f) * 98.5f;
          xSemaphoreTake(hSensorMutex, portMAX_DELAY);
          sensor_data.remote_throttle = throttle;
          xSemaphoreGive(hSensorMutex);
        } else if (strncmp(buf, "BR:", 3) == 0) {
          float val = atof(buf + 3);
          if (val > 0.0f)
            xEventGroupSetBits(hEvents, EVT_REMOTE_BRAKE);
          else
            xEventGroupClearBits(hEvents, EVT_REMOTE_BRAKE);
        }

        idx = 0;
      } else if (c != '\r') {
        buf[idx++] = c;
      }
    }
    vTaskDelay(5);
  }
}

void TASK_Display(void *pvParameters) {
  LCD_Init();
  LCD_Clear();

  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    SensorData_t  sens;
    ModelOutput_t out;
    EventBits_t   bits = xEventGroupGetBits(hEvents);

    xSemaphoreTake(hSensorMutex, portMAX_DELAY);
    sens = sensor_data;
    xSemaphoreGive(hSensorMutex);

    xSemaphoreTake(hModelMutex, portMAX_DELAY);
    out = model_output;
    xSemaphoreGive(hModelMutex);

    char line[17];
    snprintf(line, sizeof(line), "Duty:%4.0f%% G:%-3u", (double)sens.adc_value,
             (unsigned)out.gear);
    LCD_Set_Cursor(1, 1);
    LCD_Put_Str(line);

    snprintf(line, sizeof(line), "RPM:%7.1f B:%u  ", out.engine_speed,
             (unsigned)!!(bits & EVT_BRAKE));
    LCD_Set_Cursor(2, 1);
    LCD_Put_Str(line);

    vTaskDelayUntil(&xLastWakeTime, 500);
  }
}
