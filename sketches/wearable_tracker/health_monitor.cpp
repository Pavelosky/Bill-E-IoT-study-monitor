#include "health_monitor.h"
#include "biometric_data.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>

extern PubSubClient client;
extern BiometricData currentBio;
extern PomodoroInfo pomodoroInfo;

void publishHealthAlerts() {
  if (!sessionActive) return;
  
  StaticJsonDocument<300> alertDoc;
  alertDoc["nodeType"] = "WEARABLE";
  alertDoc["timestamp"] = millis();
  alertDoc["userId"] = currentUser;
  
  bool hasAlert = false;
  String alertMessage = "";
  String alertLevel = "info";
  
  // Check for extended sitting during work sessions
  unsigned long timeSinceMovement = millis() - currentBio.lastMovement;
  
  if (pomodoroInfo.currentState == WORK_SESSION && timeSinceMovement > 20 * 60 * 1000) { // 20 minutes
    alertMessage = "Consider taking a short movement break";
    alertLevel = "warning";
    hasAlert = true;
  }
  
  // Check break compliance
  if ((pomodoroInfo.currentState == SHORT_BREAK || pomodoroInfo.currentState == LONG_BREAK) 
      && timeSinceMovement > 2 * 60 * 1000) { // 2 minutes into break
    alertMessage = "Break time - time to move around!";
    alertLevel = "info";
    hasAlert = true;
  }
  
  if (hasAlert) {
    alertDoc["alert"] = alertMessage;
    alertDoc["level"] = alertLevel;
    alertDoc["timeSinceMovement"] = timeSinceMovement / 60000; // minutes
    alertDoc["pomodoroState"] = pomodoroInfo.currentState;
    
    String alertString;
    serializeJson(alertDoc, alertString);
    client.publish("bille/alerts/health", alertString.c_str());
    
    Serial.println("Health alert: " + alertMessage);
  }
}
