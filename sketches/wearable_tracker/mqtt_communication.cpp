#include "mqtt_communication.h"
#include "config.h"
#include "biometric_data.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>

extern PubSubClient client;
extern BiometricData currentBio;
extern PomodoroInfo pomodoroInfo;
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C display;

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
    String clientId = "BillE-Wearable-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      
      // Subscribe to control topics
      client.subscribe("bille/session/state");
      client.subscribe("bille/pomodoro/state");
      client.subscribe("bille/wearable/request");
      client.subscribe("bille/alerts/movement");
      
      // Announce presence
      client.publish("bille/status/wearable", "online", true);
      
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
  
  StaticJsonDocument<300> doc;
  deserializeJson(doc, message);
  
  // Handle session state updates
  if (String(topic) == "bille/session/state") {
    sessionActive = doc["active"];
    currentUser = doc["userId"].as<String>();
    
    if (sessionActive) {
      // Reset step counter for new session
      extern int stepCount;
      stepCount = 0;
      
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
  else if (String(topic) == "bille/pomodoro/state") {
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
  else if (String(topic) == "bille/alerts/movement") {
    String reminderMsg = doc["message"].as<String>();
    
    display.clearBuffer();
    display.setFont(u8g2_font_8x13_tf);
    display.setCursor(0, 30);
    display.print(reminderMsg);
    display.sendBuffer();
    
    Serial.println("Movement reminder: " + reminderMsg);
    
  }
  
  // Handle data requests
  else if (String(topic) == "bille/wearable/request") {
    publishBiometricData();
  }
}

void publishBiometricData() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  
  // Publish individual sensor values to HA
  client.publish("bille/sensors/steps", String(currentBio.stepCount).c_str());
  client.publish("bille/sensors/activity", currentBio.activity.c_str());
  
  // Publish combined biometric data
  StaticJsonDocument<400> doc;
  doc["nodeType"] = "WEARABLE";
  doc["timestamp"] = currentBio.timestamp;
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
    doc["breakCompliant"] = (millis() - currentBio.lastMovement) < 30000;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  client.publish("bille/data/biometric", jsonString.c_str());
  
  Serial.println("Biometric data published to MQTT");
}
