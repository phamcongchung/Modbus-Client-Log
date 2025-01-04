#ifndef RTC_H
#define RTC_H

#include <Arduino.h>
#include <RtcDS3231.h>
#include <Wire.h>

class RTC : public RtcDS3231<TwoWire>{
public:
  RTC(TwoWire& wire) : RtcDS3231(wire){}

  String getTimeStr();
  void saveTime(char timeBuffer[20]);
};

#endif