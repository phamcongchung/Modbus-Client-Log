#include <Arduino.h>
#include <SD.h>
#include <Wire.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <ModbusRTUClient.h>
#include "ConfigManager.h"
#include "RemoteLogger.h"
#include "LocalLogger.h"
#include "Display.h"
#include "Modem.h"
#include "GPS.h"

#define TASK_WDT_TIMEOUT  60
#define SIM_RXD           32
#define SIM_TXD           33
#define SIM_BAUD          115200
#define RTU_BAUD          9600
#define EEPROM_SIZE       20

HardwareSerial SerialAT(1);
Modem modem(SerialAT, SIM_RXD, SIM_TXD, SIM_BAUD);
TinyGsmClient client(modem);
RemoteLogger remote(client);
GPS gps(modem);
ConfigManager config;
RTC Rtc(Wire);
Display lcd(0x27, 20, 4);

static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;

uint8_t mac[6];
char macAdr[18];
char dateTime[20];
char logString[300];
String timeStamp;
String bearerToken;
RtcDateTime run_time, compiled;
std::vector<ProbeData> probeData;

bool processCsv(fs::FS &fs, const char* path, int fileNo);

// Task handles
static TaskHandle_t gpsTaskHandle = NULL;
static TaskHandle_t modbusTaskHandle = NULL;
static TaskHandle_t rtcTaskHandle = NULL;
static TaskHandle_t pushTaskHandle = NULL;
static TaskHandle_t logTaskHandle = NULL;

/*TickType_t gpsLastWake = xTaskGetTickCount();
TickType_t modbusLastWake = xTaskGetTickCount();
TickType_t rtcLastWake = xTaskGetTickCount();
TickType_t remoteLastWake = xTaskGetTickCount();
TickType_t localLastWake = xTaskGetTickCount();
const TickType_t interval = pdMS_TO_TICKS(5000);
const TickType_t modbus_interval = pdMS_TO_TICKS(10000);*/
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
/****************************************************************************************************/
void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  esp_efuse_mac_get_default(mac);
  sprintf(macAdr, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.printf("ESP32 MAC Address: %s\r\n", macAdr);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  /************************************** Initialize DS3231 *******************************************/
  Rtc.Begin();
  compiled = RtcDateTime(__DATE__, __TIME__);
  Serial.println();
  if (!Rtc.IsDateTimeValid()){
    if(Rtc.LastError() != 0){
      Serial.println("RTC communication error");
      Serial.println(Rtc.LastError());
    } else {
      Rtc.SetDateTime(compiled);
    }
  }
  if (!Rtc.GetIsRunning()){
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }
  // Compare RTC time with compile time
  run_time = Rtc.GetDateTime();
  if (run_time < compiled){
    Serial.println("RTC is older than compile time! Updating DateTime");
    Rtc.SetDateTime(compiled);
  } else if (run_time > compiled) {
    Serial.println("RTC is newer than compile time, this is expected");
  } else if (run_time == compiled) {
    Serial.println("RTC is the same as compile time, while not expected all is still fine");
  }
  // Disable unnecessary functions of the RTC DS3231
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
  delay(1000);
  /*************************************** Set Up microSD *********************************************/
  if(!SD.begin(5)){
    Serial.println("Card Mount Failed");
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
  }
  // Check SD card type
  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
  // Check SD card size
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  Serial.println("");
  /************************************ Get Config Credentials ****************************************/
  if(!config.readNetwork()){
    Serial.println(config.lastError);
  }
  if(!config.readTank()){
    Serial.println(config.lastError);
  }
  delay(1000);
  /************************************** Initialize 4G Modem *****************************************/
  Serial.println("Initializing modem...");
  if(!modem.init()){
    Serial.println("Restarting modem...");
    modem.restart();
  }
  delay(5000);
  if (modem.getSimStatus() != 1) {
    Serial.println("SIM not ready, checking for PIN...");
    if (modem.getSimStatus() == 2){
      Serial.println("SIM PIN required.");
      // Send the PIN to the modem
      modem.simUnlock(config);
      delay(1000);
      if (modem.getSimStatus() != 1) {
        Serial.println("Failed to unlock SIM.");
      }
      Serial.println("SIM unlocked successfully.");
    } else {
      Serial.println("SIM not detected or unsupported status");
    }
  }
  delay(1000);
  /**************************************** Initialize GPS ********************************************/
  if(!gps.init()){
    lcd.clearRow(0);
    lcd.print("Failed to initialize GPS");
    errLog("Failed to initialize GPS", Rtc);
  }
  /********************************** Initialize Modbus RTU client ************************************/
  if(!ModbusRTUClient.begin(RTU_BAUD))
    Serial.println("Failed to start Modbus RTU Client!");
  probeData.reserve(config.probeId.size());
  /************************************* Initialize MQTT client ***************************************/
  remote.setServer(config);
  remote.setBufferSize(1024);
  remote.setCallback(callBack);
  /****************************************************************************************************/
  if (!modem.isGprsConnected()){
    Serial.print("Connecting to APN: ");
    Serial.println(config.apn());
    if (!modem.gprsConnect(config)){
      Serial.println("GPRS failed");
    } else {
      Serial.println("GPRS connected");
    }
  }
  delay(5000);
  /****************************************************************************************************/
  remote.getApiToken("YOUR_SERVER_USER_NAME", "YOUR_SERVER_PASSWORD");
  for (int i = 0; i < 20; i++) {
    EEPROM.write(0 + i, 0);
  }
  EEPROM.commit();
  processCsv(SD, "/probe1.csv", 1);

  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(readGPS, "Read GPS", 3072, NULL, 1, &gpsTaskHandle, pro_cpu);
  xTaskCreatePinnedToCore(readModbus, "Read Modbus", 5012, NULL, 1, &modbusTaskHandle, pro_cpu);
  xTaskCreatePinnedToCore(checkRTC, "Check RTC", 2048, NULL, 2, &rtcTaskHandle, pro_cpu);
  xTaskCreatePinnedToCore(localLog, "Log to SD", 3072, NULL, 1, &logTaskHandle, app_cpu);
  xTaskCreatePinnedToCore(remotePush, "Push to MQTT", 5012, NULL, 2, &pushTaskHandle, app_cpu);

  // Initialize the Task Watchdog Timer
  esp_task_wdt_init(TASK_WDT_TIMEOUT, true);
  vTaskDelete(NULL);
}
/******************************************************************************************************/
void loop(){}

