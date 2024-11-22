#ifndef SDLOGGER_H
#define SDLOGGER_H

#include <SD.h>

void sdInit();
void dataLog();
void errorLog(const char* errorMsg);
void appendFile(fs::FS &fs, const char * path, const char * message);
void writeFile(fs::FS &fs, const char * path, const char * message);

#endif