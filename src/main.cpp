#include <Arduino.h>
#include <Wire.h>
#include <RtcDS3231.h>
#include <esp_task_wdt.h>
#include "ConfigManager.h"
#include "RemoteLogger.h"
#include "ModbusCom.h"
#include "SDLogger.h"
#include "GPS.h"

#define TASK_WDT_TIMEOUT  60
#define SIM_RXD           32
#define SIM_TXD           33
#define SIM_BAUD          115200

#define TINY_GSM_MODEM_SIM7600

#include <TinyGsmClient.h>

HardwareSerial SerialAT(1);
PubSubClient mqtt;
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
RtcDS3231<TwoWire> Rtc;

GPS gps(modem);
ConfigManager config;
ModbusCom modbus(config);
SIM sim(config, SerialAT);
SDLogger sd(config, modbus, gps, rtc);
RemoteLogger remote(config, gps, rtc, modbus, mqtt);

static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;

uint8_t mac[6];
char macAdr[18];
char date[20];
char time[20];
RtcDateTime now;
RtcDateTime compiled;

// Stores the last time an action was triggered
unsigned long previousMillis = 0;

void GPS::error(const char* err){
  sd.errLog(err);
}

void ModbusCom::error(const char* err){
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
  delay(1000);
  modbus.init();
  delay(1000);

  SerialAT.begin(SIM_BAUD, SERIAL_8N1, SIM_RXD, SIM_TXD);
  Serial.println("Initializing modem...");
  //modem.restart();
  if (modem.getSimStatus() != 1) {
    Serial.println("SIM not ready, checking for PIN...");
    if (modem.getSimStatus() == 2){
      Serial.println("SIM PIN required.");
      // Send the PIN to the modem
      modem.sendAT("+CPIN=" + config.simPin);
      delay(1000);
      if (modem.getSimStatus() != 1) {
        Serial.println("Failed to unlock SIM.");
        return;
      }
      Serial.println("SIM unlocked successfully.");
    } else {
      sd.errLog("SIM not detected or unsupported status");
      Serial.println("SIM not detected or unsupported status");
      return;
    }
  }
  delay(1000);
  remote.init(macAdr);
  delay(1000);
  gps.init();
  delay(1000);
  rtc.init();
  delay(1000);
  
  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(readGPS, "Read GPS", 3072, NULL, 1, &gpsTaskHandle, pro_cpu);
  xTaskCreatePinnedToCore(readModbus, "Read Modbus", 3072, NULL, 1, &modbusTaskHandle, pro_cpu);
  xTaskCreatePinnedToCore(checkRTC, "Check RTC", 2048, NULL, 2, &rtcTaskHandle, pro_cpu);
  xTaskCreatePinnedToCore(localLog, "Log to SD", 3072, NULL, 1, &logTaskHandle, app_cpu);
  xTaskCreatePinnedToCore(remotePush, "Push to MQTT", 3072, NULL, 1, &pushTaskHandle, app_cpu);

  // Initialize the Task Watchdog Timer
  esp_task_wdt_init(TASK_WDT_TIMEOUT, true);
  
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

bool connect() {
  Serial.print("Connecting to APN: ");
  Serial.println(config.apn);
  if (!modem.gprsConnect(config.apn.c_str(), config.gprsUser.c_str(), config.gprsPass.c_str())) {
    sd.errLog("GPRS connection failed");
    Serial.println("GPRS connection failed");
    return false;
    modem.restart();
  }
  else {
    Serial.println("GPRS connected");
    return true;
  }
}