#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <vector>

struct ProbeData {
  float volume;
  float ullage;
  float temperature;
  float product;
  float water;
};

class ConfigManager{
public:
  const char* lastError;
  // GPRSS credentials
  String apn;
  String simPin;
  String gprsUser;
  String gprsPass;
  // MQTT credentials
  String topic;
  String broker;
  String brokerUser;
  String brokerPass;
  uint16_t port;
  // External vectors
  std::vector<int> probeId;
  std::vector<uint16_t> modbusReg;

  bool getTank();
  bool getNetwork();
};

extern ConfigManager config;

#endif