#include "mqtt_client.h"
#include "config.h"
#include "environment_data.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>

extern PubSubClient client;
extern EnvironmentData currentEnv;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Create a random client ID
    String clientId = "BillE-Environment-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      
      // Subscribe to control topics
      client.subscribe("bille/environment/request");
      client.subscribe("bille/session/state");
      
      // Announce presence
      client.publish("bille/status/environment", "online", true);
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
  
  // Handle session state updates
  if (String(topic) == "bille/session/state") {
    StaticJsonDocument<200> doc;
    deserializeJson(doc, message);
    
    bool sessionActive = doc["active"];
    String userId = doc["userId"].as<String>();
    
    // Update local session state
    Serial.printf("Session update: %s for user %s\n", 
                  sessionActive ? "ACTIVE" : "INACTIVE", userId.c_str());
  }
}

void publishEnvironmentalData() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  
  // Publish individual sensor values to HA
  client.publish("bille/sensors/temperature", String(currentEnv.temperature).c_str());
  client.publish("bille/sensors/humidity", String(currentEnv.humidity).c_str());
  client.publish("bille/sensors/light", String(currentEnv.lightLevel).c_str());
  client.publish("bille/sensors/noise", String(currentEnv.noiseLevel).c_str());
  
  // Publish combined sensor data
  StaticJsonDocument<300> doc;
  doc["nodeType"] = "ENVIRONMENT";
  doc["timestamp"] = millis();
  doc["temperature"] = currentEnv.temperature;
  doc["humidity"] = currentEnv.humidity;
  doc["lightLevel"] = currentEnv.lightLevel;
  doc["noiseLevel"] = currentEnv.noiseLevel;
  doc["soundDetected"] = currentEnv.soundDetected;
  
  String jsonString;
  serializeJson(doc, jsonString);
  client.publish("bille/data/environment", jsonString.c_str());
  
  Serial.println("Environmental data published to MQTT");
}