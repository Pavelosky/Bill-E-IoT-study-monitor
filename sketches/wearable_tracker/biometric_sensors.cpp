#include "biometric_sensors.h"
#include "config.h"
#include <MPU6050.h>
#include <Arduino.h>

extern MPU6050 mpu;

void readBiometrics() {
  // Read heart rate from KY-036
  currentBio.heartRate = readHeartRate();
  
  // Read accelerometer for activity detection
  readActivityData();
  
  // Detect current activity
  currentBio.activity = detectActivity();
  
  // Update step count
  currentBio.stepCount = stepCount;
  
  // Timestamp
  currentBio.timestamp = millis();
}

int readHeartRate() {
  static unsigned long lastBeat = 0;
  static int beatCount = 0;
  static unsigned long measureStart = 0;
  static int lastValidBPM = 75;
  
  // Read sensor value
  int sensorValue = analogRead(HEART_SENSOR);
  
  // Simple peak detection for heartbeat
  static int lastValue = 0;
  static bool beatDetected = false;
  
  if (sensorValue > 600 && lastValue < 600 && !beatDetected) {
    unsigned long now = millis();
    
    if (now - lastBeat > 300) { // Minimum 300ms between beats
      beatDetected = true;
      lastBeat = now;
      beatCount++;
      
      // Blink LED on heartbeat
      digitalWrite(HEART_LED, HIGH);
      
      // Calculate BPM over 15-second windows
      if (measureStart == 0) measureStart = now;
      
      if (now - measureStart >= 15000) { // 15 seconds
        int bpm = (beatCount * 60) / 15;
        beatCount = 0;
        measureStart = now;
        
        if (bpm >= 40 && bpm <= 200) {
          lastValidBPM = bpm;
          return bpm;
        }
      }
    }
  }
  
  if (sensorValue < 500) {
    beatDetected = false;
    digitalWrite(HEART_LED, LOW);
  }
  
  lastValue = sensorValue;
  return lastValidBPM;
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
  
  // Improved step detection using absolute magnitude above gravity baseline
  if (accelMagnitude > (1.0 + ACCEL_THRESHOLD)) {
    if (!stepDetected && (currentTime - lastStepTime > STEP_DELAY)) {
      stepCount++;
      stepDetected = true;
      lastStepTime = currentTime;
      currentBio.lastMovement = currentTime;  // Update movement timestamp
      
      Serial.print("Step detected! Total steps: ");
      Serial.println(stepCount);
    }
  } else {
    stepDetected = false;
  }
  
  // Update movement tracking for other activities (subtle movements)
  float delta = abs(accelMagnitude - lastAccelMagnitude);
  if (delta > 0.1) { // Lower threshold for general movement detection
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