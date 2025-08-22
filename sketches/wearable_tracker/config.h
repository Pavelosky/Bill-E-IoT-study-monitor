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
#define OLED_SDA        D2
#define OLED_SCL        D1
#define HEART_SENSOR    A0
#define HEART_LED       D5

// Sensor thresholds
const float MOVEMENT_THRESHOLD = 0.3;
const float STEP_THRESHOLD = 0.8;
const int MIN_STEP_INTERVAL = 300; // ms
const int MIN_HEARTBEAT_INTERVAL = 300; // ms
const int HEART_SENSOR_THRESHOLD = 600;
const int HEART_SENSOR_LOW = 500;

// Timing constants
const int SENSOR_READ_INTERVAL = 5000; // ms
const int HEALTH_CHECK_INTERVAL = 30000; // ms
const int DISPLAY_UPDATE_INTERVAL = 4000; // ms
const int HEARTRATE_MEASURE_WINDOW = 15000; // ms

// Health thresholds
const int MIN_VALID_BPM = 40;
const int MAX_VALID_BPM = 200;
const int ELEVATED_HEART_RATE = 100;
const unsigned long EXTENDED_SITTING_TIME = 20 * 60 * 1000UL; // 20 minutes
const unsigned long BREAK_COMPLIANCE_TIME = 2 * 60 * 1000UL; // 2 minutes
const unsigned long MOVEMENT_RECENT_TIME = 30000; // 30 seconds

// Display settings
const int MAX_ACTIVE_MODES = 4;
const int MAX_INACTIVE_MODES = 3;

// MQTT Topics - External declarations
extern const char* TOPIC_HEARTRATE;
extern const char* TOPIC_STEPS;
extern const char* TOPIC_ACTIVITY;
extern const char* TOPIC_SESSION_STATE;
extern const char* TOPIC_POMODORO_STATE;
extern const char* TOPIC_WEARABLE_REQUEST;
extern const char* TOPIC_MOVEMENT_ALERT;
extern const char* TOPIC_BIOMETRIC_DATA;
extern const char* TOPIC_HEALTH_ALERT;
extern const char* TOPIC_WEARABLE_STATUS;

#endif