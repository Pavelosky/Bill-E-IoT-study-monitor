// ESP8266-2 Environment Monitor with MQTT
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//WiFi and MQTT configuration
#define WIFI_SSID       "TechLabNet"   
#define WIFI_PASSWORD   "BC6V6DE9A8T9"      
#define MQTT_SERVER     "192.168.1.107"     
#define MQTT_PORT       1883
#define MQTT_USER       "bille_mqtt"
#define MQTT_PASSWORD   "BillE2025_Secure!" 

// MQTT Client
WiFiClient espClient;
PubSubClient client(espClient);

// Pin definitions
#define DHT_PIN         D6
#define DHT_TYPE        DHT11
#define ANALOG_PIN      A0
#define SOUND_DIGITAL   D5

// Objects
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);


// Environmental data structure
struct EnvironmentData {
  float temperature;
  float humidity;
  int lightLevel;
  int noiseLevel;
  bool soundDetected;
  unsigned long timestamp;
};

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

void readEnvironment() {
  // Read temperature and humidity from DHT
  currentEnv.temperature = dht.readTemperature();
  currentEnv.humidity = dht.readHumidity();
  
  // Read light level from KY-018
  currentEnv.lightLevel = readKY018Light();
  
  // Read noise level from KY-038
  currentEnv.noiseLevel = readKY038Noise();
  
  // Read digital sound detection from KY-038
  currentEnv.soundDetected = digitalRead(SOUND_DIGITAL) == HIGH;
  
  // Timestamp
  currentEnv.timestamp = millis();
  
  // Validate DHT readings
  if (isnan(currentEnv.temperature) || isnan(currentEnv.humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    currentEnv.temperature = -999;
    currentEnv.humidity = -999;
  }
}

int readKY018Light() {
  int rawValue = analogRead(ANALOG_PIN);
  int lux = map(rawValue, 0, 1024, 0, 1000);
  return lux;
}

int readKY038Noise() {
  int maxSound = 0;
  int minSound = 1024;
  
  for (int i = 0; i < 100; i++) {
    int soundLevel = analogRead(ANALOG_PIN);
    if (soundLevel > maxSound) maxSound = soundLevel;
    if (soundLevel < minSound) minSound = soundLevel;
    delayMicroseconds(100);
  }
  
  return maxSound - minSound;
}


void displayEnvironment() {
  static unsigned long lastDisplay = 0;
  static int displayMode = 0;
  
  if (millis() - lastDisplay > 3000) {
    lcd.clear();
    
    switch (displayMode) {
      case 0:
        lcd.setCursor(0, 0);
        if (currentEnv.temperature != -999) {
          lcd.printf("Temp: %.1fC", currentEnv.temperature);
        } else {
          lcd.print("Temp: Error");
        }
        lcd.setCursor(0, 1);
        if (currentEnv.humidity != -999) {
          lcd.printf("Hum: %.0f%%", currentEnv.humidity);
        } else {
          lcd.print("Humidity: Error");
        }
        break;
        
      case 1:
        lcd.setCursor(0, 0);
        lcd.printf("Light: %d", currentEnv.lightLevel);
        lcd.setCursor(0, 1);
        lcd.printf("Noise: %d", currentEnv.noiseLevel);
        break;

      // TODO: For DEBUG, needs to be removed  
      case 2:
        lcd.setCursor(0, 0);
        lcd.print("MQTT: ");
        lcd.print(client.connected() ? "OK" : "ERR");
        lcd.setCursor(0, 1);
        lcd.print("WiFi: ");
        lcd.print(WiFi.status() == WL_CONNECTED ? "OK" : "ERR");
        break;
    }
    
    displayMode = (displayMode + 1) % 3;
    lastDisplay = millis();
  }
}

void printEnvironment() {
  Serial.println("=== Environmental Data ===");
  Serial.printf("Temperature: %.1f°C\n", currentEnv.temperature);
  Serial.printf("Humidity: %.1f%%\n", currentEnv.humidity);
  Serial.printf("Light Level: %d lux\n", currentEnv.lightLevel);
  Serial.printf("Noise Level: %d\n", currentEnv.noiseLevel);
  Serial.printf("Sound Detected: %s\n", currentEnv.soundDetected ? "YES" : "NO");
  Serial.println("==========================");
}

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
    String clientId = "BillE-Environment-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      
      // Subscribe to control topics
      client.subscribe("bille/environment/request");
      client.subscribe("bille/session/state");
      
      // Announce presence
      client.publish("bille/status/environment", "online", true);
      
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
  
  // Handle session state updates
  if (String(topic) == "bille/session/state") {
    StaticJsonDocument<200> doc;
    deserializeJson(doc, message);
    
    bool sessionActive = doc["active"];
    String userId = doc["userId"].as<String>();
    
    // Update local session state
    Serial.printf("Session update: %s for user %s\n", 
                  sessionActive ? "ACTIVE" : "INACTIVE", userId.c_str());
  }
}

void publishEnvironmentalData() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  
  // Publish individual sensor values to HA
  client.publish("bille/sensors/temperature", String(currentEnv.temperature).c_str());
  client.publish("bille/sensors/humidity", String(currentEnv.humidity).c_str());
  client.publish("bille/sensors/light", String(currentEnv.lightLevel).c_str());
  client.publish("bille/sensors/noise", String(currentEnv.noiseLevel).c_str());
  
  // Publish combined sensor data
  StaticJsonDocument<300> doc;
  doc["nodeType"] = "ENVIRONMENT";
  doc["timestamp"] = millis();
  doc["temperature"] = currentEnv.temperature;
  doc["humidity"] = currentEnv.humidity;
  doc["lightLevel"] = currentEnv.lightLevel;
  doc["noiseLevel"] = currentEnv.noiseLevel;
  doc["soundDetected"] = currentEnv.soundDetected;
  
  String jsonString;
  serializeJson(doc, jsonString);
  client.publish("bille/data/environment", jsonString.c_str());
  
  Serial.println("Environmental data published to MQTT");
}

void checkEnvironmentalAlerts() {
  // Smart environmental analysis for HA dashboard
  StaticJsonDocument<200> alertDoc;
  alertDoc["nodeType"] = "ENVIRONMENT";
  alertDoc["timestamp"] = millis();
  
  bool hasAlert = false;
  String alertMessage = "";
  String alertLevel = "info";
  
  // Temperature alerts
  if (currentEnv.temperature < 20) {
    alertMessage = "Temperature too low for productivity";
    alertLevel = "warning";
    hasAlert = true;
  } else if (currentEnv.temperature > 26) {
    alertMessage = "Temperature too high for focus";
    alertLevel = "warning";
    hasAlert = true;
  }
  
  // Noise alerts
  if (currentEnv.noiseLevel > 60) {
    alertMessage = "Environment too noisy for concentration";
    alertLevel = "warning";
    hasAlert = true;
  }
  
  // Light alerts
  if (currentEnv.lightLevel < 100) {
    alertMessage = "Lighting may be too dim for productivity";
    alertLevel = "info";
    hasAlert = true;
  }
  
  if (hasAlert) {
    alertDoc["alert"] = alertMessage;
    alertDoc["level"] = alertLevel;
    alertDoc["temperature"] = currentEnv.temperature;
    alertDoc["noise"] = currentEnv.noiseLevel;
    alertDoc["light"] = currentEnv.lightLevel;
    
    String alertString;
    serializeJson(alertDoc, alertString);
    client.publish("bille/alerts/environment", alertString.c_str());
    
    Serial.println("Environmental alert: " + alertMessage);
  }
}
