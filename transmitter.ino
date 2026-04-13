#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

#define RX_PIN      10
#define TX_PIN      11
#define RST_PIN      9
#define BUTTON_PIN   5

SoftwareSerial uwb(RX_PIN, TX_PIN);
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(123);

bool emgActive    = false;
bool fallActive   = false;
bool fallDetected = false;
bool lastBtn      = LOW;

unsigned long lastHeartbeat = 0;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);

  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, LOW);  delay(200);
  digitalWrite(RST_PIN, HIGH); delay(1000);

  uwb.begin(9600);
  delay(500);

  uwb.println(F("AT"));                    delay(300);
  uwb.println(F("AT+NETWORKID=AABBCCDD")); delay(300);
  uwb.println(F("AT+ADDRESS=TAG00001"));   delay(300);
  uwb.println(F("AT+MODE=0"));             delay(300);

  while (uwb.available()) uwb.read();

  if (!accel.begin(0x53)) {
    Serial.println(F("ADXL FAIL"));
    while (1);
  }

  Serial.println(F("Child/Tag ready."));
}

void loop() {
  // ── Button toggle ─────────────────────────────────────────
  bool currentBtn = digitalRead(BUTTON_PIN);
  if (currentBtn != lastBtn) {
    delay(50);
    currentBtn = digitalRead(BUTTON_PIN);

    if (currentBtn == HIGH) {
      if (fallActive) {
        // Button clears a fall alarm
        fallActive = false;
        sendData("FALL_OFF");
        Serial.println(F("FALL cleared"));
      } else {
        // Normal EMG toggle
        emgActive = !emgActive;
        sendData(emgActive ? "EMG_ON" : "EMG_OFF");
        Serial.println(emgActive ? F("EMG ON") : F("EMG OFF"));
      }
    }
    lastBtn = currentBtn;
  }

  // ── Fall Detection ───────────────────────────────────────
  sensors_event_t event;
  accel.getEvent(&event);
  float totalAcc = sqrt(
    sq(event.acceleration.x) +
    sq(event.acceleration.y) +
    sq(event.acceleration.z)
  );

  if (totalAcc < 3.0 && !fallDetected && !fallActive) {
    delay(100);
    accel.getEvent(&event);
    float afterAcc = sqrt(
      sq(event.acceleration.x) +
      sq(event.acceleration.y) +
      sq(event.acceleration.z)
    );
    if (afterAcc > 2.0) {
      fallDetected = true;
      fallActive   = true;
      for (int i = 0; i < 5; i++) {
        sendData("FALL");
        delay(100);
      }
      Serial.println(F("FALL detected"));
    }
  }

  if (totalAcc > 8.0) fallDetected = false;

  // ── Heartbeat every 3 s — just IDLE, no EMG repeat ───────
  if (millis() - lastHeartbeat >= 3000) {
    sendData("IDLE");
    lastHeartbeat = millis();
  }
}

void sendData(const char* msg) {
  uwb.print(F("AT+TAG_SEND="));
  uwb.print(strlen(msg));
  uwb.print(F(","));
  uwb.println(msg);
  Serial.print(F("Sent: ")); Serial.println(msg);
  delay(150);
}
