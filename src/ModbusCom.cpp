#include <Arduino.h>
#include <ModbusRTUClient.h>
#include "globals.h"
#include "ModbusCom.h"

void modbusInit(){
  // Initialize the Modbus RTU client
  if (!ModbusRTUClient.begin(9600)) {
  Serial.println("Failed to start Modbus RTU Client!");
  Serial.println("Reconnecting Modbus");
  ModbusRTUClient.begin(9600);
  }
  delay(1000);
  Serial.println("Modbus RTU Client initialized.");
}

void readModbus(){
  for (size_t i = 0; i < probeId.size(); ++i) {
    float* dataPointers[] = {&probeData[i].volume, &probeData[i].ullage, 
                             &probeData[i].temperature, &probeData[i].product, 
                             &probeData[i].water};
    const char* labels[] = {"Volume", "Ullage", "Temperature", "Product", "Water"};
    
    for (size_t j = 0; j < 5; ++j) {
      *dataPointers[j] = ModbusRTUClient.holdingRegisterRead<float>(probeId[i], modbusReg[j], BIGEND);

      if (*dataPointers[j] < 0) {
        Serial.printf("Failed to read %s for probe ID: %d\r\nError: ", labels[j], probeId[i]);
        Serial.println(ModbusRTUClient.lastError());
      } else {
        Serial.printf("%s: %.2f\r\n", labels[j], *dataPointers[j]);
      }
    }
  }
}