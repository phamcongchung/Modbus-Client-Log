#include <Arduino.h>
#include <string>
#include "globals.h"
#include "Modem.h"

HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);

void modemInit(){
    SerialAT.begin(115200, SERIAL_8N1, SIM_RXD, SIM_TXD);
}

void modemConnect() {
  Serial.println("Initializing modem...");
  // modem.restart();
  
  Serial.print("Connecting to APN: ");
  Serial.println(apn);
  if (!modem.gprsConnect(apn.c_str(), gprsUser.c_str(), gprsPass.c_str())) {
    Serial.println("Fail to connect to LTE network");
    ESP.restart();
  }
  else {
    Serial.println("OK");
  }

  if (modem.isGprsConnected()) {
    Serial.println("GPRS connected");
  }
}

void enGPS(){
  // Enable GPS
  Serial.println("Enabling GPS...");
  modem.sendAT("+CGPS=1,1");  // Start GPS in standalone mode
  modem.waitResponse(10000L);
  Serial.println("Waiting for GPS data...");
}