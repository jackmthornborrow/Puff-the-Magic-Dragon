# Puff: hardware

Hardware plan for stages 1–2, agreed 2026-11-07. Firmware details are in the
top-level README; this file covers parts, pins and the power path.

## Pin map (XIAO ESP32-C3)

| Pin | GPIO | Use | Notes |
|---|---|---|---|
| D10 | 10 | Main MOSFET gate (stage 1: "coil" LED) | 100 Ω series, **10 kΩ pull-down to GND** so it stays off during boot/reset |
| D2 | 4 | Status LED | 330 Ω → LED → GND. The XIAO C3 has no user LED |
| D1 | 3 | Fire button, **active-high** | Button connects 3V3 → D1 with a 10 kΩ pull-down. The same 3V3 drives the interlock FET gate (see below). Can wake from deep sleep (the C3 only wakes on GPIO0–5) |
| D0 | 2 | Battery voltage divider | ADC1. 220 k / 220 k + 100 nF. Strapping pin, input only |
| D3 | 5 | USB-present sense | 5 V → 10 k / 20 k divider → D3. Needed for "never fire while charging" |
| D4 / D5 | 6 / 7 | I2C SDA / SCL | INA228 (0x40) + SSD1306 OLED (0x3C) |
| D7 | 20 | Resistance-check MOSFET gate | 100 kΩ pull-down |
| D6 | 21 | spare | UART TX toggles at boot, so don't use it for outputs |
| D8, D9 | 8, 9 | unused | Strapping pins (D9 = BOOT button) |

### Firmware changes this pin map needs
1. **Button polarity:** the stage 1 code uses an internal pull-up with the button to GND. The hardware interlock needs the button to switch 3V3, so the pin reads **HIGH = pressed** with an external pull-down.
2. **INA228, not INA219:** use a different library and registers. The board's 15 mΩ shunt covers about 10 A, so there's no shunt swap.

## Power path (stage 2)

```
Cell+ ─ 5 A fuse ─ protection board ─┬─ XIAO BAT+  (470 µF bulk cap here)
                                     └─ INA228 ─ coil+ (dummy 1.5 Ω)
                                                  │
                                                coil−
                                                  │
                     ┌────────────────────────────┴─────────────┐
               interlock FET (gate = button 3V3 via 1 k,   15 Ω 1 W
                              100 k pull-down)                  │
                     │                                   ohm-check FET (D7)
               main FET (D10, PWM 20 kHz)                       │
                     │                                          │
Cell− ───────────────┴──────────────────────────────────────────┘
```

- **Hardware interlock:** the coil can only conduct while the button is physically held *and* firmware drives D10. If the firmware hangs, releasing the button stops the fire.
- **Resistance check:** turn on the D7 FET only, then read current with the INA228. R_coil = V / I − 15 Ω − FET resistance. That's about 0.25 A even into a dead short. Fire the main FET only when R is about 0.8–4 Ω.
- **Peak current:** with PWM every pulse is full cell voltage: 4.2 V / 1.2 Ω ≈ 3.5 A. Size everything for ≥ 4 A.
- **Low-voltage lockout:** measure the battery during the fire. The cell, protection board, fuse and wiring together drop about 0.5 V at 3.5 A. Lock out at about 3.2–3.3 V under load. The protection board's ~2.4 V cutoff is only a backstop.
- **High-current wiring:** use 18 AWG, soldered. Don't run it through the breadboard, whose contacts are rated about 1 A.
- **Dummy load:** the HS25 is only rated 25 W when bolted to an aluminium plate or heatsink.
- **Cell:** your own 18650 must be a genuine high-drain cell rated ≥ 10 A continuous (e.g. Molicel P28A, Samsung 30Q, Sony VTC6). Never solder to it; use the holder.
- **Charge / first-fire tests:** do them inside the LiPo bag.

## Parts list

AUD inc GST, checked 2026-11-07. Already on hand: breadboard + jumpers, soldering
iron, multimeter, resistor/LED/cap assortment, 18650 cell. `parts-photos.html`
(not committed) has a numbered photo of each.

### Core Electronics

| # | Qty | Part | Each |
|---|---|---|---|
| 1 | 2 | Seeed Studio XIAO ESP32-C3 (one spare) | $10.65 |
| 2 | 1 | Adafruit INA228 power monitor (ADA5832) | $29.10 |
| 3 | 3 | SOIC8 / SOT23-6 to DIP adapter (SOT-23-3 fits pads 1, 3, 5) | $2.30 |
| 4 | 1 | Tactile switch 12 mm, 10-pack (ADA1119) | $5.90 |
| 5 | 1 | Silicone wire 18 AWG, 1 m red & black (FIT0585) | $5.28 |
| 6 | 1 | LiPo safety bag 18×23 cm | $7.35 |
| 7 | 1 | Monochrome 0.91" 128×32 I2C OLED (SSD1306) | $16.14 |

### DigiKey Australia

| # | Qty | Part (DigiKey PN) | Each |
|---|---|---|---|
| 8 | 5 | AO3400A N-FET SOT-23 (785-1000-1-ND): main, interlock, ohm-check, 2 spare | $0.77 |
| 9 | 1 | Ohmite HS25 1R5 J, 1.5 Ω 25 W dummy load (HS251R5J-ND) | $10.16 |
| 10 | 3 | Littelfuse 0251005.MXL, 5 A fast axial fuse (F2317-ND) | $1.83 |
| 11 | 3 | Vishay PR01000101509JR500, 15 Ω 1 W (56-PR01000101509JR500CT-ND) | $0.40 |
| 12 | 2 | Panasonic EEU-FR1A471, 470 µF 10 V low-ESR (P124219-ND) | $1.05 |
| 13 | 1 | Keystone 1043P, 18650 holder (36-1043P-ND) | $5.72 |

DigiKey ships free over AU$60.

### AliExpress

| # | Qty | Part |
|---|---|---|
| 14 | 2 | 1S Li-ion protection board, ≥ 8–10 A (DW01 + several paralleled FETs). Search "1S 3.7V 10A lithium protection board". Avoid bare 2–3 A single-FET boards |

Needed before the cell is connected (stage 2), not for stage 1.

## Later

- Stage 3: 510 connector. With a low-side switch the threaded body is the
  *switched negative* and the centre pin is always at battery voltage, so
  insulate the thread from cell−.
- Final build MOSFET: DFN package with a thermal pad, Rds(on) < 15 mΩ at Vgs = 2.5 V.
- Check Australia's vaping laws (pharmacy-only supply since July 2024) before stage 3.
