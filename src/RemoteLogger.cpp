#include "RemoteLogger.h"
#include <ArduinoJson.h>

PubSubClient& RemoteLogger::setServer(const ConfigManager& cm){
  const char* cred1 = cm.creds.broker;
  uint16_t cred2 = cm.creds.port;
  return PubSubClient::setServer(cred1, cred2);
}

boolean RemoteLogger::connect(const char* id, const ConfigManager& cm){
  const char* cred1 = cm.creds.brokerUser;
  const char* cred2 = cm.creds.brokerPass;
  return PubSubClient::connect(id, cred1, cred2);
}

boolean RemoteLogger::subscribe(const ConfigManager& cm){
  const char* cred = cm.creds.topic;
  return PubSubClient::subscribe(cred);
}

boolean RemoteLogger::publish(const ConfigManager& cm, const char* payload, boolean retained){
  const char* cred = cm.creds.topic;
  return PubSubClient::publish(cred, payload, retained);
}

bool RemoteLogger::apiConnect(String host, uint16_t port){
  this->host = host; this->port = port;
  if(simClient->connect(this->host.c_str(), this->port, 10) != 1){
    return false;
  } else {
    return true;
  }
}

void RemoteLogger::getApiToken(String user, String pass){
  this->user = user; this->pass = pass;
  if (simClient->connect(this->host.c_str(), this->port, 10) != 1) {
    Serial.println("Failed to connect to server.");
  }
  String tokenAuth = "{\"username\":\"" + this->user + "\",\"password\":\"" + this->pass + "\"}";
  String tokenReq = "POST /api/tokens HTTP/1.1\r\n"
                    "Host: " + this->host + ":" + String(this->port) + "\r\n"
                    "Content-Type: application/json\r\n"
                    "tenant: root\r\n"
                    "Accept-Language: en-US\r\n"
                    "Connection: keep-alive\r\n"
                    "Content-Length: " + String(tokenAuth.length()) + "\r\n\r\n";
  simClient->print(tokenReq);
  simClient->print(tokenAuth);

  Serial.println("Waiting for authentication response...");
  unsigned long startTime = millis();
  String response;
  while ((millis() - startTime) < API_TIMEOUT){
    if (simClient->available()){
      response = simClient->readString();
      Serial.println(response);
      break;
    }
  }
  if (response.isEmpty()) {
    Serial.println("Error: No response from server");;
  }
  int jsonStart = response.indexOf("{");
  if (jsonStart != -1){
    response = response.substring(jsonStart);
    int jsonEnd = response.lastIndexOf("}");
    if (jsonEnd != -1)
      response = response.substring(0, jsonEnd + 1);
  } else {
    response = "";
  }
  // Parse the token from the response (assumes JSON response format)
  StaticJsonDocument<1024> jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, response);
  if (error) {
    Serial.print("Failed to get token: ");
    Serial.println(error.c_str());
  }
  if (jsonDoc.containsKey("token")) {
    this->token = jsonDoc["token"].as<String>();
    Serial.print("Extracted Token: ");
    Serial.println(this->token);
  } else {
    Serial.println("Error: 'token' field not found in the response.");
  }
}

bool RemoteLogger::sendToApi(String& jsonPayload){
  if(this->token == NULL){
    Serial.println("Error: Token is NULL");
    return false;
  }
  // Send HTTP POST request
  Serial.println("Sending data chunk...");
  String postReq = "POST /api/v1/dataloggers/addlistdatalogger HTTP/1.1\r\n"
                  "Host: YOUR_HOST_URL:6868\r\n"
                  "Content-Type: application/json\r\n"
                  "Authorization: Bearer " + this->token + "\r\n"
                  "Accept-Language: en-US\r\n"
                  "Connection: keep-alive\r\n"
                  "Content-Length: " + String(jsonPayload.length()) + "\r\n\r\n";
  simClient->print(postReq);
  simClient->print(jsonPayload);

  // Wait for server response
  Serial.println("Waiting for server response...");
  unsigned long startTime = millis();
  String response;

  while ((millis() - startTime) < API_TIMEOUT){
    if (simClient->available()){
      response = simClient->readString();
      Serial.println(response);
      break;
    }
  }
  if (response.isEmpty()) {
    Serial.println("Error: No response from server");
    return false;
  }
  if (response.startsWith("HTTP/1.1 200")) {
    Serial.println("Data successfully sent to API");
    return true;
  } else {
    Serial.println("Error: API response indicates failure");
    return false;
  }
}

void callBack(char* topic, byte* message, unsigned int len){
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < len; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}