#ifndef CONFIG_H
#define CONFIG_H

// WiFi and MQTT configuration
#define WIFI_SSID       "TechLabNet"   
#define WIFI_PASSWORD   "BC6V6DE9A8T9"      
#define MQTT_SERVER     "192.168.1.107"     
#define MQTT_PORT       1883
#define MQTT_USER       "bille_mqtt"
#define MQTT_PASSWORD   "BillE2025_Secure!" 

// Pin definitions
#define DHT_PIN         D6
#define DHT_TYPE        DHT11
#define ANALOG_PIN      A0
#define SOUND_DIGITAL   D5
#define FAN_RELAY_PIN   D7

// Fan control thresholds
#define FAN_ON_TEMP     24.0    // Turn fan ON when temp >= 24°C
#define FAN_OFF_TEMP    22.0    // Turn fan OFF when temp <= 22°C (hysteresis)


#endif