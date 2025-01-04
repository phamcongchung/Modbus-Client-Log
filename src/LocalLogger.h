// Manage logging and reading data on the local board
#ifndef LOCALLOGGER_H
#define LOCALLOGGER_H

#include <Arduino.h>
#include <FS.h>
#include "RTC.h"

String readFlash(int startAdr);
String readCsv(const String& row);
String csvToJson(String rows[], int rowCount);
void errLog(const char* msg, RTC& rtc);
void saveToFlash(int startAdr, String string);
bool appendFile(fs::FS &fs, const char* path, const char* message);

#endif