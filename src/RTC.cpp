/*#include "SDLogger.h"
#include "RTC.h"

void RTC::init(){
  // Initialize DS3231 communication
  Rtc.Begin();
  compiled = RtcDateTime(__DATE__, __TIME__);
  Serial.println();
  // Disable unnecessary functions of the RTC DS3231
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
  // Compare RTC time with compile time
  if (!Rtc.IsDateTimeValid()){
    if ((Rtc.LastError()) != 0)
    {
    Serial.print("RTC communication error = ");
    Serial.println(Rtc.LastError());
    delay(1000);
    }
    else
    {
      if (now < compiled)
      {
        Serial.println("RTC is older than compile time! (Updating DateTime)");
        Rtc.SetDateTime(compiled);
      }
      else if (now > compiled)
      {
        Serial.println("RTC is newer than compiled time! (as expected)");
      }
      else if (now == compiled)
      {
        Serial.println("RTC is the same as compile time! (not expected but acceptable)");
      }
    }
  }
}

const char* RTC::getTime(){
    if (!Rtc.GetIsRunning())
    {
      Serial.println("RTC was not actively running, starting now");
      Rtc.Begin();
      Rtc.SetIsRunning(true);
    }
    now = Rtc.GetDateTime();
    snprintf_P(date,
            countof(date),
            PSTR("%02u/%02u/%04u"),
            now.Month(),
            now.Day(),
            now.Year());
    snprintf_P(time,
            countof(time),
            PSTR("%02u:%02u:%02u"),
            now.Hour(),
            now.Minute(),
            now.Second());
    char dateTime[40];
    snprintf(dateTime, sizeof(dateTime), "%s %s", date, time);
    return dateTime;
}*/
