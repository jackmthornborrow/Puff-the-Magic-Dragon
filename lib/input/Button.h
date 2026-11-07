// Button debounce + click-group detection (pure C++, host-testable).
#pragma once
#include <stdint.h>

namespace puff {

class Debouncer {
 public:
  explicit Debouncer(uint16_t stableMs = 20) : stableMs_(stableMs) {}

  // Feed the raw reading (true = pressed). Returns true on a debounced edge.
  bool update(bool raw, uint32_t nowMs) {
    if (raw != lastRaw_) {
      lastRaw_ = raw;
      lastChangeMs_ = nowMs;
    }
    if (raw != stable_ && (nowMs - lastChangeMs_) >= stableMs_) {
      stable_ = raw;
      return true;
    }
    return false;
  }
  bool pressed() const { return stable_; }

 private:
  uint16_t stableMs_;
  bool lastRaw_ = false;
  bool stable_ = false;
  uint32_t lastChangeMs_ = 0;
};

// Counts short presses into groups. A group completes when no new press
// starts within gapMs of the last release. A press held longer than
// maxClickMs (i.e. a real puff) cancels the group in progress.
class ClickCounter {
 public:
  ClickCounter(uint16_t maxClickMs = 400, uint16_t gapMs = 500)
      : maxClickMs_(maxClickMs), gapMs_(gapMs) {}

  void onPress(uint32_t nowMs) {
    down_ = true;
    pressMs_ = nowMs;
  }

  void onRelease(uint32_t nowMs) {
    down_ = false;
    if (nowMs - pressMs_ <= maxClickMs_) {
      if (count_ < 255) count_++;
      releaseMs_ = nowMs;
    } else {
      count_ = 0;
    }
  }

  // Returns the size of a just-completed group, or 0.
  uint8_t poll(uint32_t nowMs) {
    if (down_) {
      if (count_ && nowMs - pressMs_ > maxClickMs_) count_ = 0;
      return 0;
    }
    if (count_ && nowMs - releaseMs_ >= gapMs_) {
      uint8_t n = count_;
      count_ = 0;
      return n;
    }
    return 0;
  }

  // How long the button has been held (0 if up). Used later for the 3 s
  // OTA-accept hold.
  uint32_t heldMs(uint32_t nowMs) const { return down_ ? nowMs - pressMs_ : 0; }

 private:
  uint16_t maxClickMs_;
  uint16_t gapMs_;
  bool down_ = false;
  uint8_t count_ = 0;
  uint32_t pressMs_ = 0;
  uint32_t releaseMs_ = 0;
};

}  // namespace puff
