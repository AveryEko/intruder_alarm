#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// --- CONFIGURATION ---
const char* WIFI_SSID     = "el pepe";
const char* WIFI_PASSWORD = "3guys1cup";
const char* THINGSPEAK_WRITE_API_KEY = "KOU513ED704T5O07";
const int ARDUINO_ARM_FIELD          = 6;
const int ARDUINO_INTRUDER_FIELD     = 1;
const int ARDUINO_HEARTBEAT_FIELD    = 4;

// --- SYSTEM STATE ---
bool intruderDetected = false;
bool alertSent = false;            // NEW: Only send alert once per detection
LiquidCrystal_I2C lcd(0x3F, 16, 2);
const int buzzerPin = 5;
const int irPin = 6;

// Keypad setup
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {12, 7, 8, 10};
byte colPins[COLS] = {11, 13, 9};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

bool isArmed = false;
String passcode = "1234";
String inputBuffer = "";

SoftwareSerial esp(2, 3); // RX, TX

unsigned long lastArmPoll = 0;
const unsigned long armPollInterval = 5000;

unsigned long lastHeartbeat = 0;
const unsigned long heartbeatInterval = 10000; // 10 seconds

bool pendingArmStatus = false;
bool pendingArmStatusValue = false; // true: armed, false: disarmed

bool showingIntruderScreen = false;

void setup() {
  Serial.begin(9600);
  esp.begin(9600);

  pinMode(buzzerPin, OUTPUT);
  pinMode(irPin, INPUT);

  lcd.init();
  lcd.backlight();
  lcd.print("System Initializing");
  delay(1500);
  lcd.clear();
  lcd.print("Enter Code:");

  initWiFi();
}

void loop() {
  handleKeypad();
  checkIntrusion();

  // Poll ThingSpeak for remote arm/disarm
  if (millis() - lastArmPoll > armPollInterval) {
    lastArmPoll = millis();
    pollArmDisarmFromWeb();
  }

  // Heartbeat to ThingSpeak field4
  if (millis() - lastHeartbeat > heartbeatInterval) {
    lastHeartbeat = millis();
    sendThingSpeakHeartbeat();
  }

  // Non-blocking: send arm status if needed
  if (pendingArmStatus) {
    sendThingSpeakArmStatus(pendingArmStatusValue);
    pendingArmStatus = false;
  }
}

void handleKeypad() {
  char key = keypad.getKey();
  if (key) {
    Serial.print("Key pressed: "); Serial.println(key);
    // When intruder detected, always show code entry screen
    if (intruderDetected && !showingIntruderScreen) {
      lcd.clear();
      lcd.print("Enter Code:");
      showingIntruderScreen = true;
    }
    if (key == '#') {
      if (inputBuffer == passcode) {
        if (intruderDetected) {
          isArmed = false;
          intruderDetected = false;
          alertSent = false; // Reset for next detection
          digitalWrite(buzzerPin, LOW); // TURN OFF BUZZER HERE!
          lcd.clear();
          lcd.print("System Disarmed");
          pendingArmStatusValue = isArmed;
          pendingArmStatus = true;
          delay(1500);
          lcd.clear();
          lcd.print("Enter Code:");
          showingIntruderScreen = false;
        } else {
          isArmed = !isArmed;
          lcd.clear();
          lcd.print(isArmed ? "System Armed" : "System Disarmed");
          pendingArmStatusValue = isArmed;
          pendingArmStatus = true;
          delay(1500);
          lcd.clear();
          lcd.print("Enter Code:");
        }
      } else {
        lcd.clear();
        lcd.print("Wrong Code!");
        delay(1500);
        lcd.clear();
        lcd.print("Enter Code:");
      }
      inputBuffer = "";
    } else if (key == '*') {
      inputBuffer = "";
      lcd.setCursor(0, 1);
      lcd.print("                ");
    } else {
      inputBuffer += key;
      lcd.setCursor(0, 1);
      lcd.print(inputBuffer);
    }
  }
}

void checkIntrusion() {
  // Detection logic: Only trigger if armed and IR sensor is LOW and not already in alert
  if (isArmed && digitalRead(irPin) == LOW && !intruderDetected) {
    delay(30); // minimal debounce
    if (digitalRead(irPin) == LOW) {
      triggerIntruderAlert();
    }
  }

  // Buzzer logic: KEEP BUZZING as long as intruderDetected
  if (intruderDetected) {
    digitalWrite(buzzerPin, HIGH); // BUZZER ON
    // Only send alert ONCE per detection
    if (!alertSent) {
      sendThingSpeakAlert();
      alertSent = true;
    }
    // No timeout logic!
  }
}

void triggerIntruderAlert() {
  lcd.clear();
  lcd.print("INTRUDER ALERT!");
  // Buzzer stays on until disarmed, so don't start/stop here
  intruderDetected = true;
  showingIntruderScreen = false; // Will switch to code screen on next keypad event
  // alertSent is handled in checkIntrusion
}

// Remaining functions unchanged...
void sendAT(String cmd, int wait = 400) {
  esp.println(cmd);
  unsigned long tStart = millis();
  while (millis() - tStart < wait) {
    if (esp.available()) {
      String response = esp.readStringUntil('\n');
      Serial.println(response);
    }
  }
}

void initWiFi() {
  sendAT("AT+RST", 800);
  sendAT("AT+CWMODE=1", 300);
  String wifiCmd = "AT+CWJAP=\"" + String(WIFI_SSID) + "\",\"" + String(WIFI_PASSWORD) + "\"";
  sendAT(wifiCmd, 1200);
  sendAT("AT+CWJAP?", 300);
}

