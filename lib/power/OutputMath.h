// Output control math (pure C++, host-testable).
//
// PWM straight from the cell: a coil's heating power follows RMS voltage, and
// the RMS of a PWM'd square wave is Vbat * sqrt(duty). So for a target
// effective voltage: duty = (Vtarget / Vbat)^2, capped at 1.
#pragma once
#include <stdint.h>

namespace puff {

// Duty in [0, 1]. Returns 0 for nonsensical inputs (battery or target <= 0).
float rmsDuty(uint16_t targetMv, uint16_t batteryMv);

// Effective (RMS) output voltage actually achievable: min(target, battery).
uint16_t effectiveOutputMv(uint16_t targetMv, uint16_t batteryMv);

// Converts duty to timer counts for a `bits`-bit PWM where full scale is
// (1 << bits) - 1 (arduino-esp32 ledcWrite maps that value to 100% on).
uint32_t dutyToCounts(float duty, uint8_t bits);

// Target-voltage presets, cycled with 3 clicks.
class VoltagePresets {
 public:
  static constexpr uint8_t kCount = 5;
  static constexpr uint16_t kMv[kCount] = {2400, 2800, 3200, 3600, 4000};

  uint16_t currentMv() const { return kMv[index_]; }
  uint8_t index() const { return index_; }
  void setIndex(uint8_t i) { index_ = (i < kCount) ? i : 0; }
  uint16_t next() {
    index_ = (index_ + 1) % kCount;
    return currentMv();
  }

 private:
  uint8_t index_ = 1;  // default 2.8 V
};

}  // namespace puff
