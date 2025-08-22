#include "sensors.h"
#include "config.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>

// Global instances
EnvironmentData envData;
BiometricData bioData;

extern bool sessionActive;
extern PubSubClient mqttClient;

void analyzeEnvironment() {
  // Simple environmental analysis
  bool needsAlert = false;
  String alertMsg = "";
  
  // Check temperature (optimal: 20-24°C)
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
  
  if (needsAlert && sessionActive) {
    // Flash LED for alert
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_PIN, LOW);
      delay(100);
      digitalWrite(LED_PIN, HIGH);
      delay(100);
    }
    
    Serial.println("Environment Alert: " + alertMsg);
  }
}

void publishMovementReminder() {
  StaticJsonDocument<150> doc;
  doc["message"] = "Time to move around!";
  doc["timestamp"] = millis();
  doc["reason"] = "extended_sitting";
  
  String jsonString;
  serializeJson(doc, jsonString);
  mqttClient.publish("bille/alerts/movement", jsonString.c_str());
  
  Serial.println("Movement reminder sent via MQTT");
}

void analyzeBiometrics() {
  if (!bioData.dataAvailable) return;
  
  // Check for extended sitting
  unsigned long timeSinceMovement = millis() - bioData.lastMovement;
  
  if (timeSinceMovement > 25 * 60 * 1000 && sessionActive) { // 25 minutes
    // Flash LED for movement reminder
    for (int i = 0; i < 5; i++) {
      digitalWrite(LED_PIN, LOW);
      delay(100);
      digitalWrite(LED_PIN, HIGH);
      delay(100);
    }
    
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