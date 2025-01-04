#include <Arduino.h>
#include "GPS.h"

bool GPS::init(){
  // Enable GPS
  String response = modem.sendATCmd("+CGPS=1,1", TIMEOUT);
  return response.indexOf("OK") != -1;
}

State GPS::update(){
  modem.sendAT("+CGPSINFO");
  if (modem.waitResponse(10000L, "+CGPSINFO:") == 1) {
    String data = modem.stream.readStringUntil('\n');
    // Check if the data contains invalid GPS values
    if (data.indexOf(",,,,,,,,") != -1) {
      this->location = {-1, -1, -1, -1};
      return GPS_INVALID_DATA;
    } else {
      String rawLat = getValue(data, ',', 0); String latDir = getValue(data, ',', 1);
      String rawLong = getValue(data, ',', 2); String longDir = getValue(data, ',', 3);
      this->location.latitude = coordConvert(rawLat, latDir);
      this->location.longitude = coordConvert(rawLong, longDir);
      this->location.speed = getValue(data, ',', 7).toDouble();
      this->location.altitude = getValue(data, ',', 6).toDouble();
      return GPS_OK;
    }
  } else {
    this->location = {-1, -1, -1, -1};
    return GPS_NO_RESPONSE;
  }
}
/*
const char* GPS::lastError(){
  switch (this->err){
    case GPS_OK:
      return NULL;
    case GPS_NO_RESPONSE:
      return "No respone from GPS";
    case GPS_INVALID_DATA:
      return "GPS data is invalid";
    default:
      return "Unknown GPS error";
  }
}
*/
double GPS::coordConvert(String coord, String direction){
  // First two or three digits are degrees
  int degrees = coord.substring(0, coord.indexOf('.')).toInt() / 100;
  // Remaining digits are minutes
  double minutes = coord.substring(coord.indexOf('.') - 2).toFloat();
  // Convert to decimal degrees
  double decimalDegrees = degrees + (minutes / 60);
  // South and West are negative
  if (direction == "S" || direction == "W") {
    decimalDegrees = -decimalDegrees; 
  }
  return decimalDegrees;
}

String GPS::getValue(const String& data, char separator, int index) {
  int startIndex = 0;
  for (int i = 0; i <= index; i++) {
    int endIndex = data.indexOf(separator, startIndex);
    if (i == index) {
      return (endIndex == -1) ? data.substring(startIndex) : data.substring(startIndex, endIndex);
    }
    startIndex = endIndex + 1;
  }
  return "";
}