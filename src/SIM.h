#ifndef SIM_H
#define SIM_H

#define SIM_RXD       32
#define SIM_TXD       33
#define SIM_BAUD      115200
#define TINY_GSM_MODEM_SIM7600

#include <TinyGsmClient.h>
#include "ConfigManager.h"

class SIM{
public:
    SIM(ConfigManager& config, HardwareSerial& serialAT)
        : config(config), SerialAT(serialAT), modem(SerialAT), client(modem){}

    TinyGsm& getModem(){
        return modem;
    };
    TinyGsmClient& getClient(){
        return client;
    }

    void init();
    bool connect();
    void gprsErr(const char* err);
private:
    ConfigManager& config;
    HardwareSerial& SerialAT;
    TinyGsm modem;
    TinyGsmClient client;
};

#endif