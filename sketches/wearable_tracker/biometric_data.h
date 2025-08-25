#ifndef BIOMETRIC_DATA_H
#define BIOMETRIC_DATA_H

#include <Arduino.h>

// Biometric data structure
struct BiometricData {
  String activity;
  int stepCount;
  unsigned long lastMovement;
  float acceleration;
  unsigned long timestamp;
};

//////////////// POMODORO SESSION VARIABLES /////////////////
enum PomodoroState {
  IDLE,
  WORK_SESSION,
  SHORT_BREAK,
  LONG_BREAK
};

struct PomodoroInfo {
  PomodoroState currentState;
  unsigned long timeRemaining;  // in seconds
  int completedCycles;
  bool snoozed;
  int snoozeCount;
  String stateText;
  bool dataAvailable;
  unsigned long lastUpdate;
};

#endif