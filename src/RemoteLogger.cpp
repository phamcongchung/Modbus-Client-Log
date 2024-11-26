#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "globals.h"
#include "SDLogger.h"
#include "RemoteLogger.h"

PubSubClient mqtt(client);

void mqttInit(){
  mqtt.setServer(broker.c_str(), port);
  mqtt.setBufferSize(1024);
  mqtt.setCallback(mqttCallback);
  if(!mqtt.connected()){
    mqttReconnect();
  }
}

void remotePush(){
  if(!mqtt.connected()){
    mqttReconnect();
  }
  mqtt.loop();

  // Combine date and time into a single string
  char dateTimeString[40];
  snprintf(dateTimeString, sizeof(dateTimeString), "%s %s", dateString, timeString);
  Serial.print(dateTimeString);
  StaticJsonDocument<1024> data;
  data["Device"] = macAdr;
  data["Date/Time"] = dateTimeString;

  JsonObject gps = data.createNestedObject("Gps");
  gps["Latitude"] = latitude;
  gps["Longitude"] = longitude;
  gps["Altitude"] = altitude.toFloat();
  gps["Speed"] = speed.toFloat();

  JsonArray measures = data.createNestedArray("Measure");
  for(size_t i = 0; i < probeId.size(); i++){
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
      return;
    } else {
      Serial.print("Failed, ");
      printError(mqtt.state());
      Serial.println("Try again in 5 seconds");
      vTaskDelay(pdMS_TO_TICKS(5000));
    }
  }
}

void printError(int state){
  switch (mqtt.state()) {
    case MQTT_CONNECTION_TIMEOUT:
      Serial.println("connection timed out");
      errLog("MQTT connection timed out");
      break;
    case MQTT_CONNECTION_LOST:
      Serial.println("connection lost");
      errLog("MQTT connection lost");
      break;
    case MQTT_CONNECT_FAILED:
      Serial.println("connection failed");
      errLog("MQTT connection failed");
      break;
    case MQTT_DISCONNECTED:
      Serial.println("disconnected");
      errLog("MQTT disconnected");
      break;
    case MQTT_CONNECT_BAD_PROTOCOL:
      Serial.println("bad protocol");
      errLog("MQTT bad protocol");
      break;
    case MQTT_CONNECT_BAD_CLIENT_ID:
      Serial.println("bad Client ID");
      errLog("MQTT bad client");
      break;
    case MQTT_CONNECT_UNAVAILABLE:
      Serial.println("server unavailable");
      errLog("MQTT server unavailable");
      break;
    case MQTT_CONNECT_BAD_CREDENTIALS:
      Serial.println("bad username or password");
      errLog("MQTT bad username or password");
      break;
    case MQTT_CONNECT_UNAUTHORIZED:
      Serial.println("unauthorized");
      errLog("MQTT unauthorized");
      break;
    default:
      Serial.println("unknown error");
      errLog("MQTT unknown error");
      break;
  }
}

void mqttCallback(char* topic, byte* message, unsigned int len){
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