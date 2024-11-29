/*#ifndef MODBUSCOM_H
#define MODBUSCOM_H

#include <vector>
#include "ConfigManager.h"

struct ProbeData {
  float volume;
  float ullage;
  float temperature;
  float product;
  float water;
};

class ModbusCom{
public:
  ModbusCom(ConfigManager& config) : config(config){}

  std::vector<ProbeData> probeData;

  void init();
  void read();
  void error(const char* err);
private:
  ConfigManager& config;
};

#endif*/