#ifndef REMOTELOGGER_H
#define REMOTELOGGER_H

#include <PubSubClient.h>
#include "ConfigManager.h"

#define API_TIMEOUT       5000
#define TINY_GSM_MODEM_SIM7600

#include <TinyGsmClient.h>

class RemoteLogger : public PubSubClient{
public:
  RemoteLogger(TinyGsmClient& client) : PubSubClient(client), client(&client)/*, api(client)*/{}
  // TinyGsmClient* simClient = static_cast<TinyGsmClient*>(&api);

  const char* token;

  PubSubClient& setServer(const ConfigManager& cm);
  boolean connect(const char *id, const ConfigManager& cm);
  boolean subscribe(const ConfigManager& cm);
  boolean publish(const ConfigManager& cm, const char* payload, boolean retained);
  void retrieveToken(const char* user, const char* pass);
  bool apiConnected();
  bool apiConnect(const char* host, uint16_t port);
  bool post(const char* request, const char* msg);
  bool securePost(const char* request, const char* msg);
  bool dataToApi(String& jsonPayload);
  bool errorToApi(String& jsonPayload);
private:
  TinyGsmClient *client;
  const char* user;
  const char* pass;
  const char* host;
  uint16_t port;
};

void callBack(char* topic, byte* message, unsigned int len);

#endif