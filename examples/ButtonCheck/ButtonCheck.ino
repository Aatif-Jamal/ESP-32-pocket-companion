#include <Arduino.h>

constexpr uint8_t BUTTON_PIN = 1;
constexpr uint32_t DEBOUNCE_MS = 30;

bool lastReading = HIGH;
bool stableState = HIGH;
uint32_t changedAt = 0;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  delay(1000);
  Serial.println("GPIO1 button check: Released. Connect button between GPIO1 and GND.");
}

void loop() {
  bool reading = digitalRead(BUTTON_PIN);
  uint32_t now = millis();

  if (reading != lastReading) {
    lastReading = reading;
    changedAt = now;
  }

  if (now - changedAt >= DEBOUNCE_MS && reading != stableState) {
    stableState = reading;
    Serial.println(stableState == LOW ? "PRESSED" : "Released");
  }

  delay(1);
}
