#ifndef GPS_H
#define GPS_H

#include "SIM.h"

class GPS{
public:
    GPS(SIM& sim) : sim(sim){}

    float latitude;
    float longitude;
    String latDir;
    String longDir;
    String altitude;
    String speed;

    void init();
    void update();
    void parseData(const String& gpsData);
    void error(const char* err);
    float coordConvert(String coord, String direction);
    String getValue(const String& data, char separator, int index);
private:
    SIM& sim;
};

#endif