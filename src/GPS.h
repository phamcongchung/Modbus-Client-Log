#ifndef GPS_H
#define GPS_H

void gpsInit();
void gpsUpdate();
void gpsParse(const String& gpsData);
float coordConvert(String coord, String direction);
String getValue(const String& data, char separator, int index);

#endif