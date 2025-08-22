#include "mqtt_manager.h"
#include "config.h"
#include "sensors.h"
#include "display_manager.h"
#include "activity_tracker.h"
#include <ArduinoJson.h>

// Global objects
WiFiClient espClient;
PubSubClient client(espClient);

// External variables
extern bool sessionActive;
extern String currentUser;

void setupWiFi() {
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

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Create a random client ID
    String clientId = "BillE-Wearable-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      
      // Subscribe to control topics
      client.subscribe(TOPIC_SESSION_STATE);
      client.subscribe(TOPIC_POMODORO_STATE);
      client.subscribe(TOPIC_WEARABLE_REQUEST);
      client.subscribe(TOPIC_MOVEMENT_ALERT);
      
      // Announce presence
      client.publish(TOPIC_WEARABLE_STATUS, "online", true);
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
  
  StaticJsonDocument<300> doc;
  deserializeJson(doc, message);
  
  // Handle session state updates
  if (String(topic) == TOPIC_SESSION_STATE) {
    sessionActive = doc["active"];
    currentUser = doc["userId"].as<String>();
    
    if (sessionActive) {
      // Reset step counter for new session
      resetStepCount();
      
      // Show session start notification
      display.clearBuffer();
      display.setFont(u8g2_font_8x13_tf);
      display.setCursor(0, 30);
      display.print("Session");
      display.setCursor(0, 50);
      display.print("Started!");
      display.sendBuffer();
      delay(2000);
      
      Serial.println("Session started for: " + currentUser);
    } else {
      currentUser = "";
      
      // Show session end notification
      display.clearBuffer();
      display.setFont(u8g2_font_8x13_tf);
      display.setCursor(0, 30);
      display.print("Session");
      display.setCursor(0, 50);
      display.print("Ended");
      display.sendBuffer();
      delay(2000);
      
      Serial.println("Session ended");
    }
  }
  
  // Handle Pomodoro state updates
  else if (String(topic) == TOPIC_POMODORO_STATE) {
    pomodoroInfo.currentState = (PomodoroState)doc["state"].as<int>();
    pomodoroInfo.timeRemaining = doc["timeRemaining"];
    pomodoroInfo.completedCycles = doc["completedCycles"];
    pomodoroInfo.snoozed = doc["snoozed"];
    pomodoroInfo.snoozeCount = doc["snoozeCount"];
    pomodoroInfo.stateText = doc["stateText"].as<String>();
    pomodoroInfo.dataAvailable = true;
    pomodoroInfo.lastUpdate = millis();
    
    Serial.println("Pomodoro state updated: " + pomodoroInfo.stateText);
  }
  
  // Handle movement reminders
  else if (String(topic) == TOPIC_MOVEMENT_ALERT) {
    String reminderMsg = doc["message"].as<String>();
    
    display.clearBuffer();
    display.setFont(u8g2_font_8x13_tf);
    display.setCursor(0, 30);
    display.print(reminderMsg);
    display.sendBuffer();
    
    Serial.println("Movement reminder: " + reminderMsg);
    
    // Brief flash of heart rate LED
    for (int i = 0; i < 3; i++) {
      digitalWrite(HEART_LED, HIGH);
      delay(100);
      digitalWrite(HEART_LED, LOW);
      delay(100);
    }
  }
  
  // Handle data requests
  else if (String(topic) == TOPIC_WEARABLE_REQUEST) {
    publishBiometricData();
  }
}

void publishBiometricData() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  
  // Publish individual sensor values to HA
  client.publish(TOPIC_HEARTRATE, String(currentBio.heartRate).c_str());
  client.publish(TOPIC_STEPS, String(currentBio.stepCount).c_str());
  client.publish(TOPIC_ACTIVITY, currentBio.activity.c_str());
  
  // Publish combined biometric data
  StaticJsonDocument<400> doc;
  doc["nodeType"] = "WEARABLE";
  doc["timestamp"] = currentBio.timestamp;
  doc["heartRate"] = currentBio.heartRate;
  doc["activity"] = currentBio.activity;
  doc["stepCount"] = currentBio.stepCount;
  doc["acceleration"] = currentBio.acceleration;
  doc["lastMovement"] = currentBio.lastMovement;
  doc["sessionActive"] = sessionActive;
  doc["currentUser"] = currentUser;
  
  // Add Pomodoro context
  if (pomodoroInfo.dataAvailable) {
    doc["pomodoroState"] = pomodoroInfo.currentState;
    doc["pomodoroTimeRemaining"] = pomodoroInfo.timeRemaining;
    doc["breakCompliant"] = (millis() - currentBio.lastMovement) < MOVEMENT_RECENT_TIME;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  client.publish(TOPIC_BIOMETRIC_DATA, jsonString.c_str());
  
  Serial.println("Biometric data published to MQTT");
}

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
  
  if (pomodoroInfo.currentState == WORK_SESSION && timeSinceMovement > EXTENDED_SITTING_TIME) {
    alertMessage = "Consider taking a short movement break";
    alertLevel = "warning";
    hasAlert = true;
  }
  
  // Check heart rate during work
  if (pomodoroInfo.currentState == WORK_SESSION && currentBio.heartRate > ELEVATED_HEART_RATE) {
    alertMessage = "Heart rate elevated - consider relaxing";
    alertLevel = "warning";
    hasAlert = true;
  }
  
  // Check break compliance
  if ((pomodoroInfo.currentState == SHORT_BREAK || pomodoroInfo.currentState == LONG_BREAK) 
      && timeSinceMovement > BREAK_COMPLIANCE_TIME) {
    alertMessage = "Break time - time to move around!";
    alertLevel = "info";
    hasAlert = true;
  }
  
  if (hasAlert) {
    alertDoc["alert"] = alertMessage;
    alertDoc["level"] = alertLevel;
    alertDoc["heartRate"] = currentBio.heartRate;
    alertDoc["timeSinceMovement"] = timeSinceMovement / 1000; // seconds
    alertDoc["pomodoroState"] = pomodoroInfo.currentState;
    
    String alertString;
    serializeJson(alertDoc, alertString);
    client.publish(TOPIC_HEALTH_ALERT, alertString.c_str());
    
    Serial.println("Health alert: " + alertMessage);
  }
}