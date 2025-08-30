/*
===============================================================
ESP8266-1 Main Brain - Bill-E Focus Robot
IoT Productivity System - University of London BSc Computer Science
Module: CM3040 Physical Computing and Internet-of-Things (IoT)
===============================================================

DEVICE OVERVIEW:
- Central coordination node with RFID authentication
- LCD display for user interface and system status
- Pomodoro timer management with audio feedback
- MQTT broker for inter-device communication
- Touch sensor for manual timer control

PIN CONNECTIONS:
- RST_PIN:      D1  (RFID Reset)
- SS_PIN:       D2  (RFID SPI Select)
- TOUCH_SENSOR: D0  (Capacitive touch input)
- BUZZER_PIN:   D8  (Audio feedback)
- SDA (I2C):    D3  (LCD communication)
- SCL (I2C):    D4  (LCD communication)
- SPI MOSI:     D7  (RFID communication)
- SPI MISO:     D6  (RFID communication)
- SPI SCK:      D5  (RFID communication)

HARDWARE COMPONENTS:
- ESP8266 NodeMCU v1.0
- MFRC522 RFID Reader
- 16x2 I2C LCD Display (0x27)
- Passive Buzzer
- Touch Sensor Module

MQTT TOPICS (Published):
- bille/session/state       - Session status and user info
- bille/pomodoro/state      - Timer state and progress
- bille/status/system       - System health monitoring
- bille/alerts/movement     - Movement reminders

MQTT TOPICS (Subscribed):
- bille/data/environment    - Environmental sensor data
- bille/data/biometric      - Wearable tracker data
- bille/commands/session    - Remote session control
- bille/commands/pomodoro   - Remote timer control

DEPENDENCIES:
- MFRC522 Library
- LiquidCrystal_I2C Library
- ArduinoJson Library
- ESP8266WiFi Library
- PubSubClient Library

NETWORK CONFIGURATION:
- WiFi SSID: TechLabNet
- MQTT Broker: 192.168.1.107:1883
- Authentication: bille_mqtt user
===============================================================
*/

// ESP8266-1 Main Brain
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Include modules
#include "config.h"
#include "data_structures.h"
#include "rfid_manager.h"
#include "pomodoro_timer.h"
#include "display_manager.h"
#include "audio_system.h"
#include "mqtt_handler.h"
#include "data_analysis.h"

// Objects
MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// MQTT state
bool mqttConnected = false;

// Session state - defined here, declared in data_structures.h
bool sessionActive = false;
String currentUser = "";
unsigned long sessionStart = 0;

// Global data instances - defined here, declared in data_structures.h
PomodoroSession pomodoro;
EnvironmentData envData;
BiometricData bioData;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Bill-E Main Brain with MQTT Starting...");
  
  // Initialize pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TOUCH_SENSOR, INPUT);
  
  // Initialize I2C for LCD
  Wire.begin(D3, D4);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();

  // Reset RFID properly
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, LOW);
  delay(100);
  digitalWrite(RST_PIN, HIGH);
  delay(100);
  
  // Initialize SPI and RFID
  SPI.begin();
  rfid.PCD_Init();
  
  // Test RFID
  byte version = rfid.PCD_ReadRegister(rfid.VersionReg);
  Serial.printf("RFID Version: 0x%02X\n", version);
  
  // Setup WiFi and MQTT
  setup_wifi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqtt_callback);
  
  showWelcomeScreen();
  
  Serial.println("Main Brain with MQTT Ready!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect_mqtt();
  }
  mqttClient.loop();
  
  handleRFID();
  updatePomodoroTimer();
  updateDisplay();
  
  // Publish system status every 30 seconds
  static unsigned long lastStatusUpdate = 0;
  if (millis() - lastStatusUpdate > 30000) {
    publishSystemStatus();
    lastStatusUpdate = millis();
  }
  
  // Publish Pomodoro state every 10 seconds during active session
  static unsigned long lastPomodoroUpdate = 0;
  if (sessionActive && millis() - lastPomodoroUpdate > 10000) {
    publishPomodoroState();
    lastPomodoroUpdate = millis();
  }

  // Handle touch sensor for display switching
  static bool lastTouchForDisplay = false;
  bool currentTouch = digitalRead(TOUCH_SENSOR) == HIGH;
  if (currentTouch && !lastTouchForDisplay) {
    // Double-tap detection or separate touch logic for display
  }
  lastTouchForDisplay = currentTouch;
  
  delay(100);
}
