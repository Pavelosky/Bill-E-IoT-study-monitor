// ESP8266-3 Wearable Tracker with Mesh Network
#include <U8g2lib.h>
#include <Wire.h>
#include <MPU6050.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi and MQTT configuration
#define WIFI_SSID       "TechLabNet"   
#define WIFI_PASSWORD   "BC6V6DE9A8T9"      
#define MQTT_SERVER     "192.168.1.107"     
#define MQTT_PORT       1883
#define MQTT_USER       "bille_mqtt"
#define MQTT_PASSWORD   "BillE2025_Secure!" 

// Pin definitions
#define OLED_SDA        D2
#define OLED_SCL        D1
#define HEART_SENSOR    A0
#define HEART_LED       D5

// Objects
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
MPU6050 mpu;
WiFiClient espClient;
PubSubClient client(espClient);

// Biometric data structure
struct BiometricData {
  int heartRate;
  String activity;
  int stepCount;
  unsigned long lastMovement;
  float acceleration;
  unsigned long timestamp;
};

BiometricData currentBio;


//////////////// POMODORO SESSION VARIABLES /////////////////
enum PomodoroState {
  IDLE,
  WORK_SESSION,
  SHORT_BREAK,
  LONG_BREAK
};

struct PomodoroInfo {
  PomodoroState currentState;
  unsigned long timeRemaining;  // in seconds
  int completedCycles;
  bool snoozed;
  int snoozeCount;
  String stateText;
  bool dataAvailable;
  unsigned long lastUpdate;
};

PomodoroInfo pomodoroInfo;


// Activity detection variables
float lastAccelMagnitude = 0;
int stepCount = 0;
unsigned long lastStepTime = 0;

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
    showError("MPU6050 Error");
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

void readBiometrics() {
  // Read heart rate from KY-036
  currentBio.heartRate = readHeartRate();
  
  // Read accelerometer for activity detection
  readActivityData();
  
  // Detect current activity
  currentBio.activity = detectActivity();
  
  // Update step count
  currentBio.stepCount = stepCount;
  
  // Timestamp
  currentBio.timestamp = millis();
}

int readHeartRate() {
  static unsigned long lastBeat = 0;
  static int beatCount = 0;
  static unsigned long measureStart = 0;
  static int lastValidBPM = 75;
  
  // Read sensor value
  int sensorValue = analogRead(HEART_SENSOR);
  
  // Simple peak detection for heartbeat
  static int lastValue = 0;
  static bool beatDetected = false;
  
  if (sensorValue > 600 && lastValue < 600 && !beatDetected) {
    unsigned long now = millis();
    
    if (now - lastBeat > 300) { // Minimum 300ms between beats
      beatDetected = true;
      lastBeat = now;
      beatCount++;
      
      // Blink LED on heartbeat
      digitalWrite(HEART_LED, HIGH);
      
      // Calculate BPM over 15-second windows
      if (measureStart == 0) measureStart = now;
      
      if (now - measureStart >= 15000) { // 15 seconds
        int bpm = (beatCount * 60) / 15;
        beatCount = 0;
        measureStart = now;
        
        if (bpm >= 40 && bpm <= 200) {
          lastValidBPM = bpm;
          return bpm;
        }
      }
    }
  }
  
  if (sensorValue < 500) {
    beatDetected = false;
    digitalWrite(HEART_LED, LOW);
  }
  
  lastValue = sensorValue;
  return lastValidBPM;
}

void readActivityData() {
  // Read raw accelerometer data
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  
  // Convert to g-force
  float gx = ax / 16384.0;
  float gy = ay / 16384.0;
  float gz = az / 16384.0;
  
  // Calculate acceleration magnitude
  float magnitude = sqrt(gx*gx + gy*gy + gz*gz);
  currentBio.acceleration = magnitude;
  
  // Detect movement and steps
  float delta = abs(magnitude - lastAccelMagnitude);
  
  if (delta > 0.3) { // Movement threshold
    currentBio.lastMovement = millis();
    
    // Step detection
    if (delta > 0.8 && millis() - lastStepTime > 300) {
      stepCount++;
      lastStepTime = millis();
      Serial.println("Step detected! Count: " + String(stepCount));
    }
  }
  
  lastAccelMagnitude = magnitude;
}

