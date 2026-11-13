# First-power checklist

For the stage 2 power board ([layout](stage2-perfboard.md)). Work top to
bottom and do not skip a line because the one before passed. Each phase adds
one source of energy; a fault found in phase A costs nothing, the same fault
found in phase D costs parts.

Phases A to D need no new firmware: they test the hardware by hand.
Phase E needs the stage 2 firmware (INA260 support, active-high button).

## Rules for the whole session

- Board on a non-conductive surface: wood, a ceramic tile, a silicone mat.
  No loose wire offcuts or screws nearby.
- **To kill power fast:** let go of the fire button, then pull the cell out of
  its holder. Practise the reach once before the cell goes in.
- Phases C to E happen with the cell and board inside the steel tin, lid
  resting loose on top or off, never clipped shut.
- Never leave it powered and unattended. Take the cell out when you stop.
- If anything is hot, smells, or a reading is not in the "good" column: cell
  out first, think second.

Multimeter settings used below: **Ω** resistance, **⏯** continuity beep,
**V** DC volts (20 V range), **▷|** diode test.

## Phase A: nothing connected, no power

Board alone on the bench. No breadboard jumpers, no pack, no load, wire F1
**not fitted** yet.

| # | Measure | Good | If not |
|---|---|---|---|
| A1 | Look at every joint with a magnifier, solder side | No whiskers or blobs between neighbouring pads, especially around Q1, Q2 and rows 16–18 | Reflow or wick it off |
| A2 | Ω: J-BAT BAT+ to J-BAT GND | Open (OL) | Short on H1. Find it before going on |
| A3 | Ω: INA260 Vin+ screw to GND | Climbs for a few seconds, settles above 100 kΩ | Under 1 kΩ: C1 backwards or a bridge |
| A4 | Ω: Vin+ screw to Vin− screw | Under 0.5 Ω | INA260 damaged or not seated |
| A5 | ⏯: Vin− screw to J-COIL COIL+ | Beep | F2 or H5 joint |
| A6 | Ω: COIL+ to COIL− | Open | Bridge near J-COIL |
| A7 | Ω: COIL− to GND | Open, or above 100 kΩ | A shorted FET or a bridge on H3/H4 |
| A8 | ▷\|: red on GND, black on Q1 drain (O17). Then red on O17, black on COIL− | 0.4–0.7 V each time (one FET body diode each) | 0 V: that FET is shorted. OL: wrong pin order on the adapter, or an open joint |
| A9 | Ω: J-MCU FIRE to GND | 10 kΩ (±5%) | R1/R2 wrong value or place |
| A10 | Ω: J-MCU BTN to GND | 101 kΩ | R3/R4 |
| A11 | Ω: J-MCU CHK to GND | 100 kΩ | R5/R6 |
| A12 | Ω: hole S16 to hole L14 | 15 Ω (14–16) | R7 or wire W10 |
| A13 | Ω: J-MCU 3V3 to GND | Above 1 kΩ | Short on the INA260 header |
| A14 | Ω: J-MCU SDA to 3V3, then SCL to 3V3 | About 10 kΩ each | Swapped or open I2C wire |
| A15 | ⏯: J-MCU BAT+ to Vin+ screw | Beep | W1/W2 |
| A16 | ⏯: J-BAT GND to J-MCU GND, to N17, to N13 | Beep each time | Break in the spine H2 |

Battery pack, holder still empty:

| # | Measure | Good |
|---|---|---|
| A17 | ⏯: holder `+` pin to red lead | Beep (through the fuse) |
| A18 | ⏯: red lead to black lead | No beep |
| A19 | Look | No bare metal anywhere on the pack except the holder's contacts |

## Phase B: logic only, USB power, no cell

Connect the eight J-MCU jumpers to the breadboard **except BAT+**. F1 still
not fitted. No pack, no load. Plug in USB.

| # | Do / measure | Good | If not |
|---|---|---|---|
| B1 | V: J-MCU 3V3 to GND | 3.25–3.35 V | Unplug. Check for a short at A13 |
| B2 | Touch the INA260 chip and the XIAO | Both cool | Unplug |
| B3 | V: Q2 gate (Q17) to GND, button released, then pressed | 0 V, then 3.2–3.3 V | BTN wiring, button wiring 5B |
| B4 | V: Q1 gate (M17) to GND, idle | 0 V | D10 is being driven; check the firmware is idle |
| B5 | V: Q3 gate (L12) to GND, idle | 0 V | |
| B6 | Unplug USB. Put the meter on the Q1 gate, then plug USB back in and watch it through boot | Never rises above 0.2 V | Missing 10 kΩ pull-down |

## Phase C: cell in, current-limited

The 15 Ω 1 W spare resistor stands in for wire F1. With it in the path, even
a dead short can only draw a quarter of an amp.

Setup: USB unplugged. Board in the tin. Clamp one leg of the 15 Ω in J-LINK
(W4) and the other in the INA260 **Vin+** screw. No load connected. J-MCU
BAT+ jumper still off.

| # | Do / measure | Good | If not |
|---|---|---|---|
| C1 | V: straight across the cell before fitting it | 3.4–4.2 V | Under 3.0 V: charge it in a standalone charger or ask me first |
| C2 | Put the cell in the holder, pack leads not yet connected. V: red lead to black lead | Same as C1 | 0 V is common: the protection board starts up latched off. Carry on; step C4 wakes it |
| C3 | Black lead into J-BAT GND, then red into J-BAT BAT+ | No spark, no heat | Cell out |
| C4 | V: J-BAT BAT+ to GND | Same as C1 | If 0 V: cell out, connect the J-MCU BAT+ jumper, plug USB in for 5 seconds with the cell back in (the charger wakes the protection board), unplug USB, remove the jumper again |
| C5 | V: **across the 15 Ω** | Under 0.01 V after 5 seconds | More means current is flowing with everything off. 1 V = 67 mA. Cell out, back to phase A |
| C6 | V: Vin+ screw to GND | Same as C1 | |
| C7 | Connect the J-MCU BAT+ jumper. The XIAO starts on battery. V across the 15 Ω | 0.2–0.8 V (15–50 mA) | Over 1.5 V: something on the breadboard is drawing too much |
| C8 | V: 3V3 rail on the breadboard | 3.2–3.35 V | The XIAO is browning out through the 15 Ω; remove the OLED for now |

