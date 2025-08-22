#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

// Display functions
void initializeDisplay();
void showWelcomeScreen();
void updateDisplay();

// Global LCD object
extern LiquidCrystal_I2C lcd;

#endif