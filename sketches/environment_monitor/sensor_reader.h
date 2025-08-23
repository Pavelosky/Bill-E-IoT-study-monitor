#ifndef SENSOR_READER_H
#define SENSOR_READER_H

#include "environment_data.h"

void readEnvironment();
int readKY018Light();
int readKY038Noise();

extern EnvironmentData currentEnv;

#endif