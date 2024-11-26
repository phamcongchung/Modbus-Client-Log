#include <Arduino.h>
#include "SIM.h"

void SIM::init(){
  SerialAT.begin(115200, SERIAL_8N1, SIM_RXD, SIM_TXD);
  Serial.println("Initializing modem...");
  //modem.restart();
  if (modem.getSimStatus() != 1) {
    Serial.println("SIM not ready, checking for PIN...");
    if (modem.getSimStatus() == 2){
      Serial.println("SIM PIN required.");
      // Send the PIN to the modem
      modem.sendAT("+CPIN=" + config.simPin);
      delay(1000);
      if (modem.getSimStatus() != 1) {
        Serial.println("Failed to unlock SIM.");
        return;
      }
      Serial.println("SIM unlocked successfully.");
    } else {
      Serial.println("SIM not detected or unsupported status");
      gprsErr("SIM not detected or unsupported status");
      return;
    }
  }
  connect();
}

bool SIM::connect() {
  Serial.print("Connecting to APN: ");
  Serial.println(config.apn);
  if (!modem.gprsConnect(config.apn.c_str(), config.gprsUser.c_str(), config.gprsPass.c_str())) {
    Serial.println("GPRS connection failed");
    gprsErr("GPRS connection failed");
    return false;
    modem.restart();
  }
  else {
    Serial.println("GPRS connected");
    return true;
  }
}