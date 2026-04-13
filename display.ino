#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BUZZER_PIN  12
#define RST_PIN      9

Adafruit_SSD1306 display(128, 32, &Wire, -1);
SoftwareSerial uwb(10, 11);

String incoming    = "";
String distDisplay = "---";
String statusStr   = "OK";

unsigned long lastSeen = 0;
unsigned long lastPing = 0;
unsigned long lastBeep = 0;
bool buzzerOn = false;

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) for (;;);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Starting..."));
  display.display();

  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, LOW);  delay(100);
  digitalWrite(RST_PIN, HIGH); delay(1000);

  uwb.begin(9600);
  delay(500);

  uwb.println(F("AT"));                    delay(300);
  uwb.println(F("AT+NETWORKID=AABBCCDD")); delay(300);
  uwb.println(F("AT+ADDRESS=ANCHOR01"));   delay(300);
  uwb.println(F("AT+MODE=1"));             delay(300);

  while (uwb.available()) uwb.read();

  lastSeen = millis();
  Serial.println(F("Parent/Anchor ready."));
}

void loop() {
  // Always ping — anchor holds alarm state independently
  if (millis() - lastPing >= 1000) {
    lastPing = millis();
    uwb.println(F("AT+ANCHOR_SEND=TAG00001,4,PING"));
  }

  while (uwb.available()) {
    char c = uwb.read();
    if (c == '\n') {
      incoming.trim();
      if (incoming.length() > 0) {
        Serial.println(incoming);
        parseIncoming(incoming);
      }
      incoming = "";
    } else if (c != '\r') {
      incoming += c;
    }
  }

  bool connected = (millis() - lastSeen < 8000);

  // ── Buzzer ───────────────────────────────────────────────
  if (statusStr == "EMG") {
    digitalWrite(BUZZER_PIN, HIGH);

  } else if (statusStr == "FALL") {
    if (millis() - lastBeep >= 200) {
      buzzerOn = !buzzerOn;
      digitalWrite(BUZZER_PIN, buzzerOn);
      lastBeep = millis();
    }

  } else if (!connected) {
    if (millis() - lastBeep >= 500) {
      buzzerOn = !buzzerOn;
      digitalWrite(BUZZER_PIN, buzzerOn);
      lastBeep = millis();
    }

  } else {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerOn = false;
  }

  updateDisplay(connected);
}

void parseIncoming(const String& line) {
  if (line.startsWith(F("+ANCHOR_RCV=TAG00001"))) {
    lastSeen = millis();

    int c1 = line.indexOf(',');
    int c2 = line.indexOf(',', c1 + 1);
    int c3 = line.indexOf(',', c2 + 1);

    String payload = "";
    if (c2 != -1 && c3 != -1) {
      payload = line.substring(c2 + 1, c3);
      payload.trim();
    }

    if (c3 != -1) {
      String distRaw = line.substring(c3 + 1);
      distRaw.trim();
      if (distRaw.length() > 0) distDisplay = distRaw;
    }

    if (payload.length() > 0) {
      Serial.print(F("Payload: ")); Serial.println(payload);

      if (payload == "EMG_ON") {
        statusStr = "EMG";
        Serial.println(F(">> EMG ON"));

      } else if (payload == "EMG_OFF") {
        statusStr = "OK";
        digitalWrite(BUZZER_PIN, LOW);
        buzzerOn = false;
        Serial.println(F(">> EMG OFF"));

      } else if (payload == "FALL") {
        statusStr = "FALL";
        Serial.println(F(">> FALL"));

      } else if (payload == "FALL_OFF") {
        statusStr = "OK";
        digitalWrite(BUZZER_PIN, LOW);
        buzzerOn = false;
        Serial.println(F(">> FALL cleared"));
      }
    }
  }
}

void updateDisplay(bool connected) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  if (statusStr == "FALL") {
    display.setTextSize(1); display.println(F("!! FALL DETECTED !!"));
    display.setTextSize(2); display.print(F("FALL!"));
  } else if (statusStr == "EMG") {
    display.setTextSize(1); display.println(F("!! PANIC BUTTON !!"));
    display.setTextSize(2); display.print(F("ALARM"));
  } else if (!connected) {
    display.setTextSize(1); display.println(F("NO SIGNAL"));
    display.setTextSize(2); display.print(F("OFFLINE"));
  } else {
    display.setTextSize(1); display.println(F("DISTANCE:"));
    display.setTextSize(2); display.print(distDisplay);
  }

  display.display();
}
