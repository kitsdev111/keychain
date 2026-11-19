/**
 * @file main.cpp
 * @brief Ultra-Low-Power Solar-Charged LED Fidget Keychain Firmware
 *
 * Target MCU : Microchip ATtiny1616-MNR (VQFN-20)
 * Core       : megaTinyCore (Arduino / PlatformIO)
 * Clock      : 1 MHz internal oscillator (16 MHz / 16 prescaler)
 * Sleep Draw : ~100 nA in SLEEP_MODE_PWR_DOWN (BOD disabled in fuses)
 *
 * Pinout Mapping (matches hardware/solar_fidget.kicad_sch):
 *   LED1 (Top-Left)  : PA3 (Pin 2)  - TCA0 WO3 (Hardware PWM)
 *   LED2 (Top-Right) : PB2 (Pin 12) - TCA0 WO2 (Hardware PWM)
 *   LED3 (Mid-Right) : PB1 (Pin 13) - TCA0 WO1 (Hardware PWM)
 *   LED4 (Bot-Right) : PB0 (Pin 14) - TCA0 WO0 (Hardware PWM)
 *   LED5 (Bot-Left)  : PA5 (Pin 6)  - TCA0 WO5 (Hardware PWM)
 *   LED6 (Mid-Left)  : PA4 (Pin 5)  - TCA0 WO4 (Hardware PWM)
 *   LED7 (Center)    : PC0 (Pin 15) - TCB0 WO Alt (Hardware 8-bit PWM via PORTMUX)
 *   SW1  (Button)    : PC2 (Pin 17) - Async pin-change interrupt, INPUT_PULLUP, active-LOW
 *   UPDI (Prog)      : PA0 (Pin 19) - Dedicated UPDI programming pad
 *
 * User Controls:
 *   - Short press (< 700 ms) from sleep: wakes MCU, cycles to next mode (unless locked), runs mode.
 *   - Short press during animation: skips/advances immediately for snappy fidgeting.
 *   - Long press (700 ms .. 1800 ms): toggles Brightness (Eco 35% <-> Bright 100%).
 *   - Extra-long press (>= 1800 ms): toggles Mode Lock (Locked <-> Cycle all 5 modes).
 */

#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

// -----------------------------------------------------------------------------
// Hardware Pin Definitions (megaTinyCore constants)
// -----------------------------------------------------------------------------
static constexpr uint8_t NUM_LEDS = 7;

// Ordered clockwise around outer hex ring (0..5), with center at index 6
static constexpr uint8_t LED_PINS[NUM_LEDS] = {
    PIN_PA3, // 0: LED1 (Top-Left,  TCA0 WO3)
    PIN_PB2, // 1: LED2 (Top-Right, TCA0 WO2)
    PIN_PB1, // 2: LED3 (Mid-Right, TCA0 WO1)
    PIN_PB0, // 3: LED4 (Bot-Right, TCA0 WO0)
    PIN_PA5, // 4: LED5 (Bot-Left,  TCA0 WO5)
    PIN_PA4, // 5: LED6 (Mid-Left,  TCA0 WO4)
    PIN_PC0  // 6: LED7 (Center,    TCB0 WO Alternate)
};

// Bottom-to-top bar-graph mapping for Battery Level Meter
static constexpr uint8_t BATTERY_ORDER[NUM_LEDS] = {
    4, // LED5 (Bot-Left)
    3, // LED4 (Bot-Right)
    5, // LED6 (Mid-Left)
    6, // LED7 (Center)
    2, // LED3 (Mid-Right)
    0, // LED1 (Top-Left)
    1  // LED2 (Top-Right)
};

static constexpr uint8_t BTN_PIN = PIN_PC2;

// Timing constants (ms)
static constexpr uint16_t DEBOUNCE_MS      = 25;
static constexpr uint16_t LONG_PRESS_MS    = 700;
static constexpr uint16_t MODE_LOCK_MS     = 1800;
static constexpr uint16_t CLICKER_IDLE_MS  = 3000;

