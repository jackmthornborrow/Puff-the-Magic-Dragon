// Stage 6 stub: OTA firmware updates via the Gateway's /firmware/latest.
//
// Planned behaviour:
//  - After a sync, GET /firmware/latest -> { version, url, sha256 }.
//  - If newer: signal on the status LED; install only if the user holds the
//    button 3 s AND idle (not firing, not charging, battery > ~50%).
//  - Stream into the spare OTA partition (partitions_ota.csv), verify SHA-256,
//    set boot partition, reboot.
//  - On first boot of a new image, run selfCheck(); mark valid or roll back.
//    NOTE: true pending-verify rollback needs CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE,
//    which the stock Arduino build does not enable. See README "OTA notes".
#pragma once
#include <stdint.h>

namespace ota {

struct SelfCheck {
  bool button = false;   // pin reads, not stuck low
  bool pwm = false;      // LEDC channel configured
  bool sensor = false;   // INA260 responds (stage 2+)
  bool logReadable = false;
  bool passed() const { return button && pwm && sensor && logReadable; }
};

inline void begin() {}
inline void tick(uint32_t /*nowMs*/) {}

}  // namespace ota
