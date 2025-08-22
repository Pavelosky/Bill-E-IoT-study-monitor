#ifndef POMODORO_H
#define POMODORO_H

#include <Arduino.h>

enum PomodoroState {
  IDLE,
  WORK_SESSION,
  SHORT_BREAK,
  LONG_BREAK
};

struct PomodoroSession {
  PomodoroState currentState = IDLE;
  unsigned long stateStartTime = 0;     // Start time of the current state
  unsigned long stateDuration = 0;      // Current state duration in milliseconds
  int completedCycles = 0;              // Number of completed pomodoro cycles
  int workDuration = 25;                 // Default 25 minutes
  int shortBreakDuration = 5;           // Default 5 minutes
  int longBreakDuration = 15;            // Default 15 minutes
  bool breakSnoozed = false;            // Snooze state for breaks
  int snoozeCount = 0;                  // Number of times snoozed
  unsigned long snoozeTime = 0;         // Time when snooze was activated
  bool breakComplianceChecked = false;  // Compliance check for breaks
};

// Global instance
extern PomodoroSession pomodoro;

// Pomodoro functions
void initializePomodoro();
void updatePomodoroTimer();
void transitionToNextState();
void snoozeBreak();
void checkBreakCompliance();
String getTimeRemainingText();
unsigned long getTimeRemainingSeconds();
void publishPomodoroState();

#endif