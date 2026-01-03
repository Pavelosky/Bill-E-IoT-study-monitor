/*
===============================================================
ESP8266-3 Wearable Tracker - Bill-E Focus Robot
IoT Productivity System - University of London BSc Computer Science
Module: CM3040 Physical Computing and Internet-of-Things (IoT)
===============================================================

DEVICE OVERVIEW:
- Personal activity and biometric monitoring device
- Step counting with improved accelerometer algorithms
- Activity detection (sitting, walking, running, moving)
- OLED display with multiple information screens
- Health alerts for movement reminders and break compliance

PIN CONNECTIONS:
- OLED_SDA:    D2  (I2C SDA for 128x64 OLED display)
- OLED_SCL:    D1  (I2C SCL for 128x64 OLED display)
- BUTTON_PIN:  D3  (Push button for display mode switching)
- MPU6050_SDA: D2  (I2C SDA for accelerometer, shared with OLED)
- MPU6050_SCL: D1  (I2C SCL for accelerometer, shared with OLED)

HARDWARE COMPONENTS:
- ESP8266 NodeMCU v1.0
- MPU6050 6-axis Accelerometer/Gyroscope
- 0.96" I2C OLED Display (128x64, SSD1306)
- Push Button for UI navigation

BIOMETRIC FEATURES:
- Step Detection: Threshold-based algorithm with smoothing
- Activity Classification: Sitting, Still, Moving, Walking, Running
- Movement Tracking: Last movement timestamp for health alerts
- Acceleration Monitoring: Real-time g-force measurement

DISPLAY MODES:
- Mode 0: Biometric overview (steps, activity, session context)
- Mode 1: Detailed activity status (acceleration, movement time)
- Mode 2: Pomodoro progress (timer, cycles, percentage complete)
- Mode 3: Break compliance (movement encouragement during breaks)

HEALTH MONITORING:
- Work Session Alerts: Movement reminders after 20+ minutes sitting
- Break Compliance: Encourages movement during Pomodoro breaks
- Activity Thresholds: Configurable detection parameters

MQTT TOPICS (Published):
- bille/data/biometric          - Complete biometric data package
- bille/sensors/steps           - Individual step count
- bille/sensors/activity        - Current activity classification
- bille/sensors/last_movement_minutes - Time since last movement
- bille/alerts/health           - Health and movement alerts

MQTT TOPICS (Subscribed):
- bille/session/state           - Session start/stop notifications
- bille/pomodoro/state          - Timer updates and context
- bille/wearable/request        - Data requests from main brain
- bille/alerts/movement         - Movement reminders from system

STEP DETECTION ALGORITHM:
- Smoothing: Exponential moving average (α = 0.2)
- Threshold: 0.05g step-up, -0.02g step-down detection
- Debounce: 300ms minimum interval between steps
- Validation: Acceleration magnitude analysis

DEPENDENCIES:
- U8g2lib Library (OLED display)
- MPU6050 Library (accelerometer)
- ArduinoJson Library
- ESP8266WiFi Library
- PubSubClient Library

NETWORK CONFIGURATION:
- WiFi SSID: TechLabNet
- MQTT Broker: 192.168.1.107:1883
- Authentication: bille_mqtt user
===============================================================
*/

// ESP8266-3 Wearable Tracker
#include <U8g2lib.h>
#include <Wire.h>
#include <MPU6050.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Include modules
#include "config.h"
#include "biometric_data.h"
#include "biometric_sensors.h"
#include "display_oled.h"
#include "mqtt_communication.h"
#include "health_monitor.h"

// Objects
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
MPU6050 mpu;
WiFiClient espClient;
PubSubClient client(espClient);

// Biometric data instance
BiometricData currentBio;
PomodoroInfo pomodoroInfo;

// Activity detection variables - defined here, declared as extern in biometric_sensors.h
float lastAccelMagnitude = 0;
int stepCount = 0;
unsigned long lastStepTime = 0;
bool stepDetected = false;

// Session state
bool sessionActive = false;
String currentUser = "";

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Bill-E Wearable Tracker with MQTT Starting...");
  
  // Initialize biometric data
  currentBio.lastMovement = millis();

  // Initialize I2C
  Wire.begin(OLED_SDA, OLED_SCL);
  
  // Initialize OLED display
  display.begin();
  display.enableUTF8Print();
  
  // Initialize MPU6050
  Serial.println("Initializing MPU6050...");
  mpu.initialize();

  // Initialize the button pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Test gyro connection
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful");
  } else {
    Serial.println("MPU6050 connection failed");
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tf);
    display.setCursor(0, 20);
    display.print("ERROR:");
    display.setCursor(0, 40);
    display.print("MPU6050 Error");
    display.sendBuffer();
    while (1) delay(10);
  }
  
  // Setup WiFi and MQTT
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(mqtt_callback);
  
  // Welcome screen
  showWelcomeScreen();
  
  Serial.println("Wearable Tracker with MQTT Ready!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Improved step counter algorithm enabled");
  Serial.printf("Step detection: threshold=%.1f, delay=%dms\n", ACCEL_THRESHOLD, STEP_DELAY);
  delay(2000);
}

void loop() {
  
 if (!client.connected()) {
   reconnect_mqtt();
 }
 client.loop();
 
 // Read sensors every 5 seconds
 static unsigned long lastRead = 0;
 if (millis() - lastRead > 5000) {
   readBiometrics();
   publishBiometricData();
   lastRead = millis();
 }
 
 // Check for health alerts every 30 seconds
 static unsigned long lastHealthCheck = 0;
 if (millis() - lastHealthCheck > 30000) {
   publishHealthAlerts();
   lastHealthCheck = millis();
 }
 
  // Update display
  updateDisplay();
}
