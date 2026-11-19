# Flashing the ATtiny1616 via UPDI

The Solar-Charged LED Fidget Keychain uses a **Microchip ATtiny1616-MNR** programmed over a single-wire **UPDI** (Unified Program and Debug Interface). To keep the PCB ultra-compact, there is no pin header; instead, three **1.5 mm circular test pads** (`VCC`, `GND`, `UPDI`) are exposed on the **bottom (`B.Cu`)** of the PCB at a standard **2.54 mm (0.1")** pitch.

---

## 1. Required Hardware

* **USB-to-TTL UART Adapter** (`CH340G`, `CP2102`, or `FT232RL`)
* **1× 4.7 kΩ resistor** (1/4W axial or 0603 SMD)
* **3-pin pogo-pin clip** (2.54 mm pitch) OR 3× thin silicone jumper wires / male header pins held gently against the bottom pads during the ~2-second upload

### SerialUPDI Wiring Diagram

```text
USB-UART Adapter              Keychain PCB (Bottom Pads)
────────────────              ──────────────────────────
  TXD ───[ 4.7 kΩ ]───┬───────► UPDI  (Pin 19 / PA0)
                      │
  RXD ────────────────┘
  VCC (3.3V or 5V) ───────────► VCC   (Note: disconnect/cover solar cell!)
  GND ────────────────────────► GND
```

> **Important Safety Notes:**
> 1. **Cover the solar panel** (or flash before soldering the solar panel/supercapacitor) so the solar panel does not back-feed your USB adapter.
> 2. If the **1.0F supercapacitor** is already soldered and completely discharged (0 V), wait ~2–3 seconds after connecting `VCC` from the USB-UART adapter before starting upload, or pre-charge it briefly, so `VCC` stabilizes above 2.0 V.
> 3. Do **not** exceed 5.5 V on `VCC` (set your USB-UART jumper to **3.3 V** or **5.0 V**).

---

## 2. Option A — Flashing via PlatformIO + `pymcuprog` (Recommended)

### Install `pymcuprog`
```bash
pip install pymcuprog
```

### Build & Flash with PlatformIO
From the `/firmware` directory:
```bash
# 1. Build firmware (.hex & .elf)
pio run

# 2. Flash via SerialUPDI (replace /dev/ttyUSB0 with COMx on Windows)
pio run -t upload --upload-port /dev/ttyUSB0
```

Or flash the compiled `.hex` directly using `pymcuprog`:
```bash
pymcuprog write -t uart -u /dev/ttyUSB0 -d attiny1616 -f .pio/build/serialupdi/firmware.hex --erase --verify
```

### Recommended Fuse Settings (Ultra-Low-Power 100 nA Sleep)
To achieve true ~100 nA `SLEEP_MODE_PWR_DOWN` current down to 1.8 V, ensure **Brown-Out Detector (BOD)** is disabled in sleep and **OSCCFG** is set to 16 MHz internal oscillator (divided by 16 in `setup()` to 1 MHz):
```bash
# Read current fuses
pymcuprog read -t uart -u /dev/ttyUSB0 -d attiny1616 -m fuses

# Set BODCFG = 0x00 (BOD disabled for 1.8V–5.5V supercap operation & 100 nA sleep)
# Fuse byte 1 (BODCFG) offset = 0x01
pymcuprog write -t uart -u /dev/ttyUSB0 -d attiny1616 -m fuses -o 0x01 -l 0x00
```

---

## 3. Option B — Flashing via Arduino IDE (`megaTinyCore`)

1. Open **Arduino IDE** → **File → Preferences** → Add Board Manager URL:
   ```text
   https://drazzy.com/package_drazzy.com_index.json
   ```
2. Open **Tools → Board → Boards Manager**, search for **`megaTinyCore`** by Spence Konde, and install it.
3. Configure **Tools** menu:
   * **Board:** `ATtiny1616/1606/816/806/416/406`
   * **Chip:** `ATtiny1616`
   * **Clock:** `1 MHz internal`
   * **BOD Voltage Level:** `Disabled` *(critical for 100 nA sleep and full supercap discharge down to 1.8 V)*
   * **BOD Mode when Active/Sleeping:** `Disabled / Disabled`
   * **UPDI / Reset Pin:** `UPDI (no reset)`
   * **Programmer:** `SerialUPDI - SLOW: 57600 baud` (or `230400 baud`)
4. Open `firmware/src/main.cpp` (rename or copy into an `.ino` sketch), select your **Port**, and click **Upload**.

---

## 4. Controls Quick Reference

| Action | Behavior |
| :--- | :--- |
| **Short press (< 0.7 s)** from sleep | Wakes MCU, cycles to next mode (if unlocked), plays animation, returns to ~100 nA deep sleep |
| **Short press** during animation | Immediately ends current animation / moves forward |
| **Long press (0.7 s – 1.8 s)** | Toggles brightness: **Eco (35%)** ↔ **Bright (100%)** (2 vs 3 center flashes) |
| **Extra-long press (≥ 1.8 s)** | Toggles **Mode Lock** (locks current mode vs cycling through all 5 modes) |
| **Mode 5 (Click Counter)** | Stays awake 3 s after each tap, displays 7-bit binary count (`0..127`); long press resets & sleeps |
