from build123d import *
from ocp_vscode import show, set_port, set_defaults

# ==============================================================================
# 1. Configuration
# ==============================================================================

# Dimensions
wall_th = 2.0
floor_th = 2.0
fascia_th = 4.0 
internal_height = 45.0 

# TOLERANCE SETTINGS
fit_tolerance = 0.6 

# SCREW SETTINGS (5mm Head)
screw_shaft_dia = 3.0  
screw_head_dia = 5.0   
screw_clearance = 0.4  
cb_depth = 1.5         

cb_radius = (screw_head_dia + screw_clearance) / 2
shaft_radius = screw_shaft_dia / 2

# Component Dimensions
disp_pcb_w, disp_pcb_h = 108.0, 62.0
disp_scr_w, disp_scr_h = 95.0, 62.0
disp_pcb_th = 1.7
disp_scr_th = 4.0
disp_offset_y = 10.0 

# PIN MOUNT STRATEGY (Float)
pcb_recess_from_rim = disp_scr_th - fascia_th 
standoff_shoulder_z = internal_height - pcb_recess_from_rim - disp_pcb_th
pin_dia = 2.0 
pin_rad = pin_dia / 2
pin_height = 2.0 

btn_dia = 7.0
btn_body_dia = 12.0 
btn_depth = 20.0
btn_spacing = 18.0

# ESP32
esp_w, esp_h = 51.0, 28.0
esp_ant_ext = 7.0   
esp_ant_width = 18.0 

# USB Trigger
usb_len = 28.0 
usb_wid = 11.0 
usb_h_total = 4.5
usb_recess = 3.0 
usb_conn_bottom_h = 3.6

# DRV8833
drv_w = 18.5
drv_h = 21.0 

# ==============================================================================
# 2. Layout & Ghosts
# ==============================================================================

# A. Display Ghost
ghost_pcb_z = floor_th + standoff_shoulder_z
ghost_disp_loc = Location((0, disp_offset_y, ghost_pcb_z))
with BuildPart() as ghost_disp_bp:
    with Locations(ghost_disp_loc):
        Box(disp_pcb_w, disp_pcb_h, disp_pcb_th, align=(Align.CENTER, Align.CENTER, Align.MIN))
        with Locations((0,0, disp_pcb_th)): 
            Box(disp_scr_w, disp_scr_h, disp_scr_th, align=(Align.CENTER, Align.CENTER, Align.MIN))
ghost_display = ghost_disp_bp.part

# B. Buttons
btn_y = disp_offset_y - (disp_pcb_h/2) - 15
ghost_btns = Part()
for i in [-1, 0, 1]:
    loc = Location((i * btn_spacing, btn_y, internal_height + floor_th))
    ghost_btns += Cylinder(radius=btn_dia/2, height=10, align=(Align.CENTER, Align.CENTER, Align.MIN)).move(loc)
    ghost_btns += Cylinder(radius=btn_body_dia/2, height=btn_depth, align=(Align.CENTER, Align.CENTER, Align.MAX)).move(loc)

# C. ESP32
esp_y = -35 
esp_loc = Location((0, esp_y, floor_th))
with BuildPart() as ghost_esp_bp:
    with Locations(esp_loc):
        Box(esp_w, esp_h, 4, align=(Align.CENTER, Align.CENTER, Align.MIN))
        with Locations((esp_w/2 + esp_ant_ext/2, 0, 0)):
            Box(esp_ant_ext, esp_ant_width, 4, align=(Align.CENTER, Align.CENTER, Align.MIN))
ghost_esp = ghost_esp_bp.part

# --- RIGHT SIDE LAYOUT ---
right_col_x = 48.0

# D. USB-C Trigger
padding = 6.0
calc_case_w = max(disp_pcb_w, (right_col_x + 15)*2) + padding*2 + wall_th*2
ext_wall_x = calc_case_w / 2
usb_x_pos = ext_wall_x - usb_recess - (usb_len / 2)
usb_loc = Location((usb_x_pos, 0, floor_th + (usb_conn_bottom_h/2))) 
ghost_usb = Box(usb_len, usb_wid, usb_h_total, align=(Align.CENTER, Align.CENTER, Align.MIN)).move(usb_loc)

