"""Puff enclosure, v0 concept: upright rounded bar with the cartridge standing
out of the top like a mouthpiece.

Setup (once): python3.12 -m venv cad/.venv && cad/.venv/bin/pip install -r cad/requirements.txt
Run:  cad/.venv/bin/python cad/puff_body.py
Out:  cad/out/ puff_front.stl + puff_back.stl (print), puff_assembly.step (Fusion/Onshape),
      preview.png. Only preview.png is committed; the rest is regenerated.

All sizes in mm. X = width, Y = thickness (front is -Y), Z = height.
Change the numbers in the PARAMETERS block and re-run.
"""
from pathlib import Path

from build123d import *

OUT = Path(__file__).parent / "out"
OUT.mkdir(exist_ok=True)

# ---------------------------------------------------------------- PARAMETERS
WALL = 2.0            # side walls
END_WALL = 3.0        # top and bottom walls
PLAN_R = 7.0          # corner radius seen from above
EDGE_R = 3.0          # rounding of the top and bottom edges

CELL_D, CELL_L = 18.6, 65.2      # 18650 (Molicel P28A, flat top)
BAY_D = CELL_D + 0.6             # cell bay bore
BAY_L = 74.0                     # cell + contact tabs/springs at both ends
DIVIDER = 1.2                    # wall between cell bay and electronics

COL_W = 30.0                     # electronics column width (OLED module is 27.3)
CAV_R = 4.0                      # electronics cavity corner radius

CART_D, CART_L = 10.5, 62.0      # 1 mL 510 cartridge incl. mouthpiece
CART_RECESS = 10.0               # how far the cartridge base sits below the top
WELL_D = CART_D + 1.1            # well bore (gap doubles as the air inlet)
CONN_D, CONN_L = 12.0, 9.0       # 510 connector. PLACEHOLDER: measure the salvaged one

OLED_PCB = (27.3, 27.8, 1.2)     # 0.96" SSD1306 module
OLED_GLASS = (26.7, 19.3, 1.5)
OLED_WIN = (24.0, 13.0)          # window in the front wall
OLED_SETBACK = 1.2               # glass sits this far behind the front wall (clears the corners)
XIAO = (17.8, 21.0, 1.2)         # XIAO ESP32-C3, USB-C pointing down
USB = (9.0, 7.4, 3.2)            # receptacle W x L x H
INA = (22.9, 22.8, 1.6)          # Adafruit INA260 (bench part; final PCB replaces it)
BTN_BODY, BTN_CAP_D = 12.0, 9.6  # 12 mm tactile switch with round cap
BTN_HOLE_D = 10.2

SCREW_PILOT, SCREW_CLEAR, SCREW_HEAD = 1.7, 2.3, 4.2   # M2 self-tapping
BOSS_D = 5.0

# ------------------------------------------------------------------ DERIVED
T = BAY_D + 2 * 2.4
W = WALL + BAY_D + DIVIDER + COL_W + WALL
H = BAY_L + 2 * END_WALL
X_CELL = -W / 2 + WALL + BAY_D / 2
X_COL0 = X_CELL + BAY_D / 2 + DIVIDER          # left edge of electronics cavity
X_COL = X_COL0 + COL_W / 2                     # centre of electronics column
CAV_D = T - 2 * WALL                           # cavity depth front to back
Z_CONN_TOP = H - CART_RECESS
Z_CONN_BOT = Z_CONN_TOP - CONN_L
Z_CAV_TOP = Z_CONN_BOT - 1.5
Z_CAV_BOT = END_WALL
Y_FRONT_IN = -CAV_D / 2                        # inside of front wall
Y_BACK_IN = CAV_D / 2

Z_OLED = Z_CAV_TOP - 2.0 - OLED_PCB[1] / 2     # OLED module centre
Z_WIN = Z_OLED + 2.0                           # glass sits high on the module
Z_BTN = Z_OLED - OLED_PCB[1] / 2 - 3.0 - BTN_BODY / 2
Y_XIAO = Y_BACK_IN - 1.0 - XIAO[2] / 2         # XIAO PCB centre, parts face the front
Y_USB = Y_XIAO - XIAO[2] / 2 - USB[2] / 2
SCREWS = [  # (x, z): two in the solid block beside the well, two low in the cavity
    (X_COL - 10.0, H - 8.0), (X_COL + 10.0, H - 8.0),
    (X_COL0 + 2.6, 6.0), (X_COL0 + COL_W - 3.4, 6.0),
]

BOT = (Align.CENTER, Align.CENTER, Align.MIN)


