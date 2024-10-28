#include <Arduino.h>
#include <ModbusRTUClient.h>
#include <SoftwareSerial.h>
#include <ArduinoRS485.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <RtcDS3231.h>
#include <iostream>
#include <Wire.h>
#include <string>
#include <SPI.h>
#include <FS.h>
#include <SD.h>

#define TINY_GSM_MODEM_SIM7600
#define SIM_BAUD      115200
#define PUSH_INTERVAL 60000

#include <TinyGsmClient.h>

HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);
RtcDS3231<TwoWire> Rtc(Wire);  
RtcDateTime now = Rtc.GetDateTime();
RtcDateTime compiled; // Time at which the program is compile

struct ProbeData {
  float volume;
  float ullage;
  float temperature;
  float product;
  float water;
};

static ProbeData probeData;
// Define Modbus addresses
const uint16_t modbusReg[] = {0x0036, 0x003C, 0x0034, 0x0030, 0x0032};
const size_t DATA_COUNT = sizeof(modbusReg) / sizeof(modbusReg[0]);
// GPS data
float latitude, longitude;
String latDir, longDir, altitude, speed;
String device, seriaNo;
int probeId[];
static uint8_t mac[6];
static char logString[300];
static char monitorString[300];
// GPRSS credentials
char apn[] = "";
char gprsUser[] = "";
char gprsPass[] = "";
// MQTT credentials
const char* topic = "";
const char* broker = "";
const char* clientID = "";
const char* brokerUser = "";

void appendFile(fs::FS &fs, const char * path, const char * message);
void writeFile(fs::FS &fs, const char * path, const char * message);
void mqttCallback(char* topic, byte* message, unsigned int len);
void localLog(const RtcDateTime& dt);
void remotePush(const RtcDateTime& dt);
void readData(ProbeData &data);
void parseGPS(String gpsData);
void mqttReconnect();
void getTankConfig();
void getNetworkConfig();
float convertToDecimalDegrees(String coord, String direction);
String getValue(String data, char separator, int index);

void setup() {
  getNetworkConfig();
  getTankConfig();
  delay(1000);
  // Initialize serial communication
  Serial.begin(115200);
  SerialAT.begin(115200, SERIAL_8N1, 32, 33);
  delay(3000);

 // Retrieve MAC address
  esp_efuse_mac_get_default(mac);
  // Print the MAC address
  Serial.printf("ESP32 MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  delay(500);

  Serial.println("Initializing modem...");
  modem.restart();
  
  Serial.print("Connecting to APN: ");
  Serial.println(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
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
  delay(500);

  // Initialize the Modbus RTU client
  if (!ModbusRTUClient.begin(9600)) {
    Serial.println("Failed to start Modbus RTU Client!");
    while (1);
  }
  delay(500);
  
  // Initialize DS3231 communication
  Rtc.Begin();
  compiled = RtcDateTime(__DATE__, __TIME__);
  Serial.println();
  // Disable unnecessary functions of the RTC DS3231
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);

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
  delay(500);
  // If the log.txt file doesn't exist
  // Create a file on the SD card and write the data labels
  File file = SD.open("/log.txt");
  if(!file) {
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/log.txt", "Date,Time,Latitude,Longitude,Speed(km/h),Altitude(m)"
                              "Volume(l),Ullage(l),Temperature(°C)"
                              "Product level(mm),Water level(mm)\n");
  }
  else {
    Serial.println("File already exists");  
  }
  file.close();

  // Initialize MQTT broker
  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);

  Serial.println("FAFNIR TANK LEVEL");
}

void loop() {
  if(!mqtt.connected()){
    mqttReconnect();
  }
  mqtt.loop();
  
  modem.sendAT("+CGPSINFO");
  if (modem.waitResponse(10000L, "+CGPSINFO:") == 1) {
    String gpsData = modem.stream.readStringUntil('\n');

    // Check if the data contains invalid GPS values
    if (gpsData.indexOf(",,,,,,,,") != -1) {
      Serial.println("Error: GPS data is invalid (no fix or no data available).");
    } else {
      Serial.println("Raw GPS Data: " + gpsData);
      // Call a function to parse the GPS data if valid
      parseGPS(gpsData);
    }
    delay(5000);
  } else {
    Serial.println("GPS data not available or invalid.");
  }
  
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

  readData(probeData);

  localLog(now);
  delay(5000);
}