# E. DRV8833 (MOVED DOWN TO AVOID CONE COLLISION)
# Old Y: -35. New Y: -39.
drv_loc = Location((right_col_x, -39, floor_th))
ghost_drv = Box(drv_w, drv_h, 5, align=(Align.CENTER, Align.CENTER, Align.MIN)).move(drv_loc)

ghosts = ghost_display + ghost_btns + ghost_esp + ghost_usb + ghost_drv

# ==============================================================================
# 3. Main Body Generation
# ==============================================================================

# Calculate Shell Size
min_box = ghosts.bounding_box()
case_w = max(min_box.size.X + padding*2 + wall_th*2, calc_case_w)
case_h = min_box.size.Y + padding*2 + wall_th*2
case_d = internal_height + floor_th 

# --- A. The Box Shell ---
with BuildPart() as box_bp:
    with BuildSketch() as sk_outer:
        Rectangle(case_w, case_h)
        fillet(sk_outer.vertices(), radius=6)
    extrude(amount=case_d)
    
    with BuildSketch(faces().sort_by(Axis.Z).last) as sk_inner:
        Rectangle(case_w - wall_th*2, case_h - wall_th*2)
        fillet(sk_inner.vertices(), radius=4)
    extrude(amount=-internal_height, mode=Mode.SUBTRACT)

case_box = box_bp.part

# --- B. Mounting Posts ---
posts = Part()
px = (case_w/2) - 5
py = (case_h/2) - 5
for x in [-px, px]:
    for y in [-py, py]:
        p = Cylinder(radius=3.5, height=case_d - floor_th, align=(Align.CENTER, Align.CENTER, Align.MIN))
        p = p.move(Location((x, y, floor_th)))
        h = Cylinder(radius=1.4, height=case_d, align=(Align.CENTER, Align.CENTER, Align.MIN))
        h = h.move(Location((x, y, floor_th + 5))) 
        posts += (p - h)
case_box += posts

# --- C. Internal Mounts ---

# 1. Display Standoffs
disp_mounts = Part()
# Logic: 3.0mm from X-edges (Long), 3.5mm from Y-edges (Short)
dx = disp_pcb_w/2 - 3.0
dy = disp_pcb_h/2 - 3.5

for x in [-dx, dx]:
    for y in [-dy, dy]:
        with BuildPart() as p_bp:
            # Cone Base
            Cone(bottom_radius=6.0, top_radius=3.0, height=standoff_shoulder_z, align=(Align.CENTER, Align.CENTER, Align.MIN))
            # Pin
            with Locations((0,0, standoff_shoulder_z)):
                Cylinder(radius=pin_rad, height=pin_height, align=(Align.CENTER, Align.CENTER, Align.MIN))
        disp_mounts += p_bp.part.move(Location((x, y + disp_offset_y, floor_th)))
case_box += disp_mounts

# 2. ESP32 Corners
esp_fence_block = Box(esp_w + fit_tolerance + 3, esp_h + fit_tolerance + 3, 6, align=(Align.CENTER, Align.CENTER, Align.MIN))
esp_void = Box(esp_w + fit_tolerance, esp_h + fit_tolerance, 7, align=(Align.CENTER, Align.CENTER, Align.MIN))
cut_bar_w = Box(esp_w + 20, esp_h - 10, 10, align=(Align.CENTER, Align.CENTER, Align.MIN))
cut_bar_h = Box(esp_w - 10, esp_h + 20, 10, align=(Align.CENTER, Align.CENTER, Align.MIN))
aerial_cut = Box(10, 20, 10, align=(Align.CENTER, Align.CENTER, Align.MIN)).move(Location((esp_w/2, 0, 0)))
esp_mount = (esp_fence_block - esp_void - cut_bar_w - cut_bar_h - aerial_cut).move(esp_loc)
case_box += esp_mount

