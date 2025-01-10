#include "Modem.h"

Modem& Modem::setCreds(GPRS& gprs){
  this->gprs = gprs;
  return *this;
}

void Modem::simUnlock(){
  this->sendAT("+CPIN=" + String(this->gprs.simPin));
}

bool Modem::gprsConnect(){
  return TinyGsmSim7600::gprsConnect(this->gprs.apn, this->gprs.user, this->gprs.pass);
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