/*#include <Arduino.h>
#include "SDLogger.h"

bool SDLogger::init(){
  // Initialize the microSD card
  if(SD.begin(5)){
    for(size_t i = 0; i < config.probeId.size(); i++){
    String fileName = "/log" + String(i + 1) + ".csv";
    File file = SD.open(fileName.c_str());
    if(!file){
      writeFile(SD, fileName.c_str(), "Date,Time,Latitude,Longitude,Speed(km/h),Altitude(m)"
                                      "Volume(l),Ullage(l),Temperature(°C)"
                                      "Product level(mm),Water level(mm)\n");
    }
    file.close();
    }
    File file = SD.open("/error.csv");
    if(!file){
      writeFile(SD, "/error.csv", "Date,Time,Error");
    }
    file.close();
    return true;
  } else {
    return false;
  }
}

void SDLogger::log(Location location, std::vector<ProbeData>& probeData, const char* time){
  for(size_t i = 0; i < config.probeId.size(); i++){
    String fileName = "/log" + String(i + 1) + ".csv";
    snprintf(logString, sizeof(logString), "%s;%s;%f;%f;%s;%s;%.1f;%.1f;%.1f;%.1f;%.1f\n",
            time, location.latitude, location.longitude, location.speed, location.altitude,
            probeData[i].volume, probeData[i].ullage, probeData[i].temperature,
            probeData[i].product, probeData[i].water);
    appendFile(SD, fileName.c_str(), logString);
  }
}

void SDLogger::errLog(const char* time, const char* msg){
  String errMsg = String(time) + "," + String(msg);
  File file = SD.open("/error.csv");
  appendFile(SD, "/error.csv", errMsg.c_str());
}

void appendFile(fs::FS &fs, const char* path, const char* message){
  Serial.printf("Appending file: %s", path);
  
  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.print("File does not exist, creating file...\n");
    file = fs.open(path, FILE_WRITE);  // Create the file
    if(!file){
      Serial.print("Failed to create file\n");
      return;
    }
  }
  if(file.print(message)){
    Serial.print("Message appended\n");
  } else {
    Serial.print("Append failed\n");
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.print("Failed to open file for writing\n");
    return;
  }
  if(file.print(message)) {
    Serial.print("File written\n");
  } else {
    Serial.print("Write failed\n");
  }
  file.close();
}*/