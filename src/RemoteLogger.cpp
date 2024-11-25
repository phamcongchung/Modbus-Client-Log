#include <ArduinoJson.h>
#include "SDLogger.h"
#include "RemoteLogger.h"

void RemoteLogger::init(char macAdr[18]){
  mqtt.setServer(config.broker.c_str(), config.port);
  mqtt.setBufferSize(1024);
  mqtt.setCallback(callback);
  if(!mqtt.connected()){
    reconnect(macAdr);
  }
}

void RemoteLogger::push(char macAdr[18]){
  if(!mqtt.connected()){
    reconnect(macAdr);
  }
  mqtt.loop();

  // Combine date and time into a single string
  
  Serial.print(rtc.getTime());
  StaticJsonDocument<1024> data;
  data["Device"] = macAdr;
  data["Date/Time"] = rtc.getTime();

  JsonObject gpsData = data.createNestedObject("Gps");
  gpsData["Latitude"] = gps.latitude;
  gpsData["Longitude"] = gps.longitude;
  gpsData["Altitude"] = gps.altitude.toFloat();
  gpsData["Speed"] = gps.speed.toFloat();

  JsonArray measures = data.createNestedArray("Measure");
  for(size_t i = 0; i < config.probeId.size(); i++){
    JsonObject measure = measures.createNestedObject();
    measure["Id"] = config.probeId[i];
    measure["Volume"] = modbus.probeData[i].volume;
    measure["Ullage"] = modbus.probeData[i].ullage;
    measure["Temperature"] = modbus.probeData[i].temperature;
    measure["ProductLevel"] = modbus.probeData[i].product;
    measure["WaterLevel"] = modbus.probeData[i].water;
  }
  // Serialize JSON and publish
  char buffer[1024];
  size_t n = serializeJson(data, buffer);
  mqtt.publish(config.topic.c_str(), buffer, n);
}

void RemoteLogger::reconnect(char macAdr[18]){
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(macAdr, config.brokerUser.c_str(), config.brokerPass.c_str())) {
      Serial.println("Connected");
      // Subscribe
      mqtt.subscribe(config.topic.c_str());
      return;
    } else {
      Serial.print("Failed, ");
      mqttErr(mqtt.state());
      Serial.println("Try again in 5 seconds");
      vTaskDelay(pdMS_TO_TICKS(5000));
    }
  }
}

void callback(char* topic, byte* message, unsigned int len){
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