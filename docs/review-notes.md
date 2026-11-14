# Project review, 2026-11-13

Notes from reading every file in the repo against the hardware plan. Ordered
by how much each one matters. "Firmware" items are for whoever next works on
the code.

**Update, same day:** items 1, 3, 4 (threshold only, no averaging yet) and 5
are now fixed in the firmware. Presets are 2.4 / 2.7 / 3.0 / 3.3 / 3.6 V,
the under-load cutoff is 3.2 V, the button is active-high, and
`pio run -e xiao_esp32c3_bench` builds a version that fires with USB attached.

**Update 2026-11-14:** item 4 is complete (the cutoff now needs 40 ms below
3.2 V, or one reading below 2.9 V). The watchdog from item 7 is on (8 s).
The OLED shows battery, preset, puff count and last puff length, and `i2c`
on the serial console lists the devices on the bus. None of this has run on
hardware yet.

## Fix before stage 2

### 1. Button polarity: firmware and hardware disagree
The firmware reads the button as active-low (`INPUT_PULLUP`, pressed = LOW,
`Hal_stage1.cpp`). The hardware needs active-high, because the button's 3V3
also drives the interlock FET. With final wiring and today's firmware the
logic inverts: idle reads as "pressed". The interlock stops that from firing
a real coil, but nothing works. The README's wiring table still shows the old
wiring too. **Firmware:** `pinMode(PIN_FIRE_BUTTON, INPUT)` and
`digitalRead(...) == HIGH`.

### 2. Ground is P−, not the cell's negative
The power-path sketch in `hardware.md` drew the ground rail from "Cell−".
These protection boards switch the negative side, so the circuit ground has
to be the board's P− pad and the cell's negative must reach only B−.
Otherwise the protection board is bypassed. Fixed in `hardware.md` and built
into the stage 2 layout.

### 3. You can't watch serial and fire at the same time
Serial needs USB. USB present means "charging", which blocks firing. So the
whole stage 2 bring-up would be blind. **Firmware:** a bench build flag that
lifts the charging lockout (there is already `allowFireWhileCharging`).
Firing with USB attached is electrically harmless: the charger is limited to
0.38 A and simply contributes that much.

### 4. Under-load cutoff is too low for the XIAO
`lowVoltageUnderLoadMv = 3000`. At 3.0 V on the battery rail the XIAO's 3.3 V
regulator has dropped out and the ESP32-C3 is at the edge of its range, with
Wi-Fi bursts still to come. `hardware.md` already says 3.2–3.3 V.
**Firmware:** raise to about 3200, and average a few samples so one noisy
reading doesn't end a puff.

### 5. Voltage presets go to 4.0 V
Presets are 2.4 / 2.8 / 3.2 / 3.6 / 4.0 V. For distillate cartridges the
agreed range is 2.4–3.6 V with about 3.0 V default; 4.0 V scorches oil.
**Firmware:** drop the 4.0 V preset, or make the list 2.4 / 2.7 / 3.0 / 3.3 / 3.6.

## Worth knowing

### 6. Low-side switching leaves the 510 body live
With the negative side switched, a fitted cartridge's metal body sits at
battery voltage when idle. If that body touches ground (USB shell, metal
case), the coil conducts with nothing in control. Fine on the bench and in a
plastic case with the connector insulated. For the final PCB, consider
switching the positive side instead (a P-channel FET, or an N-FET with a
high-side driver) so the body can be ground like on commercial devices. This
is the biggest open design decision and it affects the enclosure.

