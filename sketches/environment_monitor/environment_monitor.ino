// ESP8266-2 Environment Monitor with Mesh Network
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "painlessMesh.h"
#include <ArduinoJson.h>

// Mesh network config
#define MESH_PREFIX     "BillE_Focus_Mesh"
#define MESH_PASSWORD   "FocusBot2025"
#define MESH_PORT       5555

// Pin definitions
#define DHT_PIN         D6
#define DHT_TYPE        DHT11
#define ANALOG_PIN      A0
#define SOUND_DIGITAL   D5

// Objects
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
painlessMesh mesh;
Scheduler userScheduler;

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

// Function prototypes
void sendEnvironmentalData();
Task taskSendData(TASK_SECOND * 10, TASK_FOREVER, &sendEnvironmentalData);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Environment Monitor with Mesh Starting...");
  
  // Initialize pins
  pinMode(SOUND_DIGITAL, INPUT);
  
  // Initialize DHT sensor
  dht.begin();
  
  // Initialize I2C for LCD
  Wire.begin(D1, D2);
  lcd.init();
  lcd.backlight();
  
  // Initialize mesh network
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  
  // Add task to send data every 10 seconds
  userScheduler.addTask(taskSendData);
  taskSendData.enable();
  
  // Welcome message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Env Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Mesh Ready");
  
  Serial.println("Environment Monitor with Mesh Ready!");
  Serial.printf("Node ID: %u\n", mesh.getNodeId());
}

void loop() {
  mesh.update();
  
  // Read sensors every 5 seconds
  static unsigned long lastRead = 0;
  if (millis() - lastRead > 5000) {
    readEnvironment();
    displayEnvironment();
    printEnvironment();
    lastRead = millis();
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

void sendEnvironmentalData() {
  // Create JSON message with environmental data
  StaticJsonDocument<300> doc;
  doc["type"] = "ENVIRONMENTAL_DATA";
  doc["nodeType"] = "ENVIRONMENT";
  doc["nodeId"] = mesh.getNodeId();
  doc["timestamp"] = currentEnv.timestamp;
  doc["temperature"] = currentEnv.temperature;
  doc["humidity"] = currentEnv.humidity;
  doc["lightLevel"] = currentEnv.lightLevel;
  doc["noiseLevel"] = currentEnv.noiseLevel;
  doc["soundDetected"] = currentEnv.soundDetected;
  
  String message;
  serializeJson(doc, message);
  
  mesh.sendBroadcast(message);
  Serial.println("Environmental data sent to mesh: " + message);
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
        
      case 2:
        lcd.setCursor(0, 0);
        lcd.printf("Mesh: %d nodes", mesh.getNodeList().size());
        lcd.setCursor(0, 1);
        lcd.printf("ID: %u", mesh.getNodeId());
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
  Serial.printf("Mesh Nodes: %d\n", mesh.getNodeList().size());
  Serial.println("==========================");
}

// Mesh callbacks
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
  
  // Parse message and respond if needed
  StaticJsonDocument<200> doc;
  deserializeJson(doc, msg);
  
  String msgType = doc["type"];
  if (msgType == "REQUEST_ENV_DATA") {
    // Send immediate environmental data
    sendEnvironmentalData();
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection: %u\n", nodeId);
  
  // Send identification message
  StaticJsonDocument<100> doc;
  doc["type"] = "NODE_IDENTIFICATION";
  doc["nodeType"] = "ENVIRONMENT";
  doc["capabilities"] = "temperature,humidity,light,noise";
  
  String message;
  serializeJson(doc, message);
  mesh.sendSingle(nodeId, message);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections. Nodes: %d\n", mesh.getNodeList().size());
}
