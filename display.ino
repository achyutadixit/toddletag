#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// UART2 for UWB
HardwareSerial UWB(2);

#define RXD2 16
#define TXD2 17
#define BUZZER_PIN 18

String tagID = "";
long long distanceCM = 0;
long long rssiValue = 0;

unsigned long lastPingTime = 0;
unsigned long lastResponseTime = 0;

const unsigned long pingInterval = 1000;   // 1 second
const unsigned long timeoutLimit = 2500;   // Increased to 2.5 seconds to prevent "Tag Lost"
bool tagLost = false;
void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // start OFF
  Serial.begin(115200);

  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  delay(100);

  UWB.begin(115200, SERIAL_8N1, RXD2, TXD2);
  
  digitalWrite(5, HIGH);
  delay(1000); // Give the UWB module a second to fully boot after reset

  // --- THE FIX: Configure this module as an Anchor ---
  UWB.print("AT+MODE=1\r\n");
  delay(200); // Wait for it to apply

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
    while(1);
  }

  display.clearDisplay();
  display.setRotation(2); // Remove or change to 0 if your screen is upside down
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("UWB Anchor Ready");
  display.display();

  delay(2000);
  display.clearDisplay();
}

void loop() {

  unsigned long currentMillis = millis();

  // Send ping every second
  if (currentMillis - lastPingTime >= pingInterval) {
    sendPing();
    lastPingTime = currentMillis;
  }

  // Read incoming data
  // Read incoming data
  while (UWB.available()) {
    String line = UWB.readStringUntil('\n');
    line.trim();

    if (line.length() > 0) {
      Serial.println("RAW RX: " + line);
    }

    if (line.startsWith("+ANCHOR_RCV=")) {
      parsePacket(line);
      lastResponseTime = millis();
      updateDisplay();

      tagLost = false;                  // ✅ tag is back
      digitalWrite(BUZZER_PIN, LOW);    // ✅ stop buzzer
    }
  }

  // Check timeout
  if (millis() - lastResponseTime > timeoutLimit) {
    if (!tagLost) {                    // ✅ trigger ONLY once
      showTagLost();
      digitalWrite(BUZZER_PIN, HIGH);  // ✅ keep ON
      tagLost = true;
    }
  }
}

void sendPing() {
  String cmd = "AT+ANCHOR_SEND=AKASH001,1,Q\r\n";
  UWB.print(cmd);
  Serial.println("TX: " + cmd);
}

void parsePacket(String packet) {

  // Remove prefix
  packet.replace("+ANCHOR_RCV=", "");

  long long firstComma = packet.indexOf(',');
  tagID = packet.substring(0, firstComma);

  long long lastComma = packet.lastIndexOf(',');
  rssiValue = packet.substring(lastComma + 1).toInt();

  long long cmIndex = packet.indexOf(" cm");
  long long secondLastComma = packet.substring(0, cmIndex).lastIndexOf(',');

  distanceCM = packet.substring(secondLastComma + 1, cmIndex).toInt();

  Serial.println("---- Parsed Data ----");
  Serial.println("Tag ID: " + tagID);
  Serial.println("Distance: " + String(distanceCM) + " cm");
  Serial.println("RSSI: " + String(rssiValue));
  Serial.println("---------------------");
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("RYUW122");
  display.setTextSize(1);
  display.print(" _Lite");
  display.setCursor(90, 8);
  display.print("REYAX");
  display.setCursor(18, 17);
  display.print("->by AkashWave RF");
  display.setCursor(0, 24);
  display.print("---------------------");
  display.setCursor(0,33);
  display.setTextSize(2);
  display.print("D:");
  display.print(distanceCM);
  display.println("cm ");
  display.setTextSize(1);
  display.print("RSSI: ");
  display.print(rssiValue);
  display.println("dBm");
  display.display();
}

void showTagLost() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Tag Lost!");
  display.display();
}