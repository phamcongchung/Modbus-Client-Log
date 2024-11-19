#include <Arduino.h>
#include "globals.h"
#include "GPS.h"

void gpsInit(){
  // Enable GPS
  Serial.println("Enabling GPS...");
  modem.sendAT("+CGPS=1,1");  // Start GPS in standalone mode
  modem.waitResponse(10000L);
  Serial.println("Waiting for GPS data...");
}

void gpsUpdate(){
  modem.sendAT("+CGPSINFO");
  if (modem.waitResponse(10000L, "+CGPSINFO:") == 1) {
    String gpsData = modem.stream.readStringUntil('\n');

    // Check if the data contains invalid GPS values
    if (gpsData.indexOf(",,,,,,,,") != -1) {
      Serial.println("Error: GPS data is invalid (no fix or no data available).");
    } else {
      Serial.println("Raw GPS Data: " + gpsData);
      // Call a function to parse the GPS data if valid
      gpsParse(gpsData);
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  } else {
    Serial.println("GPS data not available or invalid.");
  }
}

void gpsParse(const String& gpsData){
  String rawLat = getValue(gpsData, ',', 0); latDir = getValue(gpsData, ',', 1);
  String rawLong = getValue(gpsData, ',', 2); longDir = getValue(gpsData, ',', 3);
  altitude = getValue(gpsData, ',', 6); speed = getValue(gpsData, ',', 7);
  latitude = coordConvert(rawLat, latDir);
  longitude = coordConvert(rawLong, longDir);
}

float coordConvert(String coord, String direction){
  // First two or three digits are degrees
  int degrees = coord.substring(0, coord.indexOf('.')).toInt() / 100;
  // Remaining digits are minutes
  float minutes = coord.substring(coord.indexOf('.') - 2).toFloat();
  // Convert to decimal degrees
  float decimalDegrees = degrees + (minutes / 60);
  // Apply direction (N/S or E/W)
  if (direction == "S" || direction == "W") {
    decimalDegrees = -decimalDegrees;  // South and West are negative
  }
  return decimalDegrees;
}

String getValue(const String& data, char separator, int index) {
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