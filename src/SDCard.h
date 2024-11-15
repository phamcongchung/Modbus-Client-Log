#ifndef SDCARD_H
#define SDCARD_H

#include <SD.h>

void sdInit();
void localLog();
void appendFile(fs::FS &fs, const char * path, const char * message);
void writeFile(fs::FS &fs, const char * path, const char * message);

#endif