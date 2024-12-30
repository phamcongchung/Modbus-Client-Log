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
  String simPin;
  String apn;
  String gprsUser;
  String gprsPass;
  String topic;
  String broker;
  String brokerUser;
  String brokerPass;
  uint16_t port;
};

#endif