# intruder_alarm
Hardware used: 
  Arduino Uno
  LiquidCrystal_I2C
  Keypad
  Infrared sensor
  ESP 01, wifi module
  ESP 32, camera module
  Wifi/Hotspot device

# applications used
  VSCode
  Arduino IDE (Arduino board & ESP32 module)
  BotFather (To display messages on telegram)
    .Modify bot token and chat id in Arduino code
  Thinkspeak (Controlling intruder alarm via application)
  Firebase (Realtime database)
  
# code (run npm install "module_name")
Modules required:
  Node.js
  express

To host website, run: "node app.js"

# login page (currently hardcoded, no backend)
  User details:
    username: admin
    password: 123
    username: user1
    password: password

# userInterface page (currently hardcoded, no backend)
  Pin: 123456

# STEP-BY-STEP FLOW
🔐 1. User Logs In
User visits http://localhost:3000/login.html

Enters username & password (admin / 1234)

Server creates a session

User is redirected to userInterface.html

🟢 2. User Arms the System
Clicks “Arm” button on frontend

Frontend makes a POST /arm or GET /api/arm to backend (you'll create this)

Backend sends a WiFi command (e.g. via serial or WiFi HTTP) to ESP-01 (optional, since Arduino already arms manually via keypad too)

🔴 3. Intruder Detected
IR sensor on Arduino outputs LOW when motion is detected

Arduino:

Turns on buzzer

Shows INTRUDER ALERT! on LCD

Sends HTTP request from ESP-01 to your Node.js server (e.g. /api/intrusion)

cpp
Copy code
GET /api/intrusion?msg=alert
Sets a flag to prevent multiple alerts

📸 4. ESP32-CAM Captures Image
If connected via WiFi (separate), it can:

Be triggered by Arduino via an HTTP request to ESP32-CAM’s IP (using ESP-01)

Or Arduino can broadcast a request to your Node.js backend, which in turn sends a request to ESP32-CAM

ESP32-CAM saves image or streams it to a folder, Firebase, or directly to the server

🌐 5. Website Gets Alert
Node.js backend receives /api/intrusion

It updates:

Logs

Sends alert to frontend via fetch polling, Socket.io, or refresh

(Optional) triggers UI flash or sound on frontend

🛑 6. User Disarms
Via keypad on Arduino (# key + correct passcode)

Or clicks “Disarm” on website

Website sends POST /disarm

Arduino receives it (if 2-way communication via ESP-01 or polling)

Turns off buzzer and resets