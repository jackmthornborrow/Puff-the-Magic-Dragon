// Puff — stage 1 firmware (logic prototype, USB powered, LED as coil).
//
// Loop order matters: read inputs -> safety decision -> drive output. The
// output is only ever on while Safety says firing() and nothing has stopped it
// this iteration.
#include <Arduino.h>
#include <Preferences.h>

#include "Button.h"
#include "OutputMath.h"
#include "PuffLog.h"
#include "Safety.h"
#include "StatusLed.h"
#include "config.h"
#include "hal/Hal.h"
#include "net/Ota.h"
#include "net/WifiSync.h"

using puff::FireBlock;

static puff::Safety safety;
static puff::Debouncer debouncer(DEBOUNCE_MS);
static puff::ClickCounter clicks(CLICK_MAX_MS, CLICK_GAP_MS);
static puff::VoltagePresets presets;
static PuffLog puffLog;
static StatusLed status;
static Preferences settings;
static char deviceId[13];

// Current puff in progress.
struct ActivePuff {
  bool active = false;
  uint32_t startMs = 0;
  uint32_t startEpoch = 0;
  uint16_t targetMv = 0;
  uint16_t outputMv = 0;
  uint16_t batteryMv = 0;
} puffNow;

static puff::SafetyInputs readInputs(uint32_t now) {
  return puff::SafetyInputs{now, hal::batteryMv(), hal::charging(), hal::coilMilliOhm()};
}

// ---------------------------------------------------------------------------
// Firing

static void driveOutput(uint16_t batteryMv) {
  float duty = puff::rmsDuty(presets.currentMv(), batteryMv);
  hal::outputWrite(puff::dutyToCounts(duty, PWM_BITS));
}

static void startPuff(uint32_t now) {
  puff::SafetyInputs in = readInputs(now);
  FireBlock b = safety.requestFire(in);
  if (b != FireBlock::None) {
    hal::outputOff();
    if (b != FireBlock::AwaitingRelease) {
      Serial.printf("fire refused: %s\n", puff::fireBlockName(b));
      status.blink(b == FireBlock::Locked ? 2 : 6, 60, 60, now);
    }
    return;
  }
  puffNow.active = true;
  puffNow.startMs = now;
  puffNow.startEpoch = net::epochNow();
  puffNow.targetMv = presets.currentMv();
  puffNow.batteryMv = in.batteryMv;
  puffNow.outputMv = puff::effectiveOutputMv(puffNow.targetMv, in.batteryMv);
  driveOutput(in.batteryMv);
}

static void finishPuff(uint32_t now, FireBlock reason) {
  hal::outputOff();
  if (!puffNow.active) return;
  puffNow.active = false;

  uint32_t duration = now - puffNow.startMs;
  if (duration > safety.config().maxFireMs) duration = safety.config().maxFireMs;
  if (reason != FireBlock::None) {
    Serial.printf("fire stopped: %s after %lu ms\n", puff::fireBlockName(reason),
                  (unsigned long)duration);
  }
  if (duration < MIN_LOGGED_PUFF_MS) return;

  PuffRecord rec = {};
  rec.started_ms = puffNow.startMs;
  rec.started_epoch = puffNow.startEpoch;
  rec.duration_ms = duration;
  rec.target_mv = puffNow.targetMv;
  rec.output_mv = puffNow.outputMv;
  rec.battery_mv = puffNow.batteryMv;
  rec.end_reason = static_cast<uint8_t>(reason);
  rec.fw_version = kFwVersionPacked;
  if (puffLog.append(rec)) {
    Serial.printf("puff #%lu %lu ms @ %u mV\n", (unsigned long)rec.id,
                  (unsigned long)duration, rec.output_mv);
  } else {
    Serial.println("ERROR: puff log write failed");
  }
}

// ---------------------------------------------------------------------------
// Clicks

static void handleClicks(uint8_t n, uint32_t now) {
  if (safety.onClicks(n)) {
    settings.putBool("locked", safety.locked());
    Serial.println(safety.locked() ? "locked" : "unlocked");
    status.blink(safety.locked() ? 3 : 1, safety.locked() ? 150 : 600, 150, now);
    return;
  }
  if (n == PRESET_CLICKS && !safety.locked()) {
    uint16_t mv = presets.next();
    settings.putUChar("preset", presets.index());
    Serial.printf("target %u mV\n", mv);
    status.blink(presets.index() + 1, 120, 180, now);
  }
}

// ---------------------------------------------------------------------------
// Serial console

static void printCsvHeader() {
  Serial.println("id,device_id,boot_id,started_ms,started_epoch,duration_ms,"
                 "target_mv,output_mv,battery_mv,end_reason,firmware_version,synced");
}

static void printCsvRow(const PuffRecord& r, void*) {
  Serial.printf("%lu,%s,%lu,%lu,%lu,%lu,%u,%u,%u,%s,%lu.%lu.%lu,%d\n",
                (unsigned long)r.id, deviceId, (unsigned long)r.boot_id,
                (unsigned long)r.started_ms, (unsigned long)r.started_epoch,
                (unsigned long)r.duration_ms, r.target_mv, r.output_mv, r.battery_mv,
                puff::fireBlockName(static_cast<FireBlock>(r.end_reason)),
                (unsigned long)(r.fw_version >> 16), (unsigned long)((r.fw_version >> 8) & 0xFF),
                (unsigned long)(r.fw_version & 0xFF), r.id <= puffLog.syncedThrough() ? 1 : 0);
}

