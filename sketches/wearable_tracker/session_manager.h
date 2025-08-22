#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <Arduino.h>

// Session state variables
extern bool sessionActive;
extern String currentUser;

// Session functions (if needed for future expansion)
void initializeSession();

#endif