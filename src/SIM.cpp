#include <Arduino.h>
#include "SIM.h"

//HardwareSerial SerialAT(1);
//TinyGsm modem(SerialAT);
//TinyGsmClient client(modem);

void SIM::init(){
  SerialAT.begin(115200, SERIAL_8N1, SIM_RXD, SIM_TXD);
  Serial.println("Initializing modem...");
  modem.restart();
  if (modem.getSimStatus() == 2){
    Serial.println("SIM PIN required.");
    // Send the PIN to the modem
    modem.sendAT("+CPIN=" + config.simPin);
    if (modem.getSimStatus() == 1) {
      Serial.println("SIM unlocked successfully.");
    } else {
      Serial.println("Failed to unlock SIM.");
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