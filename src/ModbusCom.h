#ifndef MODBUSCOM_H
#define MODBUSCOM_H

#include <vector>
#include "ConfigManager.h"

class ModbusCom{
public:
    struct ProbeData {
        float volume;
        float ullage;
        float temperature;
        float product;
        float water;
    };
    std::vector<ProbeData> probeData;

    ModbusCom(ConfigManager& config) : config(config){}

    void init();
    void read();
    void error(const char* err);
private:
    ConfigManager& config;
};

#endif