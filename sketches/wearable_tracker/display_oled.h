#ifndef DISPLAY_OLED_H
#define DISPLAY_OLED_H

#include <Arduino.h>

void updateDisplay();
void showBiometricData();
void showWelcomeScreen();
void showMQTTStatus();
void showActivityStatus();
void showBreakCompliance();
void drawProgressBar();
void drawStar(int x, int y, int size);

extern bool sessionActive;

#endif