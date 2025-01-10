#ifndef REMOTELOGGER_H
#define REMOTELOGGER_H

#include <PubSubClient.h>

#define API_TIMEOUT       5000
#define TINY_GSM_MODEM_SIM7600

#include <TinyGsmClient.h>
#include "structs.h"

class RemoteLogger : public PubSubClient{
public:
  RemoteLogger(TinyGsmClient& client) : PubSubClient(client), client(&client){}

  const char* token;
  MQTT mqtt;

  RemoteLogger& setCreds(MQTT& mqtt);
  PubSubClient& setServer();
  boolean connect(const char *id);
  boolean subscribe();
  boolean publish(const char* payload, boolean retained);
  
  void retrieveToken(const char* user, const char* pass);
  bool apiConnected();
  bool apiConnect(const char* host, uint16_t port);
  bool post(const char* request, const char* msg);
  bool securePost(const char* request, const char* msg);
  bool dataToApi(String& jsonPayload);
  bool errorToApi(String& jsonPayload);
private:
  TinyGsmClient *client;
  //MQTT mqtt;
  API api;
};

void callBack(char* topic, byte* message, unsigned int len);

#endif