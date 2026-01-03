# Bill-E Focus Robot - IoT Productivity System

**University of London BSc Computer Science**
**Module:** CM3040 Physical Computing and Internet-of-Things (IoT)

## Project Overview

Bill-E is an intelligent productivity companion system that creates an optimized work environment through environmental monitoring, activity tracking, and focus management. The system uses three interconnected ESP8266 devices communicating via MQTT to provide a comprehensive productivity enhancement solution.

### What It Does

- **Session Management**: RFID-based user authentication for personalized work sessions
- **Focus Timer**: Pomodoro technique implementation with visual and audio feedback
- **Environmental Control**: Real-time monitoring and automatic climate control for optimal study conditions
- **Activity Tracking**: Wearable device that monitors movement and encourages healthy work habits
- **Health Alerts**: Smart reminders for movement and break compliance
- **Data Integration**: MQTT-based communication with Home Assistant for analytics and visualization

### Key Features

1. **Main Brain (ESP8266-1)**: Central coordination hub with RFID authentication, LCD display, Pomodoro timer management, and audio feedback system
2. **Environmental Monitor (ESP8266-2)**: Temperature, humidity, light, and noise sensing with automatic fan control
3. **Wearable Tracker (ESP8266-3)**: Activity detection, step counting, and health monitoring with OLED display

## Hardware Requirements

### Main Brain Device (ESP8266-1)
- **ESP8266 NodeMCU v1.0** - Main microcontroller
- **MFRC522 RFID Reader** - User authentication
- **16x2 I2C LCD Display** (Address: 0x27) - Status display
- **Passive Buzzer** - Audio feedback
- **Touch Sensor Module** - Manual controls
- **Micro USB Cable** - Power and programming

### Environmental Monitor Device (ESP8266-2)
- **ESP8266 NodeMCU v1.0** - Main microcontroller
- **DHT11 Sensor** - Temperature and humidity monitoring
- **KY-018 Photoresistor** - Light level detection
- **KY-038 Sound Sensor** - Noise level monitoring
- **16x2 I2C LCD Display** (Address: 0x27) - Environmental data display
- **5V Relay Module** - Fan control
- **12V Desktop Fan** - Automatic climate control
- **Micro USB Cable** - Power and programming

### Wearable Tracker Device (ESP8266-3)
- **ESP8266 NodeMCU v1.0** - Main microcontroller
- **MPU6050 6-axis Accelerometer/Gyroscope** - Motion sensing
- **0.96" I2C OLED Display** (128x64, SSD1306) - Data visualization
- **Push Button** - Display mode switching
- **Micro USB Cable** - Power and programming
- **Power Bank** - Mobile power source

### Additional Requirements
- **WiFi Network** - For device communication
- **MQTT Broker** - Message queue service (e.g., Mosquitto on Raspberry Pi or Home Assistant)
- **Home Assistant** (Optional) - For advanced analytics and visualization
- **RFID Tags/Cards** - For user authentication

## Software Dependencies

### Arduino Libraries (Install via Library Manager)

**Main Brain:**
- MFRC522 (RFID library)
- LiquidCrystal_I2C
- ArduinoJson
- ESP8266WiFi
- PubSubClient (MQTT)

**Environmental Monitor:**
- DHT sensor library
- LiquidCrystal_I2C
- ArduinoJson
- ESP8266WiFi
- PubSubClient (MQTT)

**Wearable Tracker:**
- U8g2lib (OLED display)
- MPU6050
- ArduinoJson
- ESP8266WiFi
- PubSubClient (MQTT)

## Prerequisites

