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

bool RemoteLogger::apiConnect(const char* host, uint16_t port){
  this->host = host; this->port = port;
  if(client->connect(this->host, this->port, 10) != 1){
    return false;
  } else {
    return true;
  }
}

bool RemoteLogger::apiConnected(){
  return client->connected();
}

bool RemoteLogger::post(const char* request, const char* msg){
  char header[1250];
  snprintf(header, sizeof(header),
          "POST %s HTTP/1.1\r\n"
          "Host: %s:%d\r\n"
          "Content-Type: application/json\r\n"
          "tenant: root\r\n"
          "Accept-Language: en-US\r\n"
          "Connection: keep-alive\r\n"
          "Content-Length: %d\r\n\r\n",
          request, this->host, this->port, strlen(msg));
  if(!client){
    Serial.println("Client not initialized!");
    return false;
  }
  client->print(header);
  Serial.print(header);
  client->print(msg);
  return true;
}

bool RemoteLogger::securePost(const char* request, const char* msg){
  char header[1250];
  snprintf(header, sizeof(header),
          "POST %s HTTP/1.1\r\n"
          "Host: %s:%d\r\n"
          "Content-Type: application/json\r\n"
          "Authorization: Bearer %s\r\n"
          "Accept-Language: en-US\r\n"
          "Connection: keep-alive\r\n"
          "Content-Length: %d\r\n\r\n",
          request, this->host, this->port, this->token, strlen(msg));
  if(!client){
    Serial.println("Client not initialized!");
    return false;
  }
  client->print(header);
  Serial.print(header);
  client->print(msg);
  return true;
}

void RemoteLogger::retrieveToken(const char* user, const char* pass){
  this->user = user; this->pass = pass;
  if(!client->connected()){
    Serial.println("Failed to connect to server.");
    return;
  }
  char tokenAuth[128];
  snprintf(tokenAuth, sizeof(tokenAuth),
          "{\"username\":\"%s\",\"password\":\"%s\"}",
          this->user, this->pass);
  Serial.println(tokenAuth);
  this->post("/api/tokens", tokenAuth);

  Serial.println("Waiting for authentication response...");
  unsigned long startTime = millis();
  String response;
  while((millis() - startTime) < API_TIMEOUT){
    if(client->available()){
      response = client->readString();
      Serial.println(response);
      break;
    }
  }
  if(response.isEmpty()){
    Serial.println("Error: No response from server.");
    return;
  }
  int jsonStart = response.indexOf("{");
  if(jsonStart != -1){
    response = response.substring(jsonStart);
    int jsonEnd = response.lastIndexOf("}");
    if(jsonEnd != -1)
      response = response.substring(0, jsonEnd + 1);
  } else {
    response = "";
  }
  // Parse the token from the response (assumes JSON response format)
  StaticJsonDocument<1024> jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, response);
  if (error){
    Serial.print("Failed to get token: ");
    Serial.println(error.c_str());
    return;
  }
  if(jsonDoc.containsKey("token")){
    this->token = jsonDoc["token"].as<const char*>();
    Serial.print("Extracted Token: ");
    Serial.println(this->token);
  } else {
    Serial.println("Error: 'token' field not found.");
  }
}

bool RemoteLogger::errorToApi(String& jsonPayload){
  if(this->token == NULL){
    Serial.println("Error: Token is NULL");
    return false;
  }
  Serial.println("Sending error chunk...");
  this->securePost("/api/v1/errorloggers/addlisterrorogger", jsonPayload.c_str());
  
  Serial.println("Waiting for server response...");
  unsigned long startTime = millis();
  String response;
  while((millis() - startTime) < API_TIMEOUT){
    if(client->available()){
      response = client->readString();
      Serial.println(response);
      break;
    }
  }
  if(response.isEmpty()){
    Serial.println("Error: No response from server");
    return false;
  }
  if(response.startsWith("HTTP/1.1 200")){
    Serial.println("Data successfully sent to API");
    return true;
  } else {
    Serial.println("Error: API response indicates failure");
  }
  return false;
}

bool RemoteLogger::dataToApi(String& jsonPayload){
  if(this->token == NULL){
    Serial.println("Error: Token is NULL");
    return false;
  }
  Serial.println("Sending data chunk...");
  this->securePost("/api/v1/dataloggers/addlistdatalogger", jsonPayload.c_str());
  
  Serial.println("Waiting for server response...");
  unsigned long startTime = millis();
  String response;
  while((millis() - startTime) < API_TIMEOUT){
    if(client->available()){
      response = client->readString();
      Serial.println(response);
      break;
    }
  }
  if(response.isEmpty()){
    Serial.println("Error: No response from server");
    return false;
  }
  if(response.startsWith("HTTP/1.1 200")){
    Serial.println("Data successfully sent to API");
    return true;
  } else {
    Serial.println("Error: API response indicates failure");
  }
  return false;
}

void callBack(char* topic, byte* msg, unsigned int len){
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String tempMsg;
  
  for (int i = 0; i < len; i++) {
    Serial.print((char)msg[i]);
    tempMsg += (char)msg[i];
  }
  Serial.println();
}