#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

// Pomodoro states
enum PomodoroState {
  IDLE,
  WORK_SESSION,
  SHORT_BREAK,
  LONG_BREAK
};

// Biometric data structure
struct BiometricData {
  int heartRate;
  String activity;
  int stepCount;
  unsigned long lastMovement;
  float acceleration;
  unsigned long timestamp;
};

// Pomodoro info structure
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

// Global instances
extern BiometricData currentBio;
extern PomodoroInfo pomodoroInfo;

// Sensor functions
void readBiometrics();
String detectActivity();

#endif