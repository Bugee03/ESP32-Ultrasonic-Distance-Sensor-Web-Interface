
# ESP32 Ultrasonic Distance Sensor Web Interface

📏 Real-time distance measurement using an **ESP32** and ultrasonic sensor  
✔ Displays live distance on a web page served by the ESP32

---

## 🧠 Project Overview

This project connects an **HC-SR04 ultrasonic sensor** to an **ESP32 development board**.  
The ESP32 reads distance measurements and hosts a web interface to show the data live to any browser.

---

## 📡 Features

- Measures distance in cm using HC-SR04 ultrasonic sensor  
- Runs HTTP server on ESP32  
- Web page interface with real-time updates  
- Works over local Wi-Fi network  
- Simple “terminal-style” display with JavaScript fetch

---

## 📦 Repository Structure

src/ — C source code for ESP32
web/ — HTML + JavaScript web client
docs/ — wiring diagrams / instructions
assets/ — photos and media
README.md — project overview
.gitignore — ignored files


---

## 🔌 Hardware Requirements

1. ESP32 development board  
2. HC-SR04 ultrasonic distance sensor  
3. Jumper wires & breadboard  
4. USB cable to program the ESP32

📌 Connect HC-SR04 as follows:

- **VCC** → 5V or 3.3V (check sensor tolerance)  
- **GND** → ESP32 GND  
- **TRIG** → ESP32 GPIO25 (example)  
- **ECHO** → ESP32 GPIO33

*(Customize pins in code if needed)*

---

## 🚀 Build & Flash (ESP-IDF)

1. Install **ESP-IDF** via official setup.  
2. Configure Wi-Fi SSID and password in `src/ESP32_WPA_CONNECTION.c`.  
3. Build project:
```bash
idf.py build
idf.py flash
idf.py monitor

