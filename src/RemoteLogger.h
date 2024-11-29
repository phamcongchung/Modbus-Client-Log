/*#ifndef REMOTELOGGER_H
#define REMOTELOGGER_H

#include <PubSubClient.h>
#include "GPS.h"
#include "RTC.h"
#include "ModbusCom.h"
#include "ConfigManager.h"

class RemoteLogger{
public:
  RemoteLogger(ConfigManager& config, PubSubClient& mqtt)
              : config(config), mqtt(mqtt){}

  bool connect();
  void init(char macAdr[18]);
  void push(Location& location, std::vector<ProbeData>& probeData, const char* time);
  const char* lastError();
private:
  PubSubClient& mqtt;
  ConfigManager& config;

  const char* clientID;
};

void callback(char* topic, byte* message, unsigned int len);

#endif*/