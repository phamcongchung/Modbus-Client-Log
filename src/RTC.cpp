#include "globals.h"
#include "SDLogger.h"
#include "RTC.h"

void rtcInit(){
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
        Rtc.Begin();
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

const char* getTime(){
    if (!Rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        init();
        Rtc.SetIsRunning(true);
    }
    now = Rtc.GetDateTime();
    snprintf_P(dateString,
            countof(dateString),
            PSTR("%02u/%02u/%04u"),
            now.Month(),
            now.Day(),
            now.Year());
    snprintf_P(timeString,
            countof(timeString),
            PSTR("%02u:%02u:%02u"),
            now.Hour(),
            now.Minute(),
            now.Second());
    char dateTimeString[40];
    snprintf(dateTimeString, sizeof(dateTimeString), "%s %s", dateString, timeString);
    return dateTimeString;
}
