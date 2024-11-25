#include <SD.h>
#include <ArduinoJson.h>
#include "ConfigManager.h"

void ConfigManager::getNetwork(){
  File file = SD.open("/config.json");
  if (!file) {
    Serial.println("Failed to open network config file");
    return;
  }
  // Allocate a JSON document
  StaticJsonDocument<128> config;
  // Parse the JSON from the file
  DeserializationError error = deserializeJson(config, file);
  if (error) {
    Serial.print("Failed to get network config: ");
    Serial.println(error.f_str());
    return;
  }
  file.close();

  // Extract Network Configuration as String
  apn = config["NetworkConfiguration"]["Apn"].as<String>();
  simPin = config["NetworkConfiguration"]["SimPin"].as<String>();
  gprsUser = config["NetworkConfiguration"]["GprsUser"].as<String>();
  gprsPass = config["NetworkConfiguration"]["GprsPass"].as<String>();
  topic = config["NetworkConfiguration"]["Topic"].as<String>();
  broker = config["NetworkConfiguration"]["Broker"].as<String>();
  brokerUser = config["NetworkConfiguration"]["BrokerUser"].as<String>();
  brokerPass = config["NetworkConfiguration"]["BrokerPass"].as<String>();
  port = config["NetworkConfiguration"]["Port"].as<uint16_t>();
}

void ConfigManager::getTank(){
  File file = SD.open("/config.json");
  if (!file) {
    Serial.println("Failed to open tank config file");
    return;
  }
  // Allocate a JSON document
  StaticJsonDocument<512> config;
  // Parse the JSON from the file
  DeserializationError error = deserializeJson(config, file);
  if (error) {
    Serial.print("Failed to get tank config: ");
    Serial.println(error.f_str());
    return;
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
      Serial.print(modbusReg.back()); Serial.print(" ");
    }
    Serial.println("");
    probeId.push_back(tank["Id"].as<int>()); Serial.println(probeId.back());
    String device = tank["Device"]; Serial.println(device);
    String serialNo = tank["SerialNo"]; Serial.println(serialNo);
  }
}