static void printStatus() {
  Serial.printf("Puff fw %s stage %d device %s boot %lu\n", FW_VERSION_STRING, PUFF_STAGE,
                deviceId, (unsigned long)puffLog.bootId());
  Serial.printf("locked=%d low_v_lockout=%d firing=%d target=%u mV\n", safety.locked(),
                safety.lowVoltageLockout(), safety.firing(), presets.currentMv());
  Serial.printf("battery=%u mV charging=%d coil=%ld mOhm\n", hal::batteryMv(), hal::charging(),
                (long)hal::coilMilliOhm());
  Serial.printf("log: %lu puffs, next id %lu, synced through %lu\n",
                (unsigned long)puffLog.count(), (unsigned long)puffLog.nextId(),
                (unsigned long)puffLog.syncedThrough());
}

static void printHelp() {
  Serial.println(
      "commands:\n"
      "  status            device state\n"
      "  log               dump puff log as CSV\n"
      "  clear yes         delete all logged puffs\n"
      "  preset <0-4>      set target voltage preset\n"
#if PUFF_STAGE == 1
      "  sim batt <mV>     simulated battery voltage\n"
      "  sim ohm <mOhm>    simulated coil resistance (-1 = sensor fault)\n"
      "  sim chg <0|1>     simulated USB charging\n"
#endif
      "  help");
}

static void handleCommand(char* line) {
  char* cmd = strtok(line, " ");
  if (!cmd) return;
  char* a1 = strtok(nullptr, " ");
  char* a2 = strtok(nullptr, " ");

  if (!strcmp(cmd, "status")) {
    printStatus();
  } else if (!strcmp(cmd, "log")) {
    printCsvHeader();
    puffLog.forEach(printCsvRow, nullptr);
    Serial.println("# end");
  } else if (!strcmp(cmd, "clear")) {
    if (a1 && !strcmp(a1, "yes")) {
      puffLog.clear();
      Serial.println("log cleared");
    } else {
      Serial.println("type 'clear yes' to delete all puffs");
    }
  } else if (!strcmp(cmd, "preset") && a1) {
    presets.setIndex(atoi(a1));
    settings.putUChar("preset", presets.index());
    Serial.printf("target %u mV\n", presets.currentMv());
#if PUFF_STAGE == 1
  } else if (!strcmp(cmd, "sim") && a1 && a2) {
    if (!strcmp(a1, "batt")) hal::simSetBatteryMv(atoi(a2));
    else if (!strcmp(a1, "ohm")) hal::simSetCoilMilliOhm(atol(a2));
    else if (!strcmp(a1, "chg")) hal::simSetCharging(atoi(a2) != 0);
    else { printHelp(); return; }
    printStatus();
#endif
  } else {
    printHelp();
  }
}

static void pollSerial() {
  static char buf[64];
  static size_t len = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      buf[len] = '\0';
      handleCommand(buf);
      len = 0;
    } else if (len < sizeof(buf) - 1) {
      buf[len++] = c;
    }
  }
}

// ---------------------------------------------------------------------------

void setup() {
  hal::begin();  // output forced off first
  Serial.begin(115200);

  uint64_t mac = ESP.getEfuseMac();
  snprintf(deviceId, sizeof(deviceId), "%012llx", (unsigned long long)mac);

  settings.begin("settings", false);
  safety.setLocked(settings.getBool("locked", false));  // lock survives reboot
  presets.setIndex(settings.getUChar("preset", 1));

  if (!puffLog.begin()) Serial.println("ERROR: LittleFS/NVS init failed, puffs will not be logged");

  // Never fire just because the button was already down at boot.
  if (hal::buttonRaw()) safety.requireRelease();

  net::begin();
  ota::begin();

  delay(200);
  printStatus();
  Serial.println("type 'help' for commands");
}

void loop() {
  const uint32_t now = millis();

  pollSerial();

  // 1. Button edges
  if (debouncer.update(hal::buttonRaw(), now)) {
    if (debouncer.pressed()) {
      clicks.onPress(now);
      startPuff(now);
    } else {
      clicks.onRelease(now);
      bool wasFiring = safety.firing();
      safety.onButtonReleased();
      if (wasFiring) finishPuff(now, FireBlock::None);
    }
  }

  // 2. Safety while firing / idle tracking
  if (safety.firing()) {
    puff::SafetyInputs in = readInputs(now);
    FireBlock r = safety.checkWhileFiring(in);
    if (r != FireBlock::None) {
      finishPuff(now, r);  // output off inside
      status.blink(r == FireBlock::Timeout ? 4 : 6, 80, 80, now);
    } else {
      driveOutput(in.batteryMv);  // re-compensate for sag
    }
  } else {
    hal::outputOff();  // belt and braces
    safety.updateIdle(hal::batteryMv());
  }

  // 3. Click groups (lock / preset)
  if (uint8_t n = clicks.poll(now)) handleClicks(n, now);

  status.update(now);
  net::tick(now, safety.firing(), puffLog);
  ota::tick(now);
}
