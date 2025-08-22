#ifndef SOUND_EFFECTS_H
#define SOUND_EFFECTS_H

#include <Arduino.h>

// Sound effect functions
void playSessionStartSound();
void playAuthFailSound();
void playWorkSessionStartSound();
void playBreakStartSound();
void playLongBreakStartSound();
void playSessionCompleteSound();
void playSnoozeAcknowledgment();

#endif