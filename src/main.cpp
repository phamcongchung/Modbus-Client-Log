#include <Arduino.h>
#include <ModbusRTUClient.h>
#include <SoftwareSerial.h>
#include <ArduinoRS485.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <RtcDS3231.h>
#include <vector>
#include <Wire.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>

#define TINY_GSM_MODEM_SIM7600
#define SIM_RXD       32
#define SIM_TXD       33
#define SIM_BAUD      115200

#include <TinyGsmClient.h>

HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);
// RTC and mutex
RtcDS3231<TwoWire> Rtc(Wire);  
RtcDateTime now, compiled;
SemaphoreHandle_t timeMutex;

static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;

// Dynamic array for probe IDs and data
struct ProbeData {
  float volume;
  float ullage;
  float temperature;
  float product;
  float water;
};
std::vector<int> probeId;
std::vector<uint16_t> modbusReg;
std::vector<ProbeData> probeData;
// GPS data
float latitude, longitude;
String latDir, longDir, altitude, speed;
String device, seriaNo;
// Create a string to hold the formatted MAC address
static uint8_t mac[6];
static char macAdr[18];
static char logString[300];
// GPRSS credentials
String apn;
String gprsUser;
String gprsPass;
// MQTT credentials
String topic;
String broker;
String brokerUser;
String brokerPass;
uint16_t port;
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

//Function declaration
void appendFile(fs::FS &fs, const char * path, const char * message);
void writeFile(fs::FS &fs, const char * path, const char * message);
void mqttCallback(char* topic, byte* message, unsigned int len);
void getNetworkConfig();
void getTankConfig();
void mqttReconnect();
float coordConvert(String coord, String direction);
String getValue(const String& data, char separator, int index);

