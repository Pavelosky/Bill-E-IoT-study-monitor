// ESP8266-1 Main Brain with Mesh Networ
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//WiFi and MQTT configuration
#define WIFI_SSID       "TechLabNet"   
#define WIFI_PASSWORD   "BC6V6DE9A8T9"      
#define MQTT_SERVER     "192.168.1.107"     
#define MQTT_PORT       1883
#define MQTT_USER       "bille_mqtt"
#define MQTT_PASSWORD   "BillE2025_Secure!" 

// Pin definitions
const int RST_PIN = D1;
const int SS_PIN = D2;
const int LED_PIN = D0;
const int BUZZER_PIN = D8;

// MQTT Topics
const char* TOPIC_TEMPERATURE = "bille/sensors/temperature";
const char* TOPIC_HUMIDITY = "bille/sensors/humidity";
const char* TOPIC_LIGHT = "bille/sensors/light";
const char* TOPIC_NOISE = "bille/sensors/noise";
const char* TOPIC_HEARTRATE = "bille/sensors/heartrate";
const char* TOPIC_STEPS = "bille/sensors/steps";
const char* TOPIC_ACTIVITY = "bille/sensors/activity";
const char* TOPIC_POMODORO_STATE = "bille/pomodoro/state";
const char* TOPIC_POMODORO_TIME = "bille/pomodoro/time_remaining";
const char* TOPIC_POMODORO_CYCLES = "bille/pomodoro/cycles";
const char* TOPIC_SESSION_ACTIVE = "bille/session/active";
const char* TOPIC_SESSION_USER = "bille/session/user";

// Objects
MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// MQTT state
bool mqttConnected = false;

// Session state
bool sessionActive = false;
String currentUser = "";
unsigned long sessionStart = 0;

enum PomodoroState {
  IDLE,
  WORK_SESSION,
  SHORT_BREAK,
  LONG_BREAK
};

struct PomodoroSession {
  PomodoroState currentState = IDLE;
  unsigned long stateStartTime = 0;     // Start time of the current state
  unsigned long stateDuration = 0;      // Current state duration in milliseconds
  int completedCycles = 0;              // Number of completed pomodoro cycles
  int workDuration = 25;                 // Default 25 minutes
  int shortBreakDuration = 5;           // Default 5 minutes
  int longBreakDuration = 15;            // Default 15 minutes
  bool breakSnoozed = false;            // Snooze state for breaks
  int snoozeCount = 0;                  // Number of times snoozed
  unsigned long snoozeTime = 0;         // Time when snooze was activated
  bool breakComplianceChecked = false;  // Compliance check for breaks
};



// Environmental data from mesh
struct EnvironmentData {
  float temperature = 0;
  float humidity = 0;
  int lightLevel = 0;
  int noiseLevel = 0;
  bool soundDetected = false;
  unsigned long lastUpdate = 0;
  bool dataAvailable = false;
};



// Biometric data from wearable
struct BiometricData {
  int heartRate = 0;
  String activity = "";
  int stepCount = 0;
  float acceleration = 0;
  unsigned long lastMovement = 0;
  unsigned long lastUpdate = 0;
  bool dataAvailable = false;
};


// Instances of 3 objects above
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



void handleRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }
  
  String cardId = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    cardId += String(rfid.uid.uidByte[i], HEX);
  }
  
  Serial.println("Card detected: " + cardId);

   // Simple authentication - add your known card IDs here
  if (cardId == "9c13c3") { // Temporary: accept any card->  || cardId.length() > 0
    if (!sessionActive) {
      startSession(cardId);
    } else {
      // Double-tap RFID during break = snooze
      if (pomodoro.currentState == SHORT_BREAK || pomodoro.currentState == LONG_BREAK) {
        snoozeBreak();
      } else {
        endSession();
      }
    }
  } else {
    // Authentication failed
    playAuthFailSound();
    Serial.println("Authentication failed for card: " + cardId);
  }
  
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void startSession(String userId) {
  sessionActive = true;
  currentUser = userId;
  sessionStart = millis();
  
  digitalWrite(LED_PIN, HIGH);
  
  // Play success sound
  playSessionStartSound();
  
  // Initialize Pomodoro timer
  initializePomodoro();
  
  // Publish session state to MQTT
  publishSessionState();
  
  Serial.println("Session started for: " + userId);
}