1. **Arduino IDE** - Download and install from https://www.arduino.cc/en/software
2. **ESP8266 Board Package** - Install ESP8266 boards in Arduino IDE:
   - File → Preferences → Additional Board Manager URLs: `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
   - Tools → Board → Boards Manager → Search "ESP8266" → Install
3. **USB Cable** - Micro USB cable to connect ESP8266 to computer
4. **CH340/CP2102 Drivers** - USB-to-serial drivers (if needed for your ESP8266)

## Network Configuration

Before uploading, you need to configure network settings in each device's `config.h` file:

### WiFi Settings
```cpp
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
```

### MQTT Broker Settings
```cpp
const char* MQTT_SERVER = "192.168.1.xxx";  // Your MQTT broker IP
const int MQTT_PORT = 1883;
const char* MQTT_USER = "bille_mqtt";       // Your MQTT username
const char* MQTT_PASSWORD = "your_password"; // Your MQTT password
```

## Uploading Projects

### Main Brain Project
1. Open `sketches/main_brain/main_brain.ino` in Arduino IDE
2. Edit `config.h` with your WiFi and MQTT credentials
3. Select **Tools > Board > ESP8266 Boards > NodeMCU 1.0 (ESP-12E Module)**
4. Select the correct **Port** under **Tools > Port**
5. Set **Upload Speed** to 115200
6. Click **Upload** button or press Ctrl+U

### Environmental Monitor Project
1. Open `sketches/environment_monitor/environment_monitor.ino` in Arduino IDE
2. Edit `config.h` with your WiFi and MQTT credentials
3. Select **Tools > Board > ESP8266 Boards > NodeMCU 1.0 (ESP-12E Module)**
4. Select the correct **Port** under **Tools > Port**
5. Set **Upload Speed** to 115200
6. Click **Upload** button or press Ctrl+U

### Wearable Tracker Project
1. Open `sketches/wearable_tracker/wearable_tracker.ino` in Arduino IDE
2. Edit `config.h` with your WiFi and MQTT credentials
3. Select **Tools > Board > ESP8266 Boards > NodeMCU 1.0 (ESP-12E Module)**
4. Select the correct **Port** under **Tools > Port**
5. Set **Upload Speed** to 115200
6. Click **Upload** button or press Ctrl+U

## System Functionality

### Main Brain Functions
- **RFID Authentication**: Scan card/tag to start/stop work sessions
- **Pomodoro Timer**: 25-minute work sessions with 5-minute breaks
- **Touch Control**: Manual timer start/stop
- **Audio Feedback**: Buzzer alerts for session events
- **Session Tracking**: Monitors work duration and cycle count
- **Data Aggregation**: Receives and displays data from other devices

### Environmental Monitor Functions
- **Temperature Monitoring**: DHT11 sensor (0-50°C range)
- **Humidity Tracking**: Real-time humidity percentage
- **Light Level Detection**: Ambient light measurement in lux
- **Noise Monitoring**: Sound level detection
- **Automatic Fan Control**:
  - Fan turns ON when temperature ≥ 24°C
  - Fan turns OFF when temperature ≤ 22°C
  - Manual override available via MQTT
- **Environmental Alerts**: Warnings for suboptimal conditions
- **LCD Display**: Real-time environmental data

### Wearable Tracker Functions
- **Step Counting**: Advanced accelerometer-based algorithm
- **Activity Detection**: Classifies activity as sitting, still, moving, walking, or running
- **Movement Reminders**: Alerts after 20+ minutes of inactivity during work
- **Break Compliance**: Encourages movement during Pomodoro breaks
- **OLED Display Modes**:
  - Mode 0: Biometric overview (steps, activity, session context)
  - Mode 1: Detailed activity status (acceleration, movement time)
  - Mode 2: Pomodoro progress (timer, cycles, percentage)
  - Mode 3: Break compliance tracking
- **Button Control**: Cycle through display modes

## MQTT Topics

### Main Brain (Publisher)
- `bille/session/state` - Session status and user information
- `bille/pomodoro/state` - Timer state and progress
- `bille/status/system` - System health monitoring
- `bille/alerts/movement` - Movement reminders

### Environmental Monitor (Publisher)
- `bille/data/environment` - Combined environmental data
- `bille/sensors/temperature` - Temperature readings
- `bille/sensors/humidity` - Humidity percentage
- `bille/sensors/light` - Light level in lux
- `bille/sensors/noise` - Noise level
- `bille/sensors/fan_state` - Fan on/off status
- `bille/alerts/environment` - Environmental quality alerts

### Wearable Tracker (Publisher)
- `bille/data/biometric` - Complete biometric data package
- `bille/sensors/steps` - Step count
- `bille/sensors/activity` - Current activity classification
- `bille/alerts/health` - Health and movement alerts

## Home Assistant Integration

The system includes Home Assistant configuration files in `sketches/HA_config files/sensors.yaml`:

1. Copy the contents of `sensors.yaml` to your Home Assistant configuration
2. Restart Home Assistant
3. All Bill-E sensors will appear in your Home Assistant dashboard
4. Create custom dashboards with cards for:
   - Environmental conditions
   - Pomodoro timer progress
   - Health metrics and step count
   - Fan control switches

## Pin Connections

### Main Brain (ESP8266-1)
- D1 → RFID RST
- D2 → RFID SS
- D0 → Touch Sensor
- D8 → Buzzer
- D3 → LCD SDA (I2C)
- D4 → LCD SCL (I2C)
- D7 → RFID MOSI
- D6 → RFID MISO
- D5 → RFID SCK

### Environmental Monitor (ESP8266-2)
- D6 → DHT11 Data
- A0 → Light/Noise Sensor (Analog)
- D5 → Sound Digital Output
- D7 → Relay IN (Fan Control)
- D1 → LCD SDA (I2C)
- D2 → LCD SCL (I2C)

### Wearable Tracker (ESP8266-3)
- D2 → OLED SDA + MPU6050 SDA (I2C)
- D1 → OLED SCL + MPU6050 SCL (I2C)
- D3 → Push Button

## Troubleshooting

### Upload Issues
- **Port not showing**: Check USB cable connection and install CH340 or CP2102 drivers if needed
- **Upload fails**: Try holding the FLASH button on ESP8266 while clicking upload
- **Board not found**: Ensure ESP8266 board package is properly installed
- **Compilation errors**: Verify all required libraries are installed

### Hardware Issues
- **RFID not working**: Check SPI connections and verify 3.3V power (not 5V)
- **LCD blank**: Verify I2C address (default 0x27, some use 0x3F)
- **MPU6050 not detected**: Check I2C connections and run I2C scanner
- **Fan not working**: Verify relay module is powered by 5V and fan by 12V

### Network Issues
- **WiFi won't connect**: Verify SSID and password in config.h
- **MQTT not connecting**: Check broker IP address and credentials
- **Devices offline**: Ensure MQTT broker is running and accessible

### Sensor Issues
- **DHT11 reading errors**: Wait 2 seconds between readings, check data pin
- **Steps not counting**: Calibrate MPU6050, verify it's mounted securely
- **Light sensor inaccurate**: Adjust calibration values in sensor_reader.cpp

## Project Structure

```
final/
├── README.md
├── sketches/
│   ├── main_brain/
│   │   ├── main_brain.ino          # Main program
│   │   ├── config.h                # Network configuration
│   │   ├── rfid_manager.h/cpp      # RFID authentication
│   │   ├── pomodoro_timer.h/cpp    # Timer logic
│   │   ├── display_manager.h/cpp   # LCD control
│   │   ├── audio_system.h/cpp      # Buzzer control
│   │   ├── mqtt_handler.h/cpp      # MQTT communication
│   │   └── data_analysis.h/cpp     # Data processing
│   │
│   ├── environment_monitor/
│   │   ├── environment_monitor.ino # Main program
│   │   ├── config.h                # Network configuration
│   │   ├── sensor_reader.h/cpp     # Sensor reading
│   │   ├── display_controller.h/cpp# LCD display
│   │   ├── mqtt_client.h/cpp       # MQTT communication
│   │   └── environmental_analysis.h/cpp # Fan control & alerts
│   │
│   ├── wearable_tracker/
│   │   ├── wearable_tracker.ino    # Main program
│   │   ├── config.h                # Network configuration
│   │   ├── biometric_sensors.h/cpp # Sensor reading
│   │   ├── display_oled.h/cpp      # OLED display
│   │   ├── mqtt_communication.h/cpp# MQTT communication
│   │   └── health_monitor.h/cpp    # Health alerts
│   │
│   └── HA_config files/
│       └── sensors.yaml            # Home Assistant config
│
└── Bill-E Focus Robot - Final report.pdf
```

## Usage Instructions

1. **Power on all three devices** - Wait for WiFi and MQTT connections
2. **Scan RFID card** on Main Brain to start a work session
3. **Pomodoro timer starts automatically** - 25-minute work session begins
4. **Monitor environment** on Environmental Monitor LCD display
5. **Check activity** on Wearable Tracker OLED display
6. **Press button** on Wearable to cycle through display modes
7. **Receive alerts** for movement reminders and break compliance
8. **Scan RFID again** to end session

## Credits

**Project:** Bill-E Focus Robot
**Student:** University of London BSc Computer Science
**Module:** CM3040 Physical Computing and Internet-of-Things (IoT)
**Year:** 2024-2025

## License

This project is submitted as coursework for the University of London BSc Computer Science program.