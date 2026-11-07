// Non-blocking blink patterns on the status LED (never on the coil output).
#pragma once
#include <stdint.h>

#include "hal/Hal.h"

class StatusLed {
 public:
  void blink(uint8_t times, uint16_t onMs, uint16_t offMs, uint32_t nowMs) {
    remaining_ = times;
    onMs_ = onMs;
    offMs_ = offMs;
    on_ = false;
    nextMs_ = nowMs;
  }

  void update(uint32_t nowMs) {
    if (!remaining_ && !on_) return;
    if (int32_t(nowMs - nextMs_) < 0) return;
    if (on_) {
      on_ = false;
      hal::statusLed(false);
      nextMs_ = nowMs + offMs_;
    } else if (remaining_) {
      remaining_--;
      on_ = true;
      hal::statusLed(true);
      nextMs_ = nowMs + onMs_;
    }
  }

  bool busy() const { return remaining_ || on_; }

 private:
  uint8_t remaining_ = 0;
  uint16_t onMs_ = 0, offMs_ = 0;
  bool on_ = false;
  uint32_t nextMs_ = 0;
};
