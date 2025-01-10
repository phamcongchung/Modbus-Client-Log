#include "RemoteLogger.h"
#include <ArduinoJson.h>

RemoteLogger& RemoteLogger::setCreds(MQTT& mqtt){
  this->mqtt = mqtt;
  Serial.println(this->mqtt.broker);
  Serial.println(this->mqtt.port);
  return *this;
}

PubSubClient& RemoteLogger::setServer(){
  Serial.print("Set MQTT server: ");
  Serial.println(this->mqtt.broker);
  return PubSubClient::setServer(this->mqtt.broker, this->mqtt.port);
}

boolean RemoteLogger::connect(const char* id){
  Serial.print("Connect MQTT server, user: ");
  Serial.print(this->mqtt.user);
  return PubSubClient::connect(id, this->mqtt.user, this->mqtt.pass);
}

boolean RemoteLogger::subscribe(){
  Serial.print("Subcribe MQTT topic: ");
  return PubSubClient::subscribe(this->mqtt.topic);
}

boolean RemoteLogger::publish(const char* payload, boolean retained){
  return PubSubClient::publish(this->mqtt.topic, payload, retained);
}

bool RemoteLogger::apiConnect(const char* host, uint16_t port){
  this->api.host = host; this->api.port = port;
  if(client->connect(this->api.host, this->api.port, 10) != 1){
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
          request, this->api.host, this->api.port, strlen(msg));
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
          request, this->api.host, this->api.port, this->token, strlen(msg));
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
  this->api.user = user; this->api.pass = pass;
  if(!client->connected()){
    Serial.println("Failed to connect to server.");
    return;
  }
  char tokenAuth[128];
  snprintf(tokenAuth, sizeof(tokenAuth),
          "{\"username\":\"%s\",\"password\":\"%s\"}",
          this->api.user, this->api.pass);
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