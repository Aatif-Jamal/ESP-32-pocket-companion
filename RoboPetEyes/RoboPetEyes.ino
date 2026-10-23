#include <Arduino.h>
#include <U8g2lib.h>
#include <esp_system.h>
#include <math.h>

constexpr uint8_t SDA_PIN = 6;
constexpr uint8_t SCL_PIN = 7;
constexpr uint8_t PET_BUTTON_PIN = 1;
constexpr uint8_t EXPRESSION_COUNT = 8;
constexpr uint32_t FRAME_MS = 33;
constexpr uint32_t EXPRESSION_MS = 4500;
constexpr uint32_t DEBOUNCE_MS = 30;
constexpr uint32_t PET_DURATION_MS = 5000;
constexpr uint32_t FEED_DURATION_MS = 6000;

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(
    U8G2_R0, U8X8_PIN_NONE, SCL_PIN, SDA_PIN);

const char* expressionNames[] = {
  "curious", "happy", "angry", "sad",
  "surprised", "sleepy", "smug", "love"
};
const uint8_t eyeWidths[] = {30, 34, 34, 30, 24, 32, 30, 30};
const uint8_t leftHeights[] = {34, 28, 29, 29, 40, 8, 28, 30};
const uint8_t rightHeights[] = {25, 28, 29, 29, 40, 8, 6, 30};

uint8_t expression = 0;
uint8_t reaction = 0;
float eyeWidth = 30;
float leftHeight = 34;
float rightHeight = 34;
float gazeX = 0;
float gazeY = 0;
float targetGazeX = 0;
float targetGazeY = 0;
bool rawButton = false;
bool stableButton = false;
bool blinking = false;
uint32_t buttonChangedAt = 0;
uint32_t reactionStartedAt = 0;
uint32_t lastFrameAt = 0;
uint32_t lastExpressionAt = 0;
uint32_t lastLookAt = 0;
uint32_t lookInterval = 1500;
uint32_t lastBlinkAt = 0;
uint32_t blinkInterval = 2500;
uint32_t blinkStartedAt = 0;

bool buttonPressed(uint32_t now) {
  bool reading = digitalRead(PET_BUTTON_PIN) == LOW;
  if (reading != rawButton) {
    rawButton = reading;
    buttonChangedAt = now;
  }
  if (now - buttonChangedAt >= DEBOUNCE_MS && reading != stableButton) {
    stableButton = reading;
    return stableButton;
  }
  return false;
}

void drawHeart(int centerX, int centerY, int size) {
  int radius = max(1, size / 4);
  oled.drawDisc(centerX - radius, centerY - radius, radius);
  oled.drawDisc(centerX + radius, centerY - radius, radius);
  oled.drawTriangle(centerX - 2 * radius, centerY - radius,
                    centerX + 2 * radius, centerY - radius,
                    centerX, centerY + 2 * radius);
}

void drawSparkle(int centerX, int centerY, int radius) {
  oled.drawHLine(centerX - radius, centerY, radius * 2 + 1);
  oled.drawVLine(centerX, centerY - radius, radius * 2 + 1);
}

void drawEye(int centerX, int centerY, int width, int height,
             bool isLeft, uint8_t mood) {
  width = max(width, 4);
  height = max(height, 2);
  int left = centerX - width / 2;
  int top = centerY - height / 2;
  int radius = max(0, min(8, min(width, height) / 2 - 1));

  oled.setDrawColor(1);
  if (radius > 0) oled.drawRBox(left, top, width, height, radius);
  else oled.drawBox(left, top, width, height);

  if (height < 12) return;

  if (mood == 1) {
    oled.setDrawColor(0);
    oled.drawRBox(left - 2, top + height / 2, width + 4, height, 6);
  }

  if (mood == 2 || mood == 3) {
    int cutDepth = min(12, height / 2);
    bool cutRight = mood == 2 ? isLeft : !isLeft;
    oled.setDrawColor(0);
    if (cutRight) {
      oled.drawTriangle(left - 1, top - 1, left + width, top - 1,
                        left + width, top + cutDepth);
    } else {
      oled.drawTriangle(left - 1, top - 1, left + width, top - 1,
                        left - 1, top + cutDepth);
    }
  }
  oled.setDrawColor(1);
}