# 3. DRV8833 Mount
drv_wall = Box(drv_w + fit_tolerance + 3, drv_h + fit_tolerance + 3, 6, align=(Align.CENTER, Align.CENTER, Align.MIN))
drv_void = Box(drv_w + fit_tolerance, drv_h + fit_tolerance, 7, align=(Align.CENTER, Align.CENTER, Align.MIN))
drv_mount = (drv_wall - drv_void).move(drv_loc)
case_box += drv_mount

# 4. USB-C Fence
usb_outer = Box(usb_len + fit_tolerance + 3, usb_wid + fit_tolerance + 3, 6, align=(Align.CENTER, Align.CENTER, Align.MIN))
usb_inner = Box(usb_len + fit_tolerance, usb_wid + fit_tolerance, 7, align=(Align.CENTER, Align.CENTER, Align.MIN))
usb_conn_cut = Box(5, usb_wid, 10, align=(Align.CENTER, Align.CENTER, Align.MIN)).move(Location((usb_len/2, 0, 0)))
usb_mount = (usb_outer - usb_inner - usb_conn_cut).move(usb_loc)
case_box += usb_mount

# --- D. Cutouts ---
# USB Hole (10.0 x 4.5mm)
usb_hole_w = 10.0
usb_hole_h = 4.5
usb_hole_fillet = usb_hole_h / 2
# Center aligned such that bottom edge is 0.2mm below connector
usb_hole_z = floor_th + usb_conn_bottom_h - 0.2 + (usb_hole_h / 2)

with BuildPart() as usb_cutter_bp:
    with BuildSketch(Plane.YZ) as sk_usb:
        Rectangle(usb_hole_w, usb_hole_h) 
        fillet(sk_usb.vertices(), radius=usb_hole_fillet) 
    extrude(amount=20, both=True)
usb_hole = usb_cutter_bp.part.move(Location((case_w/2, usb_loc.position.Y, usb_hole_z)))
case_box -= usb_hole

# ==============================================================================
# 4. Fascia (Lid) Generation
# ==============================================================================

with BuildPart() as fascia_bp:
    with BuildSketch() as sk_fascia:
        Rectangle(case_w, case_h)
        fillet(sk_fascia.vertices(), radius=6)
    extrude(amount=fascia_th)
    
    # 1. Main Screws
    for x in [-px, px]:
        for y in [-py, py]:
            with Locations((x, y, fascia_th)): 
                CounterBoreHole(radius=shaft_radius, counter_bore_radius=cb_radius, counter_bore_depth=cb_depth)

    # 2. Display Cutout
    with Locations((0, disp_offset_y)):
        Box(disp_scr_w, disp_scr_h, fascia_th*3, mode=Mode.SUBTRACT)

    # 3. Button Holes
    with Locations((0, btn_y)):
        with Locations([(-btn_spacing, 0), (0,0), (btn_spacing, 0)]):
            Hole(radius=btn_dia/2)
            
    # 4. Alignment Traps
    with Locations((0, disp_offset_y)):
        with Locations([(x, y) for x in [-dx, dx] for y in [-dy, dy]]):
            Cylinder(radius=3.0/2, height=1.0, align=(Align.CENTER, Align.CENTER, Align.MIN), mode=Mode.SUBTRACT)

fascia = fascia_bp.part
fascia_visual = fascia.move(Location((0,0, case_d)))

# ==============================================================================
# 5. Export & Visualization
# ==============================================================================

print("Exporting...")
export_stl(case_box, "case_body.stl")
export_stl(fascia, "fascia_lid.stl")

print("Visualizing...")
try:
    show(
        case_box, 
        fascia_visual, 
        ghosts, 
        names=["Body", "Fascia", "Electronics"],
        colors=["#CCCCCC", "#333333", "red"],
        alphas=[1.0, 1.0, 0.4]
    )
except Exception as e:
    print(f"Viewer Error: {e}")

print("Done.")