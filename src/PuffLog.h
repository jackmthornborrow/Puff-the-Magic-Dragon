// Append-only puff log in LittleFS, surviving reboots.
//
// Records are fixed-size binary. When the active file fills it is rotated to
// an ".old" file (the previous .old is dropped), so roughly 2 x kMaxFileBytes
// of history is kept. IDs come from NVS and never repeat, even after `clear`,
// so they can be used as a unique key in Supabase later.
//
// Time: at puff time we usually have no wall clock, so we store boot_id +
// millis-since-boot. started_epoch is filled when NTP time was known; the sync
// stage can back-fill epoch for a boot once NTP gives that boot's offset.
#pragma once
#include <Preferences.h>
#include <stdint.h>

struct __attribute__((packed)) PuffRecord {
  uint32_t magic;          // kPuffMagic (format version in low byte)
  uint32_t id;
  uint32_t boot_id;
  uint32_t started_ms;     // millis() at fire start, this boot
  uint32_t started_epoch;  // unix seconds, 0 = unknown at the time
  uint32_t duration_ms;
  uint16_t target_mv;      // preset
  uint16_t output_mv;      // effective RMS output = min(target, battery)
  uint16_t battery_mv;     // at fire start
  uint8_t end_reason;      // puff::FireBlock (0 = released normally)
  uint8_t reserved;
  uint32_t fw_version;     // packed major<<16 | minor<<8 | patch
};
static_assert(sizeof(PuffRecord) == 36, "PuffRecord layout changed: bump kPuffMagic");

constexpr uint32_t kPuffMagic = 0x50554601;  // "PUF" v1

class PuffLog {
 public:
  using Visitor = void (*)(const PuffRecord&, void* ctx);
  static constexpr size_t kMaxFileBytes = 48 * 1024;

  bool begin();  // mounts LittleFS (formats on first boot), loads counters
  bool ready() const { return ready_; }

  // Fills id / boot_id / magic and appends. Returns false on write failure.
  bool append(PuffRecord& rec);

  void forEach(Visitor fn, void* ctx) const;  // oldest first
  uint32_t count() const;
  void clear();  // deletes records; IDs keep counting up

  uint32_t bootId() const { return bootId_; }
  uint32_t nextId() const { return nextId_; }

  // Sync cursor: every record with id <= this has been uploaded.
  uint32_t syncedThrough() const { return syncedThrough_; }
  void markSyncedThrough(uint32_t id);

 private:
  void rotateIfNeeded();

  mutable Preferences prefs_;
  bool ready_ = false;
  uint32_t nextId_ = 1;
  uint32_t bootId_ = 0;
  uint32_t syncedThrough_ = 0;
};
