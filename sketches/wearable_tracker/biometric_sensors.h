#ifndef BIOMETRIC_SENSORS_H
#define BIOMETRIC_SENSORS_H

#include "biometric_data.h"

void readBiometrics();
int readHeartRate();
void readActivityData();
String detectActivity();

extern BiometricData currentBio;
extern float lastAccelMagnitude;
extern int stepCount;
extern unsigned long lastStepTime;

#endif