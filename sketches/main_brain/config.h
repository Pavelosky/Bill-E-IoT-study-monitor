#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// WiFi and MQTT configuration
#define WIFI_SSID       "TechLabNet"   
#define WIFI_PASSWORD   "BC6V6DE9A8T9"      
#define MQTT_SERVER     "192.168.1.107"     
#define MQTT_PORT       1883
#define MQTT_USER       "bille_mqtt"
#define MQTT_PASSWORD   "BillE2025_Secure!" 

// Pin definitions
const int RST_PIN = D1;
const int SS_PIN = D2;
const int LED_PIN = D0;
const int BUZZER_PIN = D8;

// LCD I2C pins
const int SDA_PIN = D3;
const int SCL_PIN = D4;

// MQTT Topics - External declarations
extern const char* TOPIC_TEMPERATURE;
extern const char* TOPIC_HUMIDITY;
extern const char* TOPIC_LIGHT;
extern const char* TOPIC_NOISE;
extern const char* TOPIC_HEARTRATE;
extern const char* TOPIC_STEPS;
extern const char* TOPIC_ACTIVITY;
extern const char* TOPIC_POMODORO_STATE;
extern const char* TOPIC_POMODORO_TIME;
extern const char* TOPIC_POMODORO_CYCLES;
extern const char* TOPIC_SESSION_ACTIVE;
extern const char* TOPIC_SESSION_USER;

// Authentication
extern const char* AUTHORIZED_CARD;

// Display settings
const int DISPLAY_SWITCH_INTERVAL = 3000; // ms
const int DISPLAY_MODES = 5;

// System timing
const int SYSTEM_STATUS_INTERVAL = 30000; // ms
const int POMODORO_UPDATE_INTERVAL = 10000; // ms
const int LOOP_DELAY = 100; // ms

#endif