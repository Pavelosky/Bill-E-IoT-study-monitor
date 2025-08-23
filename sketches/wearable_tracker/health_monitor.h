#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include <Arduino.h>

void publishHealthAlerts();

extern bool sessionActive;
extern String currentUser;

#endif
