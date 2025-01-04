#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <vector>
#include "structs.h"

class ConfigManager{
public:
  const char* lastError;
  std::vector<int> probeId;
  std::vector<uint16_t> modbusReg;
  
  bool readTank();
  bool readNetwork();

  uint16_t port() const;
  const char* apn() const;
  const char* topic() const;
  const char* broker() const;
  const char* simPin() const;
  const char* gprsUser() const;
  const char* gprsPass() const;
  const char* brokerUser() const;
  const char* brokerPass() const;

private:
  Network creds;
  friend class Modem;
  friend class RemoteLogger;
};

#endif