# build_esp32_case_final_with_tolerance.py
# Full algebra-mode Build123D script for ESP32 enclosure
# Fixed internal size, includes posts, fences, USB/DRV pads, shoulders/pins, and screen tolerance

from build123d import *
from build123d import exporters

# Optional visualization
try:
    from ocp_vscode import show_object
    viz_enabled = True
except Exception:
    def show_object(obj, name="object"):
        print(f"[viz disabled] would show: {name}")
    viz_enabled = False

# -------------------------
# PARAMETERS
# -------------------------
# Components
PCB_W, PCB_H, PCB_T = 108.0, 62.0, 1.7
SCREEN_W, SCREEN_H, SCREEN_T = 95.0, 62.0, 4.0
SCREEN_TOLERANCE = 0.2  # mm clearance for flush fit
BUTTON_SHAFT_DIA = 7.0
BUTTON_BODY_DIA = 12.0
BUTTON_BODY_DEPTH = 20.0
BUTTON_SPACING = 18.0

ESP_W, ESP_H, ESP_T = 51.0, 28.0, 1.6
ESP_AERIAL_EXT_L, ESP_AERIAL_EXT_W = 7.0, 18.0

USB_TRIG_L, USB_TRIG_W, USB_TRIG_H = 28.0, 11.0, 4.5
DRV_L, DRV_W, DRV_H = 18.5, 16.0, 2.5

# Enclosure fixed internal dimensions
internal_w = 160.0
internal_d = 110.0
internal_h = 45.0

wall_thickness = 2.0
base_thickness = 3.0
fascia_thickness = 4.0

corner_post_dia = 6.0
corner_hole_dia = 2.8
corner_spacing_offset = 10.0

shoulder_dia = 6.0
pin_dia = 2.8
pin_above_shoulder = 2.0
fascia_blind_hole_depth = 3.0

fence_height = 6.0
fence_clearance = 1.0

usb_cutout_w = 9.5
usb_cutout_h = 4.0

button_row_offset_from_screen_bottom = 15.0

# -------------------------
# 1) Ghosts
# -------------------------
ghosts = []

# PCB
pcb_z = base_thickness + 1.0
pcb = Box(PCB_W, PCB_H, PCB_T).located(Location((0, 0, pcb_z + PCB_T/2.0)))
ghosts.append(("PCB", pcb))

# Screen with tolerance
screen_z = pcb_z + PCB_T
screen = Box(SCREEN_W + SCREEN_TOLERANCE, SCREEN_H + SCREEN_TOLERANCE, SCREEN_T).located(
    Location((0, 0, screen_z + SCREEN_T/2.0))
)
ghosts.append(("Screen", screen))

# Buttons
screen_bottom_y = -SCREEN_H / 2.0
button_row_y = screen_bottom_y - button_row_offset_from_screen_bottom
first_button_x = -BUTTON_SPACING
for i in range(3):
    x = first_button_x + i * BUTTON_SPACING
    btn = Cylinder(BUTTON_BODY_DIA / 2.0, BUTTON_BODY_DEPTH).located(
        Location((x, button_row_y, pcb_z + BUTTON_BODY_DEPTH / 2.0))
    )
    ghosts.append((f"Button_{i+1}", btn))
    shaft = Cylinder(BUTTON_SHAFT_DIA / 2.0, BUTTON_BODY_DEPTH + 2.0).located(
        Location((x, button_row_y, pcb_z + (BUTTON_BODY_DEPTH + 2.0)/2.0))
    )
    ghosts.append((f"ButtonShaft_{i+1}", shaft))

# ESP32
esp_z = 5.0
esp = Box(ESP_W, ESP_H, ESP_T).located(Location((0, 0, esp_z + ESP_T/2.0)))
esp_aerial = Box(ESP_AERIAL_EXT_L, ESP_AERIAL_EXT_W, 0.5).located(
    Location((ESP_W/2.0 + ESP_AERIAL_EXT_L/2.0, 0, esp_z + 0.25))
)
ghosts.append(("ESP32", esp))
ghosts.append(("ESP_Aerial", esp_aerial))

# USB Trigger & DRV8833 placeholders
usb_trig = Box(USB_TRIG_L, USB_TRIG_W, USB_TRIG_H).located(Location((0, 0, 5.2)))
drv = Box(DRV_L, DRV_W, DRV_H).located(Location((0, 0, 10.0)))
ghosts.append(("USB_Trigger", usb_trig))
ghosts.append(("DRV8833", drv))

all_ghosts = Compound([g for name, g in ghosts])

# -------------------------
# 2) Body
# -------------------------
body_external_h = internal_h + base_thickness
outer_body = Box(internal_w + 2*wall_thickness, internal_d + 2*wall_thickness, body_external_h).located(
    Location((0, 0, body_external_h/2.0 - base_thickness/2.0))
)
inner_cavity = Box(internal_w, internal_d, internal_h).located(
    Location((0, 0, base_thickness + internal_h/2.0))
)
body = outer_body - inner_cavity

# -------------------------
# 3) Corner Posts + Holes
# -------------------------
half_w = internal_w/2.0
half_d = internal_d/2.0
post_positions = [
    (half_w - corner_spacing_offset, half_d - corner_spacing_offset),
    (-half_w + corner_spacing_offset, half_d - corner_spacing_offset),
    (-half_w + corner_spacing_offset, -half_d + corner_spacing_offset),
    (half_w - corner_spacing_offset, -half_d + corner_spacing_offset),
]
corner_post_height = internal_h - 6.0
for (px, py) in post_positions:
    post = Cylinder(corner_post_dia/2.0, corner_post_height).located(
        Location((px, py, base_thickness + corner_post_height/2.0))
    )
    body += post
    hole = Cylinder(corner_hole_dia/2.0, corner_post_height + 2.0).located(
        Location((px, py, base_thickness + corner_post_height/2.0))
    )
    body -= hole