void endSession() {
  sessionActive = false;
  String lastUser = currentUser;
  currentUser = "";
  
  digitalWrite(LED_PIN, LOW);
  
  // Play session complete sound
  playSessionCompleteSound();
  
  // Reset Pomodoro timer
  pomodoro.currentState = IDLE;
  
  // Publish final session state
  publishSessionState();
  publishPomodoroState();
  
  showWelcomeScreen();
  
  Serial.printf("Session ended. Completed %d Pomodoro cycles.\n", pomodoro.completedCycles);
}

void updateDisplay() {
  if (sessionActive) {
    // Show session info with environmental and biometric data
    unsigned long elapsed = (millis() - sessionStart) / 1000;
    int minutes = elapsed / 60;
    int seconds = elapsed % 60;
    
    static unsigned long lastDisplaySwitch = 0;
    static int displayMode = 0;
    
    // Switch between different data views every 3 seconds
    if (millis() - lastDisplaySwitch > 3000) {
      displayMode = (displayMode + 1) % 5; // Now 5 modes including MQTT status
      lastDisplaySwitch = millis();
    }
    
    lcd.clear();
    
    switch (displayMode) {
      case 0:
        // Pomodoro timer info
        lcd.setCursor(0, 0);
        switch (pomodoro.currentState) {
          case WORK_SESSION: lcd.print("WORK"); break;
          case SHORT_BREAK: lcd.print("BREAK"); break;
          case LONG_BREAK: lcd.print("LONG BREAK"); break;
          default: lcd.print("IDLE"); break;
        }
        lcd.setCursor(0, 1);
        lcd.print("Time: " + getTimeRemainingText());
        if (pomodoro.breakSnoozed && pomodoro.snoozeCount > 0) {
          lcd.setCursor(12, 1);
          lcd.printf("S%d", pomodoro.snoozeCount);
        }
        break;
        
      case 1: {
        // Session and cycle info
        lcd.setCursor(0, 0);
        lcd.printf("Cycles: %d", pomodoro.completedCycles);
        lcd.setCursor(0, 1);
        lcd.printf("Total: %dm", minutes);
        break;
      }
        
      case 2:
        // Environmental data
        if (envData.dataAvailable) {
          lcd.setCursor(0, 0);
          lcd.printf("T:%.1fC H:%.0f%%", envData.temperature, envData.humidity);
          lcd.setCursor(0, 1);
          lcd.printf("L:%d N:%d", envData.lightLevel, envData.noiseLevel);
        } else {
          lcd.setCursor(0, 0);
          lcd.print("Environment");
          lcd.setCursor(0, 1);
          lcd.print("No Data");
        }
        break;
        
      case 3:
        // Biometric data
        if (bioData.dataAvailable) {
          lcd.setCursor(0, 0);
          lcd.printf("HR:%d Steps:%d", bioData.heartRate, bioData.stepCount);
          lcd.setCursor(0, 1);
          lcd.print("Act: " + bioData.activity.substring(0, 10));
        } else {
          lcd.setCursor(0, 0);
          lcd.print("Biometric");
          lcd.setCursor(0, 1);
          lcd.print("No Data");
        }
        break;
        
      case 4:
        // MQTT and system status
        lcd.setCursor(0, 0);
        lcd.print("MQTT: ");
        lcd.print(mqttClient.connected() ? "OK" : "ERR");
        lcd.setCursor(0, 1);
        lcd.print("WiFi: ");
        lcd.print(WiFi.status() == WL_CONNECTED ? "OK" : "ERR");
        break;
    }
  }
}





void analyzeEnvironment() {
  // Simple environmental analysis
  bool needsAlert = false;
  String alertMsg = "";
  
  // Check temperature (optimal: 20-24°C)
  if (envData.temperature < 20) {
    alertMsg = "Too Cold!";
    needsAlert = true;
  } else if (envData.temperature > 26) {
    alertMsg = "Too Hot!";
    needsAlert = true;
  }
  
  // Check noise level
  if (envData.noiseLevel > 50) {
    alertMsg = "Too Noisy!";
    needsAlert = true;
  }
  
  // Check light level (very basic check)
  if (envData.lightLevel < 50) {
    alertMsg = "Too Dark!";
    needsAlert = true;
  }
  
  if (needsAlert && sessionActive) {
    // Flash LED for alert
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_PIN, LOW);
      delay(100);
      digitalWrite(LED_PIN, HIGH);
      delay(100);
    }
    
    Serial.println("Environment Alert: " + alertMsg);
  }
}

