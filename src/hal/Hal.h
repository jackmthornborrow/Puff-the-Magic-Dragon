// Hardware abstraction. main.cpp only talks to hardware through here, so the
// stage 2 power board can replace the stage 1 simulation without touching the
// app or safety logic.
#pragma once
#include <stdint.h>

namespace hal {

void begin();

bool buttonRaw();                // true = pressed
uint16_t batteryMv();            // cell voltage (stage 1: simulated)
bool charging();                 // USB charging the cell (stage 1: simulated)
int32_t coilMilliOhm();          // < 0 = unknown (stage 1: simulated)

void outputWrite(uint32_t counts);  // PWM counts, 0 = off
void outputOff();
void statusLed(bool on);
bool i2cPresent(uint8_t addr);      // true if a device answers at addr

#if PUFF_STAGE == 1
// Bench simulation knobs, driven from the serial console, so the safety paths
// (low voltage, short, no cart, charging) can be exercised without hardware.
void simSetBatteryMv(uint16_t mv);
void simSetCoilMilliOhm(int32_t mOhm);
void simSetCharging(bool on);
#endif

}  // namespace hal
