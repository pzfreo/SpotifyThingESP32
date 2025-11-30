Master Engineering Prompt: ESP32 Case Project
Role: Act as a Senior Mechanical Design Engineer. Write a Python script using the build123d library to generate a parametric 3D printable enclosure (Body + Fascia Lid).

1. Library & Environment Constraints

Library: build123d (Algebra mode preferred).

Visualizer: ocp_vscode (Show Body, Lid, and transparent "Ghost" electronics).

Orientation: Z-up. Center components relative to Origin (0,0).

Output: Export case_body.stl and fascia_lid.stl.

2. Global Case Settings

Walls/Floor: 2.0mm thickness.

Fascia (Lid): 4.0mm thickness (increased for robust screw seating).

Internal Height: 45.0mm (to accommodate stacked components).

Fit Tolerance: 0.6mm (0.3mm gap per side) applied to all internal PCB holders.

3. Component Dimensions (The "Ghosts")

Display: PCB 108x62x1.7mm. Screen glass 95x62x4mm (Centered on PCB).

Buttons: 3x Tactile. Cap 7mm dia. Body 12mm dia x 20mm depth. Spacing 18mm.

ESP32: Body 51x28mm. Aerial extension 7mm(L) x 18mm(W).

USB-C Trigger: 28mm(L) x 11mm(W) x 4.5mm(H). Connector bottom is 3.6mm from floor.

DRV8833 Driver: 18.5mm(W) x 21.0mm(L).

4. Layout Strategy

Display: Top center (Y offset +10mm).

Buttons: Centered row below the display.

ESP32: Bottom Center (Y = -35). Rotated 180° so aerial points RIGHT.

Right Wall Column:

Top: USB-C Trigger.

Bottom: DRV8833 (Shifted to Y = -39 to avoid collision with display posts).

X-Position: X = 48.0mm (Ensures clearance for ESP32 aerial).

Left Wall: Completely empty (Reserved for USB cable access to ESP32).

5. Mounting Logic (Critical Engineering Features)

Display "Lighthouse" Standoffs:

Base: Conical, tapering from 12mm dia (floor) to 6mm dia (shoulder).

Shoulder Height: Calculated so Screen Glass is flush with the top of the 4mm Lid.

Pins: 2.0mm diameter (undersized for "Float/Tolerance").

Mounting Holes: Located 3.0mm from Long Edge (X) and 3.5mm from Short Edge (Y).

PCB "Fences":

Use 6mm high walls to hold ESP32, USB, and DRV8833 boards friction-fit.

ESP32: Must have cutout on Right wall for Aerial.

USB Trigger: Must have cutout on Right wall for Connector.

Lid Alignment:

Fascia underside must have blind "Trap" holes (3.0mm dia) to capture the Display Pins.

6. Assembly & Cutouts

Main Screws: 4x Corner posts.

Screw Type: M3 with 5.0mm Head.

Lid Holes: Counterbored.

Counterbore: 1.5mm deep (leaving 2.5mm solid shelf).

External USB-C Access:

Right wall hole for Trigger board.

Size: 10.0mm x 4.5mm (Stadium shape, includes print tolerance).

Alignment: Z-height derived from floor + 3.6mm connector offset.