#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>

void updateDisplay();
void showWelcomeScreen();
void forceSwitchDisplay();
String getMotivationalPhrase(int cycles);
bool hasEnvironmentalAlert();
String getEnvironmentalAlert();
void showEnvironmentalAlert();

#endif
