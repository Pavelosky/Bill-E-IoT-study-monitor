#include "display_oled.h"
#include "biometric_data.h"
#include <U8g2lib.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C display;
extern BiometricData currentBio;
extern PomodoroInfo pomodoroInfo;
extern PubSubClient client;

void updateDisplay() {
  static unsigned long lastDisplayUpdate = 0;
  static int displayMode = 0;

  // Check if we should show achievement message
  static int lastCompletedCycles = 0;
  if (pomodoroInfo.completedCycles > lastCompletedCycles && pomodoroInfo.dataAvailable) {
    lastCompletedCycles = pomodoroInfo.completedCycles;
  }

  // Main display rotation every 4 seconds
  if (millis() - lastDisplayUpdate > 4000) {
    // Adjust display mode count based on session state
    int maxModes = sessionActive ? 4 : 3; // More modes during active session
    displayMode = (displayMode + 1) % maxModes;
    lastDisplayUpdate = millis();
  }
  
  if (sessionActive && pomodoroInfo.dataAvailable) {
    // Enhanced display modes during active Pomodoro session
    switch (displayMode) {
      case 0:
        // Break compliance (only during breaks)
        if (pomodoroInfo.currentState == SHORT_BREAK || pomodoroInfo.currentState == LONG_BREAK) {
          showBreakCompliance();
          drawProgressBar();
        } else {
          // Fall back to biometric data during work
          showBiometricData();
          drawProgressBar();
        }
        break;
        
      case 1:
        // Heart rate and activity focus
        showBiometricData();
        break;
        
      case 2:
        // Step count and activity encouragement
        showActivityStatus();
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
    }
    else if (pomodoroInfo.currentState == SHORT_BREAK) {
      display.print("Short Break");
    }
    else if (pomodoroInfo.currentState == LONG_BREAK) {
      display.print("Long Break");
    }
    else {
      display.print("Idle");
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
  display.print("MQTT Ready!");
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

void drawProgressBar() {
  // Calculate progress based on Pomodoro state
  int totalDuration;
  switch (pomodoroInfo.currentState) {
    case WORK_SESSION: totalDuration = 25 * 60; break;      // 25 minutes
    case SHORT_BREAK: totalDuration = 5 * 60; break;        // 5 minutes  
    case LONG_BREAK: totalDuration = 15 * 60; break;        // 15 minutes
    default: return; // No progress bar for IDLE
  }
  
  // Use real-time remaining time for progress calculation
  unsigned long realTimeElapsed = (millis() - pomodoroInfo.lastUpdate) / 1000;
  unsigned long currentRemaining = (pomodoroInfo.timeRemaining > realTimeElapsed) ? 
                                   (pomodoroInfo.timeRemaining - realTimeElapsed) : 0;
  
  int elapsed = totalDuration - currentRemaining;
  int progress = (elapsed * 120) / totalDuration; // 120 pixels wide
  
  // Draw progress bar frame
  display.drawFrame(4, 58, 120, 6);
  
  // Fill progress bar
  if (progress > 0) {
    display.drawBox(5, 59, progress, 4);
  }
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