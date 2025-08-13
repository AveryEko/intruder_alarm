#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// =====================
// --- CONFIGURATION ---
// =====================
const char* WIFI_SSID     = "Lovely Family";
const char* WIFI_PASSWORD = "24022004";

const char* THINGSPEAK_READ_API_KEY  = "N6598QTZTIAWCV0H";
const char* THINGSPEAK_WRITE_API_KEY = "KOU513ED704T5O07";
const char* THINGSPEAK_CHANNEL_ID    = "3033231";
const int ARDUINO_ARM_FIELD          = 6;
const int ARDUINO_INTRUDER_FIELD     = 1;
const int ARDUINO_HEARTBEAT_FIELD    = 4; // <--- heartbeat field

// =====================
// --- SYSTEM STATE  ---
// =====================
bool intruderDetected = false;
unsigned long buzzerStartTime = 0;
const unsigned long buzzerDuration = 5000;

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

void setup() {
  Serial.begin(9600);
  esp.begin(9600);

  pinMode(buzzerPin, OUTPUT);
  pinMode(irPin, INPUT);

  lcd.init();
  lcd.backlight();
  lcd.print("System Initializing");
  delay(2000);
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
}

void handleKeypad() {
  char key = keypad.getKey();
  if (key) {
    if (key == '#') {
      if (inputBuffer == passcode) {
        if (intruderDetected) {
          isArmed = false;
          intruderDetected = false;
          digitalWrite(buzzerPin, LOW);
          lcd.clear();
          lcd.print("System Disarmed");
          delay(1000);
          lcd.clear();
          lcd.print("Enter Code:");
        } else {
          isArmed = !isArmed;
          lcd.clear();
          lcd.print(isArmed ? "System Armed" : "System Disarmed");
          delay(1000);
          lcd.clear();
          lcd.print("Enter Code:");
        }
      } else {
        lcd.clear();
        lcd.print("Wrong Code!");
        delay(1000);
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
  if (isArmed && digitalRead(irPin) == LOW && !intruderDetected) {
    delay(100);
    if (digitalRead(irPin) == LOW) {
      triggerIntruderAlert();
    }
  }

  if (intruderDetected && millis() - buzzerStartTime >= buzzerDuration) {
    digitalWrite(buzzerPin, LOW);
    lcd.clear();
    lcd.print("Enter Code:");
    intruderDetected = false;
  }
}

void triggerIntruderAlert() {
  lcd.clear();
  lcd.print("INTRUDER ALERT!");
  digitalWrite(buzzerPin, HIGH);
  sendThingSpeakAlert();
  intruderDetected = true;
  buzzerStartTime = millis();
}

void sendAT(String cmd, int wait = 2000) {
  esp.println(cmd);
  delay(wait);
  unsigned long tStart = millis();
  while (esp.available() && millis() - tStart < wait) {
    String response = esp.readStringUntil('\n');
    Serial.println(response);
  }
}

void initWiFi() {
  sendAT("AT+RST", 3000);
  sendAT("AT+CWMODE=1");
  String wifiCmd = "AT+CWJAP=\"" + String(WIFI_SSID) + "\",\"" + String(WIFI_PASSWORD) + "\"";
  sendAT(wifiCmd, 8000);
  sendAT("AT+CWJAP?", 1200);
}

void sendThingSpeakAlert() {
  String host = "api.thingspeak.com";
  String url = "/update?api_key=" + String(THINGSPEAK_WRITE_API_KEY) + "&field" + String(ARDUINO_INTRUDER_FIELD) + "=1";
  sendAT("AT+CIPSTART=\"TCP\",\"" + host + "\",80", 2500);

  String request = "GET " + url + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
  esp.print("AT+CIPSEND=");
  esp.println(request.length());

  // Wait for '>' prompt
  unsigned long tStart = millis();
  bool gotPrompt = false;
  while (millis() - tStart < 3000) {
    if (esp.available()) {
      char c = esp.read();
      if (c == '>') {
        gotPrompt = true;
        break;
      }
    }
  }
  if (gotPrompt) {
    esp.print(request);
    delay(2000);
    while (esp.available()) {
      String line = esp.readStringUntil('\n');
      Serial.println(line);
    }
  }
  sendAT("AT+CIPCLOSE", 1000);
}

// HEARTBEAT TO THINGSPEAK FIELD4
void sendThingSpeakHeartbeat() {
  String host = "api.thingspeak.com";
  String url = "/update?api_key=" + String(THINGSPEAK_WRITE_API_KEY) + "&field" + String(ARDUINO_HEARTBEAT_FIELD) + "=1";
  sendAT("AT+CIPSTART=\"TCP\",\"" + host + "\",80", 2500);

  String request = "GET " + url + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
  esp.print("AT+CIPSEND=");
  esp.println(request.length());

  // Wait for '>' prompt
  unsigned long tStart = millis();
  bool gotPrompt = false;
  while (millis() - tStart < 3000) {
    if (esp.available()) {
      char c = esp.read();
      if (c == '>') {
        gotPrompt = true;
        break;
      }
    }
  }
  if (gotPrompt) {
    esp.print(request);
    delay(2000);
    while (esp.available()) {
      String line = esp.readStringUntil('\n');
      Serial.println(line);
    }
  }
  sendAT("AT+CIPCLOSE", 1000);
}

// POLL ARM/DISARM FROM THINGSPEAK FIELD6 (CHUNKED JSON SAFE)
void pollArmDisarmFromWeb() {
  sendAT("AT+CIPCLOSE", 500);
  sendAT("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80", 3000);

  String request = "GET /channels/3033231/fields/6/last.json?api_key=N6598QTZTIAWCV0H HTTP/1.1\r\nHost: api.thingspeak.com\r\nConnection: close\r\n\r\n";
  esp.print("AT+CIPSEND=");
  esp.println(request.length());

  // Wait for '>' prompt
  unsigned long tStart = millis();
  bool gotPrompt = false;
  while (millis() - tStart < 3000) {
    if (esp.available() && esp.read() == '>') {
      gotPrompt = true;
      break;
    }
  }
  if (!gotPrompt) {
    Serial.println("ERROR: No > prompt from ESP-01");
    return;
  }

  // Send HTTP GET
  esp.print(request);

  // Read for up to 20 seconds, look for JSON
  String jsonLine = "";
  unsigned long respStart = millis();
  bool foundJson = false;
  while (millis() - respStart < 20000) {
    if (esp.available()) {
      String line = esp.readStringUntil('\n');
      line.trim(); // remove leading/trailing whitespace
      if (line.startsWith("{") && line.endsWith("}")) {
        jsonLine = line;
        foundJson = true;
        break;
      }
    }
  }
  sendAT("AT+CIPCLOSE", 500);

  if (foundJson) {
    Serial.print("JSON: "); Serial.println(jsonLine);
    int fieldIdx = jsonLine.indexOf("\"field6\":\"");
    if (fieldIdx != -1) {
      char val = jsonLine.charAt(fieldIdx + 10);
      bool remoteArmed = (val == '1');
      // Always update local state to match remote
      if (remoteArmed != isArmed) {
        isArmed = remoteArmed;
        lcd.clear();
        lcd.print(isArmed ? "System Armed" : "System Disarmed");
        Serial.println(isArmed ? "✅ Armed via Web" : "✅ Disarmed via Web");
        delay(1000);
        lcd.clear();
        lcd.print("Enter Code:");
        intruderDetected = false;
        digitalWrite(buzzerPin, LOW);
      }
    }
  } else {
    Serial.println("ERROR: JSON not found in HTTP response.");
  }
}