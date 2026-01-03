#ifndef BIOMETRIC_SENSORS_H
#define BIOMETRIC_SENSORS_H

#include "biometric_data.h"

void readBiometrics();
void readActivityData();
String detectActivity();
// void detectSteps();
void estimateStepCount();

extern BiometricData currentBio;
extern float lastAccelMagnitude;
extern int stepCount;
extern unsigned long lastStepTime;
extern bool stepDetected;

// Step detection constants
#define ACCEL_THRESHOLD 0.1
#define STEP_DELAY 100

#endif
