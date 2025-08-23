#include "display_manager.h"
#include "data_structures.h"
#include "pomodoro_timer.h"
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

extern LiquidCrystal_I2C lcd;
extern PubSubClient mqttClient;

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

// Show welcome screen
void showWelcomeScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Bill-E Focus Bot");
  lcd.setCursor(0, 1);
  lcd.print("Scan RFID card");
}