void readGPS(void *pvParameters){
  while(1){
    int status = gps.update();
    if(status != 1){
      if(status == 0){
        lcd.clearRow(0);
        lcd.print("No GPS response");
        errLog("No GPS response", Rtc);
      } else if(status == 2) {
        lcd.clearRow(0);
        lcd.print("Invalid GPS data");
        errLog("Invalid GPS data", Rtc);
      }
    }
    xTaskNotifyGive(pushTaskHandle);
    xTaskNotifyGive(logTaskHandle);
    vTaskDelay(gpsDelay);
    //vTaskDelayUntil(&gpsLastWake, interval);
  }
}
/******************************************************************************************************/
void readModbus(void *pvParameters) {
  while(1){
    char errBuffer[512];
    bool errors = false;
    for (size_t i = 0; i < config.probeId.size(); ++i){
      float* dataPointers[] = {&probeData[i].volume, &probeData[i].ullage, 
                              &probeData[i].temperature, &probeData[i].product, 
                              &probeData[i].water};
      const char* labels[] = {"Volume", "Ullage", "Temperature", "Product", "Water"};
      Serial.printf("Probe number %d:\n", config.probeId[i]);
      for (size_t j = 0; j < 5; ++j){
        *dataPointers[j] = ModbusRTUClient.holdingRegisterRead<float>(
                          config.probeId[i], config.modbusReg[j], BIGEND);
        if (*dataPointers[j] < 0){
          Serial.printf("Failed to read %s for probe ID: %d\r\nError: ", labels[j], config.probeId[i]);
          Serial.println(ModbusRTUClient.lastError());
          snprintf(errBuffer + strlen(errBuffer), 
                  512 - strlen(errBuffer), 
                  "%s error in probe %d: %s, ", 
                  labels[j], config.probeId[i], ModbusRTUClient.lastError());
          errors = true;
        } else {
          Serial.printf("%s: %.2f, ", labels[j], *dataPointers[j]);
        }
      }
      Serial.print("\n");
      if (errors) {
        errBuffer[strlen(errBuffer) - 1] = '\0';
        errLog(errBuffer, Rtc);
        memset(errBuffer, 0, sizeof(errBuffer));
      }
    }
    xTaskNotifyGive(pushTaskHandle);
    xTaskNotifyGive(logTaskHandle);
    vTaskDelay(modbusDelay);
    //vTaskDelayUntil(&modbusLastWake, modbus_interval);
  }
}
/******************************************************************************************************/
void remotePush(void *pvParameters){
  while(1){
    if(!modem.isGprsConnected()){
      Serial.print("Connecting to APN: ");
      Serial.println(config.apn());
      if (!modem.gprsConnect(config)){
        lcd.clearRow(1);
        lcd.print("GPRS failed");
        errLog("GPRS connection failed", Rtc);
      } else {
        lcd.clearRow(1);
        lcd.print("GPRS connected");
      }
    } else if(!remote.connected()){
      if (remote.connect(macAdr, config)){
        lcd.clearRow(1);
        lcd.print("GPRS connected");
        lcd.clearRow(2);
        lcd.print("MQTT connected");
        // Subscribe
        remote.subscribe(config);
      } else {
        lcd.clearRow(2);
        lcd.print("MQTT failed");
      }
    } else {
      StaticJsonDocument<1024> data;
      data["Device"] = macAdr;
      data["Date/Time"] = dateTime;

      JsonObject gpsData = data.createNestedObject("Gps");
      gpsData["Latitude"] = gps.location.latitude;
      gpsData["Longitude"] = gps.location.longitude;
      gpsData["Altitude"] = gps.location.altitude;
      gpsData["Speed"] = gps.location.speed;

      JsonArray measures = data.createNestedArray("Measure");
      for(size_t i = 0; i < config.probeId.size(); i++){
        JsonObject measure = measures.createNestedObject();
        measure["Id"] = config.probeId[i];
        measure["Volume"] = probeData[i].volume;
        measure["Ullage"] = probeData[i].ullage;
        measure["Temperature"] = probeData[i].temperature;
        measure["ProductLevel"] = probeData[i].product;
        measure["WaterLevel"] = probeData[i].water;
      }
      // Serialize JSON and publish
      char buffer[1024];
      size_t n = serializeJson(data, buffer);
      remote.loop();
      remote.publish(config, buffer, n);
    }
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    vTaskDelay(pushDelay);
    //vTaskDelayUntil(&remoteLastWake, interval);
  }
}
/******************************************************************************************************/
void localLog(void *pvParameters){
  while(1){
    for(size_t i = 0; i < config.probeId.size(); i++){
      char path[15];
      snprintf(path, sizeof(path), "/probe%d.csv", i + 1);
      snprintf(logString, sizeof(logString), "\n%s;%.1f;%.1f;%.1f;%.1f;%.1f;%.1f;%.1f;%.1f;%.1f",
              dateTime, gps.location.latitude, gps.location.longitude, gps.location.speed,
              gps.location.altitude, probeData[i].volume, probeData[i].ullage,
              probeData[i].temperature, probeData[i].product, probeData[i].water);
      appendFile(SD, path, logString);
    }
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    vTaskDelay(logDelay);
    //vTaskDelayUntil(&localLastWake, interval);
  }
}
/******************************************************************************************************/
void checkRTC(void *pvParameters){
  while(1){
    if (!Rtc.IsDateTimeValid()){
      if(Rtc.LastError() != 0){
        Serial.println("RTC communication error");
        Serial.println(Rtc.LastError());
      } else {
        Rtc.SetDateTime(compiled);
      }
    }
    if (!Rtc.GetIsRunning()){
      Serial.println("RTC was not actively running, starting now");
      Rtc.SetIsRunning(true);
    }
    Rtc.saveTime(dateTime);
    xTaskNotifyGive(pushTaskHandle);
    xTaskNotifyGive(logTaskHandle);
    vTaskDelay(rtcDelay);
    //vTaskDelayUntil(&rtcLastWake, interval);
  }
}
/********************* Read and push data from CSV file number 'fileNo' to API *************************/
bool processCsv(fs::FS &fs, const char* path, int fileNo){
  File data = fs.open(path, FILE_READ);
  if(!data){
    Serial.println("Data is non-existent");
    return false;
  }
  const int chunkSize = 5;    // Size in rows
  String rows[chunkSize];
  String jsonPayload;
  int rowCount = 0;
  timeStamp = "";             // or `timeStamp = readFlash(fileNo * 20);`
  bool startReading = false;  // Flag to check if there is already a timestamp

  while (data.available()){
    String line = data.readStringUntil('\n');
    line.trim();
    // Skip rows until the last pushed timestamp is found
    if (!startReading) {
      if (timeStamp == "" || line.startsWith(timeStamp)) {
        startReading = true;
        continue;
      } else {
        continue;
      }
    }
    // Store the line in the array
    rows[rowCount++] = line;
    // If chunk is full, send it
    if (rowCount == chunkSize){
      jsonPayload = csvToJson(rows, rowCount);
      Serial.println(jsonPayload);
      if (remote.sendToApi(jsonPayload)){
        // Update the last pushed timestamps
        timeStamp = readCsv(rows[rowCount - 1]);
        rowCount = 0;
      } else {
        Serial.println("Failed to send data chunk. Retrying...");
        delay(5000);
      }
    }
    delay(5000);
  }
  // Send any remaining rows
  if (rowCount > 0){
    jsonPayload = csvToJson(rows, rowCount);
    if (remote.sendToApi(jsonPayload)){
      timeStamp = readCsv(rows[rowCount - 1]);
    } else {
      Serial.println("Failed to send final data chunk.");
    }
  }
  saveToFlash(0, timeStamp);
  data.close();
}