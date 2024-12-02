#ifndef GPS_H
#define GPS_H

#define TINY_GSM_MODEM_SIM7600

#include <TinyGsmClient.h>

enum Error{
  GPS_OK,
  GPS_NO_RESPONSE,
  GPS_INVALID_DATA
};

struct Location{
  double speed;
  double altitude;
  double latitude;
  double longitude;
};

class GPS{
public:
  GPS(TinyGsm& modem) : modem(modem){}

  Location location;

  void init();
  Location update();
  const char* lastError();
private:
  Error err;
  TinyGsm& modem;
  double coordConvert(String coord, String direction);
  String getValue(const String& data, char separator, int index);
};

#endif