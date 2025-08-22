#include "display_manager.h"
#include "config.h"
#include "pomodoro.h"
#include "sensors.h"
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Global LCD object
LiquidCrystal_I2C lcd(0x27, 16, 2);

extern bool sessionActive;
extern String currentUser;
extern unsigned long sessionStart;
extern PubSubClient mqttClient;

void initializeDisplay() {
  // Initialize I2C for LCD
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
}

void showWelcomeScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Bill-E Focus Bot");
  lcd.setCursor(0, 1);
  lcd.print("Scan RFID card");
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
    if (millis() - lastDisplaySwitch > DISPLAY_SWITCH_INTERVAL) {
      displayMode = (displayMode + 1) % DISPLAY_MODES; // 5 modes including MQTT status
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