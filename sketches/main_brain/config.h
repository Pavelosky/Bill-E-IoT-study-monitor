// ===============================================================
// config.h - Configuration Constants for Main Brain
// Bill-E Focus Robot - IoT Productivity System
// ===============================================================

#ifndef CONFIG_H
#define CONFIG_H

//WiFi and MQTT configuration
#define WIFI_SSID       "TechLabNet"   
#define WIFI_PASSWORD   "BC6V6DE9A8T9"      
#define MQTT_SERVER     "192.168.1.107"     
#define MQTT_PORT       1883
#define MQTT_USER       "bille_mqtt"
#define MQTT_PASSWORD   "BillE2025_Secure!" 

// Pin definitions
#define RST_PIN D1
#define SS_PIN D2
#define LED_PIN D0
#define BUZZER_PIN D8

// MQTT Topics
#define TOPIC_TEMPERATURE "bille/sensors/temperature"
#define TOPIC_HUMIDITY "bille/sensors/humidity"
#define TOPIC_LIGHT "bille/sensors/light"
#define TOPIC_NOISE "bille/sensors/noise"
#define TOPIC_HEARTRATE "bille/sensors/heartrate"
#define TOPIC_STEPS "bille/sensors/steps"
#define TOPIC_ACTIVITY "bille/sensors/activity"
#define TOPIC_POMODORO_STATE "bille/pomodoro/state"
#define TOPIC_POMODORO_TIME "bille/pomodoro/time_remaining"
#define TOPIC_POMODORO_CYCLES "bille/pomodoro/cycles"
#define TOPIC_SESSION_ACTIVE "bille/session/active"
#define TOPIC_SESSION_USER "bille/session/user"

#endif