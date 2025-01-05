#ifndef REMOTELOGGER_H
#define REMOTELOGGER_H

#include <PubSubClient.h>
#include "ConfigManager.h"

#define API_TIMEOUT       5000
#define TINY_GSM_MODEM_SIM7600

#include <TinyGsmClient.h>

class RemoteLogger : public PubSubClient{
public:
  RemoteLogger(Client& client) : PubSubClient(client), api(client){}

  TinyGsmClient* simClient = static_cast<TinyGsmClient*>(&api);

  PubSubClient& setServer(const ConfigManager& cm);
  boolean connect(const char *id, const ConfigManager& cm);
  boolean subscribe(const ConfigManager& cm);
  boolean publish(const ConfigManager& cm, const char* payload, boolean retained);
  void getApiToken(String user, String pass);
  bool apiConnect(String host, uint16_t port);
  bool securePost(String& request, String& msg);
  bool post(String& request, String& msg);
  bool errorToApi(String& jsonPayload);
  bool dataToApi(String& jsonPayload);
private:
  Client& api;
  String user;
  String pass;
  String host;
  String token;
  uint16_t port;
};

void callBack(char* topic, byte* message, unsigned int len);

#endif