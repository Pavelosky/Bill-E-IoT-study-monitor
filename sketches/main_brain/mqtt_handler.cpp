#include "mqtt_handler.h"
#include "config.h"
#include "data_structures.h"
#include "pomodoro_timer.h"
#include "data_analysis.h"
#include "rfid_manager.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

extern PubSubClient mqttClient;

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
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Create a random client ID
    String clientId = "BillE-MainBrain-";
    clientId += String(random(0xffff), HEX);
    
    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      
      // Subscribe to data topics from other nodes
      mqttClient.subscribe("bille/data/environment");
      mqttClient.subscribe("bille/data/biometric");
      mqttClient.subscribe("bille/commands/session");
      mqttClient.subscribe("bille/commands/pomodoro");
      
      // Announce presence as main coordinator
      mqttClient.publish("bille/status/mainbrain", "online", true);
      
      // Request initial data from all nodes
      mqttClient.publish("bille/environment/request", "data");
      mqttClient.publish("bille/wearable/request", "data");
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
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
  
  StaticJsonDocument<400> doc;
  deserializeJson(doc, message);
  
  // Handle environmental data updates
  if (String(topic) == "bille/data/environment") {
    envData.temperature = doc["temperature"];
    envData.humidity = doc["humidity"];
    envData.lightLevel = doc["lightLevel"];
    envData.noiseLevel = doc["noiseLevel"];
    envData.soundDetected = doc["soundDetected"];
    envData.lastUpdate = millis();
    envData.dataAvailable = true;
    
    Serial.println("Environmental data updated via MQTT");
    analyzeEnvironment();
  }
  
  // Handle biometric data updates
  else if (String(topic) == "bille/data/biometric") {
    bioData.heartRate = doc["heartRate"];
    bioData.activity = doc["activity"].as<String>();
    bioData.stepCount = doc["stepCount"];
    bioData.acceleration = doc["acceleration"];
    bioData.lastMovement = doc["lastMovement"];
    bioData.lastUpdate = millis();
    bioData.dataAvailable = true;
    
    Serial.println("Biometric data updated via MQTT");
    analyzeBiometrics();
  }
  
  // Handle remote session commands (for web dashboard control)
  else if (String(topic) == "bille/commands/session") {
    String command = doc["command"];
    if (command == "start" && !sessionActive) {
      String userId = doc["userId"].as<String>();
      startSession(userId);
    } else if (command == "stop" && sessionActive) {
      endSession();
    }
  }
  
  // Handle remote Pomodoro commands
  else if (String(topic) == "bille/commands/pomodoro") {
    String command = doc["command"];
    if (command == "snooze" && sessionActive) {
      snoozeBreak();
    } else if (command == "skip" && sessionActive) {
      transitionToNextState();
    }
  }
}

void publishSessionState() {
  StaticJsonDocument<200> doc;
  doc["active"] = sessionActive;
  doc["userId"] = currentUser;
  doc["timestamp"] = millis();
  
  if (sessionActive) {
    unsigned long elapsed = (millis() - sessionStart) / 1000;
    doc["sessionDuration"] = elapsed;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  mqttClient.publish("bille/session/state", jsonString.c_str());
  mqttClient.publish("bille/session/active", sessionActive ? "true" : "false");
  
  Serial.println("Session state published to MQTT");
}

void publishPomodoroState() {
  StaticJsonDocument<300> doc;
  doc["state"] = pomodoro.currentState;
  doc["timeRemaining"] = (pomodoro.stateDuration - (millis() - pomodoro.stateStartTime)) / 1000;
  doc["completedCycles"] = pomodoro.completedCycles;
  doc["snoozed"] = pomodoro.breakSnoozed;
  doc["snoozeCount"] = pomodoro.snoozeCount;
  doc["timestamp"] = millis();
  
  String stateText;
  switch (pomodoro.currentState) {
    case WORK_SESSION: stateText = "WORK"; break;
    case SHORT_BREAK: stateText = "SHORT_BREAK"; break;
    case LONG_BREAK: stateText = "LONG_BREAK"; break;
    default: stateText = "IDLE"; break;
  }
  doc["stateText"] = stateText;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Publish to multiple topics for HA sensors
  mqttClient.publish("bille/pomodoro/state", jsonString.c_str());
  mqttClient.publish("bille/pomodoro/cycles", String(pomodoro.completedCycles).c_str());
  mqttClient.publish("bille/pomodoro/time_remaining", String(doc["timeRemaining"].as<long>()).c_str());
  mqttClient.publish("bille/pomodoro/current_state", stateText.c_str());
  
  Serial.println("Pomodoro state published: " + stateText);
}

void publishSystemStatus() {
  StaticJsonDocument<300> doc;
  doc["nodeType"] = "MAIN_BRAIN";
  doc["timestamp"] = millis();
  doc["sessionActive"] = sessionActive;
  doc["currentUser"] = currentUser;
  doc["wifiConnected"] = WiFi.status() == WL_CONNECTED;
  doc["mqttConnected"] = mqttClient.connected();
  doc["environmentDataAge"] = envData.dataAvailable ? (millis() - envData.lastUpdate) / 1000 : -1;
  doc["biometricDataAge"] = bioData.dataAvailable ? (millis() - bioData.lastUpdate) / 1000 : -1;
  
  if (sessionActive) {
    doc["pomodoroState"] = pomodoro.currentState;
    doc["pomodoroTimeRemaining"] = (pomodoro.stateDuration - (millis() - pomodoro.stateStartTime)) / 1000;
    doc["completedCycles"] = pomodoro.completedCycles;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  mqttClient.publish("bille/status/system", jsonString.c_str());
  
  Serial.println("System status published to MQTT");
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