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

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Bill-E Environment Monitor with MQTT Starting...");
  
  // Initialize pins
  pinMode(SOUND_DIGITAL, INPUT);
  
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
  lcd.print("MQTT Ready");
  
  Serial.println("Environment Monitor with MQTT Ready!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
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
    checkEnvironmentalAlerts();
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