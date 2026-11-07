// Safety module: the only place that decides whether the output may fire.
//
// Pure C++ (no Arduino headers) so it builds and is unit tested on the host
// with `pio test -e native`. Firmware feeds it measurements; it never touches
// hardware itself. If this module says stop, main must turn the output off in
// the same loop iteration.
#pragma once
#include <stdint.h>

namespace puff {

struct SafetyConfig {
  uint32_t maxFireMs = 10000;            // hard cutoff per press
  uint16_t lowVoltageCutoffMv = 3300;    // resting: lock out below this
  uint16_t lowVoltageRecoverMv = 3450;   // resting: clear lockout at/above this
  uint16_t lowVoltageUnderLoadMv = 3000; // while firing: stop below this (sag)
  int32_t minCoilMilliOhm = 800;         // below = short
  int32_t maxCoilMilliOhm = 4000;        // above = no cartridge / open circuit
  uint8_t lockClicks = 5;
  bool allowFireWhileCharging = false;   // leave false until that path is tested
};

// Why firing was refused or stopped. None = OK / normal release.
enum class FireBlock : uint8_t {
  None = 0,
  Locked,
  Charging,
  LowVoltage,
  SensorFault,     // resistance unknown (sensor missing / read failed)
  Short,
  NoCartridge,
  Timeout,         // hit maxFireMs
  AwaitingRelease, // previous fire was stopped; button must be released first
  NotFiring,
};

const char* fireBlockName(FireBlock b);

struct SafetyInputs {
  uint32_t nowMs;
  uint16_t batteryMv;
  bool charging;
  int32_t coilMilliOhm; // < 0 means unknown / sensor fault
};

class Safety {
 public:
  explicit Safety(const SafetyConfig& cfg = SafetyConfig());

  // Call on the debounced press edge. Returns None if firing may start.
  FireBlock requestFire(const SafetyInputs& in);

  // Call every loop while firing. None = keep going; anything else means the
  // output must be switched off now. Any stop latches until release.
  FireBlock checkWhileFiring(const SafetyInputs& in);

  // Call on the debounced release edge (also ends a fire in progress).
  void onButtonReleased();

  // Call every loop while not firing, to track resting battery voltage.
  void updateIdle(uint16_t batteryMv);

  // Call with each completed click group. Returns true if lock state toggled.
  bool onClicks(uint8_t count);

  // Force "release before fire" (e.g. button already held at boot).
  void requireRelease() { awaitRelease_ = true; }

  bool locked() const { return locked_; }
  void setLocked(bool v) { locked_ = v; }
  bool firing() const { return firing_; }
  bool lowVoltageLockout() const { return lowVoltage_; }
  uint32_t fireElapsedMs(uint32_t nowMs) const {
    return firing_ ? nowMs - fireStartMs_ : 0;  // unsigned: millis() wrap-safe
  }
  const SafetyConfig& config() const { return cfg_; }

 private:
  FireBlock preFireBlock(const SafetyInputs& in) const;
  FireBlock coilBlock(int32_t mOhm) const;
  void stop();

  SafetyConfig cfg_;
  bool locked_ = false;
  bool firing_ = false;
  bool awaitRelease_ = false;
  bool lowVoltage_ = false;
  uint32_t fireStartMs_ = 0;
};

}  // namespace puff
