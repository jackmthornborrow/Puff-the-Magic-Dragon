# Puff: hardware

Hardware plan for stages 1–2, agreed 2026-11-07. Firmware details are in the
top-level README; this file covers parts, pins and the power path.

Build guides:
[stage 1 breadboard](stage1-breadboard.md) ·
[stage 2 power board](stage2-perfboard.md) ·
[first-power checklist](first-power-checklist.md) ·
[stage 3 connector](stage3-510-connector.md) ·
[review notes](review-notes.md)

## Pin map (XIAO ESP32-C3)

| Pin | GPIO | Use | Notes |
|---|---|---|---|
| D10 | 10 | Main MOSFET gate (stage 1: "coil" LED) | 100 Ω series, **10 kΩ pull-down to GND** so it stays off during boot/reset |
| D2 | 4 | Status LED | 330 Ω → LED → GND. The XIAO C3 has no user LED |
| D1 | 3 | Fire button, **active-high** | Button connects 3V3 → D1 with a 10 kΩ pull-down. The same 3V3 drives the interlock FET gate (see below). Can wake from deep sleep (the C3 only wakes on GPIO0–5) |
| D0 | 2 | Battery voltage divider (optional) | ADC1. 220 k / 220 k + 100 nF. Strapping pin, input only. The INA260's bus voltage is the better battery reading |
| D3 | 5 | USB-present sense | 5 V → 10 k / 20 k divider → D3. Needed for "never fire while charging" |
| D4 / D5 | 6 / 7 | I2C SDA / SCL | INA260 (0x40) + SSD1306 OLED (0x3C) |
| D7 | 20 | Resistance-check MOSFET gate | 100 kΩ pull-down |
| D6 | 21 | spare | UART TX toggles at boot, so don't use it for outputs |
| D8, D9 | 8, 9 | unused | Strapping pins (D9 = BOOT button) |

### Firmware changes this pin map needs
1. **Button polarity:** the stage 1 code uses an internal pull-up with the button to GND. The hardware interlock needs the button to switch 3V3, so the pin reads **HIGH = pressed** with an external pull-down.
2. **INA260, not INA219:** use a different library and registers. The board's built-in 2 mΩ shunt covers 15 A, so there's no shunt swap.
3. **Coil-check calibration:** the 15 Ω resistor is ±5% (±0.75 Ω). Measure the 1.5 Ω dummy load once and store the correction. Read the INA260 at zero current at startup to remove its offset.

## Power path (stage 2)

```
Cell+ ─ 5 A fuse ─ B+ [protection board] P+ ─┬─ XIAO BAT+  (470 µF bulk cap here)
                                             └─ INA260 ─ coil+ (dummy 1.5 Ω)
                                                          │
                                                        coil−
                                                          │
                     ┌────────────────────────────────────┴─────┐
               interlock FET (gate = button 3V3 via 1 k,   15 Ω 1 W
                              100 k pull-down)                  │
                     │                                   ohm-check FET (D7)
               main FET (D10, PWM 20 kHz)                       │
                     │                                          │
GND = P− ────────────┴──────────────────────────────────────────┘
Cell− ─ B− [protection board]      (cell− connects to nothing else)
```

- **Ground is P−:** the protection board switches the negative side, so every
  "GND" in this project is its P− pad. The cell's negative goes only to B−.
- **Hardware interlock:** the coil can only conduct while the button is physically held *and* firmware drives D10. If the firmware hangs, releasing the button stops the fire.
- **Resistance check:** turn on the D7 FET only, then read current with the INA260. R_coil = V / I − 15 Ω − FET resistance. That's about 0.25 A even into a dead short. Fire the main FET only when R is about 0.8–4 Ω.
- **Peak current:** with PWM every pulse is full cell voltage: 4.2 V / 1.2 Ω ≈ 3.5 A. Size everything for ≥ 4 A.
- **Low-voltage lockout:** measure the battery during the fire. The cell, protection board, fuse and wiring together drop about 0.5 V at 3.5 A. Lock out at about 3.2–3.3 V under load. The protection board's ~2.4 V cutoff is only a backstop.
- **High-current wiring:** use 18 AWG, soldered. Don't run it through the breadboard, whose contacts are rated about 1 A.
- **Dummy load:** the HS25 is only rated 25 W when bolted to an aluminium plate or heatsink.
- **Cell:** your own 18650 must be a genuine high-drain cell rated ≥ 10 A continuous (e.g. Molicel P28A, Samsung 30Q, Sony VTC6). Never solder to it; use the holder.
- **Charging:** USB-C on the XIAO charges the cell through its BAT pads at
  about 0.38 A (roughly 8 hours for an empty P28A). The BAT pads are on the
  underside of the XIAO. There is no charge-status signal.
