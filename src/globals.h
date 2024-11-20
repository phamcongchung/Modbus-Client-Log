#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <RtcDS3231.h>
#include <Wire.h>
#include <vector>

#define SIM_RXD       32
#define SIM_TXD       33
#define SIM_BAUD      115200
#define TINY_GSM_MODEM_SIM7600

#include <TinyGsmClient.h>

extern HardwareSerial SerialAT;
extern TinyGsm modem;
extern TinyGsmClient client;
extern PubSubClient mqtt;

// Define the ProbeData structure
struct ProbeData {
  float volume;
  float ullage;
  float temperature;
  float product;
  float water;
};
// External vectors for probe IDs, Modbus registers, and probe data
extern std::vector<int> probeId;
extern std::vector<uint16_t> modbusReg;
extern std::vector<ProbeData> probeData;

// GPRSS credentials
extern String apn;
extern String simPin;
extern String gprsUser;
extern String gprsPass;
// MQTT credentials
extern String topic;
extern String broker;
extern String brokerUser;
extern String brokerPass;
extern uint16_t port;
// GPS data
extern float latitude, longitude;
extern String latDir, longDir, altitude, speed;
// Create a string to hold the formatted MAC address and log data
extern uint8_t mac[6];
extern char macAdr[18];
extern char dateString[20];
extern char timeString[20];
extern char logString[300];
// Real-time variables
extern RtcDateTime now, compiled;
extern RtcDS3231<TwoWire> Rtc;

#endif