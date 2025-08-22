#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

// Environmental data from mesh
struct EnvironmentData {
  float temperature = 0;
  float humidity = 0;
  int lightLevel = 0;
  int noiseLevel = 0;
  bool soundDetected = false;
  unsigned long lastUpdate = 0;
  bool dataAvailable = false;
};

// Biometric data from wearable
struct BiometricData {
  int heartRate = 0;
  String activity = "";
  int stepCount = 0;
  float acceleration = 0;
  unsigned long lastMovement = 0;
  unsigned long lastUpdate = 0;
  bool dataAvailable = false;
};

// Global instances
extern EnvironmentData envData;
extern BiometricData bioData;

// Analysis functions
void analyzeEnvironment();
void analyzeBiometrics();

#endif