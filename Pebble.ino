#include <Arduino.h>
#include <Wire.h>
#include "HWCDC.h"
#include "Arduino_GFX_Library.h"
#include "SensorQMI8658.hpp"
#include "pin_config.h"

HWCDC USBSerial;

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
Arduino_CO5300 *gfx = new Arduino_CO5300(bus, LCD_RESET, 0, LCD_WIDTH, LCD_HEIGHT, 22, 0, 0, 0);

SensorQMI8658 qmi;
IMUdata acc;

const float FREEFALL_G = 0.35;               // magnitude below this counts as "airborne" (true freefall reads ~0g)
const unsigned long FREEFALL_MIN_MS = 25;    // must stay airborne this long to count as a real throw, not a jostle
const float CATCH_G = 2.0;                   // magnitude above this while airborne counts as "caught"
const unsigned long AIRBORNE_TIMEOUT_MS = 3000;  // force a reveal if no clean catch-spike registers (soft lob, etc.)
const unsigned long FLICKER_MS = 80;         // how often the number changes while airborne
const unsigned long REVEAL_HOLD_MS = 2500;   // how long a safe number stays on screen
const unsigned long LOSE_HOLD_MS = 5000;     // how long the red 67 stays on screen
const int LOSE_CHANCE = 5;                   // 1 in this many catches is a loss
const int LOSE_NUMBER = 67;

enum GameState { IDLE,
                  AIRBORNE,
                  REVEALED };
GameState state = IDLE;

unsigned long airborneStartMs = 0;
unsigned long revealStartMs = 0;
unsigned long lastFlickerMs = 0;
bool wasFreefalling = false;
bool lastWasLoss = false;

void showNumber(int n, uint16_t bg, uint16_t fg) {
  gfx->fillScreen(bg);

  char buf[4];
  snprintf(buf, sizeof(buf), "%02d", n);

  gfx->setTextSize(12);
  int16_t x1, y1;
  uint16_t w, h;
  gfx->getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);

  gfx->setCursor((gfx->width() - w) / 2 - x1, (gfx->height() - h) / 2 - y1);
  gfx->setTextColor(fg);
  gfx->print(buf);
}

int rollSafeNumber() {
  int n;
  do {
    n = random(0, 100);
  } while (n == LOSE_NUMBER);
  return n;
}

void goIdle() {
  state = IDLE;
  wasFreefalling = false;
  gfx->setBrightness(200);
  gfx->fillScreen(BLACK);
  USBSerial.println("ready for next throw");
}

void setup() {
  USBSerial.begin(115200);

  if (!gfx->begin()) {
    USBSerial.println("gfx->begin() failed!");
  }
  gfx->setBrightness(200);
  gfx->fillScreen(BLACK);

  Wire.begin(IIC_SDA, IIC_SCL);
  if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
    USBSerial.println("QMI8658 not found - check wiring");
    while (1) delay(1000);
  }
  qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_1000Hz, SensorQMI8658::LPF_MODE_0);
  qmi.enableAccelerometer();

  USBSerial.println("Pebble ready - throw it!");
}

void loop() {
  if (!qmi.getDataReady() || !qmi.getAccelerometer(acc.x, acc.y, acc.z)) {
    return;
  }

  float mag = sqrt(acc.x * acc.x + acc.y * acc.y + acc.z * acc.z);
  unsigned long now = millis();

  switch (state) {
    case IDLE:
      if (mag < FREEFALL_G) {
        if (!wasFreefalling) {
          airborneStartMs = now;
          wasFreefalling = true;
        } else if (now - airborneStartMs > FREEFALL_MIN_MS) {
          state = AIRBORNE;
          airborneStartMs = now;
          lastFlickerMs = now;
          USBSerial.println("thrown -> airborne");
        }
      } else {
        wasFreefalling = false;
      }
      break;

    case AIRBORNE:
      if (mag > CATCH_G || (now - airborneStartMs > AIRBORNE_TIMEOUT_MS)) {
        lastWasLoss = random(0, LOSE_CHANCE) == 0;
        state = REVEALED;
        revealStartMs = now;

        if (lastWasLoss) {
          gfx->setBrightness(255);
          showNumber(LOSE_NUMBER, RED, BLACK);
          USBSerial.println("caught -> 67! you lose");
        } else {
          int n = rollSafeNumber();
          showNumber(n, BLACK, WHITE);
          USBSerial.print("caught -> ");
          USBSerial.println(n);
        }
      } else if (now - lastFlickerMs > FLICKER_MS) {
        showNumber(random(0, 100), BLACK, WHITE);
        lastFlickerMs = now;
      }
      break;

    case REVEALED:
      if (now - revealStartMs > (lastWasLoss ? LOSE_HOLD_MS : REVEAL_HOLD_MS)) {
        goIdle();
      }
      break;
  }
}
