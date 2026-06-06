/* **************** START *********************** */
/* Libraries, Definitions and Global Declarations */
#include "main.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include <stdint.h>
#include <stdio.h>

#include "EngTrModel.h"
#include "functions.h"
#include "lcd.h"
#include "motor.h"
#include "pwm.h"
#include "uart.h"
#include "user_uart.h"

volatile uint8_t brake_active = 0;
volatile double duty = 0;
volatile uint16_t adc_value = 0;

void TASK_Sensor(void *pvParameters);
void TASK_Controller(void *pvParameters);
void TASK_Motor(void *pvParameters);
void TASK_Display(void *pvParameters);
void TASK_Comm(void *pvParameters);

TaskHandle_t hSensor, hController, hMotor, hDisplay, hComm;

void USER_SystemClock_Config(void);

/* Superloop structure */
int main(void) {
  HAL_Init();
  USER_SystemClock_Config();

  /*
   *  T1 TASK_Sensor:     priority 5, 20 ms
   *  T2 TASK_Controller: priority 4, 40 ms
   *  T3 TASK_Motor:      priority 3, 40 ms
   *  T4 TASK_Comm:       priority 2, 100 ms
   *  T5 TASK_Display:    priority 1, 500 ms                               */
  xTaskCreate(TASK_Sensor, "Sensor", 256, NULL, 5, &hSensor);
  xTaskCreate(TASK_Controller, "Controller", 256, NULL, 4, &hController);
  xTaskCreate(TASK_Motor, "Motor", 256, NULL, 3, &hMotor);
  xTaskCreate(TASK_Comm, "Comm", 256, NULL, 2, &hComm);
  xTaskCreate(TASK_Display, "Display", 512, NULL, 1, &hDisplay);

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
      brake_active = EXT_BUTTON ? 1 : 0;
    } else {
      brake_active = 0;
    }

    ADC1->CR2 |= (0x1UL << 22U);
    while (!(ADC1->SR & (0x1UL << 1U)))
      ;
    adc_value = ADC1->DR & 0xFFFF;

    vTaskDelayUntil(&xLastWakeTime, 20);
  }
}

void TASK_Controller(void *pvParameters) {
  EngTrModel_initialize();

  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    EngTrModel_U.Throttle = 1.5f + ((float)adc_value / 4095.0f) * 98.5f;
    EngTrModel_U.BrakeTorque = brake_active ? 100.0 : 0.0;
    EngTrModel_step();

    vTaskDelayUntil(&xLastWakeTime, 40);
  }
}

void TASK_Motor(void *pvParameters) {
  PWM_GPIO_Init();
  TIM2_PWM_Init();
  TIM4_PWM_Init();
  Motor_GPIO_Init();
  Motor_All_Forward();

  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    duty = (EngTrModel_Y.VehicleSpeed / 140.0) * 100.0;
    Change_Duty_Cycle_M1((uint8_t)duty);
    Change_Duty_Cycle_M2((uint8_t)duty);
    Change_Duty_Cycle_M3((uint8_t)duty);
    Change_Duty_Cycle_M4((uint8_t)duty);

    vTaskDelayUntil(&xLastWakeTime, 40);
  }
}

void TASK_Comm(void *pvParameters) {
  USART1_Init();

  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    char buffer[32];
    uint16_t len;

    len = snprintf(buffer, sizeof(buffer), "VS:%.2f\r\n",
                   EngTrModel_Y.VehicleSpeed);
    USART1_Transmit((uint8_t *)buffer, len);

    len = snprintf(buffer, sizeof(buffer), "ES:%.2f\r\n",
                   EngTrModel_Y.EngineSpeed);
    USART1_Transmit((uint8_t *)buffer, len);

    len = snprintf(buffer, sizeof(buffer), "GR:%.2f\r\n", EngTrModel_Y.Gear);
    USART1_Transmit((uint8_t *)buffer, len);

    vTaskDelayUntil(&xLastWakeTime, 100);
  }
}

void TASK_Display(void *pvParameters) {
  LCD_Init();
  LCD_Clear();

  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    char line[17];

    snprintf(line, sizeof(line), "Duty:%4.0f%% G:%-3u", (double)duty,
             (unsigned)EngTrModel_Y.Gear);
    LCD_Set_Cursor(1, 1);
    LCD_Put_Str(line);

    snprintf(line, sizeof(line), "RPM:%10.1f  ", EngTrModel_Y.EngineSpeed);
    LCD_Set_Cursor(2, 1);
    LCD_Put_Str(line);

    vTaskDelayUntil(&xLastWakeTime, 500);
  }
}
