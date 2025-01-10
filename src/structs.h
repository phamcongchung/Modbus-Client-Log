#ifndef STRUCTS_H
#define STRUCTS_H

#include <Arduino.h>

struct ProbeData{
  float volume;
  float ullage;
  float temperature;
  float product;
  float water;
};

struct Location{
  double speed;
  double altitude;
  double latitude;
  double longitude;
};

struct GPRS{
  const char* simPin;
  const char* apn;
  const char* user;
  const char* pass;
};

struct MQTT{
  const char* broker;
  const char* topic;
  const char* user;
  const char* pass;
  uint16_t port;
};

struct API{
  const char* host;
  const char* user;
  const char* pass;
  uint16_t port;
};

#endif