# AliExpress Non-PCBA Sourcing Guide

All SMD components (`ATtiny1616-MNR`, `BAT54WS`, `BZX84C5V1`, `680 Ω` resistors, `100 nF` cap, `0603` Red LEDs, and `3×4 mm` tactile button) are assembled directly by **PCBWay Turn-Key PCBA**. Order the following external/THT parts from **AliExpress** (or LCSC/local electronics suppliers) to complete final assembly and firmware flashing.

---

## 1. Mini 5V / 5.5V Epoxy Solar Panel

| Parameter | Required Specification |
| :--- | :--- |
| **Dimensions** | **25 × 30 × 2.3–2.5 mm** (fits flush in `bottom_tray.stl` 25.4 × 30.4 mm pocket)<br>*(Alternative: 25 × 25 mm or 30 × 30 mm with adjusted `solar_w`/`solar_h` in `solar_fidget_enclosure.scad`)* |
| **Nominal Voltage (Vmp)** | **5.0 V** or **5.5 V** (Open-circuit Voc ≈ 5.8–6.6 V — safely clamped by `BZX84C5V1` 5.1 V Zener) |
| **Current / Power** | **25–40 mA** (0.15 W – 0.20 W), polycrystalline or monocrystalline epoxy-encapsulated |
| **Exact Search Keywords** | `"5V 30mA mini solar panel 25x30mm epoxy"`<br>`"5.5V mini solar cell 25*30mm DIY"`<br>`"0.15W 5V polysilicon epoxy solar panel"` |
| **Estimated Price** | ~$0.40 – $1.00 each (often sold in 5-pack or 10-pack) |
| **Assembly Note** | Solder two short 30 AWG silicone leads to the rear `+` / `-` copper tabs of the panel, route through the pass-through slot in `bottom_tray.stl`, and solder to `SOL+` / `SOL-` pads on `B.Cu`. Secure the panel flush into the pocket with thin double-sided tape or neutral-cure silicone/epoxy. |

---

## 2. 5.5V 1.0F Coin-Type Supercapacitor (`C_SC1`)

| Parameter | Required Specification |
| :--- | :--- |
| **Capacitance & Voltage** | **5.5 V 1.0 F** (also compatible with **0.47 F** or **1.5 F** EDLC coin cells) |
| **Lead Style & Pitch** | **H-type (horizontal)** or **V-type (vertical)** radial coin cell, **7.5 mm lead pitch** (note: if ordering a 5.0 mm pitch cell, gently form the flat tabs outward by 1.25 mm each side to fit the 7.5 mm PCB pads) |
| **Max Dimensions** | **Diameter ≤ 19.5 mm**, **Thickness ≤ 4.8–5.0 mm** (fits inside `bottom_tray.stl` Ø20.0 × 5.2 mm cavity) |
| **Exact Search Keywords** | `"5.5V 1.0F supercapacitor coin H type"`<br>`"5.5V 1F farad capacitor 19mm"`<br>`"5.5V 0.47F supercapacitor coin cell 7.5mm"` |
| **Reference MPNs** | Viking `SC5V5H105Z`, Kamcap `5.5V 1.0F H-Type`, Eaton `KR-5R5H105-R` *(check height ≤ 5 mm)*, Elna `DZ-5R5D105` |
| **Estimated Price** | ~$0.60 – $1.50 each |
| **Assembly Note** | **Observe polarity!** The PCB bottom silkscreen (`B.SilkS`) marks `+` (square pad, `VCC`) and `-` (round pad, `GND`). Trim leads flush to the top of the PCB after soldering so they do not touch the LEDs or top shell. |

---

## 3. UPDI Programmer Hardware (USB-to-TTL + 4.7 kΩ Resistor)

| Item | Required Specification | Exact Search Keywords | Est. Price |
| :--- | :--- | :--- | :--- |
| **USB-UART Adapter** | **CH340G**, **CP2102**, or **FT232RL** with 3.3V / 5V selector | `"CP2102 USB to TTL UART module 6 pin"` or `"CH340G USB to TTL converter"` | ~$1.20 – $2.50 |
| **Series Resistor** | **4.7 kΩ** (1/4W axial or 0603 SMD) | `"4.7k ohm resistor kit 1/4W"` | ~$0.50 (pack) |
| **Pre-made UPDI Option** | Dedicated SerialUPDI / `pymcuprog` programmer board (built-in resistor/diode) | `"SerialUPDI programmer USB type-C"` | ~$3.00 – $5.00 |
| **Pogo Test Clip (Optional)** | 3-pin **2.54 mm** single-row pogo-pin probe clip for solderless flashing | `"2.54mm 3P pogo pin test clamp"` | ~$2.50 – $4.50 |

> See [firmware/README_FLASHING.md](../firmware/README_FLASHING.md) for the exact 3-wire SerialUPDI connection (`VCC`, `GND`, `UPDI`).

---

## 4. Keychain Hardware & Hookup Wire

| Item | Required Specification | Exact Search Keywords | Est. Price |
| :--- | :--- | :--- | :--- |
| **Split Rings / Clasps** | **20–25 mm** stainless steel split keyring + swivel lobster clasp (wire gauge ≤ 1.8 mm to clear the **3.2 mm** PCB + enclosure hole) | `"25mm stainless steel split keyring lobster clasp"` | ~$1.00 (10 pcs) |
| **Hookup Wire** | **30 AWG** ultra-flexible silicone-insulated stranded wire (Red & Black) | `"30AWG flexible silicone wire red black"` | ~$1.50 (5 m) |
| **Translucent Filament (3D Print)** | Natural/Clear **PETG** or **PLA** for `top_bezel.stl` (gives soft glow through the **0.4 mm** LED diffuser window); Matte Black **PETG**/**PLA** for `bottom_tray.stl` | `"translucent clear PETG filament 1.75mm"` | — |
