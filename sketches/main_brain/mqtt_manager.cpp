#include "mqtt_manager.h"
#include "config.h"
#include "sensors.h"
#include "pomodoro.h"
#include <ArduinoJson.h>

// Global objects
WiFiClient espClient;
PubSubClient mqttClient(espClient);
bool mqttConnected = false;

// External variables
extern bool sessionActive;
extern String currentUser;
extern unsigned long sessionStart;

// External functions
void startSession(String userId);
void endSession();
void snoozeBreak();
void transitionToNextState();

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

void mqttCallback(char* topic, byte* payload, unsigned int length) {
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

void publishMQTTMessage(const char* topic, String payload) {
  if (mqttConnected && mqttClient.connected()) {
    mqttClient.publish(topic, payload.c_str());
    Serial.println("Published to " + String(topic) + ": " + payload);
  }
}

void publishSensorData() {
  if (!mqttConnected || !mqttClient.connected()) return;
  
  // Create comprehensive sensor data JSON
  StaticJsonDocument<600> doc;
  doc["timestamp"] = millis();
  
  
  // Environmental data
  if (envData.dataAvailable) {
    doc["temperature"] = envData.temperature;
    doc["humidity"] = envData.humidity;
    doc["light_level"] = envData.lightLevel;
    doc["noise_level"] = envData.noiseLevel;
    doc["sound_detected"] = envData.soundDetected;
    
    // Publish individual topics for HA sensors
    publishMQTTMessage(TOPIC_TEMPERATURE, String(envData.temperature));
    publishMQTTMessage(TOPIC_HUMIDITY, String(envData.humidity));
    publishMQTTMessage(TOPIC_LIGHT, String(envData.lightLevel));
    publishMQTTMessage(TOPIC_NOISE, String(envData.noiseLevel));
  }
  
  // Biometric data
  if (bioData.dataAvailable) {
    doc["heart_rate"] = bioData.heartRate;
    doc["step_count"] = bioData.stepCount;
    doc["activity"] = bioData.activity;
    doc["acceleration"] = bioData.acceleration;
    doc["last_movement"] = bioData.lastMovement;
    
    // Publish individual topics
    publishMQTTMessage(TOPIC_HEARTRATE, String(bioData.heartRate));
    publishMQTTMessage(TOPIC_STEPS, String(bioData.stepCount));
    publishMQTTMessage(TOPIC_ACTIVITY, bioData.activity);
  }
  
  // Pomodoro data
  doc["session_active"] = sessionActive;
  if (sessionActive) {
    doc["current_user"] = currentUser;
    doc["pomodoro_state"] = pomodoro.currentState;
    doc["pomodoro_cycles"] = pomodoro.completedCycles;
    doc["time_remaining"] = getTimeRemainingSeconds();
    
    // Publish Pomodoro topics
    publishMQTTMessage(TOPIC_SESSION_ACTIVE, sessionActive ? "true" : "false");
    publishMQTTMessage(TOPIC_SESSION_USER, currentUser);
    publishMQTTMessage(TOPIC_POMODORO_STATE, String(pomodoro.currentState));
    publishMQTTMessage(TOPIC_POMODORO_CYCLES, String(pomodoro.completedCycles));
    publishMQTTMessage(TOPIC_POMODORO_TIME, String(getTimeRemainingSeconds()));
  }
  
  // Publish complete data as JSON
  String jsonString;
  serializeJson(doc, jsonString);
  publishMQTTMessage("bille/data/complete", jsonString);
  
  Serial.println("Sensor data published to MQTT");
}