void drawMouth(uint8_t mood, int centerX, int centerY) {
  switch (mood) {
    case 1:
      oled.drawDisc(centerX, centerY - 2, 5);
      oled.setDrawColor(0);
      oled.drawBox(centerX - 6, centerY - 8, 13, 7);
      oled.setDrawColor(1);
      break;
    case 2:
      oled.drawHLine(centerX - 5, centerY + 1, 11);
      break;
    case 3:
      oled.drawLine(centerX - 5, centerY + 3, centerX, centerY);
      oled.drawLine(centerX, centerY, centerX + 5, centerY + 3);
      break;
    case 4:
      oled.drawEllipse(centerX, centerY, 3, 5);
      break;
    case 5:
      oled.drawHLine(centerX - 3, centerY + 1, 7);
      break;
    case 6:
      oled.drawLine(centerX - 5, centerY + 2, centerX + 5, centerY - 1);
      break;
    default:
      oled.drawPixel(centerX - 3, centerY);
      oled.drawHLine(centerX - 2, centerY + 1, 5);
      oled.drawPixel(centerX + 3, centerY);
      break;
  }
}

void drawFace(uint8_t mood, int centerY, int lookX, int lookY,
              float blinkScale, uint32_t now, bool smooth) {
  if (smooth) {
    eyeWidth += (eyeWidths[mood] - eyeWidth) * 0.22f;
    leftHeight += (leftHeights[mood] - leftHeight) * 0.22f;
    rightHeight += (rightHeights[mood] - rightHeight) * 0.22f;
  }
  float width = smooth ? eyeWidth : eyeWidths[mood];
  float heightLeft = smooth ? leftHeight : leftHeights[mood];
  float heightRight = smooth ? rightHeight : rightHeights[mood];
  int bob = (mood == 1 || mood == 7)
      ? static_cast<int>(sinf(now * 0.008f) * 2) : 0;
  int eyeY = centerY + lookY + bob;

  if (mood == 7 && blinkScale > 0.95f) {
    drawHeart(35 + lookX, eyeY, 28);
    drawHeart(93 + lookX, eyeY, 28);
  } else {
    drawEye(35 + lookX, eyeY, static_cast<int>(width),
            max(2, static_cast<int>(heightLeft * blinkScale)), true, mood);
    drawEye(93 + lookX, eyeY, static_cast<int>(width),
            max(2, static_cast<int>(heightRight * blinkScale)), false, mood);
  }
  drawMouth(mood, 64 + lookX / 2, centerY + 24);
}

void drawHand(int palmX, int palmY) {
  oled.setDrawColor(1);
  oled.drawRBox(palmX, palmY, 20, 13, 3);
  oled.drawRBox(palmX + 16, palmY + 2, 23, 9, 2);
  oled.drawRBox(palmX - 15, palmY, 21, 3, 1);
  oled.drawRBox(palmX - 20, palmY + 4, 26, 3, 1);
  oled.drawRBox(palmX - 17, palmY + 8, 23, 3, 1);
  oled.drawRBox(palmX - 11, palmY + 12, 19, 3, 1);
  oled.drawLine(palmX + 9, palmY + 12, palmX + 4, palmY + 18);
  oled.drawLine(palmX + 10, palmY + 12, palmX + 5, palmY + 18);
  oled.setDrawColor(0);
  oled.drawVLine(palmX + 19, palmY + 3, 7);
  oled.setDrawColor(1);
}

void drawBanana(int centerX, int centerY, int radius) {
  if (radius < 3) return;
  oled.setDrawColor(1);
  oled.drawDisc(centerX, centerY, radius);
  oled.setDrawColor(0);
  oled.drawDisc(centerX - radius / 3, centerY - radius / 3, radius - 1);
  oled.setDrawColor(1);
  oled.drawLine(centerX + radius - 2, centerY - 2,
                centerX + radius - 1, centerY - 6);
}

void drawPetting(uint32_t elapsed, uint32_t now) {
  uint8_t mood = elapsed < 800 ? 0 : (elapsed < 3700 ? 1 : 7);
  drawFace(mood, 31, 0, 0, 1.0f, now, false);

  if (elapsed < 3900) {
    float stroke = sinf(elapsed * 0.006f);
    int palmX = 65 + static_cast<int>(stroke * 15);
    if (elapsed < 400) palmX += static_cast<int>((400 - elapsed) * 70UL / 400);
    drawHand(palmX, 2);
    if (elapsed > 800) {
      drawSparkle(12, 12, 2);
      drawSparkle(113, 18, 3);
    }
  } else {
    int rise = static_cast<int>((elapsed - 3900) / 100);
    drawHeart(17, 16 - rise, 8);
    drawHeart(109, 19 - rise, 8);
  }
}

