# ESP8266 Project Upload Instructions

## Prerequisites

1. **Arduino IDE** - Download and install from https://www.arduino.cc/en/software
2. **ESP8266 Board Package** - Install ESP8266 boards in Arduino IDE
3. **USB Cable** - Micro USB cable to connect ESP8266 to compute

## Uploading Projects

### Main Brain Project
1. Open `main_brain/main_brain.ino` in Arduino IDE
2. Select **Tools > Board > ESP8266 Boards > NodeMCU 1.0 (ESP-12E Module)**
3. Select the correct **Port** under **Tools > Port**
4. Click **Upload** button or press Ctrl+U

### Wearable Tracker Project
1. Open `wearable_tracker/wearable_tracker.ino` in Arduino IDE
2. Select **Tools > Board > ESP8266 Boards > NodeMCU 1.0 (ESP-12E Module)**
3. Select the correct **Port** under **Tools > Port**
4. Click **Upload** button or press Ctrl+U

### Environmental Monitor Project
1. Open `environmental_monitor/environmental_monitor.ino` in Arduino IDE
2. Select **Tools > Board > ESP8266 Boards > NodeMCU 1.0 (ESP-12E Module)**
3. Select the correct **Port** under **Tools > Port**
4. Click **Upload** button or press Ctrl+U

## Troubleshooting

- **Port not showing**: Check USB cable connection and install CH340 or CP2102 drivers if needed
- **Upload fails**: Try holding the FLASH button on ESP8266 while clicking upload
- **Board not found**: Ensure ESP8266 board package is properly installed