#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// System state
bool intruderDetected = false;
unsigned long buzzerStartTime = 0;
const unsigned long buzzerDuration = 5000; // 5 seconds

LiquidCrystal_I2C lcd(0x3F, 16, 2);  // I2C address 0x3F
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

// System state
bool isArmed = false;
String passcode = "1234";
String inputBuffer = "";

// ESP Wi-Fi Serial
SoftwareSerial esp(2, 3); // RX, TX

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
}

void handleKeypad() {
  char key = keypad.getKey();
  if (key) {
    if (key == '#') {
      if (inputBuffer == passcode) {
        isArmed = !isArmed;
        lcd.clear();
        lcd.print(isArmed ? "System Armed" : "System Disarmed");
        delay(1000);
        lcd.clear();
        lcd.print("Enter Code:");
        intruderDetected = false;
        digitalWrite(buzzerPin, LOW);
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
    delay(100); // debounce check
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
  sendWiFiAlert();
  intruderDetected = true;
  buzzerStartTime = millis();
}

void sendAT(String cmd, int wait = 2000) {
  esp.println(cmd);
  delay(wait);
}

void initWiFi() {
  sendAT("AT+RST", 3000);
  sendAT("AT+CWMODE=1");
  sendAT("AT+CWJAP=\"YourSSID\",\"YourPassword\"", 6000);
}

void sendWiFiAlert() {
  String host = "maker.ifttt.com";
  String url = "/trigger/intruder_alert/with/key/YOUR_IFTTT_KEY";

  sendAT("AT+CIPSTART=\"TCP\",\"" + host + "\",80", 2000);
  String request = "GET " + url + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
  esp.print("AT+CIPSEND=");
  esp.println(request.length());
  delay(1000);
  esp.print(request);
  delay(3000);

  while (esp.available()) {
    String response = esp.readString();
    if (response.indexOf("ERROR") >= 0) {
      Serial.println("ESP Alert Failed");
      lcd.clear();
      lcd.print("Alert Failed!");
      return;
    }
  }
}