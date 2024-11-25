#include <Arduino.h>
#include "SDLogger.h"

void SDLogger::init(){
  // Initialize the microSD card
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

  // If the log.csv file doesn't exist
  // Create a file on the SD card and write the data labels
  for(size_t i = 0; i < config.probeId.size(); i++){
    String fileName = "/log" + String(i + 1) + ".csv";
    File file = SD.open(fileName.c_str());
    if(!file){
      Serial.println("File doens't exist. Creating file...");
      writeFile(SD, fileName.c_str(), "Date,Time,Latitude,Longitude,Speed(km/h),Altitude(m)"
                                      "Volume(l),Ullage(l),Temperature(°C)"
                                      "Product level(mm),Water level(mm)\n");
    } else {
      Serial.println("File already exists");  
    }
    file.close();
  }
  // If the error.csv file doesn't exist
  // Create a file on the SD card and write the data labels
  File file = SD.open("/error.csv");
  if(!file){
    Serial.println("File doens't exist. Creating file...");
    writeFile(SD, "/error.csv", "Date,Time,Error");
  } else {
    Serial.println("File already exists");  
  }
  file.close();
}

void SDLogger::log(){
  for(size_t i = 0; i < config.probeId.size(); i++){
    String fileName = "/log" + String(i + 1) + ".csv";
    snprintf(logString, sizeof(logString), "%s;%s;%f;%f;%s;%s;%.1f;%.1f;%.1f;%.1f;%.1f\n",
            rtc.date, rtc.time, gps.latitude, gps.longitude, gps.speed, gps.altitude,
            modbus.probeData[i].volume, modbus.probeData[i].ullage, modbus.probeData[i].temperature,
            modbus.probeData[i].product, modbus.probeData[i].water);
    appendFile(SD, fileName.c_str(), logString);
  }
}

void SDLogger::errLog(const char* msg){
  String errMsg = String(rtc.getTime()) + "," + String(msg);
  File file = SD.open("/error.csv");
  appendFile(SD, "/error.csv", errMsg.c_str());
}

void appendFile(fs::FS &fs, const char* path, const char* message){
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