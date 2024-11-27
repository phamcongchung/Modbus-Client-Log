#ifndef GPS_H
#define GPS_H

#include <TinyGsmClient.h>

class GPS{
public:
    GPS(TinyGsm& modem) : modem(modem){}

    float latitude;
    float longitude;
    String latDir;
    String longDir;
    String altitude;
    String speed;
    const char* err;

    void init();
    void update();
private:
    TinyGsm& modem;
    void parseData(const String& gpsData);
    float coordConvert(String coord, String direction);
    String getValue(const String& data, char separator, int index);
};

#endif