// ESP8266-1 Main Brain with Mesh Network - FIXED
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Wire.h>
#include "painlessMesh.h"
#include <ArduinoJson.h>

// Mesh network config
#define MESH_PREFIX     "BillE_Focus_Mesh"
#define MESH_PASSWORD   "FocusBot2025"
#define MESH_PORT       5555

// Pin definitions
const int RST_PIN = D1;
const int SS_PIN = D2;
const int LED_PIN = D0;
const int BUZZER_PIN = D8;

// Objects
MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
painlessMesh mesh;
Scheduler userScheduler;

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

EnvironmentData envData;

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

BiometricData bioData;

// Pomodoro session instance
PomodoroSession pomodoro;

// Connected nodes
uint32_t environmentNodeId = 0;
uint32_t wearableNodeId = 0;






void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Bill-E Main Brain with Mesh Starting...");
  
  // Initialize pins
  pinMode(LED_PIN, OUTPUT);
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
  
  // Initialize mesh network
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  
  // Show welcome screen
  showWelcomeScreen();
  
  Serial.println("Main Brain with Mesh Ready!");
  Serial.printf("Node ID: %u\n", mesh.getNodeId());
}

void loop() {
  mesh.update();
  handleRFID();
  updatePomodoroTimer();
  updateDisplay();
  delay(100);
}

void showWelcomeScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Bill-E Focus Bot");
  lcd.setCursor(0, 1);
  lcd.print("Scan RFID card");
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

  // Play sound for session start
  playSessionStartSound();
  
  // Initialize pomodoro timer
  initializePomodoro();

  // Notify all nodes about session start
  StaticJsonDocument<150> sessionMsg;
  sessionMsg["type"] = "SESSION_START";
  sessionMsg["userId"] = userId;
  sessionMsg["timestamp"] = millis();
  
  String message;
  serializeJson(sessionMsg, message);
  mesh.sendBroadcast(message);
  
  // Request fresh data from all nodes
  requestEnvironmentalData();
  requestBiometricData();
  
  Serial.println("Session started for: " + userId);
}

void endSession() {
  sessionActive = false;
  String lastUser = currentUser;
  currentUser = "";
  
  digitalWrite(LED_PIN, LOW);

  // Play sound for session end
  playSessionCompleteSound();

  // Stop pomodoro timer
  pomodoro.currentState = IDLE;
  
  // Notify all nodes about session end
  StaticJsonDocument<150> sessionMsg;
  sessionMsg["type"] = "SESSION_END";
  sessionMsg["userId"] = lastUser;
  sessionMsg["timestamp"] = millis();
  
  String message;
  serializeJson(sessionMsg, message);
  mesh.sendBroadcast(message);
  
  showWelcomeScreen();
  
  Serial.println("Session ended");
}

void updateDisplay() {
  if (sessionActive) {
    static unsigned long lastDisplaySwitch = 0;
    static int displayMode = 0;
    
    // Switch between different data views every 3 seconds
    if (millis() - lastDisplaySwitch > 3000) {
      displayMode = (displayMode + 1) % 4; // Now 4 modes instead of 3
      lastDisplaySwitch = millis();
    }
    
    lcd.clear();
    
    switch (displayMode) {
      case 0:
        // Pomodoro timer info
        lcd.setCursor(0, 0);
        lcd.print(getPomodoroStateText());
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
        unsigned long elapsed = (millis() - sessionStart) / 1000;
        int minutes = elapsed / 60;
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
    }
  }
}

void requestEnvironmentalData() {
  if (environmentNodeId != 0) {
    StaticJsonDocument<100> doc;
    doc["type"] = "REQUEST_ENV_DATA";
    doc["from"] = mesh.getNodeId();
    
    String message;
    serializeJson(doc, message);
    
    mesh.sendSingle(environmentNodeId, message);
    Serial.println("Requested environmental data");
  }
}