void analyzeBiometrics() {
  if (!bioData.dataAvailable) return;
  
  // Check for extended sitting
  unsigned long timeSinceMovement = millis() - bioData.lastMovement;
  
  if (timeSinceMovement > 25 * 60 * 1000 && sessionActive) { // 25 minutes
    // Flash LED for movement reminder
    for (int i = 0; i < 5; i++) {
      digitalWrite(LED_PIN, LOW);
      delay(100);
      digitalWrite(LED_PIN, HIGH);
      delay(100);
    }
    
    // Send MQTT movement reminder
    publishMovementReminder();
    
    Serial.println("Biometric Alert: Time to move!");
  }
  
  // Check heart rate
  if (bioData.heartRate > 100 && sessionActive) {
    Serial.println("Biometric Alert: High heart rate - take a break");
    
    // Publish health alert
    StaticJsonDocument<200> alertDoc;
    alertDoc["alert"] = "High heart rate detected";
    alertDoc["heartRate"] = bioData.heartRate;
    alertDoc["level"] = "warning";
    alertDoc["timestamp"] = millis();
    
    String alertString;
    serializeJson(alertDoc, alertString);
    mqttClient.publish("bille/alerts/health", alertString.c_str());
  }
}

 // Show welcome screen
  void showWelcomeScreen() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Bill-E Focus Bot");
    lcd.setCursor(0, 1);
    lcd.print("Scan RFID card");
  }

void playSessionStartSound() {
  // Cheerful ascending tone for successful login
  int melody[] = {523, 659, 784, 1047}; // C5, E5, G5, C6
  int durations[] = {200, 200, 200, 400};
  
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, melody[i], durations[i]);
    delay(durations[i] + 50);
  }
  noTone(BUZZER_PIN);
}

void playAuthFailSound() {
  // Low descending tone for failed authentication
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 200, 300);
    delay(400);
  }
  noTone(BUZZER_PIN);
}

void playWorkSessionStartSound() {
  // Single confident tone
  tone(BUZZER_PIN, 880, 500); // A5
  delay(600);
  noTone(BUZZER_PIN);
}

void playBreakStartSound() {
  // Gentle double tone
  tone(BUZZER_PIN, 659, 300); // E5
  delay(400);
  tone(BUZZER_PIN, 523, 300); // C5
  delay(400);
  noTone(BUZZER_PIN);
}

void playLongBreakStartSound() {
  // Celebration melody
  int melody[] = {523, 659, 784, 659, 523}; // C5, E5, G5, E5, C5
  for (int i = 0; i < 5; i++) {
    tone(BUZZER_PIN, melody[i], 200);
    delay(250);
  }
  noTone(BUZZER_PIN);
}

void playSessionCompleteSound() {
  // Achievement sound
  int melody[] = {784, 880, 988, 1047}; // G5, A5, B5, C6
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, melody[i], 300);
    delay(350);
  }
  noTone(BUZZER_PIN);
}

void initializePomodoro() {
  pomodoro.currentState = WORK_SESSION;
  pomodoro.stateStartTime = millis();
  pomodoro.stateDuration = pomodoro.workDuration * 60 * 1000UL; // Convert to milliseconds
  pomodoro.completedCycles = 0;
  pomodoro.breakSnoozed = false;
  pomodoro.snoozeCount = 0;
  pomodoro.breakComplianceChecked = false;
  
  playWorkSessionStartSound();
  publishPomodoroState();
  
  Serial.println("Pomodoro work session started!");
}

void updatePomodoroTimer() {
  if (!sessionActive || pomodoro.currentState == IDLE) return;
  
  unsigned long elapsed = millis() - pomodoro.stateStartTime;
  
  // Check if current state duration is complete
  if (elapsed >= pomodoro.stateDuration) {
    transitionToNextState();
  }
  
  // Check break compliance during breaks
  if ((pomodoro.currentState == SHORT_BREAK || pomodoro.currentState == LONG_BREAK) 
      && !pomodoro.breakComplianceChecked && elapsed > 60000) { // Check after 1 minute
    checkBreakCompliance();
  }
}

