#include <SD.h>
#include <ArduinoJson.h>
#include "ConfigManager.h"
/*
const char* ConfigManager::apn() const{
  return creds.apn;
}

const char* ConfigManager::simPin() const{
  return creds.simPin;
}

const char* ConfigManager::gprsUser() const{
  return creds.gprsUser;
}

const char* ConfigManager::gprsPass() const{
  return creds.gprsPass;
}

const char* ConfigManager::topic() const{
  return creds.topic;
}

const char* ConfigManager::broker() const{
  return creds.broker;
}

const char* ConfigManager::brokerUser() const{
  return creds.brokerUser;
}

const char* ConfigManager::brokerPass() const{
  return creds.brokerPass;
}

uint16_t ConfigManager::port() const{
  return creds.port;
}
*/
bool ConfigManager::readGprs(){
  File file = SD.open("/config.json");
  if (!file) {
    lastError = "Failed to open network config file";
    return false;
  }
  // Allocate a JSON document
  StaticJsonDocument<128> config;
  // Parse the JSON from the file
  DeserializationError error = deserializeJson(config, file);
  if (error) {
    lastError = ("Failed to get network config: " + String(error.f_str())).c_str();
    return false;
  }
  file.close();

  GPRS gprs;
  gprs.apn = config["GprsConfiguration"]["Apn"].as<const char*>();
  gprs.simPin = config["GprsConfiguration"]["SimPin"].as<const char*>();
  gprs.user = config["GprsConfiguration"]["GprsUser"].as<const char*>();
  gprs.pass = config["GprsConfiguration"]["GprsPass"].as<const char*>();

  modem.setCreds(gprs);
  return true;
}

bool ConfigManager::readMqtt(){
  File file = SD.open("/config.json");
  if (!file) {
    lastError = "Failed to open network config file";
    return false;
  }
  StaticJsonDocument<128> config;
  DeserializationError error = deserializeJson(config, file);
  if (error) {
    lastError = ("Failed to get network config: " + String(error.f_str())).c_str();
    return false;
  }
  file.close();

  MQTT mqtt;
  mqtt.topic = config["MqttConfiguration"]["Topic"].as<const char*>();
  mqtt.broker = config["MqttConfiguration"]["Broker"].as<const char*>();
  mqtt.user = config["MqttConfiguration"]["BrokerUser"].as<const char*>();
  mqtt.pass = config["MqttConfiguration"]["BrokerPass"].as<const char*>();
  mqtt.port = config["MqttConfiguration"]["Port"].as<uint16_t>();

  remote.setCreds(mqtt);
  return true;
}

bool ConfigManager::readTank(){
  File file = SD.open("/config.json");
  if (!file) {
    lastError = "Failed to open tank config file";
    return false;
  }
  // Allocate a JSON document
  StaticJsonDocument<512> config;
  // Parse the JSON from the file
  DeserializationError error = deserializeJson(config, file);
  if (error) {
    lastError = ("Failed to get tank config: " + String(error.f_str())).c_str();
    return false;
  }
  file.close();

  // Access the Probe Configuration array
  JsonArray tanks = config["TankConfiguration"];
  probeId.reserve(tanks.size());
  for(JsonObject tank : tanks){
    JsonArray modbusRegs = tank["ModbusRegister"];
    modbusReg.reserve(modbusRegs.size());
    for(JsonVariant regValue : modbusRegs){
      modbusReg.push_back(regValue.as<uint16_t>());
    }
    Serial.println("");
    probeId.push_back(tank["Id"].as<int>());
    String device = tank["Device"];
    String serialNo = tank["SerialNo"];
  }
  return true;
}