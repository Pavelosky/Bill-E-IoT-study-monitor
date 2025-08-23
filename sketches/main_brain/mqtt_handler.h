#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>

void setup_wifi();
void reconnect_mqtt();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void publishSessionState();
void publishPomodoroState();
void publishSystemStatus();
void publishMovementReminder();

#endif