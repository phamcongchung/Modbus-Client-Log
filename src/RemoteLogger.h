#ifndef REMOTELOGGER_H
#define REMOTELOGGER_H

#include <PubSubClient.h>

#define API_TIMEOUT       5000
#define TINY_GSM_MODEM_SIM7600

#include <TinyGsmClient.h>
#include "structs.h"

class RemoteLogger : public PubSubClient{
public:
  RemoteLogger(TinyGsmClient& mqttClient, TinyGsmClient& apiClient)
              : PubSubClient(mqttClient), apiClient(&apiClient){}

  const char* token;

  RemoteLogger& setCreds(MQTT& mqtt);
  RemoteLogger& setCreds(API& api);
  PubSubClient& setServer();
  boolean mqttConnect(const char* id);
  boolean mqttConnected();
  boolean publish(const char* payload, boolean retained);
  boolean subscribe();
  
  void retrieveToken();
  bool apiConnect();
  bool apiConnected();
  bool post(const char* request, const char* msg);
  bool securePost(const char* request, const char* msg);
  bool dataToApi(String& jsonPayload);
  bool errorToApi(String& jsonPayload);
private:
  TinyGsmClient* mqttClient;
  TinyGsmClient* apiClient;
  MQTT mqtt;
  API api;
};

void callBack(char* topic, byte* message, unsigned int len);

#endif