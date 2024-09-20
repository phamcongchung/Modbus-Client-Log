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

#define GPS_RXD 32
#define GPS_TXD 33
#define GPS_BAUD 9600

using namespace std;

RtcDS3231 <TwoWire> Rtc(Wire);
TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RXD, GPS_TXD);
WiFiClient espClient;
PubSubClient client(espClient);

float volume;
float ullage;
char logString[100];
const char* ssid = "YOUR_WIFI_NAME";
const char* broker = "YOUR_BROKER_ADDRESS";
const char* password = "YOUR_WIFI_PASS";
const char* brokerUser = "YOUR_BROKER_USER";

void updateLog(const RtcDateTime& dt);
void appendFile(fs::FS &fs, const char * path, const char * message);
void readFile(fs::FS &fs, const char * path);
void displayInfo();
void reconnect();

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.print("Compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);
  delay(500);

  // Initialize SoftwareSerial GPS communication
  gpsSerial.begin(GPS_BAUD);
  Serial.println("SoftwareSerial started at 9600 baud rate");
  delay(500);

  // Initialize WiFi communication
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

  // Initialize MQTT broker
  client.setServer(broker, 1883);

  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  Serial.println();

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
  if (!Rtc.GetIsRunning())
  {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
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

  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);

  if(!SD.begin(5)){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

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

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  delay(500);
  // start the Modbus RTU client
  if (!ModbusRTUClient.begin(9600)) {
    Serial.println("Failed to start Modbus RTU Client!");
    while (1);
  }
  delay(500);
  Serial.println("FAFNIR TANK LEVEL");
}

void loop() {
  if(!client.connected()){
    reconnect();
  }
  client.loop();
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

  RtcDateTime now = Rtc.GetDateTime();
  updateLog(now);
  delay(5000);
}

void reconnect(){
  while(!client.connected()){
    Serial.print("Connecting to ");
    Serial.println(broker);
    if(client.connect("YOUR_BROKER_CLIENT", brokerUser, NULL)){
      Serial.print("Connected to ");
      Serial.println(broker);
    } else{
      Serial.println("Trying to connect again");
      delay(5000);
    }
  }
}

void updateLog(const RtcDateTime& dt){
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
  snprintf(logString, sizeof(logString), "%s %s\nVolume: %.1f(L)\nUllage: %.1f(L)\n",
           datestring, timestring, volume, ullage);
           
  if(volume >= 0, ullage >= 0){
    appendFile(SD, "/log.txt", logString);
    client.publish("YOUR_TOPIC", logString);
  }
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

void readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}

void displayInfo()
{
  Serial.print(F("Satellites: "));
  Serial.println(gps.satellites.value());
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}