void transitionToNextState() {
  switch (pomodoro.currentState) {
    case WORK_SESSION:
      pomodoro.completedCycles++;
      
      // Long break every 4 cycles, otherwise short break
      if (pomodoro.completedCycles % 4 == 0) {
        pomodoro.currentState = LONG_BREAK;
        pomodoro.stateDuration = pomodoro.longBreakDuration * 60 * 1000UL;
        playLongBreakStartSound();
        Serial.printf("Long break started! Cycle %d completed.\n", pomodoro.completedCycles);
      } else {
        pomodoro.currentState = SHORT_BREAK;
        pomodoro.stateDuration = pomodoro.shortBreakDuration * 60 * 1000UL;
        playBreakStartSound();
        Serial.printf("Short break started! Cycle %d completed.\n", pomodoro.completedCycles);
      }
      break;
      
    case SHORT_BREAK:
    case LONG_BREAK:
      pomodoro.currentState = WORK_SESSION;
      pomodoro.stateDuration = pomodoro.workDuration * 60 * 1000UL;
      playWorkSessionStartSound();
      Serial.println("Work session started!");
      break;
      
    default:
      break;
  }
  
  pomodoro.stateStartTime = millis();
  pomodoro.breakSnoozed = false;
  pomodoro.snoozeCount = 0;
  pomodoro.breakComplianceChecked = false;
  
  publishPomodoroState();
}

void snoozeBreak() {
  if (pomodoro.currentState == SHORT_BREAK || pomodoro.currentState == LONG_BREAK) {
    pomodoro.stateDuration += 5 * 60 * 1000UL; // Add 5 minutes
    pomodoro.snoozeCount++;
    pomodoro.breakSnoozed = true;
    
    // Brief acknowledgment sound
    tone(BUZZER_PIN, 440, 200);
    delay(300);
    noTone(BUZZER_PIN);
    
    Serial.printf("Break snoozed for 5 minutes. Snooze count: %d\n", pomodoro.snoozeCount);
    publishPomodoroState();
  }
}


void checkBreakCompliance() {
  if (!bioData.dataAvailable) return;
  
  unsigned long timeSinceMovement = millis() - bioData.lastMovement;
  
  if (timeSinceMovement < 30000) { // Moved within last 30 seconds
    Serial.println("Break compliance: User is moving - good!");
  } else {
    Serial.println("Break compliance: Consider moving around!");
    // Send gentle reminder to mesh
    StaticJsonDocument<150> reminderMsg;
    reminderMsg["type"] = "MOVEMENT_REMINDER";
    reminderMsg["message"] = "Time to move around!";
    
    String message;
    serializeJson(reminderMsg, message);
  }
  
  pomodoro.breakComplianceChecked = true;
}



String getTimeRemainingText() {
  if (pomodoro.currentState == IDLE) return "00:00";
  
  unsigned long elapsed = millis() - pomodoro.stateStartTime;
  unsigned long remaining = (pomodoro.stateDuration > elapsed) ? 
                            (pomodoro.stateDuration - elapsed) / 1000 : 0;
  
  int minutes = remaining / 60;
  int seconds = remaining % 60;
  
  return String(minutes) + ":" + (seconds < 10 ? "0" : "") + String(seconds);
}

unsigned long getTimeRemainingSeconds() {
  if (pomodoro.currentState == IDLE) return 0;
  
  unsigned long elapsed = millis() - pomodoro.stateStartTime;
  unsigned long remaining = (pomodoro.stateDuration > elapsed) ? 
                            (pomodoro.stateDuration - elapsed) / 1000 : 0;
  return remaining;
}