String detectActivity() {
  unsigned long timeSinceMovement = millis() - currentBio.lastMovement;
  
  if (timeSinceMovement < 1000) {
    if (currentBio.acceleration > 1.5) {
      return "Running";
    } else if (currentBio.acceleration > 1.2) {
      return "Walking";
    } else {
      return "Moving";
    }
  } else if (timeSinceMovement < 30000) { // 30 seconds
    return "Still";
  } else {
    return "Sitting";
  }
}



void updateDisplay() {
  static unsigned long lastDisplayUpdate = 0;
  static int displayMode = 0;

  // Check if we should show achievement message
  static int lastCompletedCycles = 0;
  if (pomodoroInfo.completedCycles > lastCompletedCycles && pomodoroInfo.dataAvailable) {
    lastCompletedCycles = pomodoroInfo.completedCycles;
  }


// Main display rotation every 4 seconds
  if (millis() - lastDisplayUpdate > 4000) {
    // Adjust display mode count based on session state
    int maxModes = sessionActive ? 4 : 3; // More modes during active session
    displayMode = (displayMode + 1) % maxModes;
    lastDisplayUpdate = millis();
  }
  
  if (sessionActive && pomodoroInfo.dataAvailable) {
    // Enhanced display modes during active Pomodoro session
    switch (displayMode) {
      case 0:
        // Break compliance (only during breaks)
        if (pomodoroInfo.currentState == SHORT_BREAK || pomodoroInfo.currentState == LONG_BREAK) {
          showBreakCompliance();
        } else {
          // Fall back to biometric data during work
          showBiometricData();
        }
        break;
        
      case 1:
        // Heart rate and activity focus
        showBiometricData();
        break;
        
      case 2:
        // Step count and activity encouragement
        showActivityStatus();
        break;
        
      case 3:
        // MQTT and connection status
        showMQTTStatus();
        break;

    }
  } else {
    // Original display modes when not in active session
    switch (displayMode) {
      case 0:
        showBiometricData();
        break;
      case 1:
        showActivityStatus();
        break;
      case 2:
        showMQTTStatus();
        break;
    }
  }
}

void showBiometricData() {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  
  display.setCursor(0, 15);
  display.print("Bill-E Tracker");
  display.setCursor(0, 30);
  display.printf("HR: %d BPM", currentBio.heartRate);
  display.setCursor(0, 45);
  display.printf("Steps: %d", currentBio.stepCount);
  
  // Add Pomodoro context if available
  if (sessionActive && pomodoroInfo.dataAvailable) {
    display.setCursor(0, 60);
    if (pomodoroInfo.currentState == WORK_SESSION) {
      display.print("Focus Mode");
    }
    else if (pomodoroInfo.currentState == SHORT_BREAK) {
      display.print("Short Break");
    }
    else if (pomodoroInfo.currentState == LONG_BREAK) {
      display.print("Long Break");
    }
    else {
      display.print("Idle");
    }
  }
  
  display.sendBuffer();
}


void showWelcomeScreen() {
  display.clearBuffer();
  display.setFont(u8g2_font_8x13_tf);
  display.setCursor(0, 20);
  display.print("Bill-E");
  display.setCursor(0, 40);
  display.print("Wearable");
  display.setCursor(0, 60);
  display.print("MQTT Ready!");
  display.sendBuffer();
}

// TODO: For debugging - MQTT status to be removed
void showMQTTStatus() {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  
  display.setCursor(0, 15);
  display.print("MQTT Status");
  display.setCursor(0, 30);
  display.print("Broker: ");
  display.print(client.connected() ? "OK" : "ERR");
  display.setCursor(0, 45);
  display.print("WiFi: ");
  display.print(WiFi.status() == WL_CONNECTED ? "OK" : "ERR");
  display.setCursor(0, 60);
  display.print("IP: ");
  display.print(WiFi.localIP().toString().substring(10)); // Show last part of IP
  
  display.sendBuffer();
}

void showError(String error) {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.setCursor(0, 20);
  display.print("ERROR:");
  display.setCursor(0, 40);
  display.print(error);
  display.sendBuffer();
}

void showActivityStatus() {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  
  display.setCursor(0, 15);
  display.print("Activity:");
  display.setCursor(0, 35);
  display.setFont(u8g2_font_8x13_tf);
  display.print(currentBio.activity);
  display.setFont(u8g2_font_6x10_tf);
  display.setCursor(0, 55);
  display.printf("Accel: %.2f g", currentBio.acceleration);
  
  display.sendBuffer();
}

