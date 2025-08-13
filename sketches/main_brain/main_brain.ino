// ESP8266-1 Main Brain - Working Version
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Wire.h>

// Pin definitions
const int RST_PIN = D1;
const int SS_PIN = D2;
const int LED_PIN = D0;

// Objects
MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Use 5V for LCD

// Session state
bool sessionActive = false;
String currentUser = "";
unsigned long sessionStart = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Wall-E Main Brain Starting...");
  
  // Initialize pins
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize I2C for LCD (with 5V)
  Wire.begin(D3, D4); // SDA=D3, SCL=D4
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  
  // Initialize SPI and RFID
  SPI.begin();
  rfid.PCD_Init();
  
  // Test RFID connection
  byte version = rfid.PCD_ReadRegister(rfid.VersionReg);
  Serial.printf("RFID Version: 0x%02X\n", version);
  
  if (version == 0x00 || version == 0xFF) {
    Serial.println("ERROR: RFID not working");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RFID ERROR");
    while(1); // Stop here if RFID fails
  }
  
  // Show welcome screen
  showWelcomeScreen();
  
  Serial.println("Main Brain Ready!");
}

void loop() {
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
  
  // Read card ID
  String cardId = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    cardId += String(rfid.uid.uidByte[i], HEX);
  }
  
  Serial.println("Card detected: " + cardId);
  
  // Toggle session
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
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Session Active");
  lcd.setCursor(0, 1);
  lcd.print("User: " + userId.substring(0, 8));
  
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
    unsigned long elapsed = (millis() - sessionStart) / 1000;
    int minutes = elapsed / 60;
    int seconds = elapsed % 60;
    
    lcd.setCursor(0, 1);
    lcd.printf("Time: %02d:%02d    ", minutes, seconds);
  }
}
