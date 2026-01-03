#include "pomodoro_timer.h"
#include "data_structures.h"
#include "audio_system.h"
#include "mqtt_handler.h"
#include "config.h"
#include <Arduino.h>


void initializePomodoro() {
  pomodoro.currentState = WORK_SESSION;
  pomodoro.stateStartTime = millis();
  pomodoro.stateDuration = pomodoro.workDuration * 60 * 1000UL; // Convert to milliseconds
  pomodoro.completedCycles = 0;
  pomodoro.breakSnoozed = false;
  pomodoro.snoozeCount = 0;
  pomodoro.breakComplianceChecked = false;
  
  playWorkSessionStartSound();
  publishPomodoroState();
  
  Serial.println("Pomodoro work session started!");
}

void updatePomodoroTimer() {
  if (!sessionActive || pomodoro.currentState == IDLE) return;
  
  // Always check for touch input (force transition or confirmation)
  handleTouchConfirmation();
  
  // If we're awaiting confirmation, don't check timer
  if (pomodoro.awaitingConfirmation) {
    return;
  }
  
  unsigned long elapsed = millis() - pomodoro.stateStartTime;
  
  // Check if current state duration is complete
  if (elapsed >= pomodoro.stateDuration) {
    // Instead of transitioning immediately, wait for confirmation
    pomodoro.awaitingConfirmation = true;
    Serial.println("Timer completed - awaiting touch confirmation");
    publishPomodoroState(); // Update MQTT with awaiting status
  }
  
  // Check break compliance during breaks (existing code)
  if ((pomodoro.currentState == SHORT_BREAK || pomodoro.currentState == LONG_BREAK) 
      && !pomodoro.breakComplianceChecked && elapsed > 60000) {
    checkBreakCompliance();
  }
}

void transitionToNextState() {
  switch (pomodoro.currentState) {
    case WORK_SESSION:
      pomodoro.completedCycles++;
      
      // Long break every 4 cycles, otherwise short break
      if (pomodoro.completedCycles % 4 == 0) {
        pomodoro.currentState = LONG_BREAK;
        pomodoro.stateDuration = pomodoro.longBreakDuration * 60 * 1000UL;
        playLongBreakStartSound();
        Serial.printf("Long break started! Cycle %d completed.\n", pomodoro.completedCycles);
      } else {
        pomodoro.currentState = SHORT_BREAK;
        pomodoro.stateDuration = pomodoro.shortBreakDuration * 60 * 1000UL;
        playBreakStartSound();
        Serial.printf("Short break started! Cycle %d completed.\n", pomodoro.completedCycles);
      }
      break;
      
    case SHORT_BREAK:
    case LONG_BREAK:
      pomodoro.currentState = WORK_SESSION;
      pomodoro.stateDuration = pomodoro.workDuration * 60 * 1000UL;
      playWorkSessionStartSound();
      Serial.println("Work session started!");
      break;
      
    default:
      break;
  }
  
  pomodoro.stateStartTime = millis();
  pomodoro.breakSnoozed = false;
  pomodoro.snoozeCount = 0;
  pomodoro.breakComplianceChecked = false;
  
  playTouchAcknoledgmentSound();
  publishPomodoroState();
}

void snoozeBreak() {
  if (pomodoro.currentState == SHORT_BREAK || pomodoro.currentState == LONG_BREAK) {
    pomodoro.stateDuration += 5 * 60 * 1000UL; // Add 5 minutes
    pomodoro.snoozeCount++;
    pomodoro.breakSnoozed = true;
    
    // Brief acknowledgment sound
    tone(BUZZER_PIN, 440, 200);
    delay(300);
    noTone(BUZZER_PIN);
    
    Serial.printf("Break snoozed for 5 minutes. Snooze count: %d\n", pomodoro.snoozeCount);
    publishPomodoroState();
  }
}

void checkBreakCompliance() {
  if (!bioData.dataAvailable) return;
  
  unsigned long timeSinceMovement = millis() - bioData.lastMovement;
  
  if (timeSinceMovement < 30000) { // Moved within last 30 seconds
    Serial.println("Break compliance: User is moving - good!");
  } else {
    Serial.println("Break compliance: Consider moving around!");
  }
  
  pomodoro.breakComplianceChecked = true;
}

String getTimeRemainingText() {
  static String cachedTimeString = "00:00";
  static unsigned long lastUpdate = 0;
  
  // Only update every 500ms for optimization
  if (millis() - lastUpdate < 500) {
    return cachedTimeString;
  }
  
  if (pomodoro.currentState == IDLE) {
    cachedTimeString = "00:00";
  } else if (pomodoro.awaitingConfirmation) {
    cachedTimeString = "TOUCH";
  } else {
    unsigned long elapsed = millis() - pomodoro.stateStartTime;
    unsigned long remaining = (pomodoro.stateDuration > elapsed) ? 
                              (pomodoro.stateDuration - elapsed) / 1000 : 0;
    
    int minutes = remaining / 60;
    int seconds = remaining % 60;
    
    cachedTimeString = String(minutes) + ":" + (seconds < 10 ? "0" : "") + String(seconds);
  }
  
  lastUpdate = millis();
  return cachedTimeString;
}

unsigned long getTimeRemainingSeconds() {
  if (pomodoro.currentState == IDLE) return 0;
  
  unsigned long elapsed = millis() - pomodoro.stateStartTime;
  unsigned long remaining = (pomodoro.stateDuration > elapsed) ? 
                            (pomodoro.stateDuration - elapsed) / 1000 : 0;
  return remaining;
}

bool readTouchSensor() {
  static unsigned long lastTouchTime = 0;
  static bool lastTouchState = false;
  
  int touchValue = analogRead(TOUCH_SENSOR);
  bool currentTouchState = touchValue > 100; // Adjust threshold here for sesitivity
  
  // Simple debouncing - detect touch press (rising edge)
  if (currentTouchState && !lastTouchState) {
    if (millis() - lastTouchTime > 500) { // 500ms debounce to prevent accidental touches
      lastTouchTime = millis();
      lastTouchState = currentTouchState;
      return true; // Return true on touch press
    }
  }
  
  lastTouchState = currentTouchState;
  return false;
}

void handleTouchConfirmation() {
  if (readTouchSensor()) {
    if (pomodoro.awaitingConfirmation) {
      Serial.println("Touch confirmed - transitioning state");
      pomodoro.awaitingConfirmation = false;
      transitionToNextState();
    } else {
      Serial.println("Touch detected - forcing state transition");
      // Force immediate transition regardless of timer
      transitionToNextState();
    }
  }
}

bool isAwaitingConfirmation() {
  return pomodoro.awaitingConfirmation;
}
