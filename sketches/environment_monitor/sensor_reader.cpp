#include "sensor_reader.h"
#include "config.h"
#include <DHT.h>
#include <Arduino.h>

extern DHT dht;

void readEnvironment() {
  // Read temperature and humidity from DHT
  currentEnv.temperature = dht.readTemperature();
  currentEnv.humidity = dht.readHumidity();
  
  // Read light level from KY-018
  currentEnv.lightLevel = readKY018Light();
  
  // Read noise level from KY-038
  currentEnv.noiseLevel = readKY038Noise();
  
  // Read digital sound detection from KY-038
  currentEnv.soundDetected = digitalRead(SOUND_DIGITAL) == HIGH;
  
  // Timestamp
  currentEnv.timestamp = millis();
  
  // Validate DHT readings
  if (isnan(currentEnv.temperature) || isnan(currentEnv.humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    currentEnv.temperature = -999;
    currentEnv.humidity = -999;
  }
}

int readKY018Light() {
  int rawValue = analogRead(ANALOG_PIN);

// Here I measured the response of the sensor and the checked with the phone for the values and made a curve fit
  float a = 1.8125e8;   // coefficient
  float b = -2.7918;    // exponent

  float lux = a * pow(rawValue, b);
  return (int)lux;
}

int readKY038Noise() {
  int maxSound = 0;
  int minSound = 1024;
  
  for (int i = 0; i < 100; i++) {
    int soundLevel = analogRead(ANALOG_PIN);
    if (soundLevel > maxSound) maxSound = soundLevel;
    if (soundLevel < minSound) minSound = soundLevel;
    delayMicroseconds(100);
  }
  
  return maxSound - minSound;
}