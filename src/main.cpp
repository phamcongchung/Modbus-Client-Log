#include <Arduino.h>
#include <SD.h>
#include <Wire.h>
#include <EEPROM.h>
#include <RtcDS3231.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <esp_task_wdt.h>
#include <ModbusRTUClient.h>
#include <LiquidCrystal_I2C.h>
#include "ConfigManager.h"
#include "GPS.h"

#define TASK_WDT_TIMEOUT  60
#define SIM_RXD           32
#define SIM_TXD           33
#define SIM_BAUD          115200
#define RTU_BAUD          9600
#define EEPROM_SIZE       20

HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
GPS gps(modem);
PubSubClient mqtt(client);
ConfigManager config;
RtcDS3231<TwoWire> Rtc(Wire);
LiquidCrystal_I2C lcd(0x27, 20, 4);

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

String csv2Json(String rows[], int rowCount);
String readFromCsv(const String& row);
String readStrFromFlash(int startAdr);
bool sendData(String& jsonPayload);

void appendFile(fs::FS &fs, const char * path, const char * message);
void callback(char* topic, byte* message, unsigned int len);
void saveStr2Flash(int startAdr, String timeStamp);
void processCsv(File data, int fileNo);
void errLog(const char* msg);
void getTime(char* buffer);
void clearRow(int row);
void getToken();

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

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  // Retrieve MAC address
  esp_efuse_mac_get_default(mac);
  // Format the MAC address into the string
  sprintf(macAdr, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  // Print the MAC address
  Serial.printf("ESP32 MAC Address: %s\r\n", macAdr);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);

  // Initialize DS3231 communication
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
  
  if(!config.getNetwork()){
    Serial.println(config.lastError);
  }
  if(!config.getTank()){
    Serial.println(config.lastError);
  }
  delay(1000);

  SerialAT.begin(SIM_BAUD, SERIAL_8N1, SIM_RXD, SIM_TXD);
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
      modem.sendAT("+CPIN=" + config.simPin);
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

  // Initialize GPS
  gps.init();
  if(gps.lastError() != NULL){
    clearRow(0);
    lcd.print(gps.lastError());
    errLog(gps.lastError());
  }
  // Initialize the Modbus RTU client
  if(!ModbusRTUClient.begin(RTU_BAUD))
    Serial.println("Failed to start Modbus RTU Client!");
  probeData.reserve(config.probeId.size());
  // Initialize MQTT client
  mqtt.setServer(config.broker.c_str(), config.port);
  mqtt.setBufferSize(1024);
  mqtt.setCallback(callback);

  if (!modem.isGprsConnected()){
    Serial.print("Connecting to APN: ");
    Serial.println(config.apn);
    if (!modem.gprsConnect(config.apn.c_str(), config.gprsUser.c_str(), config.gprsPass.c_str())) {
      Serial.println("GPRS failed");
    } else {
      Serial.println("GPRS connected");
    }
  }
  delay(5000);

  getToken();
  processCsv(SD.open("/probe2.csv"), 2);
  /*for (int i = 0; i < 20; i++) {
    EEPROM.write(0 + i, 0);
  }
  EEPROM.commit();*/

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

void loop() {
  
}

void readGPS(void *pvParameters){
  while(1){
    gps.update();
    if(gps.lastError() != NULL){
      clearRow(0);
      lcd.print(gps.lastError());
      errLog(gps.lastError());
    }
    xTaskNotifyGive(pushTaskHandle);
    xTaskNotifyGive(logTaskHandle);
    vTaskDelay(gpsDelay);
    //vTaskDelayUntil(&gpsLastWake, interval);
  }
}

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
        errLog(errBuffer);
        memset(errBuffer, 0, sizeof(errBuffer));
      }
    }
    xTaskNotifyGive(pushTaskHandle);
    xTaskNotifyGive(logTaskHandle);
    vTaskDelay(modbusDelay);
    //vTaskDelayUntil(&modbusLastWake, modbus_interval);
  }
}

