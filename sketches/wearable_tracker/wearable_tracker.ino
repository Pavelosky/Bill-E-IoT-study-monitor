// ESP8266-3 Wearable Tracker - Refactored with Modular Design
#include "config.h"
#include "sensors.h"
#include "heart_rate.h"
#include "activity_tracker.h"
#include "display_manager.h"
#include "mqtt_manager.h"
#include "session_manager.h"

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Bill-E Wearable Tracker with MQTT Starting...");
  
  // Initialize heart rate monitoring
  initializeHeartRate();
  
  // Initialize display (this also sets up I2C)
  initializeDisplay();
  
  // Initialize activity tracker
  if (!initializeActivityTracker()) {
    showError("MPU6050 Error");
    while (1) delay(10);
  }
  
  // Initialize session
  initializeSession();
  
  // Setup WiFi and MQTT
  setupWiFi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(mqttCallback);
  
  // Welcome screen
  showWelcomeScreen();
  
  Serial.println("Wearable Tracker with MQTT Ready!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  delay(2000);
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  
  // Read sensors every 5 seconds
  static unsigned long lastRead = 0;
  if (millis() - lastRead > SENSOR_READ_INTERVAL) {
    readBiometrics();
    publishBiometricData();
    lastRead = millis();
  }
  
  // Check for health alerts every 30 seconds
  static unsigned long lastHealthCheck = 0;
  if (millis() - lastHealthCheck > HEALTH_CHECK_INTERVAL) {
    publishHealthAlerts();
    lastHealthCheck = millis();
  }
  
  // Update display
  updateDisplay();
}
