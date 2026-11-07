#include "OutputMath.h"

namespace puff {

constexpr uint16_t VoltagePresets::kMv[VoltagePresets::kCount];

float rmsDuty(uint16_t targetMv, uint16_t batteryMv) {
  if (targetMv == 0 || batteryMv == 0) return 0.0f;
  if (targetMv >= batteryMv) return 1.0f;
  float r = static_cast<float>(targetMv) / static_cast<float>(batteryMv);
  return r * r;
}

uint16_t effectiveOutputMv(uint16_t targetMv, uint16_t batteryMv) {
  return targetMv < batteryMv ? targetMv : batteryMv;
}

uint32_t dutyToCounts(float duty, uint8_t bits) {
  if (bits == 0 || bits > 20) return 0;
  if (!(duty > 0.0f)) return 0;  // also catches NaN
  if (duty > 1.0f) duty = 1.0f;
  const uint32_t full = (1UL << bits) - 1UL;
  return static_cast<uint32_t>(duty * static_cast<float>(full) + 0.5f);
}

}  // namespace puff
