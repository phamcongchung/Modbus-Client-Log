#include <SD.h>
#include <ArduinoJson.h>
#include "ConfigManager.h"

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

bool ConfigManager::readNetwork(){
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
  this->creds.apn = config["NetworkConfiguration"]["Apn"].as<String>().c_str();
  this->creds.simPin = config["NetworkConfiguration"]["SimPin"].as<String>().c_str();
  this->creds.gprsUser = config["NetworkConfiguration"]["GprsUser"].as<String>().c_str();
  this->creds.gprsPass = config["NetworkConfiguration"]["GprsPass"].as<String>().c_str();
  this->creds.topic = config["NetworkConfiguration"]["Topic"].as<String>().c_str();
  this->creds.broker = config["NetworkConfiguration"]["Broker"].as<String>().c_str();
  this->creds.brokerUser = config["NetworkConfiguration"]["BrokerUser"].as<String>().c_str();
  this->creds.brokerPass = config["NetworkConfiguration"]["BrokerPass"].as<String>().c_str();
  this->creds.port = config["NetworkConfiguration"]["Port"].as<uint16_t>();
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