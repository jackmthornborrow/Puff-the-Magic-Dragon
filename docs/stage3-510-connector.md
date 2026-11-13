# Stage 3: taking the 510 connector out of the donor pen

Do this only after stage 2 passes every line of the
[first-power checklist](first-power-checklist.md) on the dummy load.

The donor is the 510-thread pen bought on Amazon AU. I have not seen inside
this exact model, so the steps describe how these pens are normally built and
tell you what to look for. If yours differs, stop and send me a photo.

## What you are after

One part: the threaded metal socket at the top. It has two contacts.

- **Centre pin:** a small sprung or fixed pin in the middle, held in a
  rubber or plastic ring that insulates it from the body.
- **Body:** the threaded shell around it.

Everything else in the pen (cell, button board, charge board) is not reused.

## Safety: the pen has a lithium cell inside

It is a small one, but it is the only real hazard in this job.

- Work on a tile or metal tray, not carpet or the bed. Safety glasses on.
- **Run it down first.** Hold the pen's button in 10-second bursts until it
  will no longer heat, so the cell holds as little energy as possible.
- Never cut both cell wires at once. The cutters join them and short the cell.
  Cut one, tape its end, then cut the other and tape that.
- Never pry against, squeeze, bend or pierce the cell. If it is glued in,
  leave it in the tube and work around it.
- A cell that hisses, swells or gets hot goes straight into the steel tin,
  outside, until it is cold.
- Afterwards, tape both cell wires separately and take the cell to a battery
  recycling drop-off (most Bunnings, Officeworks, Woolworths and Aldi stores
  have a B-cycle bin). Do not bin it.

## Tools

Small flat screwdriver or plastic spudger, long-nose pliers, side cutters,
soldering iron, multimeter, tape. A vice or clamp with soft jaws helps.

## Steps

### 1. Find how it comes apart

Look at both ends before forcing anything.

- **Bottom cap:** usually a press-fit plug holding the charge socket and LED.
  It often pulls out with pliers or levers out with a fingernail gap. This is
  the usual way in.
- **Top (510 end):** the connector is normally a press-fit or a threaded
  ring in the end of the tube, sometimes with a dab of glue.
- **Screws** hidden under a sticker or the button: undo them if you find any.

### 2. Open the far end and cut the cell free

1. Pull the bottom cap and ease the board out a few millimetres.
2. Find the two wires that run from the cell to the board (usually red and
   black, soldered to the cell's tabs).
3. Cut one cell wire close to the board. Tape the cell-side end.
4. Cut the other. Tape it.
5. The pen is now dead. Check with the multimeter across the 510 socket,
   centre pin to body: 0 V, with the button pressed and not.

### 3. Free the connector

There will be two wires, or one wire and a metal tab, going to the back of
the connector: one to the centre pin, one to the body.

1. Push the connector out of the tube from the inside with a blunt rod if
   you can reach, or grip the tube in the vice (soft jaws, gently) and work
   the connector out with pliers on its rim, twisting as you pull.
2. If it will not move, warm the end of the tube with a hair dryer for a
   minute to soften the glue. Do not use a flame or the soldering iron on the
   outside: the cell is a few centimetres away.
3. As it comes out, leave its wires as long as possible. Cut them at the
   board end, not at the connector.

If the tube has to be cut to get it out, cut around the tube at the top,
well clear of the cell, with a junior hacksaw. Turn the tube as you go and
stop as soon as you are through the wall.

### 4. Check it

| Measure | Good |
|---|---|
| Ω: centre pin to body, nothing screwed in | Open (OL) |
| Ω: centre pin to its wire | Under 0.5 Ω |
| Ω: body to its wire | Under 0.5 Ω |
| Screw in a cartridge finger-tight. Ω: centre wire to body wire | The cartridge's resistance, typically 1.2–2.2 Ω. Note it down |
| Look | Insulating ring intact, pin springs back if it is a sprung type, threads clean |

### 5. Fit new leads

The original wires are thin. Replace them if they are thinner than about
22 AWG or shorter than 5 cm.

1. Use 20 AWG silicone wire if you have it, 18 AWG otherwise. Red to the
   centre pin, black to the body.
2. **Unscrew the cartridge first.** Heat travels up the pin into it.
3. Be quick at the centre pin: tin the wire, then join in under 3 seconds.
   The insulating ring softens with heat, and if it deforms the pin can
   short to the body. Let it cool fully and repeat the "open" check from
   step 4.
4. Heat-shrink both joints.

## Wiring it to Puff

| Connector | Goes to | Why |
|---|---|---|
| Centre pin (red) | J-COIL **COIL+** | Always at battery voltage |
| Body / thread (black) | J-COIL **COIL−** | The switched side |

A coil has no polarity, so it would heat either way round. This way round
matters for safety: the exposed metal body is the side the MOSFETs control,
not the side wired straight to the battery.

**The body must not touch ground.** Puff switches the negative side, so with
a cartridge fitted the body sits at battery voltage whenever it is not
firing. If the body touches anything grounded (the USB socket's metal shell
or a metal case) current flows through the coil with no
MOSFET, button or firmware in the way. Until there is a plastic enclosure,
wrap the outside of the connector in heat-shrink or Kapton, leaving only the
threads inside exposed.

## First cartridge test

1. Dummy load off, connector on J-COIL, no cartridge. Press the button:
   Puff must refuse (no cartridge).
2. Screw in a cartridge. `status` should report a resistance within about
   0.2 Ω of your step 4 multimeter reading.
3. Lowest preset. One 2-second press on the bench, watching the
   cartridge rather than inhaling from it. You should see light vapour and
   nothing else: no crackle, no smell of burning, connector cool.
4. Step up one preset at a time. For ceramic distillate cartridges the
   useful range is about 2.6–3.3 V; stay at or below 3.6 V.

## Travelling with it

Carry-on only, never checked luggage. Lock it (5 clicks) and, for flights,
take the cell out and carry it in a plastic battery case. A P28A is about
10 Wh, far under airline limits. Check the airline's own battery page before
each trip, and the destination's cannabis laws before taking any cartridge
out of Australia.
