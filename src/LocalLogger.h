// Manage logging and reading data on the local board
#ifndef LOCALLOGGER_H
#define LOCALLOGGER_H

#include <Arduino.h>
#include <type_traits>
#include <EEPROM.h>
#include <FS.h>
#include "RTC.h"

void saveToFlash(int adr, String str);
void saveToFlash(int adr, size_t value);

String readCsv(const String& row);
String dataToJson(String rows[], int rowCount);
String errorToJson(String rows[], int rowCount);
void errLog(const char* msg, RTC& rtc);

File openFile(fs::FS &fs, const char* path);
bool appendFile(fs::FS &fs, const char* path, const char* message);

#include "LocalLogger.tpp"

#endif