void parseGPS(String gpsData){
  String rawLat = getValue(gpsData, ',', 0); latDir = getValue(gpsData, ',', 1);
  String rawLong = getValue(gpsData, ',', 2); longDir = getValue(gpsData, ',', 3);
  altitude = getValue(gpsData, ',', 6); speed = getValue(gpsData, ',', 7);
  latitude = convertToDecimalDegrees(rawLat, latDir);
  longitude = convertToDecimalDegrees(rawLong, longDir);
}

String getValue(String data, char separator, int index) {
  int found = 0; // Found separator
  int strIndex[] = {0, -1}; // Array to hold the start and end index of the value
  int maxIndex = data.length() - 1; // Get the maximum index of the string

  // Loop ends when it reaches the end of the string and there are more separators than indices
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

float convertToDecimalDegrees(String coord, String direction) {
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

void readData(ProbeData &data) {
  float *dataPointers[] = {&data.volume, &data.ullage, &data.temperature, &data.product, &data.water};
  
  for (size_t i = 0; i < DATA_COUNT; ++i) {
    *dataPointers[i] = ModbusRTUClient.holdingRegisterRead<float>(4, modbusReg[i], BIGEND);
    
    if (*dataPointers[i] < 0) {
      Serial.print("Failed to read ");
      Serial.println(i == 0 ? "volume" : (i == 1 ? "ullage" : (i == 2 ? "temperature" : (i == 3 ? "product" : "water"))));
      Serial.println(ModbusRTUClient.lastError());
    }
  }
  
  // Optionally print data after reading
  Serial.print("Volume: "); Serial.println(data.volume);
  Serial.print("Ullage: "); Serial.println(data.ullage);
  Serial.print("Temperature: "); Serial.println(data.temperature);
  Serial.print("Product: "); Serial.println(data.product);
  Serial.print("Water: "); Serial.println(data.water);
}

void localLog(const RtcDateTime& dt){
  char datestring[20];
  char timestring[20];
  snprintf_P(datestring,
            countof(datestring),
            PSTR("%02u/%02u/%04u"),
            dt.Month(),
            dt.Day(),
            dt.Year());
  snprintf_P(timestring,
            countof(timestring),
            PSTR("%02u:%02u:%02u"),
            dt.Hour(),
            dt.Minute(),
            dt.Second());

  snprintf(logString, sizeof(logString), "%s;%s;%f;%f;%s;%s;%.1f;%.1f;%.1f;%.1f;%.1f\n",
           datestring, timestring, latitude, longitude, speed, altitude, probeData.volume,
           probeData.ullage, probeData.temperature, probeData.product, probeData.water);
  appendFile(SD, "/log.csv", logString);
}

void mqttReconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(clientID)) {
      Serial.println("Connected");
      // Subscribe
      mqtt.subscribe(topic);
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

  JsonObject networkConfig = config["NetworkConfiguration"];
  const char* tempApn = networkConfig["Apn"];
  if (tempApn != nullptr) {
    // Copy with a max size limit to avoid overflow
    strncpy((char*)apn, tempApn, sizeof(tempApn) - 1);
    // Ensure null termination
    apn[sizeof(apn) - 1] = '\0';
  }

  const char* tempGprsUser = networkConfig["GprsUser"];
  if (tempGprsUser != nullptr) {
    strncpy((char*)gprsUser, tempGprsUser, sizeof(tempGprsUser) - 1);
    gprsUser[sizeof(gprsUser) - 1] = '\0';
  }

  const char* tempGprsPass = networkConfig["GprsPass"];
  if (tempGprsPass != nullptr){
    strncpy((char*)gprsPass, tempGprsPass, sizeof(tempGprsPass) - 1);
    gprsPass[sizeof(gprsPass) - 1] = '\0';
  }

  topic = networkConfig["Topic"].as<const char*>();
  broker = networkConfig["Broker"].as<const char*>();
  clientID = networkConfig["ClientId"].as<const char*>();
  brokerUser = networkConfig["BrokerUser"].as<const char*>();
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

  int i = 0;
  // Access the Probe Configuration array
  JsonArray tanks = config["TankConfiguration"];
  for(JsonObject tank : tanks){
    probeId[i] = tank["Id"];
    String device = tank["Device"];
    String serialNo = tank["SerialNo"];
    i++;
  }
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

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
  Serial.printf("Appending to file: %s\n", path);

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
