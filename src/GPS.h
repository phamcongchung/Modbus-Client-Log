#ifndef GPS_H
#define GPS_H

#define TINY_GSM_MODEM_SIM7600
#define TIMEOUT          10000

#include "structs.h"
#include "Modem.h"

enum State{
  GPS_OK            = 1,
  GPS_NO_RESPONSE   = 0,
  GPS_INVALID_DATA  = 2,
};

class GPS{
public:
  GPS(Modem& modem) : modem(modem){}
  Location location;

  bool init();
  State update();
private:
  Modem& modem;
  double coordConvert(String coord, String direction);
  String getValue(const String& data, char separator, int index);
};

#endif