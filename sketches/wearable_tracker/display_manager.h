#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <U8g2lib.h>

// Display functions
void initializeDisplay();
void updateDisplay();
void showWelcomeScreen();
void showError(String error);
void showBiometricData();
void showActivityStatus();
void showMQTTStatus();
void showBreakCompliance();
void drawProgressBar();
void drawStar(int x, int y, int size);

// Global display object
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C display;

#endif