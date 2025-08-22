#include "sensors.h"
#include "config.h"
#include "heart_rate.h"
#include "activity_tracker.h"

// Global instances
BiometricData currentBio;
PomodoroInfo pomodoroInfo;

void readBiometrics() {
  // Read heart rate from KY-036
  currentBio.heartRate = readHeartRate();
  
  // Read accelerometer for activity detection
  readActivityData();
  
  // Detect current activity
  currentBio.activity = detectActivity();
  
  // Update step count
  currentBio.stepCount = getStepCount();
  
  // Timestamp
  currentBio.timestamp = millis();
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