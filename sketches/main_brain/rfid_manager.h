#ifndef RFID_MANAGER_H
#define RFID_MANAGER_H

#include <Arduino.h>

void handleRFID();
void startSession(String userId);
void endSession();

#endif