#include <Arduino.h>
#include "ModbusCom.h"
#include "globals.h"
#include "SDCard.h"
#include "Config.h"
#include "Modem.h"
#include "MQTT.h"
#include "GPS.h"
#include "RTC.h"

SemaphoreHandle_t timeMutex;

static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;

// Stores the last time an action was triggered
unsigned long previousMillis = 0;

// Task handles
static TaskHandle_t lteTaskHandle = NULL;
static TaskHandle_t gpsTaskHandle = NULL;
static TaskHandle_t modbusTaskHandle = NULL;
static TaskHandle_t rtcTaskHandle = NULL;
static TaskHandle_t pushTaskHandle = NULL;
static TaskHandle_t logTaskHandle = NULL;

// Task delay times
const TickType_t lteDelay = pdMS_TO_TICKS(5000);
const TickType_t gpsDelay = pdMS_TO_TICKS(1000);
const TickType_t modbusDelay = pdMS_TO_TICKS(1000);
const TickType_t rtcDelay = pdMS_TO_TICKS(60000);
const TickType_t logDelay = pdMS_TO_TICKS(5000);
const TickType_t pushDelay = pdMS_TO_TICKS(1000);

// Task prototypes
void connectLTE(void *pvParameters);
void readGPS(void *pvParameters);
void readModbus(void *pvParameters);
void checkRTC(void *pvParameters);
void remotePush(void *pvParameters);
void localLog(void *pvParameters);

void setup() {
  // Initialize mutex
  timeMutex = xSemaphoreCreateMutex();

  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(connectLTE, "Connect LTE", 2048, NULL, 0, &lteTaskHandle, pro_cpu);
  xTaskCreatePinnedToCore(readGPS, "Read GPS", 2048, NULL, 1, &gpsTaskHandle, pro_cpu);
  xTaskCreatePinnedToCore(readModbus, "Read Modbus", 1280, NULL, 2, &modbusTaskHandle, pro_cpu);
  xTaskCreatePinnedToCore(checkRTC, "Check RTC", 2048, NULL, 0, &rtcTaskHandle, app_cpu);
  xTaskCreatePinnedToCore(localLog, "Log to SD", 3072, NULL, 1, &logTaskHandle, app_cpu);
  xTaskCreatePinnedToCore(remotePush, "Connect MQTT", 2048, NULL, 2, &pushTaskHandle, app_cpu);

  vTaskDelete(NULL);
}

void loop() {
  
}

void readGPS(void *pvParameters){
  while(1){
    gpsUpdate();
    vTaskDelay(gpsDelay);
  }
}

void readModbus(void *pvParameters) {
  while(1){
    readModbus();
    vTaskDelay(modbusDelay);
  }
}

void remotePush(void *pvParameters){
  while(1){
    if (xSemaphoreTake(timeMutex, portMAX_DELAY) == pdTRUE) {
      remotePush();
    }
    vTaskDelay(pushDelay);
  }
}

void localLog(void *pvParameters){
  while(1){
    if (xSemaphoreTake(timeMutex, portMAX_DELAY) == pdTRUE) {
      localLog();
    }

    vTaskDelay(logDelay);
  }
}

void checkRTC(void *pvParameters){
  while(1){
    if (xSemaphoreTake(timeMutex, portMAX_DELAY) == pdTRUE) {
      getTime(now);
      xSemaphoreGive(timeMutex);
    }
    vTaskDelay(rtcDelay);
  }
}
