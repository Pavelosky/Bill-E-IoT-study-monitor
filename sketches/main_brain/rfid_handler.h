#ifndef RFID_HANDLER_H
#define RFID_HANDLER_H

#include <Arduino.h>
#include <MFRC522.h>

// RFID functions
void initializeRFID();
void handleRFID();

// Global RFID object
extern MFRC522 rfid;

#endif