#include "data_analysis.h"
#include "config.h"
#include "data_structures.h"
#include "audio_system.h"
#include "mqtt_handler.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

extern PubSubClient mqttClient;

void analyzeEnvironment() {
  // Simple environmental analysis
  bool needsAlert = false;
  String alertMsg = "";
  
  // Check temperature (optimal: 20-26°C)
  if (envData.temperature < 20) {
    alertMsg = "Too Cold!";
    needsAlert = true;
  } else if (envData.temperature > 26) {
    alertMsg = "Too Hot!";
    needsAlert = true;
  }
  
  // Check noise level
  if (envData.noiseLevel > 50) {
    alertMsg = "Too Noisy!";
    needsAlert = true;
  }
  
  // Check light level (very basic check)
  if (envData.lightLevel < 50) {
    alertMsg = "Too Dark!";
    needsAlert = true;
  }
  
}

void analyzeBiometrics() {
  if (!bioData.dataAvailable) return;
  
  // Check for extended sitting
  unsigned long timeSinceMovement = millis() - bioData.lastMovement;
  
  if (timeSinceMovement > 25 * 60 * 1000 && sessionActive) { // 25 minutes
    
    // Send MQTT movement reminder
    publishMovementReminder();
    
    Serial.println("Biometric Alert: Time to move!");
  }
  
  // Check heart rate
  if (bioData.heartRate > 100 && sessionActive) {
    Serial.println("Biometric Alert: High heart rate - take a break");
    
    // Publish health alert
    StaticJsonDocument<200> alertDoc;
    alertDoc["alert"] = "High heart rate detected";
    alertDoc["heartRate"] = bioData.heartRate;
    alertDoc["level"] = "warning";
    alertDoc["timestamp"] = millis();
    
    String alertString;
    serializeJson(alertDoc, alertString);
    mqttClient.publish("bille/alerts/health", alertString.c_str());
  }
}
