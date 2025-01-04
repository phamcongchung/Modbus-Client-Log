#include "LocalLogger.h"
#include <EEPROM.h>
#include <SD.h>

String readFlash(int startAdr) {
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

String readCsv(const String& row) {
  int commaIndex = row.indexOf(',');
  if (commaIndex != -1) {
    return row.substring(0, commaIndex);
  }
  return "";
}

String csvToJson(String rows[], int rowCount) {
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

void saveToFlash(int startAdr, String string) {
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

void errLog(const char* msg, RTC& rtc){
  // Calculate the required buffer size dynamically
  size_t msgSize = 20 + strlen(msg) + 3; // 2 for ',' and '\n', 1 for '\0'
  char* errMsg = (char*)malloc(msgSize);
  if (errMsg == NULL) {
    Serial.println("Failed to allocate memory for error message!");
    return;
  }
  snprintf(errMsg, msgSize, "\n%;%s", rtc.getTimeStr().c_str(), msg);
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
    file.close();
    return false;
  }
}