// -----------------------------------------------------------------------------
// Global State
// -----------------------------------------------------------------------------
enum FidgetMode : uint8_t {
    MODE_COMET = 0,
    MODE_BATTERY_METER,
    MODE_DICE_SPINNER,
    MODE_FIREFLY,
    MODE_CLICK_COUNTER,
    NUM_MODES
};

volatile bool g_wakeFlag = false;

static uint8_t  g_currentMode  = MODE_CLICK_COUNTER; // First wake advances to MODE_COMET (0)
static bool     g_modeLocked   = false;
static bool     g_ecoBright    = false;              // false = 100% max, true = 35% eco
static uint8_t  g_clickCount   = 0;
static uint32_t g_prngState    = 0xA53C19F7UL;

// 64-entry sine-like gamma-corrected breathing table (0..255)
static const uint8_t BREATHE_LUT[64] PROGMEM = {
      0,   0,   1,   2,   4,   7,  10,  14,
     19,  25,  32,  40,  49,  59,  70,  82,
     95, 108, 122, 136, 151, 165, 179, 192,
    205, 217, 227, 236, 244, 250, 253, 255,
    255, 253, 250, 244, 236, 227, 217, 205,
    192, 179, 165, 151, 136, 122, 108,  95,
     82,  70,  59,  49,  40,  32,  25,  19,
     14,  10,   7,   4,   2,   1,   0,   0
};

// -----------------------------------------------------------------------------
// Interrupt Service Routine for Switch Wakeup (PC2)
// -----------------------------------------------------------------------------
ISR(PORTC_PORT_vect) {
    if (PORTC.INTFLAGS & PIN2_bm) {
        PORTC.INTFLAGS = PIN2_bm; // Clear interrupt flag
        g_wakeFlag = true;
    }
}

// -----------------------------------------------------------------------------
// Low-Level Hardware Helpers
// -----------------------------------------------------------------------------

static void disableUnusedPinsForSleep() {
    // Disable digital input buffers on all unconnected GPIOs to eliminate floating leakage
    PORTA.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTA.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTA.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;

    PORTB.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTB.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTB.PIN5CTRL = PORT_ISC_INPUT_DISABLE_gc;

    PORTC.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTC.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;
}

static void initLedPwm() {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], LOW);
    }
    // Route TCB0 waveform output to PC0 (LED7) for hardware 8-bit PWM
    PORTMUX.CTRLD = PORTMUX_TCB0_ALTERNATE_gc;
    TCB0.CTRLA = 0;
    TCB0.CCMPL = 255;
    TCB0.CCMPH = 0;
    TCB0.CTRLB = TCB_CNTMODE_PWM8_gc;
    TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
}

static uint8_t scaleBrightness(uint8_t val) {
    if (!g_ecoBright || val == 0) {
        return val;
    }
    // ~35% brightness (val * 90 / 255), keeping minimum visible floor of 1
    uint16_t scaled = (static_cast<uint16_t>(val) * 90U) >> 8;
    return (scaled == 0) ? 1 : static_cast<uint8_t>(scaled);
}

static void setLedRaw(uint8_t idx, uint8_t duty) {
    if (idx >= NUM_LEDS) return;

    if (idx < 6) {
        // LEDs 0..5 are on TCA0 split-mode hardware PWM pins
        analogWrite(LED_PINS[idx], duty);
    } else {
        // LED 6 (PC0) uses TCB0 8-bit PWM
        if (duty == 0) {
            TCB0.CTRLB = TCB_CNTMODE_PWM8_gc; // Disable CCMPEN
            PORTC.OUTCLR = PIN0_bm;
        } else if (duty == 255) {
            TCB0.CTRLB = TCB_CNTMODE_PWM8_gc;
            PORTC.OUTSET = PIN0_bm;
        } else {
            TCB0.CCMPL = 255;
            TCB0.CCMPH = duty;
            TCB0.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
        }
    }
}

static void setLed(uint8_t idx, uint8_t duty) {
    setLedRaw(idx, scaleBrightness(duty));
}

static void allLedsOff() {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        setLedRaw(i, 0);
    }
}

