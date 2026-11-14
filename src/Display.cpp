#include "Display.h"

#include <Arduino.h>
#include <U8g2lib.h>

#include "config.h"
#include "hal/Hal.h"

static U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

static uint8_t batteryPercent(uint16_t mv) {
  // Resting voltage, rough: 3.3 V (lockout) = 0%, 4.15 V = 100%.
  if (mv <= 3300) return 0;
  if (mv >= 4150) return 100;
  return uint8_t((uint32_t(mv - 3300) * 100) / 850);
}

bool Display::begin() {
  present_ = hal::i2cPresent(I2C_ADDR_OLED);
  if (!present_) return false;
  oled.setBusClock(400000);
  oled.begin();
  oled.setContrast(160);
  return true;
}

void Display::update(uint32_t nowMs, const DisplayView& v) {
  if (!present_) return;
  if (drawn_ && v == shown_) return;
  if (drawn_ && nowMs - lastDrawMs_ < 250) return;  // at most 4 redraws a second
  draw(v);
  shown_ = v;
  drawn_ = true;
  lastDrawMs_ = nowMs;
}

void Display::draw(const DisplayView& v) {
  char buf[24];
  oled.clearBuffer();

  // Top row: battery
  oled.setFont(u8g2_font_6x12_tf);
  snprintf(buf, sizeof(buf), "%u%%  %u.%02uV", batteryPercent(v.batteryMv), v.batteryMv / 1000,
           (v.batteryMv % 1000) / 10);
  oled.drawStr(0, 10, buf);
  if (v.charging) oled.drawStr(104, 10, "CHG");
  else if (v.lowBattery) oled.drawStr(104, 10, "LOW");

  // Middle: target voltage, or the lock
  oled.setFont(u8g2_font_logisoso24_tf);
  if (v.locked) {
    oled.drawStr(0, 44, "LOCKED");
  } else {
    snprintf(buf, sizeof(buf), "%u.%uV", v.targetMv / 1000, (v.targetMv % 1000) / 100);
    oled.drawStr(0, 44, buf);
  }

  // Bottom row: puff count and the last puff's length
  oled.setFont(u8g2_font_6x12_tf);
  snprintf(buf, sizeof(buf), "puffs %lu", (unsigned long)v.puffs);
  oled.drawStr(0, 62, buf);
  if (v.bench) {
    oled.drawStr(92, 62, "BENCH");
  } else if (v.lastPuffMs) {
    snprintf(buf, sizeof(buf), "%lu.%lus", (unsigned long)(v.lastPuffMs / 1000),
             (unsigned long)((v.lastPuffMs % 1000) / 100));
    oled.drawStr(98, 62, buf);
  }
  oled.sendBuffer();
}
