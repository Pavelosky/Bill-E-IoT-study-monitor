#include "activity_tracker.h"
#include "config.h"
#include "sensors.h"
#include <Wire.h>

// Global MPU6050 object
MPU6050 mpu;

// Activity detection variables
float lastAccelMagnitude = 0;
int stepCount = 0;
unsigned long lastStepTime = 0;

bool initializeActivityTracker() {
  // Don't initialize I2C here - it should be done once globally
  
  // Initialize MPU6050
  Serial.println("Initializing MPU6050...");
  mpu.initialize();
  
  // Test gyro connection
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful");
    return true;
  } else {
    Serial.println("MPU6050 connection failed");
    return false;
  }
}

void readActivityData() {
  // Read raw accelerometer data
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  
  // Convert to g-force
  float gx = ax / 16384.0;
  float gy = ay / 16384.0;
  float gz = az / 16384.0;
  
  // Calculate acceleration magnitude
  float magnitude = sqrt(gx*gx + gy*gy + gz*gz);
  currentBio.acceleration = magnitude;
  
  // Detect movement and steps
  float delta = abs(magnitude - lastAccelMagnitude);
  
  if (delta > MOVEMENT_THRESHOLD) { // Movement threshold
    currentBio.lastMovement = millis();
    
    // Step detection
    if (delta > STEP_THRESHOLD && millis() - lastStepTime > MIN_STEP_INTERVAL) {
      stepCount++;
      lastStepTime = millis();
      Serial.println("Step detected! Count: " + String(stepCount));
    }
  }
  
  lastAccelMagnitude = magnitude;
}

int getStepCount() {
  return stepCount;
}

void resetStepCount() {
  stepCount = 0;
}