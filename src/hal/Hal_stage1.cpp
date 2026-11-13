// Stage 1 HAL: USB powered, LED stands in for the coil, no battery or INA260.
// Battery, charging and coil resistance are simulated values the serial
// console can change.
#include "config.h"

#if PUFF_STAGE == 1
#include <Arduino.h>

#include "hal/Hal.h"

namespace hal {

static uint16_t sBatteryMv = 4000;
static int32_t sCoilMilliOhm = 1500;
static bool sCharging = false;

void begin() {
  // Output off before anything else.
  pinMode(PIN_OUTPUT, OUTPUT);
  digitalWrite(PIN_OUTPUT, LOW);
  ledcSetup(PWM_CHANNEL, PWM_FREQ_HZ, PWM_BITS);
  ledcAttachPin(PIN_OUTPUT, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0);

  // Active-high: the button switches 3V3 onto the pin (and, from stage 2, onto
  // the interlock FET gate). The internal pull-down backs up the external 10k.
  pinMode(PIN_FIRE_BUTTON, INPUT_PULLDOWN);
  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_STATUS_LED, LOW);
}

bool buttonRaw() { return digitalRead(PIN_FIRE_BUTTON) == HIGH; }
uint16_t batteryMv() { return sBatteryMv; }
bool charging() { return sCharging; }
int32_t coilMilliOhm() { return sCoilMilliOhm; }

void outputWrite(uint32_t counts) { ledcWrite(PWM_CHANNEL, counts); }
void outputOff() { ledcWrite(PWM_CHANNEL, 0); }
void statusLed(bool on) { digitalWrite(PIN_STATUS_LED, on ? HIGH : LOW); }

void simSetBatteryMv(uint16_t mv) { sBatteryMv = mv; }
void simSetCoilMilliOhm(int32_t mOhm) { sCoilMilliOhm = mOhm; }
void simSetCharging(bool on) { sCharging = on; }

}  // namespace hal

#elif PUFF_STAGE >= 2
#error "Stage 2 HAL (MOSFET, INA260, charge detect) not written yet"
#endif
