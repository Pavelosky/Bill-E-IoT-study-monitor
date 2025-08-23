// ESP8266-1 Main Brain with Mesh Network
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Include our modules
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
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
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
  
  delay(100);
}