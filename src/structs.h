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

struct Network{
  const char* simPin;
  const char* apn;
  const char* gprsUser;
  const char* gprsPass;
  const char* topic;
  const char* broker;
  const char* brokerUser;
  const char* brokerPass;
  uint16_t port;
};

#endif