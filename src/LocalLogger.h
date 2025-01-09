// Manage logging and reading data on the local board
#ifndef LOCALLOGGER_H
#define LOCALLOGGER_H

#include <Arduino.h>
#include <type_traits>
#include <EEPROM.h>
#include <FS.h>
#include "RTC.h"

template<typename T>
void deleteFlash(int adr){
  if constexpr(std::is_same<T, String>::value){
    for(int i = 0; i < 20; i++){
      EEPROM.write(adr + i, 0);
    }
    EEPROM.commit();
  } else if constexpr(std::is_same<T, size_t>::value){
    for(int i = 0; i < sizeof(T); i++){
      EEPROM.write(adr + i, 0);
    }
    EEPROM.commit();
  }
}

template<typename T>
T readFlash(int adr){
// Handle invalid address; return default-constructed T
  if(adr < 0 || adr >= EEPROM.length()){
    return T{};
  }
  if constexpr(std::is_same<T, String>::value){
    String str = "";
    char read;
    // Read characters until null terminator is found
    for(int i = adr; i < EEPROM.length() && i < EEPROM.length(); i++){
      read = EEPROM.read(i);
      if (read == '\0') break; // Stop at null terminator
      str += (char)read;
    }
    return str;
  } else if constexpr(std::is_same<T, size_t>::value){
    size_t val = 0;
    for(size_t i = 0; i < sizeof(size_t); i++){
      val |= ((size_t)EEPROM.read(adr + i)) << (8 * i);
    }
    return val;
  }
  return T{};
}

void saveToFlash(int adr, String str);
void saveToFlash(int adr, size_t value);

String readCsv(const String& row);
String dataToJson(String rows[], int rowCount);
String errorToJson(String rows[], int rowCount);
void errLog(const char* msg, RTC& rtc);

File openFile(fs::FS &fs, const char* path);
bool appendFile(fs::FS &fs, const char* path, const char* message);

#endif