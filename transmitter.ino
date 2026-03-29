#define RXD2 16
#define TXD2 17
#define RESET_PIN 5

HardwareSerial UWB(2);

void setup() {
  Serial.begin(115200);
  UWB.begin(115200, SERIAL_8N1, RXD2, TXD2);

  // 1. Hardware Reset the UWB module to ensure a clean start
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, LOW);
  delay(100);
  digitalWrite(RESET_PIN, HIGH);
  delay(1000); // Wait for module to boot

  Serial.println("--- Starting UWB Tag Configuration ---");

  // 2. Test Connection
  sendCommand("AT");

  // 3. Set to TAG Mode (0 = Tag, 1 = Anchor)
  sendCommand("AT+MODE=0");

  // 4. Set the Tag Name (Address)
  // This MUST be exactly 8 characters and MUST match what your Anchor is looking for!
  sendCommand("AT+ADDRESS=AKASH001");

  // 5. (Optional) Enable RSSI output
  sendCommand("AT+RSSI=1");

  Serial.println("--- Tag Configuration Complete! ---");
  Serial.println("Your module is now permanently configured as a Tag.");
}

void loop() {
  // Serial Passthrough: 
  // Allows you to type manual AT commands (like "AT+ADDRESS?") 
  // into the Arduino Serial Monitor and see the Tag's reply.
  
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    UWB.print(cmd + "\r\n");
    Serial.println("Sent: " + cmd);
  }

  if (UWB.available()) {
    String response = UWB.readStringUntil('\n');
    response.trim();
    if (response.length() > 0) {
      Serial.println("UWB Reply: " + response);
    }
  }
}

// Helper function to send commands and print the response
void sendCommand(String cmd) {
  Serial.print("Sending: ");
  Serial.println(cmd);
  
  UWB.print(cmd + "\r\n");
  delay(500); // Give the module time to process and reply
  
  while (UWB.available()) {
    String response = UWB.readStringUntil('\n');
    response.trim();
    if (response.length() > 0) {
      Serial.println("Reply: " + response);
    }
  }
  Serial.println("--------------------");
}