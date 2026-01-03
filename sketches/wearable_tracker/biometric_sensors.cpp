#include "biometric_sensors.h"
#include "config.h"
#include <MPU6050.h>
#include <Arduino.h>

extern MPU6050 mpu;

void readBiometrics() {
  
  // Read accelerometer for activity detection
  readActivityData();
  
  // Detect steps 
  // detectSteps();

  //  Since step detection was unreliable, we estimate steps based on activity 
  estimateStepCount();
  
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
  
  // Update movement tracking for other activities 
  float delta = abs(accelMagnitude - lastAccelMagnitude);
  if (delta > 0.05) {
    currentBio.lastMovement = currentTime; // Update last movement
    Serial.println("Movement detected, updating lastMovement timestamp.");
  } else {
    Serial.println("No significant movement detected.");
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

// // Couldn't get this to detect steps correctly, the approach is changed to just add average steps
// // count based on the activity (walking / running)
// void detectSteps() {
//   unsigned long currentTime = millis();
//   static float smoothedAccel = 1.0;
//   static bool aboveThreshold = false;
  
//   // Simple moving average for smoothing
//   float alpha = 0.1; // Lower alpha for smoother averaging
//   smoothedAccel = alpha * smoothedAccel + (1 - alpha) * currentBio.acceleration;
  
//   // Improved step detection
//   float diff = currentBio.acceleration - smoothedAccel;
  
//   // Step up detected
//   if (!aboveThreshold && diff > 0.03 && (currentTime - lastStepTime > 200)) { // Lower threshold and time gap
//     aboveThreshold = true;
//   }
//   // Step down detected - count the step
//   else if (aboveThreshold && diff < -0.015) { // Lower threshold for step down
//     stepCount++;
//     lastStepTime = currentTime;
//     currentBio.lastMovement = currentTime;
//     aboveThreshold = false;
    
//     Serial.printf("STEP #%d! Diff: %.3f, Accel: %.3f, Smooth: %.3f\n", 
//                   stepCount, diff, currentBio.acceleration, smoothedAccel);
//   }
  
// }

void estimateStepCount(){
  if (currentBio.activity == "Walking") {
    stepCount += 2; // Average walking pace
  } else if (currentBio.activity == "Running") {
    stepCount += 3; // Average running pace
  }
}
