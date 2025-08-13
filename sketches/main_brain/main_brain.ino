// ESP8266-1 Main Brain with Mesh Network
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

// Connected nodes
uint32_t environmentNodeId = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Wall-E Main Brain with Mesh Starting...");
  
  // Initialize pins
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize I2C for LCD
  Wire.begin(D3, D4);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  
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
  lcd.print("Wall-E Focus Bot");
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
  
  // Request environmental data immediately
  requestEnvironmentalData();
  
  Serial.println("Session started for: " + userId);
}

void endSession() {
  sessionActive = false;
  currentUser = "";
  
  digitalWrite(LED_PIN, LOW);
  showWelcomeScreen();
  
  Serial.println("Session ended");
}

void updateDisplay() {
  if (sessionActive) {
    // Show session info with environmental data
    unsigned long elapsed = (millis() - sessionStart) / 1000;
    int minutes = elapsed / 60;
    int seconds = elapsed % 60;
    
    static unsigned long lastDisplaySwitch = 0;
    static bool showEnvData = false;
    
    // Switch between session info and environmental data every 3 seconds
    if (millis() - lastDisplaySwitch > 3000) {
      showEnvData = !showEnvData;
      lastDisplaySwitch = millis();
    }
    
    lcd.clear();
    
    if (showEnvData && envData.dataAvailable) {
      // Show environmental data
      lcd.setCursor(0, 0);
      lcd.printf("T:%.1fC H:%.0f%%", envData.temperature, envData.humidity);
      lcd.setCursor(0, 1);
      lcd.printf("L:%d N:%d", envData.lightLevel, envData.noiseLevel);
    } else {
      // Show session info
      lcd.setCursor(0, 0);
      lcd.print("Session Active");
      lcd.setCursor(0, 1);
      lcd.printf("Time: %02d:%02d", minutes, seconds);
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
    
  } else if (msgType == "NODE_IDENTIFICATION") {
    String nodeType = doc["nodeType"];
    if (nodeType == "ENVIRONMENT") {
      environmentNodeId = from;
      Serial.printf("Environment node identified: %u\n", from);
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
