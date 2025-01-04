#include "RTC.h"

// Return a string of RtcDateTime value
String RTC::getTimeStr(){
  char timeBuffer[20];
  RtcDateTime time = this->GetDateTime();
  snprintf_P(timeBuffer, 20,
            PSTR("%02u-%02u-%04u %02u:%02u:%02u"),
            time.Day(), time.Month(), time.Year(),
            time.Hour(), time.Minute(), time.Second());
  return String(timeBuffer);
}

// Save RtcDateTime variable in a buffer
void RTC::saveTime(char timeBuffer[20]){
  RtcDateTime time = this->GetDateTime();
  snprintf_P(timeBuffer, 20,
            PSTR("%02u-%02u-%04u %02u:%02u:%02u"),
            time.Day(), time.Month(), time.Year(),
            time.Hour(), time.Minute(), time.Second());
}