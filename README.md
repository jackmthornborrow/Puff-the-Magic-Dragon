# puff-the-magic-dragon — firmware

DIY 510 battery ("Puff") with puff tracking. This is **stage 1**: logic
prototype on a Seeed XIAO ESP32-C3, USB powered, an LED standing in for the coil.
No battery, no MOSFET, no cartridge.

## Wiring (stage 1)

| Part | XIAO pin | Notes |
|---|---|---|
| Fire button | D1 (GPIO3) → button → GND | internal pull-up |
| "Coil" LED | D10 (GPIO10) → 330 Ω → LED → GND | PWM output; becomes the MOSFET gate in stage 2 |
| Status LED | D2 (GPIO4) → 330 Ω → LED → GND | UI feedback. Keep it separate: UI blinks must never pulse a real coil |

Strapping pins D0/D8/D9 are left unused.

## Build / flash / test

```bash
pio test -e native                  # safety, power math, input tests on your Mac
pio run -e xiao_esp32c3 -t upload   # build and flash
pio device monitor                  # serial console, type 'help'
```

## Behaviour

- **Hold** = fire. Output is PWM at 20 kHz, duty = (target V / battery V)², capped at 100%.
- **3 clicks** = next voltage preset (2.4 / 2.8 / 3.2 / 3.6 / 4.0 V). Status LED blinks the preset number (1–5).
- **5 clicks** = lock / unlock (3 short blinks = locked, 1 long = unlocked). Lock survives reboot.
- **10 s cutoff**: output stops; you must release before it fires again.
- Fires shorter than 300 ms (clicks) are not logged as puffs.

Status LED: 2 fast blinks = refused (locked), 6 fast = refused/stopped by a safety check, 4 = 10 s cutoff.

### Serial commands

| Command | |
|---|---|
| `status` | state, battery, coil, log counters |
| `log` | dump puff log as CSV |
| `clear yes` | delete all puffs (IDs keep counting) |
| `preset <0-4>` | set preset |
| `sim batt <mV>` | stage 1 only: simulated battery voltage |
| `sim ohm <mΩ>` | stage 1 only: simulated coil resistance, `-1` = sensor fault |
| `sim chg <0\|1>` | stage 1 only: simulated USB charging |

Bench checklist for the safety paths: `sim batt 3200` → refuses (low voltage);
`sim batt 3500` → fires again. `sim ohm 300` → refuses (short); `sim ohm 9000`
→ refuses (no cart). `sim chg 1` → refuses. Hold 10 s → cutoff. While holding,
`sim ohm 9000` → stops mid-puff.

## Layout

```
lib/safety/     Safety.h/.cpp     cutoff, lock, low-voltage lockout, resistance check (pure C++)
lib/power/      OutputMath.h/.cpp RMS duty math, voltage presets (pure C++)
lib/input/      Button.h          debounce, click groups (pure C++)
src/main.cpp                      loop: inputs → safety → output; clicks; console
src/hal/                          hardware access; stage 1 = simulated battery/coil/charging
src/PuffLog.*                     LittleFS append-only log + NVS counters
src/net/WifiSync.h, Ota.h         stage 5 / 6 stubs
test/                             Unity tests, run on host (env:native)
partitions_ota.csv                nvs, otadata, 2 × 1.875 MB app (ota_0/ota_1), 128 KB LittleFS, coredump
```

Everything in `lib/` has no Arduino dependency, which is what lets CI run the
safety tests before a release.

## Puff record

36-byte binary record in `/puffs.bin` (rotated to `/puffs.old.bin` at 48 KB,
so about 2,700 puffs kept; once Wi-Fi sync exists, unsynced records in the
dropped file would be lost, so sync should run well before that).

| Field | Maps to `puffs` table |
|---|---|
| `id` | `id` (unique per device; NVS counter, never reused) |
| `boot_id` + `started_ms` | relative time; `started_at` = boot epoch + started_ms once NTP gives the offset |
| `started_epoch` | `started_at` directly when wall time was known (0 = unknown) |
| `duration_ms` | `duration_ms` |
| `output_mv` | `voltage` (also stores `target_mv`, `battery_mv`) |
| `end_reason` | why it stopped (`ok` = released) |
| `fw_version` | `firmware_version` |
| — | `device_id` = efuse MAC, added at upload |
| — | `synced_at` set server-side; device keeps a "synced through id" cursor in NVS |

## Notes for later stages

- **Stage 2 HAL**: write `src/hal/Hal_stage2.cpp` (MOSFET on D10 with a 10 kΩ
  gate pull-down so it stays off during boot/reset, INA219 for resistance, ADC
  divider for battery, charge-detect). Set `PUFF_STAGE=2`; the stage 1 sim
  commands disappear and the build fails until the stage 2 HAL exists.
- Resistance in stage 2 needs a short low-duty probe pulse before full fire;
  `Safety::requestFire` expects a measured value, `-1` if unknown (refuses).
- **OTA rollback**: the stock Arduino-ESP32 build does not enable
  `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`, so the "pending until self-check"
  flow needs either `framework = arduino, espidf` with a custom sdkconfig or a
  pure ESP-IDF build. Worth deciding before stage 6; the partition table is
  already dual-OTA either way.
- Nothing here turns the output on without `Safety` agreeing, and
  `allowFireWhileCharging` stays `false` until that path is tested on the dummy load.
