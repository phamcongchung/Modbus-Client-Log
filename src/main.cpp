#include <Arduino.h>
#include <esp_task_wdt.h>
#include "ConfigManager.h"
#include "RemoteLogger.h"
#include "ModbusCom.h"
#include "globals.h"
#include "SDLogger.h"
#include "SIM.h"
#include "GPS.h"
#include "RTC.h"

#define TASK_WDT_TIMEOUT 60

SemaphoreHandle_t mutex;

static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;

// External vectors for probe IDs, Modbus registers, and probe data
std::vector<int> probeId;
std::vector<uint16_t> modbusReg;
std::vector<ProbeData> probeData;

// GPRSS credentials
String apn;
String simPin;
String gprsUser;
String gprsPass;
// MQTT credentials
String topic;
String broker;
String brokerUser;
String brokerPass;
uint16_t port;
// GPS data
float latitude, longitude;
String latDir, longDir, altitude, speed;
// Create a string to hold the formatted MAC address and log data
uint8_t mac[6];
char macAdr[18];
char dateString[20];
char timeString[20];
char logString[300];
// Real-time variables
RtcDateTime now, compiled;
RtcDS3231<TwoWire> Rtc(Wire);
// Stores the last time an action was triggered
unsigned long previousMillis = 0;

// Task handles
static TaskHandle_t gpsTaskHandle = NULL;
static TaskHandle_t modbusTaskHandle = NULL;
static TaskHandle_t rtcTaskHandle = NULL;
static TaskHandle_t pushTaskHandle = NULL;
static TaskHandle_t logTaskHandle = NULL;

// Task delay times
const TickType_t gpsDelay = pdMS_TO_TICKS(5000);
const TickType_t modbusDelay = pdMS_TO_TICKS(5000);
const TickType_t rtcDelay = pdMS_TO_TICKS(5000);
const TickType_t logDelay = pdMS_TO_TICKS(5000);
const TickType_t pushDelay = pdMS_TO_TICKS(5000);

// Task prototypes
void readGPS(void *pvParameters);
void readModbus(void *pvParameters);
void checkRTC(void *pvParameters);
void remotePush(void *pvParameters);
void localLog(void *pvParameters);

void setup() {
  Serial.begin(115200);

  // Initialize the Task Watchdog Timer
  esp_task_wdt_init(TASK_WDT_TIMEOUT, true);
  // Initialize mutex
  mutex = xSemaphoreCreateMutex();

  sdInit();
  getNetworkConfig();
  getTankConfig();
  vTaskDelay(pdMS_TO_TICKS(1000));
  modbusInit();
  vTaskDelay(pdMS_TO_TICKS(1000));
  modemInit();
  vTaskDelay(pdMS_TO_TICKS(1000));
  mqttInit();
  vTaskDelay(pdMS_TO_TICKS(1000));
  gpsInit();
  vTaskDelay(pdMS_TO_TICKS(1000));
  rtcInit();
  vTaskDelay(pdMS_TO_TICKS(1000));

  // Retrieve MAC address
  esp_efuse_mac_get_default(mac);
  // Format the MAC address into the string
  sprintf(macAdr, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  // Print the MAC address
  Serial.printf("ESP32 MAC Address: %s\r\n", macAdr);
  
  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(readGPS, "Read GPS", 2048, NULL, 1, &gpsTaskHandle, pro_cpu);
  xTaskCreatePinnedToCore(readModbus, "Read Modbus", 3072, NULL, 1, &modbusTaskHandle, app_cpu);
  xTaskCreatePinnedToCore(checkRTC, "Check RTC", 2048, NULL, 2, &rtcTaskHandle, app_cpu);
  xTaskCreatePinnedToCore(localLog, "Log to SD", 3072, NULL, 1, &logTaskHandle, pro_cpu);
  xTaskCreatePinnedToCore(remotePush, "Push to MQTT", 3072, NULL, 1, &pushTaskHandle, app_cpu);

  vTaskDelete(NULL);
}

void loop() {
  
}

void readGPS(void *pvParameters){
  while(1){
    gpsUpdate();
    xTaskNotifyGive(pushTaskHandle);
    xTaskNotifyGive(logTaskHandle);
    vTaskDelay(gpsDelay);
  }
}

void readModbus(void *pvParameters) {
  while(1){
    readModbus();
    xTaskNotifyGive(pushTaskHandle);
    xTaskNotifyGive(logTaskHandle);
    vTaskDelay(modbusDelay);
  }
}

void remotePush(void *pvParameters){
  while(1){
    if(modemConnect){
      remotePush();
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
    vTaskDelay(pushDelay);
  }
}

void localLog(void *pvParameters){
  while(1){
    dataLog();
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    vTaskDelay(logDelay);
  }
}

void checkRTC(void *pvParameters){
  while(1){
    now = Rtc.GetDateTime();
    getTime(now);
    xTaskNotifyGive(pushTaskHandle);
    xTaskNotifyGive(logTaskHandle);
    vTaskDelay(rtcDelay);
  }
}
