#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <Arduino.h>

// Session functions
void startSession(String userId);
void endSession();

// Session state variables
extern bool sessionActive;
extern String currentUser;
extern unsigned long sessionStart;

#endif