void setup() {
  // Initialize mutex
  timeMutex = xSemaphoreCreateMutex();

  // Initialize serial communication
  Serial.begin(115200);
  SerialAT.begin(115200, SERIAL_8N1, SIM_RXD, SIM_TXD);
  vTaskDelay(pdMS_TO_TICKS(3000));

  // Retrieve MAC address
  esp_efuse_mac_get_default(mac);
  // Format the MAC address into the string
  sprintf(macAdr, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  // Print the MAC address
  Serial.printf("ESP32 MAC Address: %s\r\n", macAdr);
  delay(1000);

  // Initialize the microSD card
  if(!SD.begin(5)){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
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

  getNetworkConfig();
  getTankConfig();
  vTaskDelay(pdMS_TO_TICKS(1000));
  
  Serial.println("Initializing modem...");
  // modem.restart();
  
  Serial.print("Connecting to APN: ");
  Serial.println(apn);
  if (!modem.gprsConnect(apn.c_str(), gprsUser.c_str(), gprsPass.c_str())) {
    Serial.println("Fail to connect to LTE network");
    ESP.restart();
  }
  else {
    Serial.println("OK");
  }

  if (modem.isGprsConnected()) {
    Serial.println("GPRS connected");
  }
  // Enable GPS
  Serial.println("Enabling GPS...");
  modem.sendAT("+CGPS=1,1");  // Start GPS in standalone mode
  modem.waitResponse(10000L);
  Serial.println("Waiting for GPS data...");
  
  // Print out compile time
  Serial.print("Compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);
  vTaskDelay(pdMS_TO_TICKS(500));
  
  // Initialize the Modbus RTU client
  if (!ModbusRTUClient.begin(9600)) {
    Serial.println("Failed to start Modbus RTU Client!");
    Serial.println("Trying to reconnect");
    ModbusRTUClient.begin(9600);
  }
  vTaskDelay(pdMS_TO_TICKS(1000));
  
  // Initialize DS3231 communication
  Rtc.Begin();
  compiled = RtcDateTime(__DATE__, __TIME__);
  Serial.println();
  // Disable unnecessary functions of the RTC DS3231
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
  // Validate RTC date and time
  if (!Rtc.IsDateTimeValid())
  {
    if ((Rtc.LastError()) != 0)
    {
      Serial.print("RTC communication error = ");
      Serial.println(Rtc.LastError());
    }
    else
    {
      Serial.println("RTC lost confidence in the DateTime!");
      Rtc.SetDateTime(compiled);
    }
  }

  // If the log.csv file doesn't exist
  // Create a file on the SD card and write the data labels
  File file = SD.open("/log.csv");
  if(!file) {
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/log.csv", "Date,Time,Latitude,Longitude,Speed(km/h),Altitude(m)"
                              "Volume(l),Ullage(l),Temperature(°C)"
                              "Product level(mm),Water level(mm)\n");
  }
  else {
    Serial.println("File already exists");  
  }
  file.close();

  // Initialize MQTT broker
  mqtt.setServer(broker.c_str(), 1883);
  mqtt.setBufferSize(1024);
  mqtt.setCallback(mqttCallback);
  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.println("FAFNIR TANK LEVEL");

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
    modem.sendAT("+CGPSINFO");
    if (modem.waitResponse(10000L, "+CGPSINFO:") == 1) {
      String gpsData = modem.stream.readStringUntil('\n');

      // Check if the data contains invalid GPS values
      if (gpsData.indexOf(",,,,,,,,") != -1) {
        Serial.println("Error: GPS data is invalid (no fix or no data available).");
      } else {
        Serial.println("Raw GPS Data: " + gpsData);
        String rawLat = getValue(gpsData, ',', 0); latDir = getValue(gpsData, ',', 1);
        String rawLong = getValue(gpsData, ',', 2); longDir = getValue(gpsData, ',', 3);
        altitude = getValue(gpsData, ',', 6); speed = getValue(gpsData, ',', 7);
        latitude = coordConvert(rawLat, latDir);
        longitude = coordConvert(rawLong, longDir);
      }
      delay(5000);
    } else {
      Serial.println("GPS data not available or invalid.");
    }
    vTaskDelay(gpsDelay);
  }
}

void readModbus(void *pvParameters) {
  while(1){
    for (size_t i = 0; i < probeId.size(); ++i) {
      float* dataPointers[] = {&probeData[i].volume, &probeData[i].ullage, 
                              &probeData[i].temperature, &probeData[i].product, 
                              &probeData[i].water};
      const char* labels[] = {"Volume", "Ullage", "Temperature", "Product", "Water"};
      
      for (size_t j = 0; j < 5; ++j) {
        *dataPointers[j] = ModbusRTUClient.holdingRegisterRead<float>(probeId[i], modbusReg[j], BIGEND);

        if (*dataPointers[j] < 0) {
          Serial.printf("Failed to read %s for probe ID: %d\r\nError: %d\r\n", labels[j], probeId[i], ModbusRTUClient.lastError());
        } else {
          Serial.printf("%s: %.2f\r\n", labels[j], *dataPointers[j]);
        }
      }
    }
    vTaskDelay(modbusDelay);
  }
}

void remotePush(void *pvParameters){
  while(1){
    if (xSemaphoreTake(timeMutex, portMAX_DELAY) == pdTRUE) {
      if(!mqtt.connected()){
        mqttReconnect();
      }
      mqtt.loop();

      char dateString[20];
      char timeString[20];
      snprintf_P(dateString,
                countof(dateString),
                PSTR("%02u/%02u/%04u"),
                now.Month(),
                now.Day(),
                now.Year());
      snprintf_P(timeString,
                countof(timeString),
                PSTR("%02u:%02u:%02u"),
                now.Hour(),
                now.Minute(),
                now.Second());
      xSemaphoreGive(timeMutex);
      // Combine date and time into a single string
      char dateTimeString[40];
      snprintf(dateTimeString, sizeof(dateTimeString), "%s %s", dateString, timeString);

      StaticJsonDocument<1024> data;
      data["Device"] = macAdr;
      data["Date/Time"] = dateTimeString;

      JsonObject gps = data.createNestedObject("Gps");
      gps["Latitude"] = latitude;
      gps["Longitude"] = longitude;
      gps["Altitude"] = altitude.toFloat();
      gps["Speed"] = speed.toFloat();

      JsonArray measures = data.createNestedArray("Measure");
      for(int i = 0; i < probeId.size(); i++){
        JsonObject measure = measures.createNestedObject();
        measure["Id"] = probeId[i];
        measure["Volume"] = probeData[i].volume;
        measure["Ullage"] = probeData[i].ullage;
        measure["Temperature"] = probeData[i].temperature;
        measure["ProductLevel"] = probeData[i].product;
        measure["WaterLevel"] = probeData[i].water;
      }
      // Serialize JSON and publish
      char buffer[1024];
      size_t n = serializeJson(data, buffer);
      mqtt.publish(topic.c_str(), buffer, n);

    }
    vTaskDelay(pushDelay);
  }
}

void localLog(void *pvParameters){
  while(1){
    if (xSemaphoreTake(timeMutex, portMAX_DELAY) == pdTRUE) {
      char dateString[20];
      char timeString[20];
      snprintf_P(dateString,
                countof(dateString),
                PSTR("%02u/%02u/%04u"),
                now.Month(),
                now.Day(),
                now.Year());
      snprintf_P(timeString,
                countof(timeString),
                PSTR("%02u:%02u:%02u"),
                now.Hour(),
                now.Minute(),
                now.Second());
      xSemaphoreGive(timeMutex);

      snprintf(logString, sizeof(logString), "%s;%s;%f;%f;%s;%s;%.1f;%.1f;%.1f;%.1f;%.1f\n",
              dateString, timeString, latitude, longitude, speed, altitude, probeData[0].volume,
              probeData[0].ullage, probeData[0].temperature, probeData[0].product, probeData[0].water);
      appendFile(SD, "/log.csv", logString);
    }

    vTaskDelay(logDelay);
  }
}

void checkRTC(void *pvParameters){
  while(1){
    if (xSemaphoreTake(timeMutex, portMAX_DELAY) == pdTRUE) {
      RtcDateTime now = Rtc.GetDateTime();
      // Ensure the RTC is running
      if (!Rtc.GetIsRunning())
      {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
      }
      // Compare RTC time with compile time
      if (now < compiled)
      {
        Serial.println("RTC is older than compile time! (Updating DateTime)");
        Rtc.SetDateTime(compiled);
      }
      else if (now > compiled)
      {
        Serial.println("RTC is newer than compiled time! (as expected)");
      }
      else if (now == compiled)
      {
        Serial.println("RTC is the same as compile time! (not expected but acceptable)");
      }
      xSemaphoreGive(timeMutex);
    }
    vTaskDelay(rtcDelay);
  }
}

void getNetworkConfig(){
  File file = SD.open("/config.json");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  // Allocate a JSON document
  StaticJsonDocument<128> config;
  // Parse the JSON from the file
  DeserializationError error = deserializeJson(config, file);
  if (error) {
    Serial.print("Failed to parse file: ");
    Serial.println(error.f_str());
    return;
  }
  file.close();

  // Extract Network Configuration as String
  apn = config["NetworkConfiguration"]["Apn"].as<String>();
  gprsUser = config["NetworkConfiguration"]["GprsUser"].as<String>();
  gprsPass = config["NetworkConfiguration"]["GprsPass"].as<String>();
  topic = config["NetworkConfiguration"]["Topic"].as<String>();
  broker = config["NetworkConfiguration"]["Broker"].as<String>();
  brokerUser = config["NetworkConfiguration"]["BrokerUser"].as<String>();
  brokerPass = config["NetworkConfiguration"]["BrokerPass"].as<String>();
  port = config["NetworkConfiguration"]["Port"].as<uint16_t>();
}

void getTankConfig(){
  File file = SD.open("/config.json");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  // Allocate a JSON document
  StaticJsonDocument<512> config;
  // Parse the JSON from the file
  DeserializationError error = deserializeJson(config, file);
  if (error) {
    Serial.print("Failed to parse file: ");
    Serial.println(error.f_str());
    return;
  }
  file.close();

  // Access the Probe Configuration array
  JsonArray tanks = config["TankConfiguration"];
  probeId.reserve(tanks.size());
  probeData.reserve(tanks.size());
  for(JsonObject tank : tanks){
    JsonArray modbusRegs = tank["ModbusRegister"];
    modbusReg.reserve(modbusRegs.size());
    for(JsonVariant regValue : modbusRegs){
      modbusReg.push_back(regValue.as<uint16_t>());
      Serial.print(modbusReg.back()); Serial.print(" ");
    }
    Serial.println("");
    probeId.push_back(tank["Id"].as<int>()); Serial.println(probeId.back());
    String device = tank["Device"]; Serial.println(device);
    String serialNo = tank["SerialNo"]; Serial.println(serialNo);
  }
}

String getValue(const String& data, char separator, int index) {
  int startIndex = 0;
  for (int i = 0; i <= index; i++) {
    int endIndex = data.indexOf(separator, startIndex);
    if (i == index) {
      return (endIndex == -1) ? data.substring(startIndex) : data.substring(startIndex, endIndex);
    }
    startIndex = endIndex + 1;
  }
  return "";
}

float coordConvert(String coord, String direction) {
  // First two or three digits are degrees
  int degrees = coord.substring(0, coord.indexOf('.')).toInt() / 100;
  // Remaining digits are minutes
  float minutes = coord.substring(coord.indexOf('.') - 2).toFloat();
  // Convert to decimal degrees
  float decimalDegrees = degrees + (minutes / 60);
  // Apply direction (N/S or E/W)
  if (direction == "S" || direction == "W") {
    decimalDegrees = -decimalDegrees;  // South and West are negative
  }
  return decimalDegrees;
}

void mqttReconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(macAdr, brokerUser.c_str(), brokerPass.c_str())) {
      Serial.println("Connected");
      // Subscribe
      mqtt.subscribe(topic.c_str());
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqtt.state());
      Serial.println("Try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void mqttCallback(char* topic, byte* message, unsigned int len) {
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

void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\r\n", path);
  
  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("File does not exist, creating file...");
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