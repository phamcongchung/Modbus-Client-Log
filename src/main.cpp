#include <Arduino.h>
#include <ModbusRTUClient.h>
#include <ArduinoRS485.h>
#include <PubSubClient.h>
#include <iostream>
#include <string>
#include <Wifi.h>
#include <Wire.h>
#include <SPI.h>
#include "RtcDS3231.h"
#include "FS.h"
#include "SD.h"
using namespace std;

RtcDS3231 <TwoWire> Rtc(Wire);
float volume;
float ullage;
char logString[100];
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASS";
const char* brokerUser = "YOUR_BROKER_CLIENT_NAME";
const char* broker = "YOUR_BROKER_ADDRESS";

WiFiClient espClient;
PubSubClient client(espClient);

void updateLog(const RtcDateTime& dt);
void appendFile(fs::FS &fs, const char * path, const char * message);
void testFileIO(fs::FS &fs, const char * path);
void readFile(fs::FS &fs, const char * path);
void reconnect();

void setup() {
  // Initialize serial communication
  Serial.begin(115200); // Or any baud rate your Modbus device uses
  Serial.print("Compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);
  delay(500);

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

  testFileIO(SD, "/test.txt");
  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
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
  
  long lastMsg = 0;
  long now = millis();
  if(now - lastMsg > 5000){
    volume = ModbusRTUClient.holdingRegisterRead<float>(1, 0x0036, BIGEND);
    if (volume < 0) {
      Serial.print("Failed to read volume: ");
      Serial.println(ModbusRTUClient.lastError());
    } else {
      Serial.print("Volume: ");
      Serial.println(volume);
    }

    ullage = ModbusRTUClient.holdingRegisterRead<float>(1, 0x003C, BIGEND);
    if (ullage < 0) {
      Serial.print("Failed to read ullage: ");
      Serial.println(ModbusRTUClient.lastError());
    } else {
      Serial.print("Ullage: ");
      Serial.println(ullage);
    }

    RtcDateTime now = Rtc.GetDateTime();
    updateLog(now);
    client.publish("YOUR_BROKER_TOPIC", logString);
    readFile(SD, "/log.txt");
  }
}

void reconnect(){
  while(!client.connected()){
    Serial.print("Connecting to ");
    Serial.println(broker);
    if(client.connect("YOUR_CLIENT_ID", brokerUser, NULL)){
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
  appendFile(SD, "/log.txt", logString);
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

void testFileIO(fs::FS &fs, const char * path){
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if(file){
    len = file.size();
    size_t flen = len;
    start = millis();
    while(len){
      size_t toRead = len;
      if(toRead > 512){
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }


  file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for(i=0; i<2048; i++){
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}