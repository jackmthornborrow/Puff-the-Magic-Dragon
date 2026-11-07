// Stage 5 stub: Wi-Fi puff sync to the Personal Gateway.
//
// Planned behaviour:
//  - Two stored networks (home Wi-Fi, iPhone Personal Hotspot), tried in turn.
//  - On connect: NTP, then POST pending records (id > log.syncedThrough()) in
//    batches with the device token; on 2xx call log.markSyncedThrough(lastId).
//  - Back-fill started_epoch for records whose boot_id matches the current
//    boot once NTP gives the offset.
//  - Never run Wi-Fi work while firing (RF + current draw).
#pragma once
#include <stdint.h>

class PuffLog;

namespace net {

inline void begin() {}
inline void tick(uint32_t /*nowMs*/, bool /*firing*/, PuffLog& /*log*/) {}
inline uint32_t epochNow() { return 0; }  // 0 = wall clock unknown

}  // namespace net
