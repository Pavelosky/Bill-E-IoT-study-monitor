#ifndef ENVIRONMENT_DATA_H
#define ENVIRONMENT_DATA_H

// Environmental data structure
struct EnvironmentData {
  float temperature;
  float humidity;
  int lightLevel;
  int noiseLevel;
  bool soundDetected;
  unsigned long timestamp;
};

#endif