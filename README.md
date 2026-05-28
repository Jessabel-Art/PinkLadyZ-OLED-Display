# PinkLadyZ OLED Display

Custom ESP32-powered OLED animation system inspired by a modified 1992 Nissan 300ZX (Z32).

This project combines:
- embedded systems
- pixel art animation
- automotive UI concepts
- weather/time APIs
- retro cyberpunk aesthetics

Designed for a 128x64 SSD1306 OLED display using an ESP32.

---

# Features

## Boot Sequence
- startup initialization animation
- glitch transitions
- animated headlight flash
- multi-angle Z32 bitmap scenes

## Dashboard Mode
- live clock via NTP
- live weather data
- temperature display
- day/night state detection

## Idle Animations
### Cruise Mode
- animated stars
- moving scene effects
- blinking indicators

### Night Mode
- moon/stars
- rain overlay
- snow overlay
- lightning flash effects
- vaporwave grid aesthetic

### Garage Mode
- animated garage states
- taillight blinking
- parking sequence transitions

## Shutdown Animation
- garage closing sequence
- parking animation
- final sleep screen

---

# Hardware

- ESP32 Development Board
- SSD1306 128x64 OLED Display
- Breadboard / JST connectors
- Jumper wires
- USB-C ESP32 module

---

# Libraries Used

- Adafruit GFX
- Adafruit SSD1306
- ArduinoJson
- WiFi
- HTTPClient

---

# Wiring

| OLED Pin | ESP32 |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

---

# Installation

1. Install required libraries in Arduino IDE
2. Clone this repository
3. Copy `config.example.h`
4. Rename it to `config.h`
5. Add your WiFi credentials and location data
6. Upload to ESP32

---

# Privacy Setup

This project uses a private `config.h` file for WiFi and location data.

`config.h` is intentionally excluded from GitHub using `.gitignore`.

Example setup:

```cpp
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";

float latitude = 00.0000;
float longitude = -00.0000;
```

---

# Serial Test Controls

| Key | Action |
|---|---|
| p | Shutdown animation |
| b | Boot animation |
| g | Garage mode |
| d | Dashboard |
| n | Force night mode |
| c | Cruise mode |
| w | Refresh weather |

---

# Future Features

- wheel spin animation
- MPU6050 motion sensor support
- boost gauge UI
- startup sound integration
- Tamagotchi-style Z32 behavior system
- CAN bus vehicle telemetry
- OTA firmware updates

---

# Project Status

Prototype phase.

Currently running on:
- ESP32
- breadboard setup
- bitmap animation pipeline

Permanent enclosure and JST harness build planned.
