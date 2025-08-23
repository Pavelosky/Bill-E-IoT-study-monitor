#include "display_controller.h"
#include "environment_data.h"
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <Arduino.h>

extern LiquidCrystal_I2C lcd;
extern EnvironmentData currentEnv;

void displayEnvironment() {
  static unsigned long lastDisplay = 0;
  static int displayMode = 0;
  
  if (millis() - lastDisplay > 3000) {
    lcd.clear();
    
    switch (displayMode) {
      case 0:
        lcd.setCursor(0, 0);
        if (currentEnv.temperature != -999) {
          lcd.printf("Temp: %.1fC", currentEnv.temperature);
        } else {
          lcd.print("Temp: Error");
        }
        lcd.setCursor(0, 1);
        if (currentEnv.humidity != -999) {
          lcd.printf("Hum: %.0f%%", currentEnv.humidity);
        } else {
          lcd.print("Humidity: Error");
        }
        break;
        
      case 1:
        lcd.setCursor(0, 0);
        lcd.printf("Light: %d", currentEnv.lightLevel);
        lcd.setCursor(0, 1);
        lcd.printf("Noise: %d", currentEnv.noiseLevel);
        break;

      // TODO: For DEBUG, needs to be removed  
      case 2:
        lcd.setCursor(0, 0);
        lcd.print("MQTT: ");
        lcd.print(client.connected() ? "OK" : "ERR");
        lcd.setCursor(0, 1);
        lcd.print("WiFi: ");
        lcd.print(WiFi.status() == WL_CONNECTED ? "OK" : "ERR");
        break;
    }
    
    displayMode = (displayMode + 1) % 3;
    lastDisplay = millis();
  }
}

void printEnvironment() {
  Serial.println("=== Environmental Data ===");
  Serial.printf("Temperature: %.1fÂ°C\n", currentEnv.temperature);
  Serial.printf("Humidity: %.1f%%\n", currentEnv.humidity);
  Serial.printf("Light Level: %d lux\n", currentEnv.lightLevel);
  Serial.printf("Noise Level: %d\n", currentEnv.noiseLevel);
  Serial.printf("Sound Detected: %s\n", currentEnv.soundDetected ? "YES" : "NO");
  Serial.println("==========================");
}