void remotePush(void *pvParameters){
  while(1){
    if (!modem.isGprsConnected()){
      Serial.print("Connecting to APN: ");
      Serial.println(config.apn);
      if (!modem.gprsConnect(config.apn.c_str(), config.gprsUser.c_str(), config.gprsPass.c_str())) {
        clearRow(1);
        lcd.print("GPRS failed");
        errLog("GPRS connection failed");
      } else {
        clearRow(1);
        lcd.print("GPRS connected");
      }
    } else if (!mqtt.connected()){
      if (mqtt.connect(macAdr, config.brokerUser.c_str(), config.brokerPass.c_str())){
        clearRow(1);
        lcd.print("GPRS connected");
        clearRow(2);
        lcd.print("MQTT connected");
        // Subscribe
        mqtt.subscribe(config.topic.c_str());
      } else {
        clearRow(2);
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
      mqtt.loop();
      mqtt.publish(config.topic.c_str(), buffer, n);
    }
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    vTaskDelay(pushDelay);
    //vTaskDelayUntil(&remoteLastWake, interval);
  }
}

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
    getTime(dateTime);
    xTaskNotifyGive(pushTaskHandle);
    xTaskNotifyGive(logTaskHandle);
    vTaskDelay(rtcDelay);
    //vTaskDelayUntil(&rtcLastWake, interval);
  }
}

void getToken(){
  if (client.connect("YOUR_API_HOST", 6868, 10) != 1) {
    Serial.println("Failed to connect to server.");
  }

  String tokenAuth = "{\"username\":\"YOUR_SERVER_USER_NAME\",\"password\":\"YOUR_SERVER_PASSWORD\"}";
  String tokenReq = "POST /api/tokens HTTP/1.1\r\n"
                    "Host: YOUR_HOST_URL:6868\r\n"
                    "Content-Type: application/json\r\n"
                    "tenant: root\r\n"
                    "Accept-Language: en-US\r\n"
                    "Connection: keep-alive\r\n"
                    "Content-Length: " + String(tokenAuth.length()) + "\r\n\r\n";
  client.print(tokenReq);
  client.print(tokenAuth);

  Serial.println("Waiting for authentication response...");
  String response;
  while (client.connected() || client.available()) {
    if (client.available()){
      response = client.readString();
      Serial.println(response);
      break;
    }
  }
  
  int jsonStart = response.indexOf("{");
  if (jsonStart != -1){
    response = response.substring(jsonStart);
    int jsonEnd = response.lastIndexOf("}");
    if (jsonEnd != -1)
      response = response.substring(0, jsonEnd + 1);
  } else {
    response = "";
  }

  // Parse the token from the response (assumes JSON response format)
  StaticJsonDocument<1024> jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, response);
  if (error) {
    Serial.print("Failed to get token: ");
    Serial.println(error.c_str());
  }
  if (jsonDoc.containsKey("token")) {
    bearerToken = jsonDoc["token"].as<String>();
    Serial.print("Extracted Token: ");
    Serial.println(bearerToken);
  } else {
    Serial.println("Error: 'token' field not found in the response.");
  }
}

bool sendData(String& jsonPayload){
  // Send HTTP POST request
  Serial.println("Sending data chunk...");
  String postReq = "POST /api/v1/dataloggers/addlistdatalogger HTTP/1.1\r\n"
                  "Host: YOUR_HOST_URL:6868\r\n"
                  "Content-Type: application/json\r\n"
                  "Authorization: Bearer " + bearerToken + "\r\n"
                  "Accept-Language: en-US\r\n"
                  "Connection: keep-alive\r\n"
                  "Content-Length: " + String(jsonPayload.length()) + "\r\n\r\n";
  client.print(postReq);
  client.print(jsonPayload);

  // Wait for server response
  Serial.println("Waiting for server response...");
  while (client.connected() || client.available()){
    if (client.available()){
      String response = client.readString();
      Serial.println(response);
      break;
    }
  }
  return true;
}

// Read and push data from CSV file number 'fileNo' to API
void processCsv(File data, int fileNo){
  const int chunkSize = 5;
  String rows[chunkSize];
  String jsonPayload;
  int rowCount = 0;

  timeStamp = ""; //readStrFromFlash(fileNo * 20);
  // Flag to check if there is already a timestamp
  bool startReading = false;

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
      jsonPayload = csv2Json(rows, rowCount);
      Serial.println(jsonPayload);
      if (sendData(jsonPayload)){
        // Update the last pushed timestamps
        timeStamp = readFromCsv(rows[rowCount - 1]);
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
    jsonPayload = csv2Json(rows, rowCount);
    if (sendData(jsonPayload)){
      timeStamp = readFromCsv(rows[rowCount - 1]);
    } else {
      Serial.println("Failed to send final data chunk.");
    }
  }
  saveStr2Flash(0, timeStamp);
  data.close();
}

