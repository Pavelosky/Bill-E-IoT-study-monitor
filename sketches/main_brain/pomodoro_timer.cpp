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
  
  unsigned long elapsed = millis() - pomodoro.stateStartTime;
  
  // Check if current state duration is complete
  if (elapsed >= pomodoro.stateDuration) {
    transitionToNextState();
  }
  
  // Check break compliance during breaks
  if ((pomodoro.currentState == SHORT_BREAK || pomodoro.currentState == LONG_BREAK) 
      && !pomodoro.breakComplianceChecked && elapsed > 60000) { // Check after 1 minute
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
    // Send gentle reminder to mesh
    // Note: Original code had mesh network functionality here
  }
  
  pomodoro.breakComplianceChecked = true;
}

String getTimeRemainingText() {
  if (pomodoro.currentState == IDLE) return "00:00";
  
  unsigned long elapsed = millis() - pomodoro.stateStartTime;
  unsigned long remaining = (pomodoro.stateDuration > elapsed) ? 
                            (pomodoro.stateDuration - elapsed) / 1000 : 0;
  
  int minutes = remaining / 60;
  int seconds = remaining % 60;
  
  return String(minutes) + ":" + (seconds < 10 ? "0" : "") + String(seconds);
}

unsigned long getTimeRemainingSeconds() {
  if (pomodoro.currentState == IDLE) return 0;
  
  unsigned long elapsed = millis() - pomodoro.stateStartTime;
  unsigned long remaining = (pomodoro.stateDuration > elapsed) ? 
                            (pomodoro.stateDuration - elapsed) / 1000 : 0;
  return remaining;
}