### 7. A stuck resistance-check pin cooks the 15 Ω resistor
If D7 ever sticks high, R7 dissipates about 1 W continuously, its full
rating, and drains the cell at a quarter of an amp. It can't be put under the
interlock FET (that FET can't pass current properly with 15 Ω below it).
Cheap hardware fix, same holes on the board: replace R6 (100 Ω) with a
100 nF capacitor and R5 (100 kΩ) with 1 MΩ. D7 then only reaches the gate as
a pulse of about a tenth of a second however long the pin stays high. Try it
once the plain version works. **Firmware:** also enable the task watchdog.

### 8. The INA260 is the better battery gauge
Its bus-voltage reading (1.25 mV steps, about 0.1%) is on the battery rail
already, at rest and under load. The ESP32-C3's ADC is several percent off
and D0 is a strapping pin sitting at a mid-level voltage at boot. Seeed
documents that exact divider, so it works, but it is now optional. If
dropped, D0 is free for a game button.

### 9. Charging is slow, and there is no "charged" signal
The XIAO charges at 0.38 A: about 8 hours for an empty P28A. There is no
charge-status pin and the charge LED stays off on battery. **Firmware:**
infer "charged" from USB present and voltage holding at about 4.18 V. If
8 hours becomes annoying, a 1 A charger IC belongs on the final PCB.

### 10. Standby drain depends on firmware
Deep sleep is about 44 µA for the XIAO, but only if the firmware also puts
the INA260 in power-down (it draws 0.3 mA awake) and turns the OLED off.
Done properly the cell lasts months on the shelf; awake without Wi-Fi it
lasts about four days. D1 can wake from deep sleep on a HIGH level, which
matches the active-high button.

### 11. The output voltage is lower than the preset says
`rmsDuty` assumes the whole battery voltage reaches the coil. About 0.1–0.2 Ω
of FETs, wiring and contacts is in series, which costs roughly 8% of the
voltage (15% of the power) into a 1.5 Ω coil. **Firmware idea:** the INA260
measures the real current, so duty can be trimmed until
current × coil resistance equals the target. The D3–D5 readings in the
first-power log give the starting numbers.

### 12. Resistance during a puff
`checkWhileFiring` asks for coil resistance on every loop. With the sensor on
the battery side, resistance while firing is
`bus voltage × duty ÷ average current`, minus the FETs. The INA260 averages
over about a millisecond, which is 20 PWM cycles, so that works. The HAL
should compute it there and cache it, not run the 15 Ω check mid-puff.

### 13. Every click fires the coil briefly
Three clicks to change preset means three short fires, each with its own
resistance check in stage 2. Most commercial batteries do the same. A
100–150 ms delay before the output starts would remove it at the cost of a
slightly late first puff.

### 14. Protection boards usually start latched off
After a cell is first connected, many 1S boards show 0 V at the output until
they see a charger. Plugging in USB for a few seconds wakes them. It is in
the checklist (C4) because it looks exactly like a dead board.

### 15. MOSFET heat is fine, with one caveat
At 3.3 V gate drive each AO3400A is about 25 mΩ: roughly 0.15 W each at the
worst case, warm but not hot. The thin traces on the SOT-23 adapters are the
weak point, which is why the layout says to flood them with solder. If either
FET runs hot, parallel a second one on top; there are four spares.

### 16. Stale references
`README.md`, `Ota.h` and `Hal_stage1.cpp` still say INA219. The README's
"Notes for later stages" should point at `docs/hardware.md`.

## Ideas for later

- **Over-current trip:** the INA260's Alert pin can be set to pull low above
  a current limit. **Do not wire Alert to the interlock FET's gate** (my
  first suggestion, withdrawn 2026-11-14): Alert is an open-drain output with
  a pull-up to 3V3, and that pull-up would hold the interlock FET on with the
  button released, defeating the interlock. Wire Alert to D0 instead (J-MCU
  pin 7, wire W11) and let the firmware cut the output on its interrupt.
  Before relying on it, measure Alert to Vcc on the breakout with power off:
  about 10 kΩ means the pull-up is fitted; if it reads open, add a 10 kΩ from
  D0 to 3V3, because D0 must be high at boot.
- **Preheat:** about 2.0 V for a few seconds, for thick oil. The interlock
  means the coil only conducts while the button is held, so it has to be
  "double-click, then hold", not a hands-off timer.
- **Puff counter on the OLED:** today's count and seconds, battery percent,
  coil resistance, preset. The data is all there already.
- **Game buttons:** D8 and D9, each to GND with the internal pull-up (see
  the pin table in `hardware.md`). D9 is also BOOT, so holding that button at
  power-up enters the bootloader; harmless but worth knowing.
- **Antenna:** it must sit away from the cell and any metal, or Wi-Fi sync
  will be poor. Plan its spot in the enclosure early.
- **Final PCB:** INA260 chip, DFN MOSFETs, charger and XIAO footprint on one
  board about 20 × 60 mm, sitting beside the cell.
