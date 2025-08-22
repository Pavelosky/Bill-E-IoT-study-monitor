#include <Arduino.h>
#include "config.h"

// MQTT Topics - Definitions
const char* TOPIC_HEARTRATE = "bille/sensors/heartrate";
const char* TOPIC_STEPS = "bille/sensors/steps";
const char* TOPIC_ACTIVITY = "bille/sensors/activity";
const char* TOPIC_SESSION_STATE = "bille/session/state";
const char* TOPIC_POMODORO_STATE = "bille/pomodoro/state";
const char* TOPIC_WEARABLE_REQUEST = "bille/wearable/request";
const char* TOPIC_MOVEMENT_ALERT = "bille/alerts/movement";
const char* TOPIC_BIOMETRIC_DATA = "bille/data/biometric";
const char* TOPIC_HEALTH_ALERT = "bille/alerts/health";
const char* TOPIC_WEARABLE_STATUS = "bille/status/wearable";