void requestBiometricData() {
  if (wearableNodeId != 0) {
    StaticJsonDocument<100> doc;
    doc["type"] = "REQUEST_BIO_DATA";
    doc["from"] = mesh.getNodeId();
    
    String message;
    serializeJson(doc, message);
    
    mesh.sendSingle(wearableNodeId, message);
    Serial.println("Requested biometric data");
  }
}

// Mesh callbacks
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
  
  StaticJsonDocument<400> doc;
  deserializeJson(doc, msg);
  
  String msgType = doc["type"];
  
  if (msgType == "ENVIRONMENTAL_DATA") {
    // Update environmental data
    envData.temperature = doc["temperature"];
    envData.humidity = doc["humidity"];
    envData.lightLevel = doc["lightLevel"];
    envData.noiseLevel = doc["noiseLevel"];
    envData.soundDetected = doc["soundDetected"];
    envData.lastUpdate = millis();
    envData.dataAvailable = true;
    
    Serial.println("Environmental data updated");
    
    // Analyze environment and provide feedback
    analyzeEnvironment();
    
  } else if (msgType == "BIOMETRIC_DATA") {
    // Update biometric data
    bioData.heartRate = doc["heartRate"];
    bioData.activity = doc["activity"].as<String>();  // Fix: explicit conversion
    bioData.stepCount = doc["stepCount"];
    bioData.acceleration = doc["acceleration"];
    bioData.lastMovement = doc["lastMovement"];
    bioData.lastUpdate = millis();
    bioData.dataAvailable = true;
    
    Serial.println("Biometric data updated");
    
    // Analyze biometric data
    analyzeBiometrics();
    
  } else if (msgType == "NODE_IDENTIFICATION") {
    String nodeType = doc["nodeType"];
    if (nodeType == "ENVIRONMENT") {
      environmentNodeId = from;
      Serial.printf("Environment node identified: %u\n", from);
    } else if (nodeType == "WEARABLE") {
      wearableNodeId = from;
      Serial.printf("Wearable node identified: %u\n", from);
    }
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection: %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections. Nodes: %d\n", mesh.getNodeList().size());
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
    Serial.println("Biometric Alert: Time to move!");
  }
  
  // Check heart rate
  if (bioData.heartRate > 100 && sessionActive) {
    Serial.println("Biometric Alert: High heart rate - take a break");
  }
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
  broadcastPomodoroState();
  
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
  
  broadcastPomodoroState();
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
    broadcastPomodoroState();
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
    mesh.sendBroadcast(message);
  }
  
  pomodoro.breakComplianceChecked = true;
}

void broadcastPomodoroState() {
  StaticJsonDocument<300> doc;
  doc["type"] = "POMODORO_STATE";
  doc["state"] = pomodoro.currentState;
  doc["timeRemaining"] = (pomodoro.stateDuration - (millis() - pomodoro.stateStartTime)) / 1000;
  doc["completedCycles"] = pomodoro.completedCycles;
  doc["snoozed"] = pomodoro.breakSnoozed;
  doc["snoozeCount"] = pomodoro.snoozeCount;
  
  String stateText;
  switch (pomodoro.currentState) {
    case WORK_SESSION: stateText = "WORK"; break;
    case SHORT_BREAK: stateText = "SHORT_BREAK"; break;
    case LONG_BREAK: stateText = "LONG_BREAK"; break;
    default: stateText = "IDLE"; break;
  }
  doc["stateText"] = stateText;
  
  String message;
  serializeJson(doc, message);
  mesh.sendBroadcast(message);
  
  Serial.println("Pomodoro state broadcasted: " + stateText);
}

String getPomodoroStateText() {
  switch (pomodoro.currentState) {
    case WORK_SESSION: return "WORK";
    case SHORT_BREAK: return "BREAK";
    case LONG_BREAK: return "LONG BREAK";
    default: return "IDLE";
  }
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
