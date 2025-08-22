#include "rfid_handler.h"
#include "config.h"
#include "sound_effects.h"
#include "pomodoro.h"
#include <SPI.h>

// Global RFID object
MFRC522 rfid(SS_PIN, RST_PIN);

// External functions
void startSession(String userId);
void endSession();
void snoozeBreak();

extern bool sessionActive;

void initializeRFID() {
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
  if (cardId == AUTHORIZED_CARD) {
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