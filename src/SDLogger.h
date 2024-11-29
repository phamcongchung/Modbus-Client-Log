/*#ifndef SDLOGGER_H
#define SDLOGGER_H

#include <SD.h>
#include "GPS.h"
#include "ModbusCom.h"
#include "ConfigManager.h"

class SDLogger{
public:
  using LogCallback = std::function<void(const String&)>;

  SDLogger(ConfigManager& config, LogCallback logger = nullptr) : config(config), logger(logger){}

  const char* err;

  bool init();
  void log(Location location, std::vector<ProbeData>& probeData, const char* time);
  void errLog(const char* time, const char* msg);
private:
  char logString[300];
  ConfigManager& config;
  LogCallback logger;

  void appendFile(fs::FS &fs, const char * path, const char * message);
  void writeFile(fs::FS &fs, const char * path, const char * message);
};

#endif*/