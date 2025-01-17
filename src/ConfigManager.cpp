#include <SD.h>
#include <ArduinoJson.h>
#include "ConfigManager.h"

bool ConfigManager::readGprs(){
  File file = SD.open("/config.json");
  if (!file) {
    lastError = "Failed to open config file";
    return false;
  }
  // Allocate a JSON document
  JsonDocument config;
  // Parse the JSON from the file
  DeserializationError error = deserializeJson(config, file);
  if (error) {
    lastError = ("Failed to get GPRS config: " + String(error.f_str())).c_str();
    return false;
  }
  file.close();

  GPRS gprs;
  gprs.apn = config["GprsConfiguration"]["Apn"].as<String>();
  gprs.simPin = config["GprsConfiguration"]["SimPin"].as<String>();
  gprs.user = config["GprsConfiguration"]["User"].as<String>();
  gprs.pass = config["GprsConfiguration"]["Password"].as<String>();

  modem.setCreds(gprs);
  return true;
}

bool ConfigManager::readMqtt(){
  File file = SD.open("/config.json");
  if (!file) {
    lastError = "Failed to open config file";
    return false;
  }
  JsonDocument config;
  DeserializationError error = deserializeJson(config, file);
  if (error) {
    lastError = ("Failed to get MQTT config: " + String(error.f_str())).c_str();
    return false;
  }
  file.close();

  MQTT mqtt;
  mqtt.topic = config["MqttConfiguration"]["Topic"].as<String>();
  mqtt.broker = config["MqttConfiguration"]["Broker"].as<String>();
  mqtt.user = config["MqttConfiguration"]["User"].as<String>();
  mqtt.pass = config["MqttConfiguration"]["Password"].as<String>();
  mqtt.port = config["MqttConfiguration"]["Port"].as<uint16_t>();

  remote.setCreds(mqtt);
  return true;
}

bool ConfigManager::readApi(){
  File file = SD.open("/config.json");
  if (!file) {
    lastError = "Failed to open config file";
    return false;
  }
  JsonDocument config;
  DeserializationError error = deserializeJson(config, file);
  if (error) {
    lastError = ("Failed to get API config: " + String(error.f_str())).c_str();
    return false;
  }
  file.close();

  API api;
  api.host = config["ApiConfiguration"]["Host"].as<String>();
  api.user = config["ApiConfiguration"]["Username"].as<String>();
  api.pass = config["ApiConfiguration"]["Password"].as<String>();
  api.port = config["ApiConfiguration"]["Port"].as<uint16_t>();

  remote.setCreds(api);
  return true;
}

bool ConfigManager::readTank(){
  File file = SD.open("/config.json");
  if (!file) {
    lastError = "Failed to open config file";
    return false;
  }
  // Allocate a JSON document
  JsonDocument config;
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
    probeId.push_back(tank["Id"].as<int>());
    String device = tank["Device"];
    String serialNo = tank["SerialNo"];
  }
  return true;
}