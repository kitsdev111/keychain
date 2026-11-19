// ============================================================================
// Solar-Charged Ultra-Low-Power LED Fidget Keychain - Parametric Enclosure
// Units: millimeters (mm)
// Matches PCB: 20.0 x 32.0 x 1.6 mm (hardware/solar_fidget.kicad_pcb)
//
// Parts included:
//   1. Bottom Tray  : 25.4 x 30.4 mm exterior solar pocket, wire slots to SOL+/SOL-,
//                     cavity for 5.5V 1.0F coin supercap, snap-fit lip, keychain loop.
//   2. Top Bezel    : 0.4 mm translucent LED diffuser window (or open cutout),
//                     captive button plunger bore, snap-fit skirt, aligned keychain hole.
//   3. Button Plunger: Captive stepped actuator pin riding directly on SW1.
// ============================================================================

$fn = 48;

// --- Export Selector ("all", "top", "bottom", "plunger", "exploded") ---
part_to_render = "all";

// --- PCB Parameters (Exact Match to KiCad Layout) ---
pcb_w          = 20.0;
pcb_h          = 32.0;
pcb_t          = 1.6;
pcb_r          = 2.5;
pcb_clear      = 0.30;  // XY clearance around PCB

// PCB Feature Offsets (relative to PCB bottom-left = (0,0))
h1_pcb_x       = 3.8;   // 3.2 mm plated keychain hole
h1_pcb_y       = 28.2;
h1_dia         = 3.2;

sw1_pcb_x      = 14.2;  // 3x4 mm SMD tactile switch
sw1_pcb_y      = 28.2;
sw1_h          = 2.0;

led_cx_pcb     = 10.0;  // 7-LED hex cluster center
led_cy_pcb     = 11.5;
led_win_r      = 8.0;   // Diffuser window radius (covers R=6.2mm ring + 0603 LEDs)

sol_pad_y_pcb  = 4.6;   // SOL+ / SOL- pad Y on PCB bottom

// --- Solar Panel Pocket (25 x 30 mm 5V/5.5V Mini Epoxy Solar Panel) ---
solar_w        = 25.4;  // 25.0 mm + 0.4 mm fit margin
solar_h        = 30.4;  // 30.0 mm + 0.4 mm fit margin
solar_depth    = 2.3;   // Recessed pocket depth on back face

// --- Supercapacitor Cavity (5.5V 1.0F Coin Cell, Ø19.2 x 4.8 mm) ---
sc_dia         = 20.0;  // Ø19.2 mm + clearance
sc_depth       = 5.2;   // Depth below PCB bottom face

// --- Outer Shell Dimensions ---
wall           = 1.5;
case_w         = 28.0;  // Fits 25.4mm solar pocket + 1.3mm rim
case_h         = 39.5;  // Places H1 completely above solar pocket!
case_r         = 3.6;

// PCB placement inside enclosure (origin = enclosure bottom-left)
pcb_ox         = (case_w - pcb_w) / 2.0;  // 4.0 mm
pcb_oy         = 6.0;                     // Y=6.0..38.0 -> H1 at Y=34.2, above solar pocket (1.5..31.9)

// Z-stack heights (Bottom Tray exterior back = Z=0)
tray_floor     = solar_depth + 1.0;       // 3.3 mm solid floor behind supercap
pcb_z_bot      = tray_floor + sc_depth;   // 8.5 mm
pcb_z_top      = pcb_z_bot + pcb_t;       // 10.1 mm
top_ceil_z     = pcb_z_top + 2.8;         // 12.9 mm (total thickness ~12.9 mm)
top_roof_t     = 1.2;
diffuser_t     = 0.4;                     // 0.4 mm translucent diffuser (set 0.0 for open cutout)

// Snap-fit & Alignment
snap_lip_h     = 1.4;
snap_tol       = 0.18;

// --- Utility Modules ---
module rounded_rect_2d(w, h, r) {
    offset(r = r) offset(delta = -r) square([w, h], center = false);
}

module rounded_box(w, h, z, r) {
    linear_extrude(height = z) rounded_rect_2d(w, h, r);
}

// --- 1. Bottom Tray ---
module bottom_tray() {
    h1_x = pcb_ox + h1_pcb_x;
    h1_y = pcb_oy + h1_pcb_y;
    sol_x = (case_w - solar_w) / 2.0;
    sol_y = 1.4;