Now the load. Remove the BAT+ jumper again and plug in USB, so the XIAO runs
from USB and the power board runs from the cell through the 15 Ω. Pull the
FIRE jumper off D10 and the CHK jumper off D7 so you can drive them by hand.
Connect the dummy load to J-COIL.

| # | Do / measure | Good | If not |
|---|---|---|---|
| C9 | V: COIL− to GND, everything idle | Within 0.1 V of the cell voltage (pulled up through the load) | 0 V: a FET is conducting when it should be off. Cell out |
| C10 | V across the 15 Ω, idle | Under 0.01 V | |
| C11 | Touch the loose CHK jumper to the `+` rail (3V3). V across the 15 Ω | A little under half the cell voltage (1.6–2.0 V) | 0 V: Q3 path open. Check Q3's pin order and R7 |
| C12 | Let go of CHK. V across the 15 Ω | Back under 0.01 V within a second | |
| C13 | Loose FIRE jumper to `+` rail, button **not** pressed | Under 0.01 V. The interlock is holding | Any current: Q2 is shorted or its gate is floating |
| C14 | FIRE still on `+`, **press the button** | About 90% of the cell voltage (3.2–3.8 V) across the 15 Ω | 0 V: Q1 or Q2 not switching. Check gate voltages as in B3 |
| C15 | Release the button, FIRE still on `+` | Drops to zero at once | |
| C16 | FIRE jumper to `−` rail, press the button | Under 0.01 V | Q1 is shorted |

C13 to C16 are the truth table for the hardware interlock: current flows only
when the button **and** the FIRE line are both on.

## Phase D: full current, by hand

Cell out. Replace the 15 Ω with wire F1 (18 AWG, both screws tight). Dummy
load bolted to its plate and connected. XIAO on USB, BAT+ jumper off, FIRE
jumper loose as in phase C. Cell back in.

| # | Do / measure | Good | If not |
|---|---|---|---|
| D1 | V: Vin+ screw to GND, idle | Cell voltage | |
| D2 | V: COIL− to GND, idle | Cell voltage | |
| D3 | Meter on Vin+ to GND. FIRE to `+` rail, press the button for **2 seconds**, read, release | Drops by 0.1–0.4 V while pressed, recovers on release. About 2.5 A is flowing | Drops more than 0.7 V: a poor joint, a tired cell or a weak holder contact |
| D4 | Same again with the meter across the load (COIL+ to COIL−) | Within 0.3 V of the D3 reading | Lower: voltage is being lost in the FETs or wiring |
| D5 | Same again, meter from COIL− to GND | Under 0.25 V (both FETs together) | Higher: FET not fully on. Check the gate reads 3.2 V or more |
| D6 | Straight after a 5-second press, touch Q1, Q2, the fuse, each screw terminal | Warm at most. The load itself gets properly hot; that is its job | Anything too hot to hold a finger on: stop and find the resistance |
| D7 | Put the FIRE jumper back on D10, CHK back on D7, BAT+ jumper back on | | |

Write the D3–D5 numbers in the log below. They become the calibration
figures for the firmware's voltage-drop compensation.

## Phase E: with stage 2 firmware

Serial needs USB plugged in, and USB plugged in normally means "charging,
refuse to fire". The stage 2 firmware needs a bench setting that lifts that
lockout for E1 to E6; E8 then checks the lockout with the setting off.

| # | Do | Good |
|---|---|---|
| E1 | `status` over serial, load connected | Battery within 0.05 V of the multimeter. Coil reads 1.4–1.6 Ω |
| E2 | Disconnect the load, press the button | Refused: no cartridge |
| E3 | Clamp a short wire across J-COIL, press the button | Refused: short. Nothing gets warm |
| E4 | Load back on, lowest preset, hold 2 seconds | Fires, logs a puff |
| E5 | Hold for 10 seconds | Cuts off at 10 s |
| E6 | Pull the load lead out mid-fire | Stops within a moment, logs why |
| E7 | Unplug USB and repeat E4 on battery alone | Fires. No reset or brownout |
| E8 | Plug in USB, press the button | Refused: charging |

## First charge

In the tin, on the tile, and stay with it for the first one.

1. Cell in, everything connected normally, plug in USB.
2. The XIAO charges at about 0.38 A, so a P28A from half empty takes around
   four hours and from flat about eight.
3. Every half hour: battery voltage rising, cell at room temperature, XIAO
   only slightly warm.
4. It is done when the voltage sits at 4.15–4.20 V and stops rising. Above
   4.25 V: unplug.

## Measurement log

| Reading | Value | Date |
|---|---|---|
| Cell voltage as received (C1) | | |
| Idle current, power board only (C5 volts ÷ 15) | | |
| Idle current with XIAO (C7 volts ÷ 15) | | |
| Battery voltage idle / while firing (D3) | | |
| Voltage across the load while firing (D4) | | |
| Voltage across both FETs while firing (D5) | | |
| Dummy load resistance, by multimeter | | |
| Dummy load resistance, as Puff reports it (E1) | | |
| Voltage at end of first charge | | |