void drawProgressBar() {
  // Calculate progress based on Pomodoro state
  int totalDuration;
  switch (pomodoroInfo.currentState) {
    case WORK_SESSION: totalDuration = 25 * 60; break;      // 25 minutes
    case SHORT_BREAK: totalDuration = 5 * 60; break;        // 5 minutes  
    case LONG_BREAK: totalDuration = 15 * 60; break;        // 15 minutes
    default: return; // No progress bar for IDLE
  }
  
  // Use real-time remaining time for progress calculation
  unsigned long realTimeElapsed = (millis() - pomodoroInfo.lastUpdate) / 1000;
  unsigned long currentRemaining = (pomodoroInfo.timeRemaining > realTimeElapsed) ? 
                                   (pomodoroInfo.timeRemaining - realTimeElapsed) : 0;
  
  int elapsed = totalDuration - currentRemaining;
  int progress = (elapsed * 120) / totalDuration; // 120 pixels wide
  
  // Draw progress bar frame
  display.drawFrame(4, 58, 120, 6);
  
  // Fill progress bar
  if (progress > 0) {
    display.drawBox(5, 59, progress, 4);
  }
}

void showBreakCompliance() {
  unsigned long timeSinceMovement = millis() - currentBio.lastMovement;
  
  display.clearBuffer();
  display.setFont(u8g2_font_8x13_tf);
  
  if (timeSinceMovement < 30000) { // Moved recently
    display.setCursor(0, 20);
    display.print("Great!");
    display.setCursor(0, 40);
    display.print("Keep Moving");
    
    // Happy face
    display.drawCircle(100, 30, 12);
    display.drawDisc(95, 25, 2);
    display.drawDisc(105, 25, 2);
    display.drawCircle(100, 32, 6);
    
  } else { // Not moving
    display.setCursor(0, 20);
    display.print("Time to Move!");
    display.setCursor(0, 40);
    display.print("Stand & Stretch");
    
    // Alert icon
    display.drawTriangle(100, 15, 108, 35, 92, 35);
    display.setFont(u8g2_font_6x10_tf);
    display.setCursor(98, 30);
    display.print("!");
  }
  
  display.sendBuffer();
}

void drawStar(int x, int y, int size) {
  display.drawLine(x, y - size, x, y + size);
  display.drawLine(x - size, y, x + size, y);
  display.drawLine(x - size/2, y - size/2, x + size/2, y + size/2);
  display.drawLine(x - size/2, y + size/2, x + size/2, y - size/2);
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
    String clientId = "BillE-Wearable-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      
      // Subscribe to control topics
      client.subscribe("bille/session/state");
      client.subscribe("bille/pomodoro/state");
      client.subscribe("bille/wearable/request");
      client.subscribe("bille/alerts/movement");
      
      // Announce presence
      client.publish("bille/status/wearable", "online", true);
      
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
  
  StaticJsonDocument<300> doc;
  deserializeJson(doc, message);
  
  // Handle session state updates
  if (String(topic) == "bille/session/state") {
    sessionActive = doc["active"];
    currentUser = doc["userId"].as<String>();
    
    if (sessionActive) {
      // Reset step counter for new session
      stepCount = 0;
      
      // Show session start notification
      display.clearBuffer();
      display.setFont(u8g2_font_8x13_tf);
      display.setCursor(0, 30);
      display.print("Session");
      display.setCursor(0, 50);
      display.print("Started!");
      display.sendBuffer();
      delay(2000);
      
      Serial.println("Session started for: " + currentUser);
    } else {
      currentUser = "";
      
      // Show session end notification
      display.clearBuffer();
      display.setFont(u8g2_font_8x13_tf);
      display.setCursor(0, 30);
      display.print("Session");
      display.setCursor(0, 50);
      display.print("Ended");
      display.sendBuffer();
      delay(2000);
      
      Serial.println("Session ended");
    }
  }
  
  // Handle Pomodoro state updates
  else if (String(topic) == "bille/pomodoro/state") {
    pomodoroInfo.currentState = (PomodoroState)doc["state"].as<int>();
    pomodoroInfo.timeRemaining = doc["timeRemaining"];
    pomodoroInfo.completedCycles = doc["completedCycles"];
    pomodoroInfo.snoozed = doc["snoozed"];
    pomodoroInfo.snoozeCount = doc["snoozeCount"];
    pomodoroInfo.stateText = doc["stateText"].as<String>();
    pomodoroInfo.dataAvailable = true;
    pomodoroInfo.lastUpdate = millis();
    
    Serial.println("Pomodoro state updated: " + pomodoroInfo.stateText);
  }
  
  // Handle movement reminders
  else if (String(topic) == "bille/alerts/movement") {
    String reminderMsg = doc["message"].as<String>();
    
    display.clearBuffer();
    display.setFont(u8g2_font_8x13_tf);
    display.setCursor(0, 30);
    display.print(reminderMsg);
    display.sendBuffer();
    
    Serial.println("Movement reminder: " + reminderMsg);
    
    // Brief flash of heart rate LED
    for (int i = 0; i < 3; i++) {
      digitalWrite(HEART_LED, HIGH);
      delay(100);
      digitalWrite(HEART_LED, LOW);
      delay(100);
    }
  }
  
  // Handle data requests
  else if (String(topic) == "bille/wearable/request") {
    publishBiometricData();
  }
}

