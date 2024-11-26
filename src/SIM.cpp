#include <Arduino.h>
#include <string>
#include "globals.h"
#include "SDLogger.h"
#include "SIM.h"

HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);

void modemInit(){
  SerialAT.begin(115200, SERIAL_8N1, SIM_RXD, SIM_TXD);
  Serial.println("Initializing modem...");
  //modem.restart();
  if (modem.getSimStatus() == 2){
    Serial.println("SIM PIN required.");
    // Send the PIN to the modem
    modem.sendAT("+CPIN=" + simPin);
    if (modem.getSimStatus() == 1) {
      Serial.println("SIM unlocked successfully.");
    } else {
      Serial.println("Failed to unlock SIM.");
    }
  }
  modemConnect();
}

bool modemConnect() {
  Serial.print("Connecting to APN: ");
  Serial.println(apn);
  if (!modem.gprsConnect(apn.c_str(), gprsUser.c_str(), gprsPass.c_str())) {
    Serial.println("GPRS connection failed");
    errorLog("GPRS connection failed");
    return false;
    modem.restart();
  }
  else {
    Serial.println("GPRS connected");
    return true;
  }
}

void gpsEn(){
  // Enable GPS
  Serial.println("Enabling GPS...");
  modem.sendAT("+CGPS=1,1");  // Start GPS in standalone mode
  modem.waitResponse(10000L);
  Serial.println("Waiting for GPS data...");
}