void publishMQTTMessage(const char* topic, String payload) {
  if (mqttConnected && mqttClient.connected()) {
    mqttClient.publish(topic, payload.c_str());
    Serial.println("Published to " + String(topic) + ": " + payload);
  }
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
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Create a random client ID
    String clientId = "BillE-MainBrain-";
    clientId += String(random(0xffff), HEX);
    
    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      
      // Subscribe to data topics from other nodes
      mqttClient.subscribe("bille/data/environment");
      mqttClient.subscribe("bille/data/biometric");
      mqttClient.subscribe("bille/commands/session");
      mqttClient.subscribe("bille/commands/pomodoro");
      
      // Announce presence as main coordinator
      mqttClient.publish("bille/status/mainbrain", "online", true);
      
      // Request initial data from all nodes
      mqttClient.publish("bille/environment/request", "data");
      mqttClient.publish("bille/wearable/request", "data");
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
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
  
  StaticJsonDocument<400> doc;
  deserializeJson(doc, message);
  
  // Handle environmental data updates
  if (String(topic) == "bille/data/environment") {
    envData.temperature = doc["temperature"];
    envData.humidity = doc["humidity"];
    envData.lightLevel = doc["lightLevel"];
    envData.noiseLevel = doc["noiseLevel"];
    envData.soundDetected = doc["soundDetected"];
    envData.lastUpdate = millis();
    envData.dataAvailable = true;
    
    Serial.println("Environmental data updated via MQTT");
    analyzeEnvironment();
  }
  
  // Handle biometric data updates
  else if (String(topic) == "bille/data/biometric") {
    bioData.heartRate = doc["heartRate"];
    bioData.activity = doc["activity"].as<String>();
    bioData.stepCount = doc["stepCount"];
    bioData.acceleration = doc["acceleration"];
    bioData.lastMovement = doc["lastMovement"];
    bioData.lastUpdate = millis();
    bioData.dataAvailable = true;
    
    Serial.println("Biometric data updated via MQTT");
    analyzeBiometrics();
  }
  
  // Handle remote session commands (for web dashboard control)
  else if (String(topic) == "bille/commands/session") {
    String command = doc["command"];
    if (command == "start" && !sessionActive) {
      String userId = doc["userId"].as<String>();
      startSession(userId);
    } else if (command == "stop" && sessionActive) {
      endSession();
    }
  }
  
  // Handle remote Pomodoro commands
  else if (String(topic) == "bille/commands/pomodoro") {
    String command = doc["command"];
    if (command == "snooze" && sessionActive) {
      snoozeBreak();
    } else if (command == "skip" && sessionActive) {
      transitionToNextState();
    }
  }
}