void publishBiometricData() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  
  // Publish individual sensor values to HA
  client.publish("bille/sensors/heartrate", String(currentBio.heartRate).c_str());
  client.publish("bille/sensors/steps", String(currentBio.stepCount).c_str());
  client.publish("bille/sensors/activity", currentBio.activity.c_str());
  
  // Publish combined biometric data
  StaticJsonDocument<400> doc;
  doc["nodeType"] = "WEARABLE";
  doc["timestamp"] = currentBio.timestamp;
  doc["heartRate"] = currentBio.heartRate;
  doc["activity"] = currentBio.activity;
  doc["stepCount"] = currentBio.stepCount;
  doc["acceleration"] = currentBio.acceleration;
  doc["lastMovement"] = currentBio.lastMovement;
  doc["sessionActive"] = sessionActive;
  doc["currentUser"] = currentUser;
  
  // Add Pomodoro context
  if (pomodoroInfo.dataAvailable) {
    doc["pomodoroState"] = pomodoroInfo.currentState;
    doc["pomodoroTimeRemaining"] = pomodoroInfo.timeRemaining;
    doc["breakCompliant"] = (millis() - currentBio.lastMovement) < 30000;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  client.publish("bille/data/biometric", jsonString.c_str());
  
  Serial.println("Biometric data published to MQTT");
}

void publishHealthAlerts() {
  if (!sessionActive) return;
  
  StaticJsonDocument<300> alertDoc;
  alertDoc["nodeType"] = "WEARABLE";
  alertDoc["timestamp"] = millis();
  alertDoc["userId"] = currentUser;
  
  bool hasAlert = false;
  String alertMessage = "";
  String alertLevel = "info";
  
  // Check for extended sitting during work sessions
  unsigned long timeSinceMovement = millis() - currentBio.lastMovement;
  
  if (pomodoroInfo.currentState == WORK_SESSION && timeSinceMovement > 20 * 60 * 1000) { // 20 minutes
    alertMessage = "Consider taking a short movement break";
    alertLevel = "warning";
    hasAlert = true;
  }
  
  // Check heart rate during work
  if (pomodoroInfo.currentState == WORK_SESSION && currentBio.heartRate > 100) {
    alertMessage = "Heart rate elevated - consider relaxing";
    alertLevel = "warning";
    hasAlert = true;
  }
  
  // Check break compliance
  if ((pomodoroInfo.currentState == SHORT_BREAK || pomodoroInfo.currentState == LONG_BREAK) 
      && timeSinceMovement > 2 * 60 * 1000) { // 2 minutes into break
    alertMessage = "Break time - time to move around!";
    alertLevel = "info";
    hasAlert = true;
  }
  
  if (hasAlert) {
    alertDoc["alert"] = alertMessage;
    alertDoc["level"] = alertLevel;
    alertDoc["heartRate"] = currentBio.heartRate;
    alertDoc["timeSinceMovement"] = timeSinceMovement / 1000; // seconds
    alertDoc["pomodoroState"] = pomodoroInfo.currentState;
    
    String alertString;
    serializeJson(alertDoc, alertString);
    client.publish("bille/alerts/health", alertString.c_str());
    
    Serial.println("Health alert: " + alertMessage);
  }
}
