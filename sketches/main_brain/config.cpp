#include <Arduino.h>
#include "config.h"

// MQTT Topics - Definitions
const char* TOPIC_TEMPERATURE = "bille/sensors/temperature";
const char* TOPIC_HUMIDITY = "bille/sensors/humidity";
const char* TOPIC_LIGHT = "bille/sensors/light";
const char* TOPIC_NOISE = "bille/sensors/noise";
const char* TOPIC_HEARTRATE = "bille/sensors/heartrate";
const char* TOPIC_STEPS = "bille/sensors/steps";
const char* TOPIC_ACTIVITY = "bille/sensors/activity";
const char* TOPIC_POMODORO_STATE = "bille/pomodoro/state";
const char* TOPIC_POMODORO_TIME = "bille/pomodoro/time_remaining";
const char* TOPIC_POMODORO_CYCLES = "bille/pomodoro/cycles";
const char* TOPIC_SESSION_ACTIVE = "bille/session/active";
const char* TOPIC_SESSION_USER = "bille/session/user";

// Authentication
const char* AUTHORIZED_CARD = "9c13c3";