void drawFeeding(uint32_t elapsed, uint32_t now) {
  if (elapsed < 1600) {
    drawFace(4, 24, 3, 0, 1.0f, now, false);
    oled.setDrawColor(0);
    oled.drawBox(49, 40, 31, 22);
    oled.setDrawColor(1);
    oled.drawEllipse(64, 51, 9, 10);
    int bananaX = 112 - static_cast<int>(elapsed * 43UL / 1600);
    drawBanana(bananaX, 49, 11);
  } else if (elapsed < 3600) {
    drawFace(0, 23, 0, 0, 1.0f, now, false);
    oled.setDrawColor(0);
    oled.drawBox(45, 39, 39, 25);
    oled.setDrawColor(1);
    bool mouthOpen = ((elapsed - 1600) / 180) % 2 == 0;
    if (mouthOpen) {
      oled.drawRFrame(49, 41, 30, 21, 6);
      oled.drawHLine(54, 44, 20);
      oled.drawVLine(60, 42, 4);
      oled.drawVLine(68, 42, 4);
    } else {
      oled.drawRBox(47, 47, 34, 8, 3);
      oled.setDrawColor(0);
      oled.drawHLine(51, 50, 26);
      oled.setDrawColor(1);
    }
    int bananaRadius = 10 - static_cast<int>((elapsed - 1600) / 220);
    if (mouthOpen) drawBanana(65, 51, bananaRadius);
    if ((elapsed / 120) % 2 == 0) {
      oled.drawPixel(42, 43);
      oled.drawPixel(85, 48);
      oled.drawPixel(82, 57);
    }
  } else if (elapsed < 4300) {
    drawFace(6, 27, 0, 0, 1.0f, now, false);
    oled.drawCircle(15, 43, 3);
    oled.drawCircle(113, 43, 3);
  } else {
    drawFace(1, 25, 0, 0, 1.0f, now, false);
    drawSparkle(10, 12, 3);
    drawSparkle(117, 17, 3);
    oled.setFont(u8g2_font_5x7_tf);
    oled.drawStr(52, 63, "BURP!");
  }
}

void drawIdle(uint32_t now) {
  if (now - lastExpressionAt >= EXPRESSION_MS) {
    lastExpressionAt = now;
    expression = (expression + 1) % EXPRESSION_COUNT;
    targetGazeX = 0;
    targetGazeY = 0;
    Serial.println(expressionNames[expression]);
  }
  if (now - lastLookAt >= lookInterval) {
    lastLookAt = now;
    lookInterval = random(900, 2400);
    targetGazeX = random(-5, 6);
    targetGazeY = random(-3, 4);
  }
  gazeX += (targetGazeX - gazeX) * 0.16f;
  gazeY += (targetGazeY - gazeY) * 0.16f;

  if (!blinking && now - lastBlinkAt >= blinkInterval) {
    blinking = true;
    blinkStartedAt = now;
  }
  float blinkScale = 1.0f;
  if (blinking) {
    uint32_t elapsed = now - blinkStartedAt;
    if (elapsed >= 180) {
      blinking = false;
      lastBlinkAt = now;
      blinkInterval = random(2200, 5000);
    } else if (elapsed < 70) {
      blinkScale = 1.0f - elapsed / 70.0f;
    } else if (elapsed < 110) {
      blinkScale = 0;
    } else {
      blinkScale = (elapsed - 110) / 70.0f;
    }
  }
  drawFace(expression, 28, static_cast<int>(gazeX),
            static_cast<int>(gazeY), blinkScale, now, true);
}

void setup() {
  Serial.begin(115200);
  pinMode(PET_BUTTON_PIN, INPUT_PULLUP);
  randomSeed(esp_random());
  oled.setI2CAddress(0x3C << 1);
  oled.setBusClock(100000);
  oled.begin();
  oled.setPowerSave(0);
  oled.setContrast(140);
  lastExpressionAt = millis();
  lastBlinkAt = millis();
  Serial.println("RoboPet ready: button GPIO1 to GND");
}

void loop() {
  uint32_t now = millis();
  if (buttonPressed(now) && reaction == 0) {
    reaction = static_cast<uint8_t>(random(1, 3));
    reactionStartedAt = now;
    blinking = false;
    Serial.println(reaction == 1 ? "Petting!" : "Banana time!");
  }

  if (now - lastFrameAt < FRAME_MS) {
    delay(1);
    return;
  }
  lastFrameAt = now;

  uint32_t elapsed = now - reactionStartedAt;
  uint32_t duration = reaction == 1 ? PET_DURATION_MS : FEED_DURATION_MS;
  if (reaction != 0 && elapsed >= duration) {
    reaction = 0;
    lastExpressionAt = now;
    lastBlinkAt = now;
    lastLookAt = now;
  }

  oled.clearBuffer();
  oled.setDrawColor(1);
  if (reaction == 1) drawPetting(elapsed, now);
  else if (reaction == 2) drawFeeding(elapsed, now);
  else drawIdle(now);
  oled.sendBuffer();
}
