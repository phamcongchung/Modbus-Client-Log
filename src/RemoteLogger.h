#ifndef REMOTELOGGER_H
#define REMOTELOGGER_H

void mqttInit();
void remotePush();
void mqttReconnect();
void printError(int state);
void mqttCallback(char* topic, byte* message, unsigned int len);

#endif