// Board pins and firmware-wide constants.
#pragma once
#include <stdint.h>

#ifndef PUFF_STAGE
#error "PUFF_STAGE must be set in platformio.ini build_flags"
#endif

// ---- Firmware version (set in platformio.ini; CI will set it from the tag) --
#ifndef FW_VERSION_MAJOR
#define FW_VERSION_MAJOR 0
#define FW_VERSION_MINOR 0
#define FW_VERSION_PATCH 0
#endif
#define PUFF_STR2(x) #x
#define PUFF_STR(x) PUFF_STR2(x)
#define FW_VERSION_STRING \
  PUFF_STR(FW_VERSION_MAJOR) "." PUFF_STR(FW_VERSION_MINOR) "." PUFF_STR(FW_VERSION_PATCH)
constexpr uint32_t kFwVersionPacked =
    (uint32_t(FW_VERSION_MAJOR) << 16) | (uint32_t(FW_VERSION_MINOR) << 8) | uint32_t(FW_VERSION_PATCH);

// ---- Pins (Seeed XIAO ESP32-C3) -------------------------------------------
// Avoid strapping pins GPIO2 (D0), GPIO8 (D8), GPIO9 (D9) for anything that
// could be pulled at boot.
constexpr uint8_t PIN_FIRE_BUTTON = 3;   // D1, button to 3V3 (active-high), 10k pull-down.
                                         // The same 3V3 drives the interlock FET gate.
constexpr uint8_t PIN_OUTPUT = 10;       // D10, LED now / MOSFET gate in stage 2
constexpr uint8_t PIN_STATUS_LED = 4;    // D2, separate UI LED (never blink the coil)

constexpr uint8_t PIN_I2C_SDA = 6;       // D4: OLED 0x3C, INA260 0x40 (stage 2)
constexpr uint8_t PIN_I2C_SCL = 7;       // D5
constexpr uint8_t I2C_ADDR_OLED = 0x3C;

// ---- PWM ------------------------------------------------------------------
constexpr uint8_t PWM_CHANNEL = 0;
constexpr uint32_t PWM_FREQ_HZ = 20000;
constexpr uint8_t PWM_BITS = 10;

// ---- Input timing -----------------------------------------------------------
constexpr uint16_t DEBOUNCE_MS = 20;
constexpr uint16_t CLICK_MAX_MS = 400;   // longer than this = a hold, not a click
constexpr uint16_t CLICK_GAP_MS = 500;   // group ends after this much quiet
constexpr uint8_t PRESET_CLICKS = 3;

// ---- Logging --------------------------------------------------------------
constexpr uint32_t MIN_LOGGED_PUFF_MS = 300;  // shorter fires are clicks, not puffs
