/*
===============================================================
ESP8266-2 Environment Monitor - Bill-E Focus Robot
IoT Productivity System - University of London BSc Computer Science
Module: CM3040 Physical Computing and Internet-of-Things (IoT)
===============================================================

DEVICE OVERVIEW:
- Environmental monitoring for optimal study conditions
- Temperature, humidity, light, and noise level sensing
- Automatic fan control based on temperature thresholds
- LCD display for real-time environmental data
- MQTT client for data transmission to main brain

PIN CONNECTIONS:
- DHT_PIN:      D6  (DHT11 Temperature/Humidity sensor)
- ANALOG_PIN:   A0  (KY-018 Light sensor & KY-038 Noise sensor)
- SOUND_DIGITAL: D5 (KY-038 Digital sound detection)
- FAN_RELAY_PIN: D7 (Relay module for fan control)
- SDA (I2C):    D1  (LCD communication)
- SCL (I2C):    D2  (LCD communication)

HARDWARE COMPONENTS:
- ESP8266 NodeMCU v1.0
- DHT11 Temperature/Humidity Sensor
- KY-018 Photoresistor Light Sensor
- KY-038 Sound Detection Sensor
- 16x2 I2C LCD Display (0x27)
- 5V Relay Module
- 12V Desktop Fan (controlled via relay)

SENSOR SPECIFICATIONS:
- DHT11: Temperature (0-50°C, ±2°C), Humidity (20-90%, ±5%)
- KY-018: Light level measurement with curve-fitted lux calculation
- KY-038: Noise level detection (analog) + sound threshold (digital)

FAN CONTROL THRESHOLDS:
- Fan ON: Temperature >= 24.0°C
- Fan OFF: Temperature <= 22.0°C
- Manual override supported via MQTT commands

MQTT TOPICS (Published):
- bille/data/environment     - Combined environmental data
- bille/sensors/temperature  - Individual temperature reading
- bille/sensors/humidity     - Individual humidity reading
- bille/sensors/light        - Individual light level
- bille/sensors/noise        - Individual noise level
- bille/sensors/fan_state    - Fan on/off status
- bille/status/fan          - Detailed fan control info
- bille/alerts/environment  - Environmental quality alerts

MQTT TOPICS (Subscribed):
- bille/environment/request  - Data request from main brain
- bille/session/state        - Session status updates
- bille/commands/fan         - Fan control commands (manual_on/manual_off/auto)

DEPENDENCIES:
- DHT Library
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

// ESP8266-2 Environment Monitor with MQTT
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Include our modules
#include "config.h"
#include "environment_data.h"
#include "sensor_reader.h"
#include "display_controller.h"
#include "mqtt_client.h"
#include "environmental_analysis.h"

// MQTT Client
WiFiClient espClient;
PubSubClient client(espClient);

// Objects
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Environmental data instance
EnvironmentData currentEnv;

// Fan control state variables - defined here, declared as extern in environmental_analysis.h
bool fanState = false;           
bool manualOverride = false;     
bool manualFanState = false;     

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Bill-E Environment Monitor with MQTT Starting...");
  
  // Initialize pins
  pinMode(SOUND_DIGITAL, INPUT);
  pinMode(FAN_RELAY_PIN, OUTPUT);
  digitalWrite(FAN_RELAY_PIN, LOW);  // Start with fan OFF
  
  // Initialize DHT sensor
  dht.begin();
  
  // Initialize I2C for LCD
  Wire.begin(D1, D2);
  lcd.init();
  lcd.backlight();
  
  // Setup WiFi and MQTT
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(mqtt_callback);
  
  
  // Welcome message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Env Monitor");
  lcd.setCursor(0, 1);
  lcd.print("MQTT + Fan Ready");
  
  Serial.println("Environment Monitor with MQTT and Fan Control Ready!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Fan relay pin: D7");
  Serial.printf("Auto fan thresholds: ON >= %.1f°C, OFF <= %.1f°C\n", FAN_ON_TEMP, FAN_OFF_TEMP);
}

void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();
  
  // Read sensors every 10 seconds
  static unsigned long lastRead = 0;
  if (millis() - lastRead > 10000) {
    readEnvironment();
    publishEnvironmentalData();
    checkEnvironmentalAlerts();  // This now includes fan control
    lastRead = millis();
  }
  
  // Update display every 5 seconds
  static unsigned long lastDisplay = 0;
  if (millis() - lastDisplay > 5000) {
    displayEnvironment();
    printEnvironment();
    lastDisplay = millis();
  }
}
