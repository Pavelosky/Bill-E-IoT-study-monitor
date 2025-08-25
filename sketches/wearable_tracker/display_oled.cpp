#include "display_oled.h"
#include "biometric_data.h"
#include "config.h"
#include <U8g2lib.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C display;
extern BiometricData currentBio;
extern PomodoroInfo pomodoroInfo;
extern PubSubClient client;

void updateDisplay() {
  static int currentMode = 0;
  static unsigned long lastUpdate = 0;
  
  // Handle button press
  if (readButton()) {
    Serial.println("Button detected in updateDisplay()");
    
    // Determine max modes based on session state
    int maxModes;
    if (sessionActive && pomodoroInfo.dataAvailable) {
      maxModes = 4; // 4 screens during session
    } else {
      maxModes = 2; // 2 screens when idle
    }
    
    // Cycle to next mode
    currentMode = (currentMode + 1) % maxModes;
    Serial.println("Switched to mode: " + String(currentMode));
    
    // Force immediate update
    lastUpdate = 0;
  }
  
  // Update display every 2 seconds OR immediately after mode change
  if (millis() - lastUpdate > 2000) {
    switch (currentMode) {
      case 0:
        showBiometricData();
        break;
      case 1:
        showActivityStatus();
        break;
      case 2:
        if (sessionActive && pomodoroInfo.dataAvailable) {
          showPomodoroProgress();
        } else {
          currentMode = 0; // Fallback to mode 0 if no session
          showBiometricData();
        }
        break;
      case 3:
        if (sessionActive && (pomodoroInfo.currentState == SHORT_BREAK || pomodoroInfo.currentState == LONG_BREAK)) {
          showBreakCompliance();
        } else {
          showPomodoroProgress(); // Show progress instead during work
        }
        break;
    }
    lastUpdate = millis();
  }
}

void showBiometricData() {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  
  display.setCursor(0, 15);
  display.print("Bill-E Tracker");
  
  display.setCursor(0, 30);
  display.printf("Steps: %d", currentBio.stepCount);
  
  display.setCursor(0, 45);
  display.printf("Activity: %s", currentBio.activity.c_str());
  
  // Add Pomodoro context if available
  if (sessionActive && pomodoroInfo.dataAvailable) {
    display.setCursor(0, 60);
    display.setFont(u8g2_font_4x6_tf);
    switch (pomodoroInfo.currentState) {
      case WORK_SESSION: display.print("FOCUS MODE"); break;
      case SHORT_BREAK: display.print("SHORT BREAK"); break;
      case LONG_BREAK: display.print("LONG BREAK"); break;
      default: display.print("IDLE"); break;
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

void showBreakCompliance() {
  unsigned long timeSinceMovement = millis() - currentBio.lastMovement;
  
  display.clearBuffer();
  display.setFont(u8g2_font_8x13_tf);
  
  if (timeSinceMovement < 30000) { // Moved recently
    display.setCursor(0, 30);
    display.print("Great!");
    display.setCursor(0, 50);
    display.print("Keep Moving");
    
    // Happy face
    display.drawCircle(100, 30, 12);
    display.drawDisc(95, 25, 2);
    display.drawDisc(105, 25, 2);
    display.drawCircle(100, 32, 6);
    
  } else { // Not moving
    display.setCursor(0, 30);
    display.print("Time to Move!");
    display.setCursor(0, 50);
    display.print("Stand & Stretch");
    
    // Alert icon
    display.drawTriangle(100, 15, 108, 35, 92, 35);
    display.setFont(u8g2_font_6x10_tf);
    display.setCursor(98, 30);
    display.print("!");
  }
  
  display.sendBuffer();
}

bool readButton() {
  static unsigned long lastButtonTime = 0;
  static bool lastButtonState = HIGH;
  
  bool currentButtonState = digitalRead(BUTTON_PIN);
  
  // Detect button press (falling edge with debounce)
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    if (millis() - lastButtonTime > 200) { // 200ms debounce
      lastButtonTime = millis();
      lastButtonState = currentButtonState;
      return true;
    }
  }
  
  lastButtonState = currentButtonState;
  return false;
}

void nextDisplayMode() {
  static int displayMode = 0;
  
  if (sessionActive && pomodoroInfo.dataAvailable) {
    displayMode = (displayMode + 1) % 4; // 4 modes during session
  } else {
    displayMode = (displayMode + 1) % 3; // 3 modes when idle
  }
  
  // Force immediate display update
  switch (displayMode) {
    case 0:
      showBiometricData();
      break;
    case 1:
      showActivityStatus();
      break;
    case 2:
      if (sessionActive && pomodoroInfo.dataAvailable) {
        showPomodoroProgress();
      } else {
        showBiometricData();
      }
      break;
    case 3:
      if (sessionActive && pomodoroInfo.dataAvailable) {
        showBreakCompliance();
      }
      break;
  }
}

void handleButtonPress() {
  if (readButton()) {
    Serial.println("Button pressed - changing display mode");
    nextDisplayMode();
  }
}

void showPomodoroProgress() {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  
  // Calculate progress
  int totalDuration;
  String stateText;
  switch (pomodoroInfo.currentState) {
    case WORK_SESSION: 
      totalDuration = 25 * 60; 
      stateText = "FOCUS TIME";
      break;
    case SHORT_BREAK: 
      totalDuration = 5 * 60; 
      stateText = "SHORT BREAK";
      break;
    case LONG_BREAK: 
      totalDuration = 15 * 60; 
      stateText = "LONG BREAK";
      break;
    default: 
      totalDuration = 1;
      stateText = "IDLE";
      break;
  }
  
  // Calculate real-time remaining
  unsigned long realTimeElapsed = (millis() - pomodoroInfo.lastUpdate) / 1000;
  unsigned long currentRemaining = (pomodoroInfo.timeRemaining > realTimeElapsed) ? 
                                   (pomodoroInfo.timeRemaining - realTimeElapsed) : 0;
  
  int elapsed = totalDuration - currentRemaining;
  int progressPercent = (elapsed * 100) / totalDuration;
  int progressPixels = (elapsed * 110) / totalDuration; // 110 pixels wide bar
  
  // Display state and cycle info
  display.setCursor(0, 12);
  display.print(stateText);
  
  display.setCursor(0, 25);
  display.printf("Cycle: %d", pomodoroInfo.completedCycles);
  
  // Time remaining
  int minutes = currentRemaining / 60;
  int seconds = currentRemaining % 60;
  display.setCursor(0, 38);
  display.printf("Time: %d:%02d", minutes, seconds);
  
  // Progress percentage
  display.setCursor(80, 38);
  display.printf("%d%%", progressPercent);
  
  // Progress bar frame
  display.drawFrame(5, 45, 114, 12);
  display.setCursor(8, 54);
  display.setFont(u8g2_font_4x6_tf);
  display.print("PROGRESS");
  
  // Fill progress bar
  if (progressPixels > 0) {
    display.drawBox(6, 46, progressPixels, 10);
  }
  
  display.sendBuffer();
}
