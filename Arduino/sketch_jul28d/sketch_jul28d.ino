#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// System state
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
  delay(10000);
  lcd.clear();
  lcd.print("Enter Code:");

  Serial.println("🔧 Initializing WiFi...");
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
        Serial.println(isArmed ? "✅ System Armed" : "✅ System Disarmed");
        updateFirebaseStatus(isArmed ? "Armed" : "Disarmed");
        delay(1000);
        lcd.clear();
        lcd.print("Enter Code:");
        intruderDetected = false;
        digitalWrite(buzzerPin, LOW);
      } else {
        lcd.clear();
        lcd.print("Wrong Code!");
        Serial.println("❌ Wrong Code Entered");
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
    delay(100); // debounce
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
  Serial.println("🚨 Intruder Detected!");
  digitalWrite(buzzerPin, HIGH);
  updateFirebaseStatus("Intruder Alert");
  sendWiFiAlert();
  intruderDetected = true;
  buzzerStartTime = millis();
}

void sendAT(String cmd, int wait = 2000) {
  Serial.print("➡️ Sending: ");
  Serial.println(cmd);
  esp.println(cmd);
  delay(wait);

  while (esp.available()) {
    String response = esp.readStringUntil('\n');
    Serial.print("⬅️ ESP: ");
    Serial.println(response);
  }
}

void initWiFi() {
  sendAT("AT+RST", 3000);
  sendAT("AT");
  sendAT("AT+CWMODE=1");
  sendAT("AT+CWJAP=\"el pepe\",\"3guys1cup\"", 8000); // Change SSID/PASS
  sendAT("AT+CIFSR");
}

void sendWiFiAlert() {
  String botToken = "7961952677:AAF1moKF-FOMdqzjziN6WFI9-sKeFf7qCB4";
  String chatID = "1677478547";
  String message = "🚨 Intruder Alert! System has been triggered.";

  message.replace(" ", "%20");
  message.replace("!", "%21");
  message.replace("🚨", "%F0%9F%9A%A8");

  String host = "api.telegram.org";
  String url = "/bot" + botToken + "/sendMessage?chat_id=" + chatID + "&text=" + message;

  // Confirm host value
  Serial.println("👀 Host: " + host);
  sendAT("AT+CIPSTART=\"TCP\",\"" + host + "\",80", 2000);

  String request = "GET " + url + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";

  esp.print("AT+CIPSEND=");
  esp.println(request.length());
  delay(1000);

  bool readyToSend = false;
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) {
    if (esp.find(">")) {
      readyToSend = true;
      break;
    }
  }

  if (!readyToSend) {
    Serial.println("❌ ESP not ready to send");
    lcd.clear();
    lcd.print("Alert Failed!");
    return;
  }

  delay(500); // Ensure ESP is ready
  esp.print(request);
  delay(3000);

  bool error = false;
  while (esp.available()) {
    String line = esp.readStringUntil('\n');
    Serial.println("⬅️ ESP Response: " + line);
    if (line.indexOf("ERROR") >= 0 || line.indexOf("403") >= 0 || line.indexOf("401") >= 0) {
      error = true;
    }
    if (line.indexOf("200 OK") >= 0 || line.indexOf("message_id") >= 0) {
      error = false;
    }
  }

  lcd.clear();
  lcd.print(error ? "Alert Failed!" : "Alert Sent!");
  Serial.println(error ? "❌ Telegram Alert Failed" : "✅ Telegram Alert Sent");
}

void updateFirebaseStatus(String status) {
  String firebaseHost = "your-project-id.firebaseio.com"; // Replace with real host
  String secret = "YOUR_DATABASE_SECRET";                 // Replace with real secret

  String path = "/intruderStatus.json?auth=" + secret;
  String data = "{\"status\":\"" + status + "\"}";

  sendAT("AT+CIPSTART=\"TCP\",\"" + firebaseHost + "\",80", 2000);
  String header = "PUT " + path + " HTTP/1.1\r\nHost: " + firebaseHost +
                  "\r\nContent-Type: application/json\r\nContent-Length: " + data.length() +
                  "\r\n\r\n" + data;

  esp.print("AT+CIPSEND=");
  esp.println(header.length());
  delay(1000);
  esp.print(header);
  delay(3000);

  while (esp.available()) {
    String response = esp.readStringUntil('\n');
    Serial.println("⬅️ Firebase Response: " + response);
  }
}