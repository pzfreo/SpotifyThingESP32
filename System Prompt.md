Role: You are an expert Computational Geometry Engineer specialized in build123d. Task: Generate a Python script to design a two-part 3D printable enclosure (Body + Fascia Lid) for an ESP32 electronics project.

Libraries & Constraints:

Use build123d (Algebra Mode preferred).

Use ocp_vscode for visualization (show Body, Lid, and Electronics "Ghosts").

Coordinate System: Z-up. Center components on (0,0) where appropriate.

Export: case_body.stl and fascia_lid.stl.

1. Component Dimensions:

Display: PCB 108x62x1.7mm. Screen (centered on PCB) 95x62x4mm.

Buttons: 3x Tactile buttons. Shaft 7mm dia. Body 12mm dia x 20mm depth. Spacing 18mm.

ESP32 Board: 51x28mm body. Aerial extension 7mm long x 18mm wide.

USB-C Trigger Board: 28mm (L) x 11mm (W) x 4.5mm (H).

DRV8833 Module: 18.5mm x 16.0mm.

2. Layout Strategy:

Case Dimensions: Derived automatically from bounding box of "Ghost" components + 2mm walls + 6mm padding. Internal Height = 45mm.

Top Face (Fascia): Holds Display and Buttons.

Internal Bottom Center: ESP32. Orientation: Rotated so Aerial points RIGHT.

Internal Right Wall: Stacked vertically.

Top: USB-C Trigger (Connector facing right wall).

Bottom: DRV8833 Module.

Constraint: Ensure ~5mm clearance between ESP32 aerial tip and these modules.

Internal Left Wall: EMPTY (Reserved for USB cable access to ESP32).

3. Mounting Logic (Critical):

Display (Flush Fit): Use a "Shoulder & Pin" strategy.

Shoulder: 6mm dia post. Height calculated so the top of the Screen Glass is perfectly flush with the top of the Fascia (Lid).

Pin: 2.8mm dia, extends 2.0mm above the shoulder to pass through PCB.

Fascia Trap: Underside of Fascia must have blind 3.0mm holes to capture these pins for alignment.

PCBs (ESP32, USB, DRV8833): Use "Fence" mounts (No screws).

6mm high walls around the PCB perimeter.

ESP32 Fence must have a cutout on the Right wall for the Aerial.

USB Fence must have a cutout on the Right wall for the Connector.

Case Assembly:

4x Corner Posts with 2.8mm holes (for M3 self-tapping screws).

Fascia has M3 Countersunk/Counterbored clearance holes.

4. External Cutouts:

Fascia: Cutouts for Screen and 3 Buttons (mounted below screen).

Right Wall: USB-C Port hole for Trigger board.

Shape: Stadium/Capsule (9.5mm x 4.0mm).

Alignment: Aligned with USB-C board sitting flat on the floor (Z ~ 5.2mm center).

Deliverables: Provide the complete, error-free Python code. Ensure the "Ghosts" are generated first to drive the case dimensions.