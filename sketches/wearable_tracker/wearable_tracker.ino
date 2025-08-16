// ESP8266-3 Wearable Tracker with Mesh Network
#include <U8g2lib.h>
#include <Wire.h>
#include <MPU6050.h>
#include "painlessMesh.h"
#include <ArduinoJson.h>

// Mesh network config
// Mesh network config
#define MESH_PREFIX     "BillE_Focus_Mesh"
#define MESH_PASSWORD   "FocusBot2025"
#define MESH_PORT       5555

// Pin definitions
#define OLED_SDA        D2
#define OLED_SCL        D1
#define HEART_SENSOR    A0
#define HEART_LED       D5

// Objects
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
MPU6050 mpu;
painlessMesh mesh;
Scheduler userScheduler;

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

// Add motivational messages arrays
const char* workMotivation[] = {
  "Stay Focused!",
  "You Got This!",
  "Deep Work Mode",
  "Focus Time!",
  "Make It Count!",
  "Zone In!",
  "Productive!",
  "Laser Focus!"
};

const char* breakMotivation[] = {
  "Time to Move!",
  "Stretch Break",
  "Walk Around",
  "Rest & Recharge",
  "Take a Breath",
  "Move Your Body",
  "Step Away",
  "Refresh Time"
};

const char* achievementMessages[] = {
  "Cycle Complete!",
  "Well Done!",
  "Great Progress!",
  "Keep Going!",
  "Awesome Work!",
  "You're Crushing It!",
  "Fantastic!"
};

int currentMotivationIndex = 0;

// Activity detection variables
float lastAccelMagnitude = 0;
int stepCount = 0;
unsigned long lastStepTime = 0;

// Session state
bool sessionActive = false;
String currentUser = "";

// Function prototypes
void sendBiometricData();
Task taskSendData(TASK_SECOND * 5, TASK_FOREVER, &sendBiometricData);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Bill-E Wearable Tracker with Mesh Starting...");
  
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
  
  // Test connection
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful");
  } else {
    Serial.println("MPU6050 connection failed");
    showError("MPU6050 Error");
    while (1) delay(10);
  }
  
  // Initialize mesh network
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  
  // Add task to send data every 5 seconds
  userScheduler.addTask(taskSendData);
  taskSendData.enable();
  
  // Welcome screen
  showWelcomeScreen();
  
  Serial.println("Wearable Tracker with Mesh Ready!");
  Serial.printf("Node ID: %u\n", mesh.getNodeId());
  delay(2000);
}