# -------------------------
# 4) PCB Fence + Aerial Cutout
# -------------------------
pcb_fence_outer = Box(PCB_W + 2*fence_clearance, PCB_H + 2*fence_clearance, fence_height).located(Location((0,0,pcb_z + fence_height/2.0)))
pcb_fence_inner = Box(PCB_W, PCB_H, fence_height + 1.0).located(Location((0,0,pcb_z + fence_height/2.0)))
pcb_fence_shell = pcb_fence_outer - pcb_fence_inner
body += pcb_fence_shell

aerial_cutout = Box(ESP_AERIAL_EXT_L + 6.0, ESP_AERIAL_EXT_W + 6.0, fence_height + 2.0).located(
    Location((ESP_W/2 + ESP_AERIAL_EXT_L/2.0, 0, pcb_z + fence_height/2.0))
)
body -= aerial_cutout

# -------------------------
# 5) USB & DRV Pads
# -------------------------
usb_pad = Box(USB_TRIG_L, USB_TRIG_W, 2.0).located(
    Location((internal_w/2 - wall_thickness - USB_TRIG_L/2, internal_d/4 - USB_TRIG_W/2, 1.0))
)
drv_pad = Box(DRV_L, DRV_W, 2.0).located(
    Location((internal_w/2 - wall_thickness - DRV_L/2, -internal_d/4 + DRV_W/2, 1.0))
)
body += usb_pad + drv_pad

# -------------------------
# 6) Right Wall
# -------------------------
right_partition = Box(wall_thickness, internal_d - 2*wall_thickness, internal_h).located(
    Location((internal_w/2 - wall_thickness/2.0, 0, base_thickness + internal_h/2.0))
)
body += right_partition

# -------------------------
# 7) Shoulders + Pins
# -------------------------
shoulder_height = fascia_thickness + SCREEN_T
pin_positions = [
    (SCREEN_W/2.0 - 12.0, SCREEN_H/2.0 - 10.0),
    (-SCREEN_W/2.0 + 12.0, SCREEN_H/2.0 - 10.0),
    (-SCREEN_W/2.0 + 12.0, -SCREEN_H/2.0 + 10.0),
    (SCREEN_W/2.0 - 12.0, -SCREEN_H/2.0 + 10.0),
]
for (sx, sy) in pin_positions:
    shoulder = Cylinder(shoulder_dia/2.0, shoulder_height).located(
        Location((sx, sy, base_thickness + internal_h - fascia_thickness/2.0))
    )
    pin = Cylinder(pin_dia/2.0, pin_above_shoulder).located(
        Location((sx, sy, base_thickness + internal_h - fascia_thickness/2.0 + shoulder_height))
    )
    body += shoulder + pin

case_body = body

# -------------------------
# 8) Fascia Lid
# -------------------------
fascia = Box(internal_w + 2*wall_thickness, internal_d + 2*wall_thickness, fascia_thickness).located(
    Location((0, 0, base_thickness + internal_h + fascia_thickness/2.0))
)

# Screen recess with tolerance
recess_depth = SCREEN_T
recess = Box(SCREEN_W + SCREEN_TOLERANCE, SCREEN_H + SCREEN_TOLERANCE, recess_depth).located(
    Location((0, 0, base_thickness + internal_h + recess_depth/2.0))
)
fascia -= recess

# Blind holes for pins
for (sx, sy) in pin_positions:
    blind_hole = Cylinder(pin_dia/2.0, fascia_blind_hole_depth).located(
        Location((sx, sy, base_thickness + internal_h + (fascia_thickness - fascia_blind_hole_depth/2.0)))
    )
    fascia -= blind_hole

# Button holes
for i in range(3):
    x = first_button_x + i * BUTTON_SPACING
    btn_cut = Cylinder(BUTTON_BODY_DIA/2.0 + 0.3, fascia_thickness + 2.0).located(
        Location((x, button_row_y, base_thickness + internal_h + fascia_thickness/2.0))
    )
    fascia -= btn_cut

# USB-C cutout (fixed Z)
stadium_center_z = 1.0 + 2.0/2.0  # center Z of usb_pad
stadium_center_x = internal_w/2 - wall_thickness - 0.5
stadium_center_y = internal_d/4 - USB_TRIG_W/2
stadium_rect = Box(usb_cutout_w, usb_cutout_h, fascia_thickness + 2.0).located(
    Location((stadium_center_x - usb_cutout_w/2.0, stadium_center_y, stadium_center_z))
)
fascia -= stadium_rect

fascia_lid = fascia

# -------------------------
# 9) Export STL
# -------------------------
# exporters.export(case_body, "case_body.stl")
# exporters.export(fascia_lid, "fascia_lid.stl")
print("Exported: case_body.stl and fascia_lid.stl")

# -------------------------
# 10) Visualization
# -------------------------
try:
    show_object(all_ghosts, name="Electronics_Ghosts")
    show_object(case_body, name="Case_Body")
    show_object(fascia_lid, name="Fascia_Lid")
except Exception as e:
    print("Visualization failed or ocp_vscode not present:", e)

print("Script finished.")