def ycyl(d, length, x, z, y0):
    """Cylinder along Y starting at y0, running towards +Y."""
    return Pos(x, y0 + length / 2, z) * Rot(X=90) * Cylinder(d / 2, length)


# ---------------------------------------------------------------------- BODY
body = extrude(RectangleRounded(W, T, PLAN_R), H)
body = fillet(body.edges().group_by(Axis.Z)[0] + body.edges().group_by(Axis.Z)[-1], EDGE_R)

cell_bay = Pos(X_CELL, 0, END_WALL) * Cylinder(BAY_D / 2, BAY_L, align=BOT)
cavity = Pos(X_COL, 0, Z_CAV_BOT) * extrude(RectangleRounded(COL_W, CAV_D, CAV_R), Z_CAV_TOP - Z_CAV_BOT)
well = Pos(X_COL, 0, Z_CONN_TOP) * Cylinder(WELL_D / 2, CART_RECESS + 1, align=BOT)
conn_pocket = Pos(X_COL, 0, Z_CONN_BOT) * Cylinder((CONN_D + 0.3) / 2, CONN_L, align=BOT)
wire_hole = Pos(X_COL, 0, Z_CAV_TOP - 1) * Cylinder(3.0, 4, align=BOT)
# wire passages through the divider, top and bottom of the cell
passages = [
    Pos(X_COL0 - DIVIDER / 2, 0, z) * Box(DIVIDER + 2, 5, 6)
    for z in (END_WALL + 4, Z_CAV_TOP - 4)
]
body -= [cell_bay, cavity, well, conn_pocket, wire_hole] + passages

# screw bosses across the cavity (the top two are in solid plastic already)
for x, z in SCREWS[2:]:
    body += ycyl(BOSS_D, CAV_D, x, z, Y_FRONT_IN)

# front openings
body -= Pos(X_COL, -T / 2, Z_WIN) * Box(OLED_WIN[0], 3 * WALL, OLED_WIN[1])
body -= ycyl(BTN_HOLE_D, 3 * WALL, X_COL, Z_BTN, -T / 2 - WALL)
# USB-C: slot for the receptacle plus a relief for the plug's overmould
usb_slot = Pos(X_COL, Y_USB, -1) * extrude(RectangleRounded(USB[0] + 0.6, USB[2] + 0.5, 1.6), END_WALL + 2)
usb_relief = Pos(X_COL, Y_USB, -1) * extrude(RectangleRounded(12.6, 7.0, 3.0), END_WALL - 1.2 + 1)
body -= [usb_slot, usb_relief]

# screws: enter from the back, bite into the front half
for x, z in SCREWS:
    body -= ycyl(SCREW_PILOT, 7.0, x, z, -7.0)                 # pilot in front half
    body -= ycyl(SCREW_CLEAR, T, x, z, 0.0)                    # clearance in back half
    body -= ycyl(SCREW_HEAD, 3.0, x, z, T / 2 - 2.2)           # head recess

big = Box(4 * W, 2 * T, 4 * H)
front = body & (Pos(0, -T, 0) * big)
back = body & (Pos(0, T, 0) * big)

# ----------------------------------------------------------- REFERENCE PARTS
cell = Pos(X_CELL, 0, END_WALL + (BAY_L - CELL_L) / 2) * Cylinder(CELL_D / 2, CELL_L, align=BOT)
conn = Pos(X_COL, 0, Z_CONN_BOT) * Cylinder(CONN_D / 2, CONN_L, align=BOT)
cart = (Pos(X_COL, 0, Z_CONN_TOP) * Cylinder(CART_D / 2, CART_L - 12, align=BOT)
        + Pos(X_COL, 0, Z_CONN_TOP + CART_L - 12) * Cylinder(4.2, 12, align=BOT))
y_oled = Y_FRONT_IN + OLED_SETBACK + OLED_GLASS[2] + OLED_PCB[2] / 2
oled = (Pos(X_COL, y_oled, Z_OLED) * Box(OLED_PCB[0], OLED_PCB[2], OLED_PCB[1])
        + Pos(X_COL, Y_FRONT_IN + OLED_SETBACK + OLED_GLASS[2] / 2, Z_WIN) * Box(OLED_GLASS[0], OLED_GLASS[2], OLED_GLASS[1]))
z_xiao = Z_CAV_BOT + XIAO[1] / 2
xiao = (Pos(X_COL, Y_XIAO, z_xiao) * Box(XIAO[0], XIAO[2], XIAO[1])
        + Pos(X_COL, Y_USB, Z_CAV_BOT - 1.5 + USB[1] / 2) * Box(USB[0], USB[2], USB[1]))