void publishSessionState() {
  StaticJsonDocument<200> doc;
  doc["active"] = sessionActive;
  doc["userId"] = currentUser;
  doc["timestamp"] = millis();
  
  if (sessionActive) {
    unsigned long elapsed = (millis() - sessionStart) / 1000;
    doc["sessionDuration"] = elapsed;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  mqttClient.publish("bille/session/state", jsonString.c_str());
  mqttClient.publish("bille/session/active", sessionActive ? "true" : "false");
  
  Serial.println("Session state published to MQTT");
}

void publishPomodoroState() {
  StaticJsonDocument<300> doc;
  doc["state"] = pomodoro.currentState;
  doc["timeRemaining"] = (pomodoro.stateDuration - (millis() - pomodoro.stateStartTime)) / 1000;
  doc["completedCycles"] = pomodoro.completedCycles;
  doc["snoozed"] = pomodoro.breakSnoozed;
  doc["snoozeCount"] = pomodoro.snoozeCount;
  doc["timestamp"] = millis();
  
  String stateText;
  switch (pomodoro.currentState) {
    case WORK_SESSION: stateText = "WORK"; break;
    case SHORT_BREAK: stateText = "SHORT_BREAK"; break;
    case LONG_BREAK: stateText = "LONG_BREAK"; break;
    default: stateText = "IDLE"; break;
  }
  doc["stateText"] = stateText;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Publish to multiple topics for HA sensors
  mqttClient.publish("bille/pomodoro/state", jsonString.c_str());
  mqttClient.publish("bille/pomodoro/cycles", String(pomodoro.completedCycles).c_str());
  mqttClient.publish("bille/pomodoro/time_remaining", String(doc["timeRemaining"].as<long>()).c_str());
  mqttClient.publish("bille/pomodoro/current_state", stateText.c_str());
  
  Serial.println("Pomodoro state published: " + stateText);
}

void publishSystemStatus() {
  StaticJsonDocument<300> doc;
  doc["nodeType"] = "MAIN_BRAIN";
  doc["timestamp"] = millis();
  doc["sessionActive"] = sessionActive;
  doc["currentUser"] = currentUser;
  doc["wifiConnected"] = WiFi.status() == WL_CONNECTED;
  doc["mqttConnected"] = mqttClient.connected();
  doc["environmentDataAge"] = envData.dataAvailable ? (millis() - envData.lastUpdate) / 1000 : -1;
  doc["biometricDataAge"] = bioData.dataAvailable ? (millis() - bioData.lastUpdate) / 1000 : -1;
  
  if (sessionActive) {
    doc["pomodoroState"] = pomodoro.currentState;
    doc["pomodoroTimeRemaining"] = (pomodoro.stateDuration - (millis() - pomodoro.stateStartTime)) / 1000;
    doc["completedCycles"] = pomodoro.completedCycles;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  mqttClient.publish("bille/status/system", jsonString.c_str());
  
  Serial.println("System status published to MQTT");
}

void publishMovementReminder() {
  StaticJsonDocument<150> doc;
  doc["message"] = "Time to move around!";
  doc["timestamp"] = millis();
  doc["reason"] = "extended_sitting";
  
  String jsonString;
  serializeJson(doc, jsonString);
  mqttClient.publish("bille/alerts/movement", jsonString.c_str());
  
  Serial.println("Movement reminder sent via MQTT");
}


void connectMQTT() {
  while (!mqttClient.connected() && WiFi.status() == WL_CONNECTED) {
    Serial.print("Attempting MQTT connection...");
    
    String clientId = "BillE-MainBrain-";
    clientId += String(random(0xffff), HEX);
    
    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      mqttConnected = true;
      
      // Subscribe to command topics
      mqttClient.subscribe("bille/commands/snooze");
      mqttClient.subscribe("bille/commands/end_session");
      
      // Announce connection
      publishMQTTMessage("bille/status", "Main Brain Connected");
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void publishSensorData() {
  if (!mqttConnected || !mqttClient.connected()) return;
  
  // Create comprehensive sensor data JSON
  StaticJsonDocument<600> doc;
  doc["timestamp"] = millis();
  
  
  // Environmental data
  if (envData.dataAvailable) {
    doc["temperature"] = envData.temperature;
    doc["humidity"] = envData.humidity;
    doc["light_level"] = envData.lightLevel;
    doc["noise_level"] = envData.noiseLevel;
    doc["sound_detected"] = envData.soundDetected;
    
    // Publish individual topics for HA sensors
    publishMQTTMessage(TOPIC_TEMPERATURE, String(envData.temperature));
    publishMQTTMessage(TOPIC_HUMIDITY, String(envData.humidity));
    publishMQTTMessage(TOPIC_LIGHT, String(envData.lightLevel));
    publishMQTTMessage(TOPIC_NOISE, String(envData.noiseLevel));
  }
  
  // Biometric data
  if (bioData.dataAvailable) {
    doc["heart_rate"] = bioData.heartRate;
    doc["step_count"] = bioData.stepCount;
    doc["activity"] = bioData.activity;
    doc["acceleration"] = bioData.acceleration;
    doc["last_movement"] = bioData.lastMovement;
    
    // Publish individual topics
    publishMQTTMessage(TOPIC_HEARTRATE, String(bioData.heartRate));
    publishMQTTMessage(TOPIC_STEPS, String(bioData.stepCount));
    publishMQTTMessage(TOPIC_ACTIVITY, bioData.activity);
  }
  
  // Pomodoro data
  doc["session_active"] = sessionActive;
  if (sessionActive) {
    doc["current_user"] = currentUser;
    doc["pomodoro_state"] = pomodoro.currentState;
    doc["pomodoro_cycles"] = pomodoro.completedCycles;
    doc["time_remaining"] = getTimeRemainingSeconds();
    
    // Publish Pomodoro topics
    publishMQTTMessage(TOPIC_SESSION_ACTIVE, sessionActive ? "true" : "false");
    publishMQTTMessage(TOPIC_SESSION_USER, currentUser);
    publishMQTTMessage(TOPIC_POMODORO_STATE, String(pomodoro.currentState));
    publishMQTTMessage(TOPIC_POMODORO_CYCLES, String(pomodoro.completedCycles));
    publishMQTTMessage(TOPIC_POMODORO_TIME, String(getTimeRemainingSeconds()));
  }
  
  // Publish complete data as JSON
  String jsonString;
  serializeJson(doc, jsonString);
  publishMQTTMessage("bille/data/complete", jsonString);
  
  Serial.println("Sensor data published to MQTT");
}