static void enterDeepSleep() {
    // 1. Shut down all LEDs and PWM output drivers cleanly
    allLedsOff();
    TCB0.CTRLA = 0;
    TCB0.CTRLB = 0;
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        digitalWrite(LED_PINS[i], LOW);
    }

    // 2. Disable ADC and Voltage Reference
    ADC0.CTRLA = 0;
    VREF.CTRLA = 0;

    // 3. Ensure button is released before arming wakeup interrupt
    while (digitalRead(BTN_PIN) == LOW) {
        delay(10);
    }
    delay(DEBOUNCE_MS);

    // 4. Arm asynchronous falling-edge interrupt on PC2
    cli();
    g_wakeFlag = false;
    PORTC.INTFLAGS = PIN2_bm;
    PORTC.PIN2CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sei();

    // 5. Enter ~100 nA deep sleep until button press
    sleep_cpu();

    // --- Woken up by PC2 falling-edge interrupt ---
    sleep_disable();
    PORTC.PIN2CTRL = PORT_PULLUPEN_bm; // Disable pin interrupt while awake (poll cleanly)

    // Re-enable LED PWM hardware
    initLedPwm();
}

// -----------------------------------------------------------------------------
// Utility: Fast Xorshift32 PRNG + ADC Bandgap Supply Voltage Readout
// -----------------------------------------------------------------------------
static uint32_t nextRandom() {
    g_prngState ^= (g_prngState << 13);
    g_prngState ^= (g_prngState >> 17);
    g_prngState ^= (g_prngState << 5);
    return g_prngState;
}

/**
 * @brief Measures VDD (supercapacitor voltage) in millivolts using the
 *        ATtiny1616's internal 1.1V bandgap reference (no external parts).
 */
static uint16_t readVccMillivolts() {
    // Select 1.1V internal reference for VREF block
    VREF.CTRLA = VREF_ADC0REFSEL_1V1_gc;

    // Configure ADC0: VDD as reference, 1 MHz / 16 = 62.5 kHz ADC clock
    ADC0.CTRLB = 0;
    ADC0.CTRLC = ADC_SAMPCAP_bm | ADC_REFSEL_VDDREF_gc | ADC_PRESC_DIV16_gc;
    ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;
    ADC0.CTRLA = ADC_ENABLE_bm;

    // Allow bandgap VREF startup settling (~60 us min)
    delayMicroseconds(120);

    // Discard first reading after reference switch
    ADC0.COMMAND = ADC_STCONV_bm;
    while (ADC0.COMMAND & ADC_STCONV_bm) {}
    (void)ADC0.RES;

    // Average 4 readings
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 4; i++) {
        ADC0.COMMAND = ADC_STCONV_bm;
        while (ADC0.COMMAND & ADC_STCONV_bm) {}
        sum += ADC0.RES;
    }

    ADC0.CTRLA = 0;
    VREF.CTRLA = 0;

    uint16_t avg = static_cast<uint16_t>(sum >> 2);
    if (avg == 0) return 5500;

    // Stir PRNG entropy with LSB jitter from ADC + timer
    g_prngState ^= (sum ^ micros());

    // V_ref = 1.1V -> ADC = 1024 * 1.1 / VDD => VDD_mV = (1100 * 1024) / ADC
    return static_cast<uint16_t>(1126400UL / avg);
}

/**
 * @brief Non-blocking delay that returns true immediately if user taps SW1.
 */
static bool delayInterruptible(uint16_t ms) {
    uint32_t start = millis();
    while ((millis() - start) < ms) {
        if (digitalRead(BTN_PIN) == LOW) {
            delay(DEBOUNCE_MS);
            if (digitalRead(BTN_PIN) == LOW) {
                return true;
            }
        }
        delay(4);
    }
    return false;
}

static void feedbackPulse(uint8_t ledIdx, uint8_t flashes) {
    allLedsOff();
    for (uint8_t i = 0; i < flashes; i++) {
        setLedRaw(ledIdx, 180);
        delay(80);
        setLedRaw(ledIdx, 0);
        delay(80);
    }
}

