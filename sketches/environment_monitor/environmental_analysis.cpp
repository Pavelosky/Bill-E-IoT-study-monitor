#include "environmental_analysis.h"
#include "environment_data.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>

extern PubSubClient client;
extern EnvironmentData currentEnv;

void checkEnvironmentalAlerts() {
  // Smart environmental analysis for HA dashboard
  StaticJsonDocument<200> alertDoc;
  alertDoc["nodeType"] = "ENVIRONMENT";
  alertDoc["timestamp"] = millis();
  
  bool hasAlert = false;
  String alertMessage = "";
  String alertLevel = "info";
  
  // Temperature alerts
  if (currentEnv.temperature < 20) {
    alertMessage = "Temperature too low for productivity";
    alertLevel = "warning";
    hasAlert = true;
  } else if (currentEnv.temperature > 26) {
    alertMessage = "Temperature too high for focus";
    alertLevel = "warning";
    hasAlert = true;
  }
  
  // Noise alerts
  if (currentEnv.noiseLevel > 60) {
    alertMessage = "Environment too noisy for concentration";
    alertLevel = "warning";
    hasAlert = true;
  }
  
  // Light alerts
  if (currentEnv.lightLevel < 100) {
    alertMessage = "Lighting may be too dim for productivity";
    alertLevel = "info";
    hasAlert = true;
  }
  
  if (hasAlert) {
    alertDoc["alert"] = alertMessage;
    alertDoc["level"] = alertLevel;
    alertDoc["temperature"] = currentEnv.temperature;
    alertDoc["noise"] = currentEnv.noiseLevel;
    alertDoc["light"] = currentEnv.lightLevel;
    
    String alertString;
    serializeJson(alertDoc, alertString);
    client.publish("bille/alerts/environment", alertString.c_str());
    
    Serial.println("Environmental alert: " + alertMessage);
  }
}