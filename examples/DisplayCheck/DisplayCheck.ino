#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

constexpr uint8_t SDA_PIN = 6;
constexpr uint8_t SCL_PIN = 7;
constexpr uint8_t OLED_ADDRESS = 0x3C;

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(
    U8G2_R0, U8X8_PIN_NONE, SCL_PIN, SDA_PIN);

uint32_t lastUpdate = 0;
unsigned long counter = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  oled.setI2CAddress(OLED_ADDRESS << 1);
  oled.setBusClock(100000);
  oled.begin();
  oled.setPowerSave(0);
  oled.setContrast(140);
  Serial.println("Display check: SDA=6, SCL=7, SH1106, address 0x3C.");
}

void loop() {
  if (millis() - lastUpdate < 1000) {
    delay(1);
    return;
  }
  lastUpdate = millis();

  Wire.beginTransmission(OLED_ADDRESS);
  uint8_t result = Wire.endTransmission();
  if (result != 0) {
    Serial.print("OLED did not acknowledge. I2C result: ");
    Serial.println(result);
    return;
  }

  oled.clearBuffer();
  oled.drawFrame(0, 0, 128, 64);
  oled.setFont(u8g2_font_6x12_tf);
  oled.drawStr(8, 22, "Pocket Companion");
  oled.drawStr(8, 39, "OLED SDA6 / SCL7");
  oled.setCursor(8, 55);
  oled.print("Frame: ");
  oled.print(counter++);
  oled.sendBuffer();
  Serial.println("OLED acknowledged; frame sent.");
}
