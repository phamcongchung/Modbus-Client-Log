#ifndef SDLOGGER_H
#define SDLOGGER_H

#include <SD.h>
#include "RTC.h"
#include "GPS.h"
#include "SIM.h"
#include "ModbusCom.h"
#include "ConfigManager.h"

class SDLogger{
public:
    SDLogger(ConfigManager& config, ModbusCom& modbus, GPS& gps, RTC& rtc)
            : config(config), modbus(modbus), gps(gps), rtc(rtc){}

    void init();
    void log();
    void errLog(const char* msg);
private:
    ConfigManager& config;
    ModbusCom& modbus;
    GPS& gps;
    RTC& rtc;
    char logString[300];
};
void appendFile(fs::FS &fs, const char * path, const char * message);
void writeFile(fs::FS &fs, const char * path, const char * message);

#endif