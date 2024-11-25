#ifndef RTC_H
#define RTC_H

#include <Wire.h>
#include <RtcDS3231.h>

class RTC{
public:
    RTC(TwoWire& wire) : Rtc(wire){}

    char date[20];
    char time[20];
    RtcDateTime now;
    RtcDateTime compiled;

    void init();
    const char* getTime();
private:
    RtcDS3231<TwoWire> Rtc;
};

#endif