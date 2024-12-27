#ifndef CSVMANAGER_H
#define CSVMANAGER_H

#include <Arduino.h>
#include <SD.h>

String timeStamp;
String bearerToken;

String csv2Json(String rows[], int rowCount);
String readFromCsv(const String& row);
String readStrFromFlash(int startAdr);
bool sendData(String& jsonPayload);

void saveStr2Flash(int startAdr, String timeStamp);
void processCsv(File data, int fileNo);
void getToken();

#endif