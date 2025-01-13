#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <vector>
#include "RemoteLogger.h"
#include "Modem.h"
#include "structs.h"

class ConfigManager{
public:
  ConfigManager(RemoteLogger& extRemote, Modem& extModem) : remote(extRemote), modem(extModem) {}

  const char* lastError;
  std::vector<int> probeId;
  std::vector<uint16_t> modbusReg;
  
  bool readTank();
  bool readGprs();
  bool readMqtt();
  bool readApi();
private:
  Modem& modem;
  RemoteLogger& remote;
  friend class Modem;
  friend class RemoteLogger;
};

#endif