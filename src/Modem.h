#ifndef MODEM_H
#define MODEM_H

#include <Arduino.h>
#include <TinyGsmClientSIM7600.h>
#include "ConfigManager.h"
#include "Display.h"

class Modem : public TinyGsmSim7600{

public:
  Modem(HardwareSerial &extSerial, int rx, int tx, uint32_t baudRate)
    : TinyGsmSim7600(extSerial), serial(&extSerial){
    serial->begin(baudRate, SERIAL_8N1, rx, tx);
  }

  void simUnlock(const ConfigManager& cm);
  bool gprsConnect(const ConfigManager& cm);
  String sendATCmd(const String& cmd, unsigned long timeout);

private:
  HardwareSerial *serial;
};

#endif