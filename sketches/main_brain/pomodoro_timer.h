#ifndef POMODORO_TIMER_H
#define POMODORO_TIMER_H

#include <Arduino.h>

void initializePomodoro();
void updatePomodoroTimer();
void transitionToNextState();
void snoozeBreak();
void checkBreakCompliance();
String getTimeRemainingText();
unsigned long getTimeRemainingSeconds();

#endif