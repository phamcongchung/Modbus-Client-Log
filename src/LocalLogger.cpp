#include "LocalLogger.h"
#include <SD.h>

void saveToFlash(int adr, String str){
  int length = str.length();
  if (adr + length + 1 > EEPROM.length()){
    Serial.println("Error: Not enough EEPROM space to save the string.");
    return;
  }
  // Convert the String to a char array
  char charArray[length + 1];
  str.toCharArray(charArray, length + 1);
  // Write each character to EEPROM
  for (int i = 0; i < length; i++){
    EEPROM.write(adr + i, charArray[i]);
  }
  // Write null terminator
  EEPROM.write(adr + length, '\0');
  // Commit changes (if using ESP32 or ESP8266)
  EEPROM.commit();
}

void saveToFlash(int adr, size_t value){
  for(size_t i = 0; i < sizeof(size_t); i++){
    EEPROM.write(adr + i, (value >> (8 * i)) & 0xFF);
  }
  EEPROM.commit();
}

// Read csv rows and return time data
String readCsv(const String& row){
  int commaIndex = row.indexOf(';');
  if (commaIndex != -1){
    return row.substring(0, commaIndex);
  }
  return "";
}

String dataToJson(String rows[], int rowCount){
  String jsonPayload = "[";
  for (int i = 0; i < rowCount; i++){
    if(i > 0) jsonPayload += ",";
    // Split the row by semicolons
    String fields[10];
    int fieldIndex = 0;
    while(rows[i].length() > 0 && fieldIndex < 10){
      int delimiterIndex = rows[i].indexOf(';');
      if (delimiterIndex == -1) {
        fields[fieldIndex++] = rows[i];
        rows[i] = "";
      } else {
        fields[fieldIndex++] = rows[i].substring(0, delimiterIndex);
        rows[i] = rows[i].substring(delimiterIndex + 1);
      }
    }
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

String errorToJson(String rows[], int rowCount){
  String jsonPayload = "[";
  for(int i = 0; i < rowCount; i++){
    if(i > 0) jsonPayload += ",";
    // Split the row by semicolons
    String fields[2];
    int fieldIndex = 0;
    while(rows[i].length() > 0 && fieldIndex < 2){
      int delimiterIndex = rows[i].indexOf(';');
      if(delimiterIndex == -1){
        fields[fieldIndex++] = rows[i];
        rows[i] = "";
      } else {
        fields[fieldIndex++] = rows[i].substring(0, delimiterIndex);
        rows[i] = rows[i].substring(delimiterIndex + 1);
      }
    }
    jsonPayload += "{";
    jsonPayload += "\"time\":\"" + fields[0] + "\",";
    jsonPayload += "\"errorMessage\":\"" + fields[1] + "\"";
    jsonPayload += "}";
  }
  jsonPayload += "]";
  return jsonPayload;
}

void errLog(const char* msg, RTC& rtc){
  // Calculate the required buffer size dynamically
  size_t msgSize = 20 + strlen(msg) + 3; // 2 for ',' and '\n', 1 for '\0'
  char* errMsg = (char*)malloc(msgSize);
  if (errMsg == NULL) {
    Serial.println("Failed to allocate memory for error message!");
    return;
  }
  snprintf(errMsg, msgSize, "\n%s;%s", rtc.getTimeStr().c_str(), msg);
  appendFile(SD, "/error.csv", errMsg);
  free(errMsg);
}

bool appendFile(fs::FS &fs, const char* path, const char* message){
  Serial.printf("Appending file: %s...", path);
  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.print("File does not exist, creating file...");
    file = fs.open(path, FILE_WRITE);  // Create the file
    if(!file){
      Serial.println("Failed to create file");
      return false;
    }
  }
  if(file.print(message)){
    Serial.println("Message appended");
    file.close();
    return true;
  } else {
    Serial.println("Append failed");
  }
  file.close();
  return false;
}

File openFile(fs::FS &fs, const char* path){
  File data = fs.open(path, FILE_READ);
  if(!data){
    Serial.println("Data is non-existent");
  }
  return data;
}