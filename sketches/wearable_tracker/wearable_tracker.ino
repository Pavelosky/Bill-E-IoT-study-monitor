// ESP8266-3 Wearable Tracker - Fixed MPU6050
#include <U8g2lib.h>
#include <Wire.h>
#include <MPU6050.h>

// Pin definitions
#define OLED_SDA        D2
#define OLED_SCL        D1
#define HEART_SENSOR    A0
#define HEART_LED       D5

// Objects
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
MPU6050 mpu;

// Biometric data structure
struct BiometricData {
  int heartRate;
  String activity;
  int stepCount;
  unsigned long lastMovement;
  float acceleration;
  unsigned long timestamp;
};

BiometricData currentBio;

// Activity detection variables
float lastAccelMagnitude = 0;
int stepCount = 0;
unsigned long lastStepTime = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Bill-E Wearable Tracker Starting...");
  
  // Initialize pins
  pinMode(HEART_LED, OUTPUT);
  digitalWrite(HEART_LED, LOW);
  
  // Initialize I2C
  Wire.begin(OLED_SDA, OLED_SCL);
  
  // Initialize OLED display
  display.begin();
  display.enableUTF8Print();
  
  // Initialize MPU6050
  Serial.println("Initializing MPU6050...");
  mpu.initialize();
  
  // Test connection
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful");
  } else {
    Serial.println("MPU6050 connection failed");
    showError("MPU6050 Error");
    while (1) delay(10);
  }
  
  // Welcome screen
  showWelcomeScreen();
  
  Serial.println("Wearable Tracker Ready!");
  delay(2000);
}

void loop() {
  // Read all sensors
  readBiometrics();
  
  // Update display
  updateDisplay();
  
  // Print debug info
  printBiometrics();
  
  delay(1000); // Update every second
}

void readBiometrics() {
  // Read heart rate from KY-036
  currentBio.heartRate = readHeartRate();
  
  // Read accelerometer for activity detection
  readActivityData();
  
  // Detect current activity
  currentBio.activity = detectActivity();
  
  // Update step count
  currentBio.stepCount = stepCount;
  
  // Timestamp
  currentBio.timestamp = millis();
}

int readHeartRate() {
  // KY-036 heart rate sensor reading
  static unsigned long lastBeat = 0;
  static int beatCount = 0;
  static unsigned long measureStart = 0;
  static int lastValidBPM = 75;
  
  // Read sensor value
  int sensorValue = analogRead(HEART_SENSOR);
  
  // Simple peak detection for heartbeat
  static int lastValue = 0;
  static bool beatDetected = false;
  
  if (sensorValue > 600 && lastValue < 600 && !beatDetected) {
    // Potential heartbeat detected
    unsigned long now = millis();
    
    if (now - lastBeat > 300) { // Minimum 300ms between beats (200 BPM max)
      beatDetected = true;
      lastBeat = now;
      beatCount++;
      
      // Blink LED on heartbeat
      digitalWrite(HEART_LED, HIGH);
      
      // Calculate BPM over 15-second windows
      if (measureStart == 0) measureStart = now;
      
      if (now - measureStart >= 15000) { // 15 seconds
        int bpm = (beatCount * 60) / 15; // Convert to BPM
        beatCount = 0;
        measureStart = now;
        
        // Return reasonable BPM (40-200 range)
        if (bpm >= 40 && bpm <= 200) {
          lastValidBPM = bpm;
          return bpm;
        }
      }
    }
  }
  
  if (sensorValue < 500) {
    beatDetected = false;
    digitalWrite(HEART_LED, LOW);
  }
  
  lastValue = sensorValue;
  
  // Return last valid reading
  return lastValidBPM;
}

void readActivityData() {
  // Read raw accelerometer data
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  
  // Convert to g-force (MPU6050 default range is ±2g)
  float gx = ax / 16384.0;
  float gy = ay / 16384.0;
  float gz = az / 16384.0;
  
  // Calculate acceleration magnitude
  float magnitude = sqrt(gx*gx + gy*gy + gz*gz);
  
  currentBio.acceleration = magnitude;
  
  // Detect movement and steps
  float delta = abs(magnitude - lastAccelMagnitude);
  
  if (delta > 0.3) { // Movement threshold
    currentBio.lastMovement = millis();
    
    // Step detection
    if (delta > 0.8 && millis() - lastStepTime > 300) { // Step threshold and timing
      stepCount++;
      lastStepTime = millis();
      Serial.println("Step detected! Count: " + String(stepCount));
    }
  }
  
  lastAccelMagnitude = magnitude;
}

String detectActivity() {
  unsigned long timeSinceMovement = millis() - currentBio.lastMovement;
  
  if (timeSinceMovement < 1000) {
    if (currentBio.acceleration > 1.5) {
      return "Running";
    } else if (currentBio.acceleration > 1.2) {
      return "Walking";
    } else {
      return "Moving";
    }
  } else if (timeSinceMovement < 30000) { // 30 seconds
    return "Still";
  } else {
    return "Sitting";
  }
}

void updateDisplay() {
  static unsigned long lastDisplayUpdate = 0;
  static int displayMode = 0;
  
  if (millis() - lastDisplayUpdate > 3000) { // Change display every 3 seconds
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tf);
    
    switch (displayMode) {
      case 0:
        // Heart rate and steps
        display.setCursor(0, 15);
        display.print("Bill-E Tracker");
        display.setCursor(0, 30);
        display.printf("HR: %d BPM", currentBio.heartRate);
        display.setCursor(0, 45);
        display.printf("Steps: %d", currentBio.stepCount);
        break;
        
      case 1:
        // Activity status
        display.setCursor(0, 15);
        display.print("Activity:");
        display.setCursor(0, 35);
        display.setFont(u8g2_font_8x13_tf);
        display.print(currentBio.activity);
        display.setFont(u8g2_font_6x10_tf);
        display.setCursor(0, 55);
        display.printf("Accel: %.2f g", currentBio.acceleration);
        break;
        
      case 2:
        // System info
        display.setCursor(0, 15);
        display.print("System Status");
        display.setCursor(0, 30);
        display.printf("Uptime: %lus", millis()/1000);
        display.setCursor(0, 45);
        unsigned long timeSinceMove = (millis() - currentBio.lastMovement) / 1000;
        display.printf("Last move: %lus", timeSinceMove);
        break;
    }
    
    display.sendBuffer();
    displayMode = (displayMode + 1) % 3;
    lastDisplayUpdate = millis();
  }
}

void showWelcomeScreen() {
  display.clearBuffer();
  display.setFont(u8g2_font_8x13_tf);
  display.setCursor(0, 20);
  display.print("Bill-E");
  display.setCursor(0, 40);
  display.print("Wearable");
  display.setCursor(0, 60);
  display.print("Ready!");
  display.sendBuffer();
}

void showError(String error) {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.setCursor(0, 20);
  display.print("ERROR:");
  display.setCursor(0, 40);
  display.print(error);
  display.sendBuffer();
}

void printBiometrics() {
  Serial.println("=== Biometric Data ===");
  Serial.printf("Heart Rate: %d BPM\n", currentBio.heartRate);
  Serial.printf("Activity: %s\n", currentBio.activity.c_str());
  Serial.printf("Step Count: %d\n", currentBio.stepCount);
  Serial.printf("Acceleration: %.2f g\n", currentBio.acceleration);
  Serial.printf("Last Movement: %lu ms ago\n", millis() - currentBio.lastMovement);
  Serial.println("======================");
}
