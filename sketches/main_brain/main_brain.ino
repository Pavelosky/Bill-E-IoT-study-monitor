// ESP8266-1 Main Brain - Refactored with Modular Design
#include "config.h"
#include "sensors.h"
#include "sound_effects.h"
#include "pomodoro.h"
#include "display_manager.h"
#include "rfid_handler.h"
#include "mqtt_manager.h"
#include "session_manager.h"

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Bill-E Main Brain with MQTT Starting...");
  
  // Initialize pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize display
  initializeDisplay();
  
  // Initialize RFID
  initializeRFID();
  
  // Setup WiFi and MQTT
  setupWiFi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  
  showWelcomeScreen();
  
  Serial.println("Main Brain with MQTT Ready!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
  
  handleRFID();
  updatePomodoroTimer();
  updateDisplay();
  
  // Publish system status every 30 seconds
  static unsigned long lastStatusUpdate = 0;
  if (millis() - lastStatusUpdate > SYSTEM_STATUS_INTERVAL) {
    publishSystemStatus();
    lastStatusUpdate = millis();
  }
  
  // Publish Pomodoro state every 10 seconds during active session
  static unsigned long lastPomodoroUpdate = 0;
  if (sessionActive && millis() - lastPomodoroUpdate > POMODORO_UPDATE_INTERVAL) {
    publishPomodoroState();
    lastPomodoroUpdate = millis();
  }
  
  delay(LOOP_DELAY);
}