// -----------------------------------------------------------------------------
// Fidget Modes (0..4)
// -----------------------------------------------------------------------------

// Mode 0: Bouncing Comet / Larson Scanner with smooth PWM tail
static void runModeComet() {
    // Path weaves smoothly across all 7 LEDs (outer ring + center cross)
    static const uint8_t path[12] = {0, 1, 2, 6, 5, 4, 3, 2, 1, 6, 4, 5};
    uint8_t tail[NUM_LEDS] = {0};

    for (uint8_t step = 0; step < 24; step++) {
        // Exponential decay on all LEDs
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            tail[i] = (static_cast<uint16_t>(tail[i]) * 140U) >> 8;
        }
        uint8_t head = path[step % 12];
        tail[head] = 255;

        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            setLed(i, tail[i]);
        }
        if (delayInterruptible(75)) return;
    }

    // Fade tail out smoothly
    for (uint8_t f = 0; f < 10; f++) {
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            tail[i] >>= 1;
            setLed(i, tail[i]);
        }
        delay(35);
    }
}

// Mode 1: Solar Supercapacitor Battery Level Meter (1.8V .. 5.1V)
static void runModeBatteryMeter() {
    uint16_t vcc = readVccMillivolts();

    // Map 1900 mV .. 4900 mV across 1..7 LEDs
    uint8_t level = 1;
    if (vcc >= 4800)      level = 7;
    else if (vcc >= 4300) level = 6;
    else if (vcc >= 3800) level = 5;
    else if (vcc >= 3300) level = 4;
    else if (vcc >= 2800) level = 3;
    else if (vcc >= 2300) level = 2;
    else                  level = 1;

    // Sequential bar-graph fill up
    for (uint8_t i = 0; i < level; i++) {
        uint8_t led = BATTERY_ORDER[i];
        for (uint8_t b = 32; b <= 240; b += 52) {
            setLed(led, b);
            if (delayInterruptible(16)) return;
        }
        setLed(led, 255);
    }

    // Top-level indicator pulse if nearly full (>= 6 bars) or low (1 bar)
    if (level == 1) {
        for (uint8_t k = 0; k < 3; k++) {
            setLed(BATTERY_ORDER[0], 40);
            if (delayInterruptible(150)) return;
            setLed(BATTERY_ORDER[0], 255);
            if (delayInterruptible(150)) return;
        }
    } else {
        if (delayInterruptible(1100)) return;
    }

    // Smooth fade down
    for (int16_t b = 255; b >= 0; b -= 25) {
        for (uint8_t i = 0; i < level; i++) {
            setLed(BATTERY_ORDER[i], static_cast<uint8_t>(b));
        }
        delay(25);
    }
}

// Mode 2: Dice / Random Roulette Spinner
static void runModeDiceSpinner() {
    uint8_t winner = static_cast<uint8_t>(nextRandom() % 6U);
    uint8_t totalSteps = 18U + winner; // Guarantees >= 3 full laps, ending on winner
    uint16_t stepDelay = 35;

    for (uint8_t s = 0; s <= totalSteps; s++) {
        uint8_t idx = s % 6U;
        allLedsOff();
        setLed(idx, 255);
        // Trailing glow on previous outer LED
        setLed((idx + 5U) % 6U, 64);

        if (delayInterruptible(stepDelay)) return;

        // Progressive deceleration
        if (s > 8) {
            stepDelay += (s - 7U) * 7U;
        }
    }

    // Celebrate winner with 3 crisp pulses + center dot
    for (uint8_t k = 0; k < 3; k++) {
        allLedsOff();
        setLed(winner, 255);
        setLed(6, 120);
        if (delayInterruptible(160)) return;
        setLed(winner, 30);
        setLed(6, 0);
        if (delayInterruptible(120)) return;
    }
}

