/*#include <ModbusRTUClient.h>
#include "ModbusCom.h"

void ModbusCom::init(){
  // Initialize the Modbus RTU client
  if (!ModbusRTUClient.begin(9600)) {
    Serial.println("Failed to start Modbus RTU Client!");
  }
  probeData.reserve(config.probeId.size());
}

void ModbusCom::read(){
  for (size_t i = 0; i < config.probeId.size(); ++i) {
    float* dataPointers[] = {&probeData[i].volume, &probeData[i].ullage, 
                             &probeData[i].temperature, &probeData[i].product, 
                             &probeData[i].water};
    const char* labels[] = {"Volume", "Ullage", "Temperature", "Product", "Water"};
    
    for (size_t j = 0; j < 5; ++j) {
      *dataPointers[j] = ModbusRTUClient.holdingRegisterRead<float>(config.probeId[i], config.modbusReg[j],BIGEND);

      if (*dataPointers[j] < 0) {
        Serial.printf("Failed to read %s for probe ID: %d\r\nError: ", labels[j], config.probeId[i]);
        Serial.println(ModbusRTUClient.lastError());
        error(ModbusRTUClient.lastError());
      } else {
        Serial.printf("%s: %.2f\r\n", labels[j], *dataPointers[j]);
      }
    }
  }
}*/