#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// MQTT functions
void setupWiFi();
void reconnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void publishSessionState();
void publishSystemStatus();
void publishMQTTMessage(const char* topic, String payload);
void publishSensorData();

// Global objects
extern WiFiClient espClient;
extern PubSubClient mqttClient;
extern bool mqttConnected;

#endif