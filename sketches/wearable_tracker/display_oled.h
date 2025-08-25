#ifndef DISPLAY_OLED_H
#define DISPLAY_OLED_H

#include <Arduino.h>

void updateDisplay();
void showBiometricData();
void showWelcomeScreen();
void showMQTTStatus();
void showActivityStatus();
void showBreakCompliance();
void showPomodoroProgress();
void drawStar(int x, int y, int size);
void handleButtonPress();
bool readButton();
void nextDisplayMode();

extern bool sessionActive;

#endif