void loop() {
  mesh.update();
  
  // Read sensors every second
  static unsigned long lastRead = 0;
  if (millis() - lastRead > 1000) {
    readBiometrics();
    lastRead = millis();
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

void sendBiometricData() {
  // Create JSON message with biometric data
  StaticJsonDocument<300> doc;
  doc["type"] = "BIOMETRIC_DATA";
  doc["nodeType"] = "WEARABLE";
  doc["nodeId"] = mesh.getNodeId();
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

  String message;
  serializeJson(doc, message);
  
  mesh.sendBroadcast(message);
  Serial.println("Biometric data sent to mesh");
}

void updateDisplay() {
  static unsigned long lastDisplayUpdate = 0;
  static int displayMode = 0;
  static unsigned long motivationChangeTime = 0;
  static unsigned long achievementShowTime = 0;
  static bool showingAchievement = false;

  // Check if we should show achievement message
  static int lastCompletedCycles = 0;
  if (pomodoroInfo.completedCycles > lastCompletedCycles && pomodoroInfo.dataAvailable) {
    showingAchievement = true;
    achievementShowTime = millis();
    lastCompletedCycles = pomodoroInfo.completedCycles;
  }
  
  // Show achievement for 3 seconds
  if (showingAchievement) {
    if (millis() - achievementShowTime < 3000) {
      showAchievement();
      return;
    } else {
      showingAchievement = false;
    }
  }
  
  // Change motivational messages every 30 seconds
  if (millis() - motivationChangeTime > 30000) {
    currentMotivationIndex++;
    motivationChangeTime = millis();
  }

// Main display rotation every 4 seconds
  if (millis() - lastDisplayUpdate > 4000) {
    // Adjust display mode count based on session state
    int maxModes = sessionActive ? 6 : 3; // More modes during active session
    displayMode = (displayMode + 1) % maxModes;
    lastDisplayUpdate = millis();
  }
  
  if (sessionActive && pomodoroInfo.dataAvailable) {
    // Enhanced display modes during active Pomodoro session
    switch (displayMode) {
      case 0:
        // Pomodoro countdown (primary display)
        showPomodoroCountdown();
        break;
        
      case 1:
        // Motivational message
        showMotivationalMessage();
        break;
        
      case 2:
        // Break compliance (only during breaks)
        if (pomodoroInfo.currentState == SHORT_BREAK || pomodoroInfo.currentState == LONG_BREAK) {
          showBreakCompliance();
        } else {
          // Fall back to biometric data during work
          showBiometricData();
        }
        break;
        
      case 3:
        // Heart rate and activity focus
        showBiometricData();
        break;
        
      case 4:
        // Step count and activity encouragement
        showActivityStatus();
        break;
        
      case 5:
        // Mesh and session info
        showSessionInfo();
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
        showSessionInfo();
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
    } else {
      display.print(getActivityEncouragement());
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
  display.print("Mesh Ready!");
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

void showSessionInfo() {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  
  display.setCursor(0, 15);
  if (sessionActive) {
    display.print("Session: ON");
    display.setCursor(0, 30);
    display.print("User: " + currentUser.substring(0, 8));
    if (pomodoroInfo.dataAvailable) {
      display.setCursor(0, 45);
      display.printf("Cycles: %d", pomodoroInfo.completedCycles);
    }
  } else {
    display.print("Session: OFF");
  }
  display.setCursor(0, 60);
  display.printf("Mesh: %d nodes", mesh.getNodeList().size());
  
  display.sendBuffer();
}

// Mesh callbacks
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
  
  StaticJsonDocument<300> doc;
  deserializeJson(doc, msg);
  
  String msgType = doc["type"];
  
  if (msgType == "SESSION_START") {
    sessionActive = true;
    currentUser = doc["userId"].as<String>();  // Fix: explicit conversion
    
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
    
  } else if (msgType == "SESSION_END") {
    sessionActive = false;
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
    
  } else if (msgType == "REQUEST_BIO_DATA") {
    // Send immediate biometric data
    sendBiometricData();
  } else if (msgType == "POMODORO_STATE") {
    // Update Pomodoro information
    pomodoroInfo.currentState = (PomodoroState)doc["state"].as<int>();
    pomodoroInfo.timeRemaining = doc["timeRemaining"];
    pomodoroInfo.completedCycles = doc["completedCycles"];
    pomodoroInfo.snoozed = doc["snoozed"];
    pomodoroInfo.snoozeCount = doc["snoozeCount"];
    pomodoroInfo.stateText = doc["stateText"].as<String>();
    pomodoroInfo.dataAvailable = true;
    pomodoroInfo.lastUpdate = millis();
    
    Serial.println("Pomodoro state updated: " + pomodoroInfo.stateText);
    Serial.printf("Time remaining: %lu seconds\n", pomodoroInfo.timeRemaining);
  
  } else if (msgType == "MOVEMENT_REMINDER") {
    // Show movement reminder on display
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
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection: %u\n", nodeId);
  
  // Send identification message
  StaticJsonDocument<150> doc;
  doc["type"] = "NODE_IDENTIFICATION";
  doc["nodeType"] = "WEARABLE";
  doc["capabilities"] = "heart_rate,activity,steps,acceleration";
  doc["version"] = "1.0";
  
  String message;
  serializeJson(doc, message);
  mesh.sendSingle(nodeId, message);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections. Nodes: %d\n", mesh.getNodeList().size());
}

void showPomodoroCountdown() {
  display.clearBuffer();
  display.setFont(u8g2_font_8x13_tf);
  
  // Show state and countdown
  display.setCursor(0, 15);
  display.print(pomodoroInfo.stateText);
  
  // Show countdown timer
  display.setFont(u8g2_font_10x20_tf);
  display.setCursor(0, 40);
  
  int minutes = pomodoroInfo.timeRemaining / 60;
  int seconds = pomodoroInfo.timeRemaining % 60;
  
  if (minutes < 100) {
    display.printf("%02d:%02d", minutes, seconds);
  } else {
    display.printf("%d:%02d", minutes, seconds);
  }
  
  // Show cycle count
  display.setFont(u8g2_font_6x10_tf);
  display.setCursor(0, 55);
  display.printf("Cycles: %d", pomodoroInfo.completedCycles);
  
  // Show snooze indicator
  if (pomodoroInfo.snoozed && pomodoroInfo.snoozeCount > 0) {
    display.setCursor(70, 55);
    display.printf("Snooze:%d", pomodoroInfo.snoozeCount);
  }
  
  // Add progress bar for visual appeal
  drawProgressBar();
  
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
  
  int elapsed = totalDuration - pomodoroInfo.timeRemaining;
  int progress = (elapsed * 120) / totalDuration; // 120 pixels wide
  
  // Draw progress bar frame
  display.drawFrame(4, 58, 120, 6);
  
  // Fill progress bar
  if (progress > 0) {
    display.drawBox(5, 59, progress, 4);
  }
}

void showMotivationalMessage() {
  display.clearBuffer();
  display.setFont(u8g2_font_8x13_tf);
  
  const char* message;
  switch (pomodoroInfo.currentState) {
    case WORK_SESSION:
      message = workMotivation[currentMotivationIndex % 8];
      break;
    case SHORT_BREAK:
    case LONG_BREAK:
      message = breakMotivation[currentMotivationIndex % 8];
      break;
    default:
      message = "Ready to Focus?";
      break;
  }
  
  // Center the message
  int textWidth = display.getStrWidth(message);
  int x = (128 - textWidth) / 2;
  
  display.setCursor(x, 35);
  display.print(message);
  
  // Add some visual flair
  if (pomodoroInfo.currentState == WORK_SESSION) {
    // Work session - show focused icon
    display.drawCircle(64, 15, 8);
    display.drawDisc(64, 15, 4);
  } else if (pomodoroInfo.currentState != IDLE) {
    // Break time - show movement icon
    display.drawTriangle(60, 10, 68, 10, 64, 20);
    display.drawTriangle(60, 20, 68, 20, 64, 10);
  }
  
  display.sendBuffer();
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

void showAchievement() {
  display.clearBuffer();
  display.setFont(u8g2_font_10x20_tf);
  
  const char* message = achievementMessages[pomodoroInfo.completedCycles % 7];
  
  // Center the message
  int textWidth = display.getStrWidth(message);
  int x = (128 - textWidth) / 2;
  
  display.setCursor(x, 30);
  display.print(message);
  
  // Show celebration stars
  for (int i = 0; i < 6; i++) {
    int starX = 20 + i * 20;
    int starY = 50;
    drawStar(starX, starY, 3);
  }
  
  display.sendBuffer();
}

void drawStar(int x, int y, int size) {
  display.drawLine(x, y - size, x, y + size);
  display.drawLine(x - size, y, x + size, y);
  display.drawLine(x - size/2, y - size/2, x + size/2, y + size/2);
  display.drawLine(x - size/2, y + size/2, x + size/2, y - size/2);
}

String getActivityEncouragement() {
  if (pomodoroInfo.currentState == SHORT_BREAK || pomodoroInfo.currentState == LONG_BREAK) {
    unsigned long timeSinceMovement = millis() - currentBio.lastMovement;
    
    if (timeSinceMovement < 10000) {
      return "Moving: Great!";
    } else if (timeSinceMovement < 30000) {
      return "Keep Moving";
    } else {
      return "Time to Move!";
    }
  }
  return ""; // No encouragement during work
}
