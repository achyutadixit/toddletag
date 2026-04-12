#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define RX_PIN 10
#define TX_PIN 11
#define RST_PIN 9

Adafruit_SSD1306 display(128, 32, &Wire, -1);
SoftwareSerial uwb(RX_PIN, TX_PIN);

String incoming = "";
float distance = 0;
bool connected = false;
unsigned long lastRanging = 0;

void setup() {
  Serial.begin(115200);
  
  // 1. Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;); // Halt if OLED fails
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("BOOTING ANCHOR..."));
  display.display();

  // 2. Hardware Reset UWB
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, LOW);  delay(100);
  digitalWrite(RST_PIN, HIGH); delay(1000);

  // 3. Start Serial at 9600 (CRITICAL FOR NANO)
  uwb.begin(9600); 

  // 4. Configure Module
  uwb.println(F("AT+NETWORKID=AABBCCDD"));
  delay(200);
  uwb.println(F("AT+ADDRESS=ANCHOR01"));
  delay(200);
  uwb.println(F("AT+MODE=1"));
  
  display.clearDisplay();
  display.println(F("ANCHOR READY"));
  display.display();
}

void loop() {
  // Request ranging every 500ms
  if (millis() - lastRanging >= 500) {
    lastRanging = millis();
    uwb.println(F("AT+ANCHOR_SEND=TAG00001,4,PING"));
  }

  // Process incoming data
  while (uwb.available()) {
    char c = uwb.read();
    if (c == '\n') {
      incoming.trim();
      if (incoming.startsWith(F("+ANCHOR_RCV"))) {
        // Parse: +ANCHOR_RCV=TAG00001,4,PING,155,RSSI
        int c1 = incoming.indexOf(',');
        int c2 = incoming.indexOf(',', c1 + 1);
        int c3 = incoming.indexOf(',', c2 + 1);
        int c4 = incoming.indexOf(',', c3 + 1);
        if (c3 >= 0) {
          String distStr = (c4 >= 0) ? incoming.substring(c3 + 1, c4) : incoming.substring(c3 + 1);
          distance = distStr.toFloat() / 100.0; // cm to meters
          connected = true;
          lastRanging = millis(); // Refresh timeout
        }
      }
      incoming = "";
    } else if (c != '\r') {
      incoming += c;
    }
  }

  // Timeout if no signal for 2 seconds
  if (connected && (millis() - lastRanging > 2000)) {
    connected = false;
  }

  // UI Update (Optimized)
  static unsigned long lastUI = 0;
  if (millis() - lastUI > 200) {
    lastUI = millis();
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(1);
    display.print(F("TAG: "));
    display.println(connected ? F("CONNECTED") : F("SEARCHING..."));
    display.setTextSize(2);
    display.setCursor(0,14);
    if(connected) {
      display.print(distance); display.print(F("m"));
    } else {
      display.print(F("---"));
    }
    display.display();
  }
}