    difference() {
        union() {
            // Main tray body up to PCB top
            rounded_box(case_w, case_h, pcb_z_bot + 0.8, case_r);
            // Inner snap-fit rim rising around PCB
            translate([wall + snap_tol, wall + snap_tol, pcb_z_bot + 0.8])
                linear_extrude(height = snap_lip_h)
                    difference() {
                        rounded_rect_2d(case_w - 2*(wall + snap_tol), case_h - 2*(wall + snap_tol), case_r - wall);
                        offset(delta = -1.0)
                            rounded_rect_2d(case_w - 2*(wall + snap_tol), case_h - 2*(wall + snap_tol), case_r - wall);
                    }
        }

        // Interior cavity for PCB (above tray_floor + sc_depth)
        translate([pcb_ox - pcb_clear, pcb_oy - pcb_clear, pcb_z_bot])
            rounded_box(pcb_w + 2*pcb_clear, pcb_h + 2*pcb_clear, 10.0, pcb_r + pcb_clear);

        // Supercapacitor circular recess under PCB (at center of LED/supercap zone)
        translate([pcb_ox + led_cx_pcb, pcb_oy + led_cy_pcb, tray_floor])
            cylinder(d = sc_dia, h = sc_depth + 0.5);

        // Channel under upper PCB for UPDI pads & solder clearance (1.5 mm)
        translate([pcb_ox + 1.5, pcb_oy + 19.5, pcb_z_bot - 1.5])
            cube([pcb_w - 3.0, 11.0, 2.0]);

        // Exterior Back Solar Panel Recessed Pocket (at Z=0)
        translate([sol_x, sol_y, -0.01])
            rounded_box(solar_w, solar_h, solar_depth + 0.01, 1.2);

        // Wire pass-through slots from solar pocket to SOL+ / SOL- pads
        translate([pcb_ox + 6.5, pcb_oy + sol_pad_y_pcb - 1.5, -0.01])
            cube([7.0, 3.0, pcb_z_bot + 0.2]);

        // Keychain mounting hole aligned with PCB H1 (3.2 mm)
        translate([h1_x, h1_y, -0.01])
            cylinder(d = h1_dia + 0.2, h = 20.0);

        // Snap-fit groove along outer sides
        translate([-0.01, 10.0, pcb_z_bot + 0.9])
            cube([0.5, 16.0, 0.7]);
        translate([case_w - 0.49, 10.0, pcb_z_bot + 0.9])
            cube([0.5, 16.0, 0.7]);
    }

    // Reinforced Keychain Boss around H1 (sits inside PCB hole clearance? No, supports PCB bottom!)
    // Solid support pillar under PCB around H1
    difference() {
        translate([h1_x, h1_y, 0])
            cylinder(d = 6.2, h = pcb_z_bot);
        translate([h1_x, h1_y, -0.01])
            cylinder(d = h1_dia + 0.2, h = pcb_z_bot + 0.2);
    }
}

// --- 2. Top Bezel ---
module top_bezel() {
    h1_x = pcb_ox + h1_pcb_x;
    h1_y = pcb_oy + h1_pcb_y;
    sw_x = pcb_ox + sw1_pcb_x;
    sw_y = pcb_oy + sw1_pcb_y;
    led_x = pcb_ox + led_cx_pcb;
    led_y = pcb_oy + led_cy_pcb;

    skirt_bot = pcb_z_bot + 0.8;
    total_h = top_ceil_z - skirt_bot;
    ceil_local_z = total_h - top_roof_t;

    translate([0, 0, skirt_bot]) {
        difference() {
            union() {
                // Outer shell cap
                rounded_box(case_w, case_h, total_h, case_r);
            }

            // Hollow interior skirt clearing Bottom Tray snap rim & PCB
            translate([wall, wall, -0.01])
                rounded_box(case_w - 2*wall, case_h - 2*wall, ceil_local_z + 0.01, case_r - wall);

            // LED Diffuser Window: leaves thin 0.4 mm translucent roof (or 0 for open cutout)
            translate([led_x, led_y, ceil_local_z - 0.01])
                cylinder(r = led_win_r, h = top_roof_t - diffuser_t + 0.02);

            // Tactile Button Plunger Bore (Ø3.6 mm)
            translate([sw_x, sw_y, ceil_local_z - 0.01])
                cylinder(d = 3.6, h = top_roof_t + 0.05);

            // Keychain Mounting Hole (Ø3.4 mm aligned with H1)
            translate([h1_x, h1_y, -0.01])
                cylinder(d = h1_dia + 0.2, h = total_h + 0.1);
        }

        // Keychain Reinforcement Boss down to PCB top
        difference() {
            translate([h1_x, h1_y, pcb_z_top - skirt_bot])
                cylinder(d = 6.0, h = ceil_local_z - (pcb_z_top - skirt_bot));
            translate([h1_x, h1_y, -0.01])
                cylinder(d = h1_dia + 0.2, h = total_h + 0.1);
        }

        // Tactile Button Plunger Guide Collar
        difference() {
            translate([sw_x, sw_y, ceil_local_z - 1.0])
                cylinder(d = 5.4, h = 1.05);
            translate([sw_x, sw_y, ceil_local_z - 1.1])
                cylinder(d = 3.6, h = 1.3);
        }
    }
}

// --- 3. Button Actuator Plunger ---
module button_plunger() {
    sw_x = pcb_ox + sw1_pcb_x;
    sw_y = pcb_oy + sw1_pcb_y;
    plunger_base_z = pcb_z_top + sw1_h + 0.1;

    translate([sw_x, sw_y, plunger_base_z]) {
        // Captive flange (Ø4.8 mm, 0.6 mm thick)
        cylinder(d = 4.8, h = 0.6);
        // Actuator stem protruding 0.6 mm above Top Bezel roof
        cylinder(d = 3.2, h = (top_ceil_z - plunger_base_z) + 0.6);
    }
}

// --- Render Selection ---
if (part_to_render == "bottom") {
    bottom_tray();
} else if (part_to_render == "top") {
    translate([0, 0, top_ceil_z]) rotate([180, 0, 0]) top_bezel();
} else if (part_to_render == "plunger") {
    translate([0, 0, -(pcb_z_top + sw1_h + 0.1)]) button_plunger();
} else if (part_to_render == "exploded") {
    bottom_tray();
    translate([0, 0, 12]) top_bezel();
    translate([0, 0, 22]) button_plunger();
} else {
    // Assembled view
    color("#222222") bottom_tray();
    color("#444444", 0.75) top_bezel();
    color("#ff4422") button_plunger();
}