ina = Pos(X_COL, Y_XIAO, z_xiao + XIAO[1] / 2 + 2 + INA[1] / 2) * Box(INA[0], INA[2], INA[1])
btn = (Pos(X_COL, Y_FRONT_IN + 2.0, Z_BTN) * Box(BTN_BODY, 4.0, BTN_BODY)
       + ycyl(BTN_CAP_D, 4.5, X_COL, Z_BTN, -T / 2 - 1.0))
parts = {"cell": cell, "connector": conn, "cartridge": cart, "oled": oled,
         "xiao": xiao, "ina260": ina, "button": btn}

# ------------------------------------------------------------------- CHECKS
print(f"Body: {W:.1f} wide x {T:.1f} thick x {H:.1f} tall; cartridge stands {CART_L - CART_RECESS:.0f} above")
print(f"Electronics cavity: {COL_W:.1f} x {CAV_D:.1f} x {Z_CAV_TOP - Z_CAV_BOT:.1f}")
shell = front + back
for name, p in parts.items():
    v = (p & shell).volume
    print(f"  {name:10s} overlap with body: {v:7.1f} mm3" + ("  <-- COLLISION" if v > 1 else ""))
names = list(parts)
for i, a in enumerate(names):
    for b in names[i + 1:]:
        v = (parts[a] & parts[b]).volume
        if v > 1:
            print(f"  {a} hits {b}: {v:.1f} mm3  <-- COLLISION")

# ------------------------------------------------------------------ EXPORTS
export_stl(front, str(OUT / "puff_front.stl"))
export_stl(back, str(OUT / "puff_back.stl"))
asm = Compound(label="puff", children=[
    Compound(label="front", children=[front]), Compound(label="back", children=[back]),
    *[Compound(label=k, children=[v]) for k, v in parts.items()]])
export_step(asm, str(OUT / "puff_assembly.step"))


def drawing(shapes, eye, fname):
    """Hidden-line drawing of the shapes as one object."""
    vis, _ = Compound(children=list(shapes)).project_to_viewport(
        eye, viewport_up=(0, 0, 1), look_at=(0, 0, H / 2 + 15))
    svg = ExportSVG(scale=4, margin=10)
    svg.add_layer("vis", line_weight=0.35)
    svg.add_shape(vis, layer="vis")
    svg.write(str(OUT / fname))


allparts = list(parts.values())
drawing([front, back, cart, btn], (-140, -260, 190), "view_outside.svg")
drawing([back] + allparts, (-90, -300, 150), "view_inside.svg")


def preview(fname):
    """Shaded picture: outside, inside (front half removed), back, underside."""
    import numpy as np
    import pyvista as pv

    colours = {"shell": "#2f6f8f", "cell": "#7cb342", "connector": "#b0892f", "cartridge": "#e6c66a",
               "oled": "#10131f", "xiao": "#3a3a3a", "ina260": "#5e35b1", "button": "#d84315"}

    def mesh(shape):
        verts, faces = shape.tessellate(0.05, 0.2)
        v = np.array([(p.X, p.Y, p.Z) for p in verts])
        f = np.hstack([np.full((len(faces), 1), 3), np.array(faces)]).ravel()
        return pv.PolyData(v, f)

    shell = [("shell", front), ("shell", back)]
    views = [("Outside", shell + [("cartridge", cart), ("button", btn), ("oled", oled)], (-0.45, -1, 0.35)),
             ("Front half removed", [("shell", back)] + list(parts.items()), (-0.25, -1, 0.3)),
             ("Back", shell + [("cartridge", cart)], (0.45, 1, 0.35)),
             ("Underside", shell + [("xiao", xiao), ("button", btn)], (-0.3, -0.8, -1))]
    pl = pv.Plotter(off_screen=True, shape=(1, 4), window_size=(2200, 1000), border=False)
    for i, (title, items, d) in enumerate(views):
        pl.subplot(0, i)
        for k, shp in items:
            pl.add_mesh(mesh(shp), color=colours[k], smooth_shading=True, split_sharp_edges=True,
                        specular=0.2, ambient=0.25)
        pl.add_text(title, font_size=11, color="black")
        focus = np.array([0, 0, H / 2 + (0 if "Under" in title else 22)])
        pl.camera.focal_point = focus
        pl.camera.position = focus + 400 * np.array(d) / np.linalg.norm(d)
        pl.camera.up = (0, 0, 1)
        pl.enable_parallel_projection()
        pl.camera.parallel_scale = 50 if "Under" in title else 72
    pl.set_background("white")
    pl.screenshot(str(OUT / fname))


preview("preview.png")
print("written to", OUT)