void sendThingSpeakAlert() {
  String host = "api.thingspeak.com";
  String url = "/update?api_key=" + String(THINGSPEAK_WRITE_API_KEY) + "&field" + String(ARDUINO_INTRUDER_FIELD) + "=1";
  Serial.println("Sending alert to ThingSpeak: " + url);
  sendAT("AT+CIPSTART=\"TCP\",\"" + host + "\",80", 400);

  String request = "GET " + url + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
  esp.print("AT+CIPSEND=");
  esp.println(request.length());
  delay(150);

  // Wait for '>' prompt for up to 400ms
  unsigned long tStart = millis();
  bool gotPrompt = false;
  while (millis() - tStart < 400) {
    if (esp.available()) {
      String line = esp.readStringUntil('\n');
      Serial.println(line);
      if (line.indexOf(">") >= 0) {
        gotPrompt = true;
        break;
      }
    }
  }
  if (gotPrompt) {
    esp.print(request);
    delay(200);
    while (esp.available()) {
      String line = esp.readStringUntil('\n');
      Serial.println(line);
    }
  } else {
    Serial.println("ERROR: No > prompt from ESP-01");
    lcd.clear();
    lcd.print("Alert Failed!");
    delay(150);
    lcd.clear();
    lcd.print("Enter Code:");
  }
  sendAT("AT+CIPCLOSE", 200);
}

void sendThingSpeakArmStatus(bool armed) {
  String host = "api.thingspeak.com";
  String url = "/update?api_key=" + String(THINGSPEAK_WRITE_API_KEY) + "&field" + String(ARDUINO_ARM_FIELD) + "=" + (armed ? "1" : "0");
  Serial.println("Sending to ThingSpeak: " + url); // Add this
  sendAT("AT+CIPSTART=\"TCP\",\"" + host + "\",80", 400);

  String request = "GET " + url + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
  esp.print("AT+CIPSEND=");
  esp.println(request.length());
  delay(150);

  unsigned long tStart = millis();
  bool gotPrompt = false;
  while (millis() - tStart < 400) {
    if (esp.available()) {
      String line = esp.readStringUntil('\n');
      Serial.println(line);
      if (line.indexOf(">") >= 0) {
        gotPrompt = true;
        break;
      }
    }
  }
  if (gotPrompt) {
    esp.print(request);
    delay(200);
    while (esp.available()) {
      String line = esp.readStringUntil('\n');
      Serial.println(line);
    }
  }
  sendAT("AT+CIPCLOSE", 200);
}

void sendThingSpeakHeartbeat() {
  String host = "api.thingspeak.com";
  String url = "/update?api_key=" + String(THINGSPEAK_WRITE_API_KEY) + "&field" + String(ARDUINO_HEARTBEAT_FIELD) + "=1";
  sendAT("AT+CIPSTART=\"TCP\",\"" + host + "\",80", 400);

  String request = "GET " + url + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
  esp.print("AT+CIPSEND=");
  esp.println(request.length());
  delay(150);

  unsigned long tStart = millis();
  bool gotPrompt = false;
  while (millis() - tStart < 400) {
    if (esp.available()) {
      String line = esp.readStringUntil('\n');
      Serial.println(line);
      if (line.indexOf(">") >= 0) {
        gotPrompt = true;
        break;
      }
    }
  }
  if (gotPrompt) {
    esp.print(request);
    delay(200);
    while (esp.available()) {
      String line = esp.readStringUntil('\n');
      Serial.println(line);
    }
  }
  sendAT("AT+CIPCLOSE", 200);
}

void pollArmDisarmFromWeb() {
  sendAT("AT+CIPCLOSE", 100);
  sendAT("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80", 400);

  String request = "GET /channels/3033231/fields/6/last.json?api_key=N6598QTZTIAWCV0H HTTP/1.1\r\nHost: api.thingspeak.com\r\nConnection: close\r\n\r\n";
  esp.print("AT+CIPSEND=");
  esp.println(request.length());

  unsigned long tStart = millis();
  bool gotPrompt = false;
  while (millis() - tStart < 400) {
    if (esp.available() && esp.read() == '>') {
      gotPrompt = true;
      break;
    }
  }
  if (!gotPrompt) {
    Serial.println("ERROR: No > prompt from ESP-01");
    return;
  }

  esp.print(request);

  String jsonLine = "";
  unsigned long respStart = millis();
  bool foundJson = false;
  while (millis() - respStart < 1000) {
    if (esp.available()) {
      String line = esp.readStringUntil('\n');
      line.trim();
      if (line.startsWith("{") && line.endsWith("}")) {
        jsonLine = line;
        foundJson = true;
        break;
      }
    }
  }
  sendAT("AT+CIPCLOSE", 100);

  if (foundJson) {
    Serial.print("JSON: "); Serial.println(jsonLine);
    int fieldIdx = jsonLine.indexOf("\"field6\":\"");
    if (fieldIdx != -1) {
      char val = jsonLine.charAt(fieldIdx + 10);
      bool remoteArmed = (val == '1');
      if (remoteArmed != isArmed) {
        isArmed = remoteArmed;
        lcd.clear();
        lcd.print(isArmed ? "System Armed" : "System Disarmed");
        Serial.println(isArmed ? "✅ Armed via Web" : "✅ Disarmed via Web");
        delay(150);
        lcd.clear();
        lcd.print("Enter Code:");
        intruderDetected = false;
        alertSent = false;
        digitalWrite(buzzerPin, LOW);
        showingIntruderScreen = false;
      }
    }
  } else {
    Serial.println("ERROR: JSON not found in HTTP response.");
  }
}