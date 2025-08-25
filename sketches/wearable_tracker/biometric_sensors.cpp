#include "biometric_sensors.h"
#include "config.h"
#include <MPU6050.h>
#include <Arduino.h>

extern MPU6050 mpu;

void readBiometrics() {
  
  // Read accelerometer for activity detection
  readActivityData();
  
  // Detect current activity
  currentBio.activity = detectActivity();
  
  // Update step count
  currentBio.stepCount = stepCount;
  
  // Timestamp
  currentBio.timestamp = millis();
}


void readActivityData() {
  // Read raw accelerometer data
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  
  // Convert to g-force
  float accelX = ax / 16384.0;
  float accelY = ay / 16384.0;
  float accelZ = az / 16384.0;
  
  // Calculate acceleration magnitude
  float accelMagnitude = sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ);
  currentBio.acceleration = accelMagnitude;
  
  unsigned long currentTime = millis();
  
  // Improved step detection algorithm
  static float accelBuffer[10] = {0}; // Rolling buffer for smoothing
  static int bufferIndex = 0;
  static float smoothedAccel = 1.0;
  static bool stepInProgress = false;
  static float peakValue = 0;
  
  // Update rolling buffer
  accelBuffer[bufferIndex] = accelMagnitude;
  bufferIndex = (bufferIndex + 1) % 10;
  
  // Calculate smoothed acceleration
  float sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += accelBuffer[i];
  }
  smoothedAccel = sum / 10.0;
  
  // Step detection with peak detection and validation
  float threshold = 1.15; // Adjusted threshold
  float variance = accelMagnitude - smoothedAccel;
  
  if (!stepInProgress && variance > threshold && (currentTime - lastStepTime > 300)) {
    stepInProgress = true;
    peakValue = variance;
  } 
  else if (stepInProgress) {
    if (variance > peakValue) {
      peakValue = variance; // Track peak
    } 
    else if (variance < (threshold * 0.3) && peakValue > threshold) {
      // Step completed - validate it's a real step
      if (currentTime - lastStepTime > 300 && currentTime - lastStepTime < 2000) {
        stepCount++;
        stepInProgress = false;
        lastStepTime = currentTime;
        currentBio.lastMovement = currentTime;
        Serial.print("Step detected! Total steps: ");
        Serial.println(stepCount);
      } else {
        stepInProgress = false; // Invalid step timing
      }
    }
  }
  
  // Reset step detection if too long since peak
  if (stepInProgress && (currentTime - lastStepTime > 2000)) {
    stepInProgress = false;
  }
  
  // Update movement tracking for other activities
  float delta = abs(accelMagnitude - lastAccelMagnitude);
  if (delta > 0.05) {
    currentBio.lastMovement = currentTime;
  }
  
  lastAccelMagnitude = accelMagnitude;
}

String detectActivity() {
  unsigned long timeSinceMovement = millis() - currentBio.lastMovement;
  
  if (timeSinceMovement < 1000) {
    if (currentBio.acceleration > 1.5) {
      return "Running";
    } else if (currentBio.acceleration > 1.2) {
      return "Walking";
    } else {
      return "Moving";
    }
  } else if (timeSinceMovement < 30000) { // 30 seconds
    return "Still";
  } else {
    return "Sitting";
  }
}