- **Charge / first-fire tests:** do them in a steel tin with the lid left loose (never sealed), on a non-flammable surface away from anything that can burn, and stay nearby.

## Parts list

AUD. DigiKey prices are before GST; the others include it. Checked 2026-11-09. Already on hand: breadboard + jumpers, soldering
iron, multimeter, resistor/LED/cap assortment. The no-name "LL 2400mAh" cell
on hand is not suitable, so buy the P28A below. `parts-photos.html`
(not committed) has a numbered photo of each.

Fire and game buttons (12 mm tactile) and 18 AWG hookup wire are already on hand.

### DigiKey Australia

| # | Qty | Part (DigiKey PN) | Each |
|---|---|---|---|
| 1 | 2 | Seeed Studio XIAO ESP32-C3, one spare (1597-113991054-ND) | $7.75 |
| 2 | 1 | Adafruit INA260 power monitor, 4226 (1528-2955-ND) | $14.69 |
| 8 | 7 | AO3400A N-FET SOT-23 (785-1000-1-ND): main, interlock, ohm-check, 4 spare | $0.77 |
| 9 | 1 | Ohmite HS25 1R5 J, 1.5 Ω 25 W dummy load (HS251R5J-ND) | $10.16 |
| 10 | 3 | Littelfuse 0251005.MXL, 5 A fast axial fuse (F2317-ND) | $1.83 |
| 11 | 3 | Vishay PR01000101509JR500, 15 Ω 1 W (56-PR01000101509JR500CT-ND) | $0.40 |
| 12 | 2 | Panasonic EEU-FR1A471, 470 µF 10 V low-ESR (P124219-ND) | $1.05 |
| 13 | 1 | Keystone 1043P, 18650 holder (36-1043P-ND) | $5.72 |

DigiKey ships free over AU$60 (AU$24 below that). This cart is AU$60.25 before GST.

### AliExpress

| # | Qty | Part |
|---|---|---|
| 14 | 2 | 1S 3.7 V protection board, **10A 4MOS** variant (MWSS PCB Store, AliExpress item 1005003093782029), AU$2.74 each, free shipping. Avoid bare 2–3 A single-FET boards |
| 3 | 1 pack | SOT-23-3 to SIP3 adapter boards, 20 pcs (AliExpress item 1005006626188055), AU$2.30, free shipping |
| 16 | 1 | 1/4 W 1% metal film resistor kit, 300 pcs, 30 values, 10 Ω–1 MΩ (AliExpress item 1005006863902202), AU$3.83, free shipping |
| 7 | 1 | White 0.96" 128×64 I2C OLED, SSD1306, 4-pin (AliExpress item 1005006141235306), AU$1.49, free shipping. Check the VCC/GND order on the board |

Needed before the cell is connected (stage 2), not for stage 1.

### TinkerTech AU

| # | Qty | Part | Each |
|---|---|---|---|
| 15 | 1 | Molicel P28A 18650, 2800 mAh, 35 A, flat top, unprotected | $10.99 |

$10 flat-rate shipping.

## Later

- Stage 3: 510 connector. With a low-side switch the threaded body is the
  *switched negative* and the centre pin is always at battery voltage, so
  insulate the thread from cell−.
- Final build MOSFET: DFN package with a thermal pad, Rds(on) < 15 mΩ at Vgs = 2.5 V.
- Enclosure: plastic, with the 510 connector's body insulated from ground
  (see review note 6), or move to high-side switching on the final PCB.
