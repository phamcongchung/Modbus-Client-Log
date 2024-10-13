#include <Arduino.h>
#include <ModbusRTUClient.h>
#include <SoftwareSerial.h>
#include <ArduinoRS485.h>
#include <PubSubClient.h>
#include <RtcDS3231.h>
#include <TinyGPS++.h>
#include <iostream>
#include <string>
#include <Wifi.h>
#include <Wire.h>
#include <SPI.h>
#include "FS.h"
#include "SD.h"

#define TINY_GSM_MODEM_SIM7600
// #define GPS_RXD       32
// #define GPS_TXD       33
#define SIM_BAUD      115200
#define PUSH_INTERVAL 60000

using namespace std;

#include <TinyGsmClient.h>

// SoftwareSerial gpsSerial(GPS_RXD, GPS_TXD);
HardwareSerial SerialAT(2);
// TinyGPSPlus gps;
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);
RtcDS3231 <TwoWire> Rtc(Wire);  
RtcDateTime now = Rtc.GetDateTime();
RtcDateTime compiled; // Time at which the program is compiled

float volume;
float ullage;
String speed;
String latitude;
String altitude;
String longitude;
char logString[100];
char monitorString[150];
// GPRSS credentials
const char apn[] = "v-internet";
const char gprsUser[] = "";
const char gprsPass[] = "";
// WiFi credentials
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASS";
// MQTT credentials
const char* topic = "YOUR_TOPIC";
const char* broker = "YOUR_BROKER_ADDRESS";
const char* clientID = "YOUR_BROKER_CLIENT";
const char* brokerUser = "YOUR_BROKER_USER";

void appendFile(fs::FS &fs, const char * path, const char * message);
void writeFile(fs::FS &fs, const char * path, const char * message);
void mqttCallback(char* topic, byte* message, unsigned int len);
void logger(const RtcDateTime& dt);
void parseGPS(String gpsData);
void mqttReconnect();
// void gpsUpdate();
String getValue(String data, char separator, int index);

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  SerialAT.begin(115200);
  delay(3000);

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

  // Initialize SoftwareSerial GPS communication
  /*
  gpsSerial.begin(GPS_BAUD);
  Serial.println("SoftwareSerial GPS started at 9600 baud rate");
  gpsUpdate();
  delay(500);
  */

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
    writeFile(SD, "/log.txt", "Date,Time,Volume(l),Ullage(l),Coordinates,\
                              Speed(km/h),Altitude(m)\n");
  }
  else {
    Serial.println("File already exists");  
  }
  file.close();

  // Initialize WiFi communication
  /*
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi is conncted at IP address: ");
  Serial.println(WiFi.localIP());
  delay (500);
  */

  // Initialize MQTT broker
  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);

  Serial.println("FAFNIR TANK LEVEL");
}

void loop() {
  /*
  if (millis() > 30000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    delay(5000);
  }
  if (gpsSerial.available() > 0)
    if (gps.encode(gpsSerial.read()))
      if (gps.location.isUpdated())
        gpsUpdate();
  */

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

  volume = ModbusRTUClient.holdingRegisterRead<float>(4, 0x0036, BIGEND);
  if (volume < 0) {
    Serial.print("Failed to read volume: ");
    Serial.println(ModbusRTUClient.lastError());
  } else {
    Serial.print("Volume: ");
    Serial.println(volume);
  }
  ullage = ModbusRTUClient.holdingRegisterRead<float>(4, 0x003C, BIGEND);
  if (ullage < 0) {
    Serial.print("Failed to read ullage: ");
    Serial.println(ModbusRTUClient.lastError());
  } else {
    Serial.print("Ullage: ");
    Serial.println(ullage);
  }

  if(!mqtt.connected()){
    mqttReconnect();
  }
  mqtt.loop();

  logger(now);
  delay(5000);
}

/*
void gpsUpdate()
{
  Serial.print(F("Satellites: "));
  Serial.println(gps.satellites.value());
  Serial.print(F("Coordinates: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    latitude = gps.location.lat();
    Serial.print(F(" "));
    Serial.println(gps.location.lng(), 6);
    longtitude = gps.location.lng();
    Serial.print("SPEED(km/h): "); 
    Serial.println(gps.speed.kmph());
    speed = gps.speed.kmph();
    Serial.print("ALT(min): "); 
    Serial.println(gps.altitude.meters());
    altitude = gps.altitude.meters();
  }
  else
  {
    Serial.println(F("INVALID"));
  }
}
*/

void parseGPS(String gpsData){
  // Split the string by commas
  int index = 0;
  latitude = getValue(gpsData, ',', index++) + getValue(gpsData, ',', index++);
  longitude = getValue(gpsData, ',', index++) + getValue(gpsData, ',', index++);
  altitude = getValue(gpsData, ',', index+3);
  speed = getValue(gpsData, ',', index++);
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

void logger(const RtcDateTime& dt){
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
  snprintf(logString, sizeof(logString), "%s,%s,%lf,%lf,%lf %lf,%f,%f\n",
           datestring, timestring, latitude, longitude, speed, altitude, volume, ullage);
  appendFile(SD, "/log.txt", logString);
  snprintf(monitorString, sizeof(monitorString), "%s %s\nLatitude: %lf\nLongtitude: %lf\
          \nSpeed: %.1lf(km/h)\nAltitude: %.1lf(m)\nVolume: %.1f(l)\nUllage: %.1f(l)",
          datestring, timestring, latitude, longitude, speed, altitude, volume, ullage);
  mqtt.publish(topic, monitorString);
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
