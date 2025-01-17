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
  String simPin;
  String apn;
  String user;
  String pass;
};

struct MQTT{
  String broker;
  String topic;
  String user;
  String pass;
  uint16_t port;
};

struct API{
  String host;
  String user;
  String pass;
  uint16_t port;
};

#endif