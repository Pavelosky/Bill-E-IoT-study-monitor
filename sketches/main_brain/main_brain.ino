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

// Objects
MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
painlessMesh mesh;
Scheduler userScheduler;

// Session state
bool sessionActive = false;
String currentUser = "";
unsigned long sessionStart = 0;

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
  
  if (!sessionActive) {
    startSession(cardId);
  } else {
    endSession();
  }
  
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void startSession(String userId) {
  sessionActive = true;
  currentUser = userId;
  sessionStart = millis();
  
  digitalWrite(LED_PIN, HIGH);
  
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
    // Show session info with environmental and biometric data
    unsigned long elapsed = (millis() - sessionStart) / 1000;
    int minutes = elapsed / 60;
    int seconds = elapsed % 60;
    
    static unsigned long lastDisplaySwitch = 0;
    static int displayMode = 0;
    
    // Switch between different data views every 3 seconds
    if (millis() - lastDisplaySwitch > 3000) {
      displayMode = (displayMode + 1) % 3;
      lastDisplaySwitch = millis();
    }
    
    lcd.clear();
    
    switch (displayMode) {
      case 0:
        // Session info
        lcd.setCursor(0, 0);
        lcd.print("Session Active");
        lcd.setCursor(0, 1);
        lcd.printf("Time: %02d:%02d", minutes, seconds);
        break;
        
      case 1:
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
        
      case 2:
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