String readFromCsv(const String& row) {
  int commaIndex = row.indexOf(',');
  if (commaIndex != -1) {
    return row.substring(0, commaIndex);
  }
  return "";
}

void saveStr2Flash(int startAdr, String string) {
  int length = string.length();
  if (startAdr + length + 1 > EEPROM.length()){
    Serial.println("Error: Not enough EEPROM space to save the string.");
    return;
  }
  // Convert the String to a char array
  char charArray[length + 1];
  string.toCharArray(charArray, length + 1);
  // Write each character to EEPROM
  for (int i = 0; i < length; i++) {
    EEPROM.write(startAdr + i, charArray[i]);
  }
  // Write null terminator
  EEPROM.write(startAdr + length, '\0');
  // Commit changes (if using ESP32 or ESP8266)
  EEPROM.commit();
}

String readStrFromFlash(int startAdr) {
  String result = "";
  char read;
  // Read characters until null terminator is found
  for (int i = startAdr; i < EEPROM.length(); i++) {
    read = EEPROM.read(i);
    if (read == '\0') break; // Stop at null terminator
    result += read;
  }
  return result;
}

String csv2Json(String rows[], int rowCount) {
  String jsonPayload = "[";  // Start JSON array

  for (int i = 0; i < rowCount; i++) {
    if (i > 0) jsonPayload += ",";  // Add a comma between JSON objects

    // Split the row by semicolons
    String fields[10];
    int fieldIndex = 0;

    while (rows[i].length() > 0 && fieldIndex < 10) {
      int delimiterIndex = rows[i].indexOf(';');
      if (delimiterIndex == -1) {
        fields[fieldIndex++] = rows[i];
        rows[i] = "";
      } else {
        fields[fieldIndex++] = rows[i].substring(0, delimiterIndex);
        rows[i] = rows[i].substring(delimiterIndex + 1);
      }
    }

    // Construct JSON object for the current row
    jsonPayload += "{";
    jsonPayload += "\"time\":\"" + fields[0] + "\",";
    jsonPayload += "\"latitude\":" + fields[1] + ",";
    jsonPayload += "\"longitude\":" + fields[2] + ",";
    jsonPayload += "\"speed\":" + fields[3] + ",";
    jsonPayload += "\"height\":" + fields[4] + ",";
    jsonPayload += "\"volumeFuel\":" + fields[5] + ",";
    jsonPayload += "\"volumeEmpty\":" + fields[6] + ",";
    jsonPayload += "\"temperature\":" + fields[7] + ",";
    jsonPayload += "\"levelFuel\":" + fields[8] + ",";
    jsonPayload += "\"levelWater\":" + fields[9];
    jsonPayload += "}";
  }

  jsonPayload += "]";  // End JSON array
  return jsonPayload;
}

void getTime(char* buffer){
  run_time = Rtc.GetDateTime();
  snprintf_P(buffer, 20,
            PSTR("%02u-%02u-%04u %02u:%02u:%02u"),
            run_time.Day(),run_time.Month(), run_time.Year(),
            run_time.Hour(), run_time.Minute(), run_time.Second());
}

void errLog(const char* msg){
  char err_time[20];
  getTime(err_time);
  // Calculate the required buffer size dynamically
  size_t msgSize = strlen(err_time) + strlen(msg) + 3; // 2 for ',' and '\n', 1 for '\0'
  char* errMsg = (char*)malloc(msgSize);
  if (errMsg == NULL) {
    Serial.println("Failed to allocate memory for error message!");
    return;
  }
  snprintf(errMsg, msgSize, "\n%s,%s", err_time, msg);
  appendFile(SD, "/error.csv", errMsg);
  free(errMsg);
}

void clearRow(int row) {
  lcd.setCursor(0, row);
  for (int i = 0; i < 20; i++)
    lcd.print(' ');
  lcd.setCursor(0, row);
}

void callback(char* topic, byte* message, unsigned int len){
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < len; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}

void appendFile(fs::FS &fs, const char* path, const char* message){
  Serial.printf("Appending file: %s...", path);
  
  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.print("File does not exist, creating file...");
    file = fs.open(path, FILE_WRITE);  // Create the file
    if(!file){
      Serial.println("Failed to create file");
      return;
    }
  }
  if(file.print(message)){
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}