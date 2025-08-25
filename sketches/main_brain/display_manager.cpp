#include "display_manager.h"
#include "data_structures.h"
#include "pomodoro_timer.h"
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

extern LiquidCrystal_I2C lcd;
extern PubSubClient mqttClient;

// Motivational phrases for different cycle counts
const String motivationalPhrases[] = {
  "Let's Focus!",           // 0 cycles
  "Great Start!",           // 1 cycle
  "Keep Going!",            // 2 cycles
  "You're on Fire!",        // 3 cycles
  "Well Done!",             // 4 cycles
  "Fantastic!",             // 5 cycles
  "Unstoppable!",           // 6 cycles
  "Amazing Work!",          // 7 cycles
  "You're a Pro!",          // 8+ cycles
};

const int maxPhrases = 9;

// Environmental alert thresholds
const int NOISE_THRESHOLD = 4;
const int LIGHT_THRESHOLD = 270;


void updateDisplay() {
  static bool lastSessionState = false;
  
  // Check if session state changed
  if (sessionActive != lastSessionState) {
    lcd.clear();
    lastSessionState = sessionActive;
  }
  
  if (sessionActive) {
    static unsigned long lastDisplaySwitch = 0;
    static int displayMode = 0;
    
    // Check if we have environmental alerts
    bool alertActive = hasEnvironmentalAlert();
    int maxModes = alertActive ? 3 : 2;
    
    // Switch between different data views every 4 seconds
    if (millis() - lastDisplaySwitch > 4000) {
      displayMode = (displayMode + 1) % maxModes;
      lastDisplaySwitch = millis();
      lcd.clear();
    }

    switch (displayMode) {
      case 0:
        // Pomodoro timer info
        lcd.setCursor(0, 0);
        if (pomodoro.awaitingConfirmation) {
          lcd.print("TOUCH TO CONTINUE");
          lcd.setCursor(0, 1);
          lcd.print("Timer Complete!");
        } else {
          const char* stateText;
          switch (pomodoro.currentState) {
            case WORK_SESSION: stateText = "WORK"; break;
            case SHORT_BREAK: stateText = "BREAK"; break;
            case LONG_BREAK: stateText = "LONG BREAK"; break;
            default: stateText = "IDLE"; break;
          }
          lcd.print(stateText);
          
          lcd.setCursor(0, 1);
          lcd.print("Time: " + getTimeRemainingText());
      
          if (pomodoro.breakSnoozed && pomodoro.snoozeCount > 0) {
            lcd.setCursor(14, 1);
            lcd.printf("S%d", pomodoro.snoozeCount);
          }
        }
        break;
        
      case 1: { 
        // Pomodoro cycles and motivation
        lcd.setCursor(0, 0);
        lcd.printf("Cycles: %d", pomodoro.completedCycles);
        lcd.setCursor(0, 1);
        String phrase = getMotivationalPhrase(pomodoro.completedCycles);
        lcd.print(phrase);
        
        // Add visual indicator for current state
        if (phrase.length() < 14) {
          lcd.setCursor(15, 1);
          switch (pomodoro.currentState) {
            case WORK_SESSION: lcd.print("W"); break;
            case SHORT_BREAK: lcd.print("B"); break;
            case LONG_BREAK: lcd.print("L"); break;
            default: lcd.print("?"); break;
          }
        }
        break;
      }  
        
      case 2:
        // Environmental alerts (only shows if there's an alert)
        if (alertActive) {
          showEnvironmentalAlert();
        } else {
          // Fallback to mode 0
          displayMode = 0;
        }
        break;
    }
  } else {
    // Show welcome screen when no session is active
    showWelcomeScreen();
  }
}

void showWelcomeScreen() {
  lcd.setCursor(0, 0);
  lcd.print("Bill-E Focus Bot");
  lcd.setCursor(0, 1);
  lcd.print("Scan RFID card");
}

String getMotivationalPhrase(int cycles) {
  if (cycles >= maxPhrases) {
    return motivationalPhrases[maxPhrases - 1]; // Use last phrase for 8+ cycles
  }
  return motivationalPhrases[cycles];
}

bool hasEnvironmentalAlert() {
  if (!envData.dataAvailable) {
    return false;
  }
  
  return (envData.noiseLevel > NOISE_THRESHOLD || envData.lightLevel < LIGHT_THRESHOLD);
}

void showEnvironmentalAlert() {
  if (envData.noiseLevel > NOISE_THRESHOLD) {
    // Noise alert
    lcd.setCursor(0, 0);
    lcd.print("Very noisy");
    lcd.setCursor(0, 1);
    lcd.print("Use ear plugs");
  } else if (envData.lightLevel < LIGHT_THRESHOLD) {
    // Light alert  
    lcd.setCursor(0, 0);
    lcd.print("Too dark");
    lcd.setCursor(0, 1);
    lcd.print("Turn on the lamp");
  }
}
