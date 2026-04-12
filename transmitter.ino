#include <SoftwareSerial.h>

#define RX_PIN 10
#define TX_PIN 11
#define RST_PIN 9

SoftwareSerial uwb(RX_PIN, TX_PIN);

void setup() {
  Serial.begin(115200);
  
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, LOW);  delay(100);
  digitalWrite(RST_PIN, HIGH); delay(1000);

  // Start Serial at 9600 (CRITICAL FOR NANO)
  uwb.begin(9600); 

  uwb.println(F("AT+NETWORKID=AABBCCDD"));
  delay(200);
  uwb.println(F("AT+ADDRESS=TAG00001"));
  delay(200);
  uwb.println(F("AT+MODE=0")); // 0 = TAG
  
  Serial.println(F("Tag is active and listening at 9600 baud..."));
}

void loop() {
  // In TAG mode, the module handles the response automatically.
  // We just monitor the serial for debug info.
  if (uwb.available()) {
    Serial.write(uwb.read());
  }
}
