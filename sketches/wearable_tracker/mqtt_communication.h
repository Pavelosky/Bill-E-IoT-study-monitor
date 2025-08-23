#ifndef MQTT_COMMUNICATION_H
#define MQTT_COMMUNICATION_H

#include <Arduino.h>

void setup_wifi();
void reconnect_mqtt();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void publishBiometricData();

extern bool sessionActive;
extern String currentUser;

#endif
