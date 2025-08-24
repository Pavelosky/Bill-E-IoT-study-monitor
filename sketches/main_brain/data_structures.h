// ===============================================================
// data_models.h - Data Structures and Enums for Main Brain
// Bill-E Focus Robot - IoT Productivity System
// ===============================================================

#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <Arduino.h>

// Session state
extern bool sessionActive;
extern String currentUser;
extern unsigned long sessionStart;

enum PomodoroState {
  IDLE,
  WORK_SESSION,
  SHORT_BREAK,
  LONG_BREAK
};

struct PomodoroSession {
  PomodoroState currentState = IDLE;
  unsigned long stateStartTime = 0;     
  unsigned long stateDuration = 0;      
  int completedCycles = 0;              
  int workDuration = 25;                
  int shortBreakDuration = 5;           
  int longBreakDuration = 15;           
  bool breakSnoozed = false;            
  int snoozeCount = 0;                  
  unsigned long snoozeTime = 0;         
  bool breakComplianceChecked = false;  
  bool awaitingConfirmation = false;
};

// Environmental data from mesh
struct EnvironmentData {
  float temperature = 0;
  float humidity = 0;
  int lightLevel = 0;
  int noiseLevel = 0;
  bool soundDetected = false;
  unsigned long lastUpdate = 0;
  bool dataAvailable = false;
};

// Biometric data from wearable
struct BiometricData {
  int heartRate = 0;
  String activity = "";
  int stepCount = 0;
  float acceleration = 0;
  unsigned long lastMovement = 0;
  unsigned long lastUpdate = 0;
  bool dataAvailable = false;
};

// Global instances
extern PomodoroSession pomodoro;
extern EnvironmentData envData;
extern BiometricData bioData;

#endif