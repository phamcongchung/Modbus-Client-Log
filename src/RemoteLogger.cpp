/*#include <ArduinoJson.h>
#include "SDLogger.h"
#include "RemoteLogger.h"

void RemoteLogger::init(char macAdr[18]){
  clientID = macAdr;
  mqtt.setServer(config.broker.c_str(), config.port);
  mqtt.setBufferSize(1024);
  mqtt.setCallback(callback);
}

void RemoteLogger::push(Location& location, std::vector<ProbeData>& probeData,
                        const char* time){
  StaticJsonDocument<1024> data;
  data["Device"] = String(clientID);
  data["Date/Time"] = time;

  JsonObject gpsData = data.createNestedObject("Gps");
  gpsData["Latitude"] = location.latitude;
  gpsData["Longitude"] = location.longitude;
  gpsData["Altitude"] = location.altitude;
  gpsData["Speed"] = location.speed;

  JsonArray measures = data.createNestedArray("Measure");
  for(size_t i = 0; i < config.probeId.size(); i++){
    JsonObject measure = measures.createNestedObject();
    measure["Id"] = config.probeId[i];
    measure["Volume"] = probeData[i].volume;
    measure["Ullage"] = probeData[i].ullage;
    measure["Temperature"] = probeData[i].temperature;
    measure["ProductLevel"] = probeData[i].product;
    measure["WaterLevel"] = probeData[i].water;
  }
  // Serialize JSON and publish
  char buffer[1024];
  size_t n = serializeJson(data, buffer);
  mqtt.publish(config.topic.c_str(), buffer, n);
}

bool RemoteLogger::connect(){
  if (!mqtt.connected()) {
    if (mqtt.connect(clientID, config.brokerUser.c_str(), config.brokerPass.c_str())) {
      mqtt.subscribe(config.topic.c_str());
      return true;
    } else {
      return false;
    }
  }
}

const char* RemoteLogger::lastError(){
  const char* errstr;
  switch (mqtt.state()) {
    case MQTT_CONNECTION_TIMEOUT:
      errstr = "MQTT connection timeout";
      return errstr;
      break;
    case MQTT_CONNECTION_LOST:
      errstr = "MQTT connection lost";
      return errstr;
      break;
    case MQTT_CONNECT_FAILED:
      errstr = "MQTT connection failed";
      return errstr;
      break;
    case MQTT_DISCONNECTED:
      errstr = "MQTT disconnected";
      return errstr;
      break;
    case MQTT_CONNECT_BAD_PROTOCOL:
      errstr = "MQTT bad protocol";
      return errstr;
      break;
    case MQTT_CONNECT_BAD_CLIENT_ID:
      errstr = "MQTT bad Client ID";
      return errstr;
      break;
    case MQTT_CONNECT_UNAVAILABLE:
      errstr = "MQTT server unavailable";
      return errstr;
      break;
    case MQTT_CONNECT_BAD_CREDENTIALS:
      errstr = "MQTT bad username or password";
      return errstr;
      break;
    case MQTT_CONNECT_UNAUTHORIZED:
      errstr = "MQTT unauthorized";
      return errstr;
      break;
    default:
      errstr = "MQTT unknown error";
      return errstr;
      break;
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
}*/