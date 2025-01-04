#include "Modem.h"

void Modem::simUnlock(const ConfigManager& cm){
  this->sendAT("+CPIN=" + String(cm.creds.simPin));
}

bool Modem::gprsConnect(const ConfigManager& cm) {
  const char* cred1 = cm.creds.apn;
  const char* cred2 = cm.creds.gprsUser;
  const char* cred3 = cm.creds.gprsPass;
  return TinyGsmSim7600::gprsConnect(cred1, cred2, cred3);
}

String Modem::sendATCmd(const String& cmd, unsigned long timeout){
  this->sendAT(cmd);
  String response;
  if(this->waitResponse(timeout) == 1){
    while (serial->available()){
      char c = serial->read();
      response += c;
      if (response.endsWith("\r\n")){
        break;
      }
    }
  }
  response.trim();
  return response;
}