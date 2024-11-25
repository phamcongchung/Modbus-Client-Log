#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <vector>

class ConfigManager{
public:
    // GPRSS credentials
    String apn;
    String simPin;
    String gprsUser;
    String gprsPass;
    // MQTT credentials
    String topic;
    String broker;
    String brokerUser;
    String brokerPass;
    uint16_t port;
    // External vectors
    std::vector<int> probeId;
    std::vector<uint16_t> modbusReg;

    void getNetwork();
    void getTank();
};

#endif