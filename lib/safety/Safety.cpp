#include "Safety.h"

namespace puff {

const char* fireBlockName(FireBlock b) {
  switch (b) {
    case FireBlock::None: return "ok";
    case FireBlock::Locked: return "locked";
    case FireBlock::Charging: return "charging";
    case FireBlock::LowVoltage: return "low_voltage";
    case FireBlock::SensorFault: return "sensor_fault";
    case FireBlock::Short: return "short";
    case FireBlock::NoCartridge: return "no_cartridge";
    case FireBlock::Timeout: return "timeout";
    case FireBlock::AwaitingRelease: return "awaiting_release";
    case FireBlock::NotFiring: return "not_firing";
  }
  return "unknown";
}

Safety::Safety(const SafetyConfig& cfg) : cfg_(cfg) {}

void Safety::updateIdle(uint16_t batteryMv) {
  if (lowVoltage_) {
    if (batteryMv >= cfg_.lowVoltageRecoverMv) lowVoltage_ = false;
  } else if (batteryMv < cfg_.lowVoltageCutoffMv) {
    lowVoltage_ = true;
  }
}

FireBlock Safety::coilBlock(int32_t mOhm) const {
  if (mOhm < 0) return FireBlock::SensorFault;
  if (mOhm < cfg_.minCoilMilliOhm) return FireBlock::Short;
  if (mOhm > cfg_.maxCoilMilliOhm) return FireBlock::NoCartridge;
  return FireBlock::None;
}

FireBlock Safety::preFireBlock(const SafetyInputs& in) const {
  if (awaitRelease_) return FireBlock::AwaitingRelease;
  if (locked_) return FireBlock::Locked;
  if (in.charging && !cfg_.allowFireWhileCharging) return FireBlock::Charging;
  if (lowVoltage_) return FireBlock::LowVoltage;
  return coilBlock(in.coilMilliOhm);
}

FireBlock Safety::requestFire(const SafetyInputs& in) {
  if (firing_) return FireBlock::None;  // already firing; keep original start
  updateIdle(in.batteryMv);
  FireBlock b = preFireBlock(in);
  if (b == FireBlock::None) {
    firing_ = true;
    fireStartMs_ = in.nowMs;
    sagging_ = false;
  }
  return b;
}

void Safety::stop() {
  firing_ = false;
  awaitRelease_ = true;
}

FireBlock Safety::checkWhileFiring(const SafetyInputs& in) {
  if (!firing_) return FireBlock::NotFiring;

  // Under-load sag must persist for lowVoltageHoldMs, unless it is severe.
  bool lowNow = false;
  if (in.batteryMv < cfg_.lowVoltageUnderLoadMv) {
    if (!sagging_) {
      sagging_ = true;
      sagStartMs_ = in.nowMs;
    }
    lowNow = in.batteryMv < cfg_.lowVoltageHardMv ||
             in.nowMs - sagStartMs_ >= cfg_.lowVoltageHoldMs;
  } else {
    sagging_ = false;
  }

  FireBlock r = FireBlock::None;
  if (in.nowMs - fireStartMs_ >= cfg_.maxFireMs) {
    r = FireBlock::Timeout;
  } else if (in.charging && !cfg_.allowFireWhileCharging) {
    r = FireBlock::Charging;
  } else if (lowNow) {
    r = FireBlock::LowVoltage;
    lowVoltage_ = true;
  } else {
    r = coilBlock(in.coilMilliOhm);  // cart removed / shorted mid-puff
  }

  if (r != FireBlock::None) stop();
  return r;
}

void Safety::onButtonReleased() {
  firing_ = false;
  awaitRelease_ = false;
}

bool Safety::onClicks(uint8_t count) {
  if (count != cfg_.lockClicks || firing_) return false;
  locked_ = !locked_;
  return true;
}

}  // namespace puff
