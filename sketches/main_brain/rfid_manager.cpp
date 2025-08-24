#include "rfid_manager.h"
#include "config.h"
#include "data_structures.h"
#include "pomodoro_timer.h"
#include "audio_system.h"
#include "mqtt_handler.h"
#include "display_manager.h"
#include <MFRC522.h>
#include <Arduino.h>

extern MFRC522 rfid;

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