// Mode 3: Breathing Firefly
static void runModeFirefly() {
    // 2 smooth sine-wave breathing cycles
    for (uint8_t cycle = 0; cycle < 2; cycle++) {
        for (uint8_t i = 0; i < 64; i++) {
            uint8_t val = pgm_read_byte(&BREATHE_LUT[i]);
            uint8_t ringVal = val >> 2; // Soft halo on outer ring

            setLed(6, val); // Center LED breathes at full depth
            for (uint8_t r = 0; r < 6; r++) {
                setLed(r, ringVal);
            }
            if (delayInterruptible(28)) return;
        }
        if (delayInterruptible(120)) return;
    }
}

// Mode 4: Reaction / Click-Counter Fidget (7-bit binary display 0..127)
static void displayCountBinary(uint8_t count) {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        setLed(i, (count & (1U << i)) ? 220 : 0);
    }
}

static void runModeClickCounter() {
    g_clickCount = (g_clickCount + 1U) & 0x7FU;
    displayCountBinary(g_clickCount);

    uint32_t lastActivity = millis();
    while ((millis() - lastActivity) < CLICKER_IDLE_MS) {
        if (digitalRead(BTN_PIN) == LOW) {
            delay(DEBOUNCE_MS);
            if (digitalRead(BTN_PIN) == LOW) {
                uint32_t pressStart = millis();
                while (digitalRead(BTN_PIN) == LOW) {
                    if ((millis() - pressStart) >= LONG_PRESS_MS) {
                        // Long press resets counter and exits to sleep
                        g_clickCount = 0;
                        feedbackPulse(6, 2);
                        return;
                    }
                    delay(10);
                }
                // Short press increments counter & extends active window
                g_clickCount = (g_clickCount + 1U) & 0x7FU;
                displayCountBinary(g_clickCount);
                lastActivity = millis();
            }
        }
        delay(10);
    }
}

// -----------------------------------------------------------------------------
// Setup & Main Loop
// -----------------------------------------------------------------------------
void setup() {
    disableUnusedPinsForSleep();

    pinMode(BTN_PIN, INPUT_PULLUP);
    initLedPwm();

    // Seed PRNG from VREF/ADC noise at power-on
    g_prngState ^= readVccMillivolts();

    // Quick boot pulse on center LED to confirm power-on
    feedbackPulse(6, 1);

    // Immediately drop into ~100 nA deep sleep until user presses the button
    enterDeepSleep();
}

void loop() {
    // Woken from deep sleep by button press
    delay(DEBOUNCE_MS);

    if (digitalRead(BTN_PIN) == LOW) {
        uint32_t pressStart = millis();
        bool brightnessToggled = false;
        bool lockToggled = false;

        while (digitalRead(BTN_PIN) == LOW) {
            uint32_t held = millis() - pressStart;
            if (!brightnessToggled && held >= LONG_PRESS_MS) {
                g_ecoBright = !g_ecoBright;
                brightnessToggled = true;
                // Visual cue: 2 center flashes for Eco, 3 for Bright
                feedbackPulse(6, g_ecoBright ? 2 : 3);
            }
            if (!lockToggled && held >= MODE_LOCK_MS) {
                // Undo the brightness toggle so extra-long press cleanly toggles Mode Lock
                g_ecoBright = !g_ecoBright;
                g_modeLocked = !g_modeLocked;
                lockToggled = true;
                // Visual cue: ring flash for Locked/Unlocked
                feedbackPulse(0, g_modeLocked ? 3 : 1);
            }
            delay(10);
        }

        if (!brightnessToggled && !lockToggled) {
            // Standard short press: advance mode (unless locked) and play
            if (!g_modeLocked) {
                g_currentMode = (g_currentMode + 1U) % NUM_MODES;
            }

            switch (g_currentMode) {
                case MODE_COMET:         runModeComet();        break;
                case MODE_BATTERY_METER: runModeBatteryMeter(); break;
                case MODE_DICE_SPINNER:  runModeDiceSpinner();  break;
                case MODE_FIREFLY:       runModeFirefly();      break;
                case MODE_CLICK_COUNTER: runModeClickCounter(); break;
                default:                 g_currentMode = 0;     break;
            }
        }
    }

    // Always return to ultra-low-power SLEEP_MODE_PWR_DOWN after cycle
    enterDeepSleep();
}
