#include <SD.h>
#include <ArduinoJson.h>
#include "ConfigManager.h"

bool ConfigManager::getNetwork(){
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

  // Extract Network Configuration as String
  apn = config["NetworkConfiguration"]["Apn"].as<String>().c_str();
  simPin = config["NetworkConfiguration"]["SimPin"].as<String>().c_str();
  gprsUser = config["NetworkConfiguration"]["GprsUser"].as<String>().c_str();
  gprsPass = config["NetworkConfiguration"]["GprsPass"].as<String>().c_str();
  topic = config["NetworkConfiguration"]["Topic"].as<String>().c_str();
  broker = config["NetworkConfiguration"]["Broker"].as<String>().c_str();
  brokerUser = config["NetworkConfiguration"]["BrokerUser"].as<String>().c_str();
  brokerPass = config["NetworkConfiguration"]["BrokerPass"].as<String>().c_str();
  port = config["NetworkConfiguration"]["Port"].as<uint16_t>();
  return true;
}

bool ConfigManager::getTank(){
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