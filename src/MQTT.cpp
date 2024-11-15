#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "globals.h"
#include "MQTT.h"

PubSubClient mqtt(client);

void mqttInit(){
  mqtt.setServer(broker.c_str(), port);
  mqtt.setBufferSize(1024);
  mqtt.setCallback(mqttCallback);
}

void remotePush(){
  if(!mqtt.connected()){
    mqttReconnect();
  }
  mqtt.loop();

  // Combine date and time into a single string
  char dateTimeString[40];
  snprintf(dateTimeString, sizeof(dateTimeString), "%s %s", dateString, timeString);

  StaticJsonDocument<1024> data;
  data["Device"] = macAdr;
  data["Date/Time"] = dateTimeString;

  JsonObject gps = data.createNestedObject("Gps");
  gps["Latitude"] = latitude;
  gps["Longitude"] = longitude;
  gps["Altitude"] = altitude.toFloat();
  gps["Speed"] = speed.toFloat();

  JsonArray measures = data.createNestedArray("Measure");
  for(int i = 0; i < probeId.size(); i++){
    JsonObject measure = measures.createNestedObject();
    measure["Id"] = probeId[i];
    measure["Volume"] = probeData[i].volume;
    measure["Ullage"] = probeData[i].ullage;
    measure["Temperature"] = probeData[i].temperature;
    measure["ProductLevel"] = probeData[i].product;
    measure["WaterLevel"] = probeData[i].water;
  }
  // Serialize JSON and publish
  char buffer[1024];
  size_t n = serializeJson(data, buffer);
  mqtt.publish(topic.c_str(), buffer, n);
}

void mqttReconnect(){
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(macAdr, brokerUser.c_str(), brokerPass.c_str())) {
      Serial.println("Connected");
      // Subscribe
      mqtt.subscribe(topic.c_str());
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqtt.state());
      Serial.println("Try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(1000);
    }
  }
}

void mqttCallback(char* topic, byte* message, unsigned int len) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < len; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}