// OLED status screen (SSD1306 128x64 on I2C). Optional: if no display answers
// at boot, every call here does nothing.
//
// A full redraw takes about 25 ms on the I2C bus, so main never calls update()
// while firing. The screen simply keeps its last frame during a puff.
#pragma once
#include <stdint.h>

struct DisplayView {
  uint16_t targetMv = 0;
  uint16_t batteryMv = 0;
  uint32_t puffs = 0;
  uint32_t lastPuffMs = 0;
  bool locked = false;
  bool charging = false;
  bool lowBattery = false;
  bool bench = false;

  bool operator==(const DisplayView& o) const {
    return targetMv == o.targetMv && batteryMv / 20 == o.batteryMv / 20 && puffs == o.puffs &&
           lastPuffMs == o.lastPuffMs && locked == o.locked && charging == o.charging &&
           lowBattery == o.lowBattery && bench == o.bench;
  }
};

class Display {
 public:
  bool begin();                       // false = no display found
  bool present() const { return present_; }
  void update(uint32_t nowMs, const DisplayView& v);  // redraws only on change

 private:
  void draw(const DisplayView& v);
  bool present_ = false;
  bool drawn_ = false;
  uint32_t lastDrawMs_ = 0;
  DisplayView shown_;
};
