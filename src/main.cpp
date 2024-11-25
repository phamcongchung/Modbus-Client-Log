#include <Arduino.h>
#include <esp_task_wdt.h>
#include "ConfigManager.h"
#include "RemoteLogger.h"
#include "ModbusCom.h"
#include "SDLogger.h"
#include "SIM.h"
#include "GPS.h"
#include "RTC.h"

#define TASK_WDT_TIMEOUT 60

SemaphoreHandle_t mutex;
HardwareSerial SerialAT(1);
ConfigManager config;
ModbusCom modbus(config);
SIM sim(config, SerialAT);
GPS gps(sim);
RTC rtc(Wire);
SDLogger sd(config, modbus, gps, rtc);
RemoteLogger remote(config, sim, gps, rtc, modbus);

static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;

uint8_t mac[6];
char macAdr[18];

// Stores the last time an action was triggered
unsigned long previousMillis = 0;

void GPS::error(const char* err){
  sd.errLog(err);
}

void ModbusCom::error(const char* err){
  sd.errLog(err);
}

void SIM::gprsErr(const char* err){
  sd.errLog(err);
}

void RemoteLogger::mqttErr(int state){
  switch (state) {
    case MQTT_CONNECTION_TIMEOUT:
      Serial.println("connection timed out");
      sd.errLog("MQTT connection timed out");
      break;
    case MQTT_CONNECTION_LOST:
      Serial.println("connection lost");
      sd.errLog("MQTT connection lost");
      break;
    case MQTT_CONNECT_FAILED:
      Serial.println("connection failed");
      sd.errLog("MQTT connection failed");
      break;
    case MQTT_DISCONNECTED:
      Serial.println("disconnected");
      sd.errLog("MQTT disconnected");
      break;
    case MQTT_CONNECT_BAD_PROTOCOL:
      Serial.println("bad protocol");
      sd.errLog("MQTT bad protocol");
      break;
    case MQTT_CONNECT_BAD_CLIENT_ID:
      Serial.println("bad Client ID");
      sd.errLog("MQTT bad Client ID");
      break;
    case MQTT_CONNECT_UNAVAILABLE:
      Serial.println("server unavailable");
      sd.errLog("MQTT server unavailable");
      break;
    case MQTT_CONNECT_BAD_CREDENTIALS:
      Serial.println("bad username or password");
      sd.errLog("MQTT bad username or password");
      break;
    case MQTT_CONNECT_UNAUTHORIZED:
      Serial.println("unauthorized");
      sd.errLog("MQTT unauthorized");
      break;
    default:
      Serial.println("unknown error");
      sd.errLog("MQTT unknown error");
      break;
  }
}

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

  // Retrieve MAC address
  esp_efuse_mac_get_default(mac);
  // Format the MAC address into the string
  sprintf(macAdr, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  // Print the MAC address
  Serial.printf("ESP32 MAC Address: %s\r\n", macAdr);

  sd.init();
  config.getNetwork();
  config.getTank();
  vTaskDelay(pdMS_TO_TICKS(1000));
  modbus.init();
  vTaskDelay(pdMS_TO_TICKS(1000));
  sim.init();
  vTaskDelay(pdMS_TO_TICKS(1000));
  remote.init(macAdr);
  vTaskDelay(pdMS_TO_TICKS(1000));
  gps.init();
  vTaskDelay(pdMS_TO_TICKS(1000));
  rtc.init();
  vTaskDelay(pdMS_TO_TICKS(1000));
  
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
    gps.update();
    xTaskNotifyGive(pushTaskHandle);
    xTaskNotifyGive(logTaskHandle);
    vTaskDelay(gpsDelay);
  }
}

void readModbus(void *pvParameters) {
  while(1){
    modbus.read();
    xTaskNotifyGive(pushTaskHandle);
    xTaskNotifyGive(logTaskHandle);
    vTaskDelay(modbusDelay);
  }
}

void remotePush(void *pvParameters){
  while(1){
    if(sim.connect()){
      remote.push(macAdr);
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
    vTaskDelay(pushDelay);
  }
}

void localLog(void *pvParameters){
  while(1){
    sd.log();
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    vTaskDelay(logDelay);
  }
}

void checkRTC(void *pvParameters){
  while(1){
    rtc.getTime();
    xTaskNotifyGive(pushTaskHandle);
    xTaskNotifyGive(logTaskHandle);
    vTaskDelay(rtcDelay);
  }
}
