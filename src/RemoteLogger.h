#ifndef REMOTELOGGER_H
#define REMOTELOGGER_H

#include <PubSubClient.h>
#include "SIM.h"
#include "GPS.h"
#include "RTC.h"
#include "SDLogger.h"
#include "ModbusCom.h"
#include "ConfigManager.h"

class RemoteLogger{
public:
    RemoteLogger(ConfigManager& config, SIM& sim, GPS& gps, RTC& rtc, ModbusCom& modbus)
                : config(config), sim(sim), gps(gps), rtc(rtc), modbus(modbus), 
                client(sim.getClient()), mqtt(client){}

    void init(char macAdr[18]);
    void push(char macAdr[18]);
    void reconnect(char macAdr[18]);
    void mqttErr(int state);
private:
    SIM& sim;
    GPS& gps;
    RTC& rtc;
    ModbusCom& modbus;
    ConfigManager& config;
    PubSubClient mqtt;
    TinyGsmClient& client;
};

void callback(char* topic, byte* message, unsigned int len);

#endif