#ifndef ACTIVITY_TRACKER_H
#define ACTIVITY_TRACKER_H

#include <Arduino.h>
#include <MPU6050.h>

// Activity tracking functions
bool initializeActivityTracker();
void readActivityData();
int getStepCount();
void resetStepCount();

// Global MPU6050 object
extern MPU6050 mpu;

#endif