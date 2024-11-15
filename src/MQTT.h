#ifndef MQTT_H
#define MQTT_H

void mqttInit();
void remotePush();
void mqttReconnect();
void mqttCallback(char* topic, byte* message, unsigned int len);

#endif