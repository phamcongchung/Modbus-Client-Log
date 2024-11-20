#include <ArduinoJson.h>
#include <vector>
#include <SD.h>
#include "globals.h"
#include "Config.h"

void getNetworkConfig(){
  File file = SD.open("/config.json");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  // Allocate a JSON document
  StaticJsonDocument<128> config;
  // Parse the JSON from the file
  DeserializationError error = deserializeJson(config, file);
  if (error) {
    Serial.print("Failed to parse file: ");
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

void getTankConfig(){
  File file = SD.open("/config.json");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  // Allocate a JSON document
  StaticJsonDocument<512> config;
  // Parse the JSON from the file
  DeserializationError error = deserializeJson(config, file);
  if (error) {
    Serial.print("Failed to parse file: ");
    Serial.println(error.f_str());
    return;
  }
  file.close();

  // Access the Probe Configuration array
  JsonArray tanks = config["TankConfiguration"];
  probeId.reserve(tanks.size());
  probeData.reserve(tanks.size());
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