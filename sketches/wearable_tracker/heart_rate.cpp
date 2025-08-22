#include "heart_rate.h"
#include "config.h"

void initializeHeartRate() {
  pinMode(HEART_LED, OUTPUT);
  digitalWrite(HEART_LED, LOW);
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
  
  if (sensorValue > HEART_SENSOR_THRESHOLD && lastValue < HEART_SENSOR_THRESHOLD && !beatDetected) {
    unsigned long now = millis();
    
    if (now - lastBeat > MIN_HEARTBEAT_INTERVAL) { // Minimum time between beats
      beatDetected = true;
      lastBeat = now;
      beatCount++;
      
      // Blink LED on heartbeat
      digitalWrite(HEART_LED, HIGH);
      
      // Calculate BPM over measurement windows
      if (measureStart == 0) measureStart = now;
      
      if (now - measureStart >= HEARTRATE_MEASURE_WINDOW) { // 15 seconds
        int bpm = (beatCount * 60) / (HEARTRATE_MEASURE_WINDOW / 1000);
        beatCount = 0;
        measureStart = now;
        
        if (bpm >= MIN_VALID_BPM && bpm <= MAX_VALID_BPM) {
          lastValidBPM = bpm;
          return bpm;
        }
      }
    }
  }
  
  if (sensorValue < HEART_SENSOR_LOW) {
    beatDetected = false;
    digitalWrite(HEART_LED, LOW);
  }
  
  lastValue = sensorValue;
  return lastValidBPM;
}