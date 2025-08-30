#include "environmental_analysis.h"
#include "environment_data.h"
#include "config.h"
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
  if (currentEnv.noiseLevel > 4) {
    alertMessage = "Environment too noisy for concentration";
    alertLevel = "warning";
    hasAlert = true;
  }
  
  // Light alerts
  if (currentEnv.lightLevel < 270) {
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

  controlFan();
}

void controlFan() {
  bool newFanState = fanState;
  
  // Check if manual override is active
  if (manualOverride) {
    newFanState = manualFanState;
    Serial.println("Fan manual override active: " + String(newFanState ? "ON" : "OFF"));
  } else {
    // Automatic temperature-based control with hysteresis
    if (currentEnv.temperature >= FAN_ON_TEMP && !fanState) {
      newFanState = true;
      Serial.printf("Auto fan ON: Temperature %.1f°C >= %.1f°C\n", currentEnv.temperature, FAN_ON_TEMP);
    } else if (currentEnv.temperature <= FAN_OFF_TEMP && fanState) {
      newFanState = false;
      Serial.printf("Auto fan OFF: Temperature %.1f°C <= %.1f°C\n", currentEnv.temperature, FAN_OFF_TEMP);
    }
  }
  
  // Update fan state if changed
  if (newFanState != fanState) {
    fanState = newFanState;
    digitalWrite(FAN_RELAY_PIN, fanState ? HIGH : LOW);
    
    Serial.println("Fan state changed to: " + String(fanState ? "ON" : "OFF"));
    publishFanStatus();
  }
}

void setFanManualOverride(bool enabled, bool state) {
  manualOverride = enabled;
  if (enabled) {
    manualFanState = state;
    Serial.println("Manual override enabled, fan set to: " + String(state ? "ON" : "OFF"));
  } else {
    Serial.println("Manual override disabled, returning to automatic control");
  }
  
  // Immediately apply the control logic
  controlFan();
}

void publishFanStatus() {
  // Publish individual fan status
  client.publish("bille/sensors/fan_state", fanState ? "ON" : "OFF");
  client.publish("bille/sensors/fan_manual_override", manualOverride ? "true" : "false");
  
  // Publish detailed fan status
  StaticJsonDocument<200> fanDoc;
  fanDoc["nodeType"] = "ENVIRONMENT";
  fanDoc["timestamp"] = millis();
  fanDoc["fanState"] = fanState;
  fanDoc["manualOverride"] = manualOverride;
  fanDoc["temperature"] = currentEnv.temperature;
  fanDoc["controlMode"] = manualOverride ? "manual" : "automatic";
  
  if (!manualOverride) {
    fanDoc["autoReason"] = fanState ? "temperature_high" : "temperature_ok";
  }
  
  String fanString;
  serializeJson(fanDoc, fanString);
  client.publish("bille/status/fan", fanString.c_str());
  
  Serial.println("Fan status published to MQTT");
}
