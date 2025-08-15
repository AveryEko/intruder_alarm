# Intruder Alarm

A comprehensive DIY intruder alarm system using Arduino, ESP32-CAM, ThingSpeak, Telegram, and web dashboard technologies.

---

## 🚨 Overview

**Intruder Alarm** is a modular, IoT-based security system designed for home or small office use. It combines hardware sensors, wireless communication, cloud logging, instant alerts, and a modern web UI to deliver real-time protection and monitoring.

---

## 🛠️ Hardware Used

- **ESP32-CAM**: Motion detection, image capture, WiFi connectivity
- **Arduino Uno/Nano**: Sensor hub (PIR), communicates with ESP modules
- **ESP-01/ESP8266**: WiFi module for Arduino
- **Sensors**: PIR motion sensor, buzzer, relay
- **Input**: Keypad
- **Ouput**: LCD Screen
- **Cloud Services**:
  - **ThingSpeak**: Data logging, control, status
  - **Telegram Bot**: Instant alerts to your phone
  - **Cloudinary**: Image hosting for intruder snapshots

---

## 📦 Features

- **Remote Arm/Disarm** via web dashboard (ThingSpeak control)
- **Instant Intruder Alerts** sent to Telegram
- **Live Hardware Status** (heartbeat monitoring of ESP32 and Arduino)
- **Snapshot Gallery**: Intruder images captured and uploaded to Cloudinary, viewable in the web dashboard
- **Cloud Logging**: All status and events recorded to ThingSpeak for historical review and control
- **Modular Design**: Easily add/remove sensors or notification channels

---

## 🖥️ Web Dashboard

- Built with **HTML, CSS, JavaScript** (Bootstrap for styling)
- Controls for arming/disarming both ESP32-CAM and Arduino
- Displays hardware online/offline status
- Shows recent intruder alerts and snapshot gallery
- Responsive layout for desktop and mobile

---

## 🗂️ Repository Structure

```
/arduino/             # Arduino Uno/Nano code for sensors and WiFi communication
/esp32-cam/           # ESP32-CAM firmware for image capture, ThingSpeak, Telegram
/public/              # Web dashboard (HTML, CSS, JS)
/public/css/          # Custom dashboard styles
/public/js/           # Dashboard scripts (UI, ThingSpeak API, gallery)
/public/UI.html       # Main dashboard page
/public/login.html    # Login/authentication page
/public/img/          # Icons and UI images
/README.md            # This file
```

---

## ⚡ How It Works

1. **Sensors** (PIR, door) trigger Arduino.
2. **Arduino** (with ESP-01) logs data/status into ThingSpeak, sends heartbeat.
3. **ESP32-CAM** checks ThingSpeak for "armed" status and intruder flag.
4. On detection, **ESP32-CAM**:
    - Captures snapshots
    - Uploads images to Cloudinary
    - Logs event and image URL to ThingSpeak
    - Sends Telegram alert to user
5. **Web Dashboard** displays live status, recent alerts, and image gallery.

---

## 📝 Setup & Installation

### Hardware

1. **Wire up sensors to Arduino** (see `/arduino/` sketches for pin mappings).
2. **Connect ESP-01 to Arduino** (serial, 3.3V, GND).
3. **Set up ESP32-CAM** on separate power (can run standalone).

### Software

1. **Flash Arduino code** from `/arduino/` folder.
2. **Flash ESP32-CAM code** from `/esp32-cam/` folder.
3. Edit WiFi, ThingSpeak, Telegram, Cloudinary credentials in code.
4. **Deploy web dashboard**: upload `/public/` folder to your web hosting.

### Cloud Services

- **ThingSpeak**: Create channel, fields for arm/disarm, heartbeat, status, alerts, snapshots.
- **Telegram Bot**: Create bot, get token, chat ID.
- **Cloudinary**: Create account, get upload preset and cloud name for image hosting.

---

## 🔒 Security Notes

- Change default passwords in all scripts and dashboard.
- Use HTTPS for web dashboard hosting.
- Secure your Telegram bot token and Cloudinary credentials.

---

## 💡 Customization

- Add more sensors (gas, vibration, etc.) via Arduino.
- Integrate SMS, email, or other notification channels.
- Extend dashboard for multi-zone support.

---

## 👤 Authors & Credits

- [AveryEko](https://github.com/AveryEko)  
- [IJIn00001](https://github.com/IJIn00001)  
- Community resources, open source libraries

---

## 📜 License

This project is open-source. See [LICENSE](LICENSE) for details.

---

## 📞 Support & Issues

Open an [issue](https://github.com/AveryEko/intruder_alarm/issues) for questions, bugs, or feature requests.
