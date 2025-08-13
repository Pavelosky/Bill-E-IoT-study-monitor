// ESP8266-2 Environment Monitor - KY modules
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// Pin definitions
#define DHT_PIN         D6
#define DHT_TYPE        DHT11
#define ANALOG_PIN      A0      // Shared between KY-018 and KY-038
#define LIGHT_SELECT    D3      // Control pin to select light sensor
#define MIC_SELECT      D4      // Control pin to select microphone
#define SOUND_DIGITAL   D5      // KY-038 digital output (optional)

// Objects
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Environmental data structure
struct EnvironmentData {
  float temperature;
  float humidity;
  int lightLevel;
  int noiseLevel;
  bool soundDetected;  // From KY-038 digital output
  unsigned long timestamp;
};

EnvironmentData currentEnv;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Environment Monitor Starting...");
  
  // Initialize pins for analog multiplexing (if needed)
  pinMode(LIGHT_SELECT, OUTPUT);
  pinMode(MIC_SELECT, OUTPUT);
  pinMode(SOUND_DIGITAL, INPUT);
  
  // Initialize DHT sensor
  dht.begin();
  
  // Initialize I2C for LCD
  Wire.begin(D1, D2); // SDA=D1, SCL=D2
  lcd.init();
  lcd.backlight();
  
  // Welcome message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Environment");
  lcd.setCursor(0, 1);
  lcd.print("Monitor Ready");
  
  Serial.println("Environment Monitor Ready!");
  Serial.println("Reading from KY-018 (light) and KY-038 (mic)");
  delay(2000);
}

void loop() {
  // Read all environmental sensors
  readEnvironment();
  
  // Display data locally
  displayEnvironment();
  
  // Print to Serial for debugging
  printEnvironment();
  
  delay(5000); // Read every 5 seconds
}

void readEnvironment() {
  // Read temperature and humidity from DHT
  currentEnv.temperature = dht.readTemperature();
  currentEnv.humidity = dht.readHumidity();
  
  // Read light level from KY-018
  currentEnv.lightLevel = readKY018Light();
  
  // Read noise level from KY-038
  currentEnv.noiseLevel = readKY038Noise();
  
  // Read digital sound detection from KY-038
  currentEnv.soundDetected = digitalRead(SOUND_DIGITAL) == HIGH;
  
  // Timestamp
  currentEnv.timestamp = millis();
  
  // Validate DHT readings
  if (isnan(currentEnv.temperature) || isnan(currentEnv.humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    currentEnv.temperature = -999;
    currentEnv.humidity = -999;
  }
}

int readKY018Light() {
  // Read from KY-018 photoresistor module
  // The module outputs higher voltage in bright light
  int rawValue = analogRead(ANALOG_PIN);
  
  // KY-018 typically outputs:
  // Bright light: ~1000-1024
  // Dark: ~0-100
  // Convert to lux approximation (0-1000 range)
  int lux = map(rawValue, 0, 1024, 0, 1000);
  
  Serial.printf("KY-018 raw: %d, mapped: %d lux\n", rawValue, lux);
  return lux;
}

int readKY038Noise() {
  // Read from KY-038 microphone module
  // Sample for a short period to get peak-to-peak amplitude
  int maxSound = 0;
  int minSound = 1024;
  int sampleCount = 100;
  
  for (int i = 0; i < sampleCount; i++) {
    int soundLevel = analogRead(ANALOG_PIN);
    
    if (soundLevel > maxSound) maxSound = soundLevel;
    if (soundLevel < minSound) minSound = soundLevel;
    
    delayMicroseconds(100); // Quick sampling
  }
  
  // Return peak-to-peak amplitude
  int amplitude = maxSound - minSound;
  
  Serial.printf("KY-038 min: %d, max: %d, amplitude: %d\n", minSound, maxSound, amplitude);
  return amplitude;
}

void displayEnvironment() {
  // Cycle through different displays every 3 seconds
  static unsigned long lastDisplay = 0;
  static int displayMode = 0;
  
  if (millis() - lastDisplay > 3000) {
    lcd.clear();
    
    switch (displayMode) {
      case 0:
        // Temperature and Humidity
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
        // Light and Noise
        lcd.setCursor(0, 0);
        lcd.printf("Light: %d", currentEnv.lightLevel);
        lcd.setCursor(0, 1);
        lcd.printf("Noise: %d", currentEnv.noiseLevel);
        break;
        
      case 2:
        // Sound detection status
        lcd.setCursor(0, 0);
        lcd.print("Sound Detect:");
        lcd.setCursor(0, 1);
        lcd.print(currentEnv.soundDetected ? "YES" : "NO");
        break;
    }
    
    displayMode = (displayMode + 1) % 3;
    lastDisplay = millis();
  }
}

void printEnvironment() {
  Serial.println("=== Environmental Data ===");
  Serial.printf("Temperature: %.1f°C\n", currentEnv.temperature);
  Serial.printf("Humidity: %.1f%%\n", currentEnv.humidity);
  Serial.printf("Light Level: %d lux\n", currentEnv.lightLevel);
  Serial.printf("Noise Level: %d\n", currentEnv.noiseLevel);
  Serial.printf("Sound Detected: %s\n", currentEnv.soundDetected ? "YES" : "NO");
  Serial.printf("Timestamp: %lu\n", currentEnv.timestamp);
  Serial.println("==========================");
}
