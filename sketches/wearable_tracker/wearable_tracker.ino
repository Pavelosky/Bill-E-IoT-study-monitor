// ESP8266-3 Wearable Tracker
#include <U8g2lib.h>
#include <Wire.h>
#include <MPU6050.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Include our modules
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
  
  // Initialize pins
  pinMode(HEART_LED, OUTPUT);
  digitalWrite(HEART_LED, LOW);
  
  // Initialize I2C
  Wire.begin(OLED_SDA, OLED_SCL);
  
  // Initialize OLED display
  display.begin();
  display.enableUTF8Print();
  
  // Initialize MPU6050
  Serial.println("Initializing MPU6050...");
  mpu.initialize();
  
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
