#include "PuffLog.h"

#include <LittleFS.h>

static const char* kLogPath = "/puffs.bin";
static const char* kOldPath = "/puffs.old.bin";

static size_t fileSize(const char* path) {
  if (!LittleFS.exists(path)) return 0;
  File f = LittleFS.open(path, "r");
  if (!f) return 0;
  size_t s = f.size();
  f.close();
  return s;
}

bool PuffLog::begin() {
  if (!LittleFS.begin(/*formatOnFail=*/true)) return false;
  if (!prefs_.begin("pufflog", /*readOnly=*/false)) return false;
  nextId_ = prefs_.getUInt("next_id", 1);
  bootId_ = prefs_.getUInt("boot_id", 0) + 1;
  prefs_.putUInt("boot_id", bootId_);
  syncedThrough_ = prefs_.getUInt("synced", 0);
  ready_ = true;
  return true;
}

void PuffLog::rotateIfNeeded() {
  if (fileSize(kLogPath) + sizeof(PuffRecord) <= kMaxFileBytes) return;
  if (LittleFS.exists(kOldPath)) LittleFS.remove(kOldPath);
  LittleFS.rename(kLogPath, kOldPath);
}

bool PuffLog::append(PuffRecord& rec) {
  if (!ready_) return false;
  rec.magic = kPuffMagic;
  rec.id = nextId_;
  rec.boot_id = bootId_;

  rotateIfNeeded();
  File f = LittleFS.open(kLogPath, FILE_APPEND);
  if (!f) return false;
  size_t written = f.write(reinterpret_cast<const uint8_t*>(&rec), sizeof(rec));
  f.close();
  if (written != sizeof(rec)) return false;

  nextId_++;
  prefs_.putUInt("next_id", nextId_);
  return true;
}

static void visitFile(const char* path, PuffLog::Visitor fn, void* ctx) {
  if (!LittleFS.exists(path)) return;
  File f = LittleFS.open(path, "r");
  if (!f) return;
  PuffRecord rec;
  while (f.read(reinterpret_cast<uint8_t*>(&rec), sizeof(rec)) == sizeof(rec)) {
    if (rec.magic == kPuffMagic) fn(rec, ctx);
  }
  f.close();
}

void PuffLog::forEach(Visitor fn, void* ctx) const {
  if (!ready_) return;
  visitFile(kOldPath, fn, ctx);
  visitFile(kLogPath, fn, ctx);
}

uint32_t PuffLog::count() const {
  if (!ready_) return 0;
  return (fileSize(kOldPath) + fileSize(kLogPath)) / sizeof(PuffRecord);
}

void PuffLog::clear() {
  if (!ready_) return;
  LittleFS.remove(kOldPath);
  LittleFS.remove(kLogPath);
}

void PuffLog::markSyncedThrough(uint32_t id) {
  syncedThrough_ = id;
  prefs_.putUInt("synced", id);
}
