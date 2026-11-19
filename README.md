# Solar-Charged Ultra-Low-Power LED Fidget Keychain

A manufacturing-ready, ultra-compact (**20.0 × 32.0 mm**) solar-harvesting LED fidget keychain powered by a **Microchip ATtiny1616-MNR** (`1.8 V–5.5 V`), a **5.5 V 1.0 F supercapacitor**, and a **mini 5.5 V epoxy solar cell**.

---

## Repository Structure

```text
├── hardware/                 # KiCad 8 schematic, PCB layout, and project files
│   ├── solar_fidget.kicad_pro
│   ├── solar_fidget.kicad_sch
│   └── solar_fidget.kicad_pcb
├── pcba_production/          # PCBWay Turn-Key PCBA manufacturing package
│   ├── gerbers.zip           # RS-274X Gerbers + Excellon PTH/NPTH drills
│   ├── BOM_PCBWay.csv        # PCBWay formatted BOM (LCSC part numbers, THT/DNP marked)
│   ├── CPL_PCBWay.csv        # Top-side SMD Pick-and-Place coordinates (mm)
│   ├── Assembly_Drawing.pdf  # Dual-side assembly & polarity guide
│   └── Assembly_Drawing.svg
├── firmware/                 # PlatformIO / Arduino (megaTinyCore) firmware @ 1 MHz
│   ├── platformio.ini
│   ├── src/main.cpp          # 5 fidget modes + ~100 nA SLEEP_MODE_PWR_DOWN
│   └── README_FLASHING.md    # SerialUPDI flashing & fuse guide
├── enclosure/                # Parametric 3D-printable snap-fit keychain shell
│   ├── solar_fidget_enclosure.scad
│   ├── top_bezel.stl         # 0.4 mm diffuser window + button collar (print roof-down)
│   ├── bottom_tray.stl       # 25.4x30.4 mm solar pocket + supercap cavity + lanyard loop
│   └── button_plunger.stl    # Captive tactile switch actuator pin
└── sourcing/
    └── aliexpress_parts.md   # Sourcing guide for solar panel, supercap, UPDI & hardware
```

---

## Key Hardware & Design Highlights

* **MCU:** `ATtiny1616-MNR` (VQFN-20, 3×3 mm, LCSC `C507118`) running at **1 MHz internal clock** directly from the supercapacitor (`1.8 V–5.5 V` unregulated).
* **Solar Harvesting & Protection:**
  * `BAT54WS` (SOD-323) low-Vf Schottky reverse-blocking diode (`SOL+` → `VCC`).
  * `BZX84C5V1` (SOT-23) 5.1 V Zener shunt clamp protecting the MCU and supercapacitor.
  * `5.5 V 1.0 F` THT coin supercapacitor (`7.5 mm` pitch) + 2× optional `1210` parallel MLCC/supercap pads (`DNP`) + `100 nF` `0603` decoupling at `VDD`/`GND`.
* **Display & UI:** 7× `0603` Red LEDs (`Vf ≈ 2.0 V`, individual `680 Ω` resistors) in a 2-3-2 hexagonal rosette (`LED1–LED6` on hardware `TCA0` PWM, `LED7` on hardware `TCB0` PWM) + `3×4 mm` SMD tactile button on async pin-change interrupt `PC2`.
* **PCB Finish & Custom Watermark:**
  * **PCBWay Spec:** 2-layer `1.6 mm` FR4, **Matte Black** solder mask, **White** silkscreen, **ENIG (Gold)** surface finish recommended so the exposed bare-copper **Fox Logo + `kitsdev` watermark** (`F.Cu` + `F.Mask` opening) and `VCC`/`GND`/`UPDI` bottom pogo pads shine cleanly and resist oxidation.
  * **Single-Sided SMD PCBA:** All 19 assembled SMD components are on `F.Cu` (`Top`), keeping PCBWay SMT assembly costs minimal.
