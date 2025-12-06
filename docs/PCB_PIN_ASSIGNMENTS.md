# VAIL SUMMIT PCB Design - Pin Assignments

**Board:** ESP32-S3 Feather
**Firmware Version:** 0.2-4inch
**Last Updated:** 2025-01-22

This document provides complete pin assignments for designing a custom PCB for the VAIL SUMMIT morse code trainer.

---

## Pin Assignment Summary Table

| GPIO | Function | Direction | Pull | Connection | Notes |
|------|----------|-----------|------|------------|-------|
| 3 | I2C_SDA | Bidirectional | External | CardKB, Battery Monitor | STEMMA QT connector |
| 4 | I2C_SCL | Bidirectional | External | CardKB, Battery Monitor | STEMMA QT connector |
| 5 | TOUCH_DAH | Input | None | Capacitive Touch Pad | DAH touch sensor |
| 6 | DIT_PIN | Input | Internal Pullup | 3.5mm Jack Tip | Paddle dit input (active LOW) |
| 8 | TOUCH_DIT | Input | None | Capacitive Touch Pad | DIT touch sensor |
| 9 | DAH_PIN | Input | Internal Pullup | 3.5mm Jack Ring | Paddle dah input (active LOW) |
| 10 | TFT_CS | Output | None | ST7796S Display | SPI Chip Select |
| 11 | TFT_RST | Output | None | ST7796S Display | Hardware Reset |
| 12 | TFT_DC | Output | None | ST7796S Display | Data/Command select |
| 14 | I2S_BCK | Output | None | MAX98357A | I2S Bit Clock |
| 15 | I2S_LCK | Output | None | MAX98357A | I2S Word Select (LRC) |
| 16 | I2S_DATA | Output | None | MAX98357A | I2S Data Output |
| 17 | RADIO_DAH | Output | None | Radio Jack Ring | Ham radio dah key output |
| 18 | RADIO_DIT | Output | None | Radio Jack Tip | Ham radio dit key output |
| 35 | SPI_MOSI | Output | None | Display + SD Card | Shared SPI bus |
| 36 | SPI_SCK | Output | None | Display + SD Card | Shared SPI bus |
| 37 | SPI_MISO | Input | None | Display + SD Card | Shared SPI bus |
| 38 | SD_CS | Output | None | SD Card | SD card chip select |
| 39 | TFT_BL | Output | None | Display Backlight | Backlight control (active LOW) |

---

## Detailed Pin Assignments by Subsystem

### 1. Display Interface (ST7796S 4.0" LCD - 480×320)

**Interface:** Hardware SPI
**Resolution:** 480×320 pixels
**Orientation:** Landscape (rotation = 3)

| Pin | GPIO | Function | Connection |
|-----|------|----------|------------|
| CS | 10 | Chip Select | Display CS pin |
| RST | 11 | Hardware Reset | Display RESET pin |
| DC | 12 | Data/Command | Display D/C pin |
| MOSI | 35 | SPI Data Out | Display SDI/MOSI pin |
| SCK | 36 | SPI Clock | Display SCK pin |
| MISO | 37 | SPI Data In | Display SDO/MISO pin (required for ST7796S) |
| BL | 39 | Backlight Control | Display backlight control (active LOW) |

**Notes:**
- Uses LovyanGFX library for rendering
- BGR color order (not RGB): Set `cfg.rgb_order = false`
- MISO is required for proper initialization
- Backlight controlled via GPIO 39 (LOW = ON, HIGH = OFF)
- Turns off automatically during deep sleep to save power

---

### 2. SD Card Interface

**Interface:** SPI (shared bus with display)
**Format:** FAT32 only (exFAT not supported)
**Size:** 4-32 GB SDHC recommended

| Pin | GPIO | Function | Connection |
|-----|------|----------|------------|
| CS | 38 | Chip Select | SD Card CS pin (unique) |
| MOSI | 35 | SPI Data Out | SD Card DI/MOSI (shared with display) |
| SCK | 36 | SPI Clock | SD Card CLK/SCK (shared with display) |
| MISO | 37 | SPI Data In | SD Card DO/MISO (shared with display) |

**Notes:**
- Shares SPI bus with display (only CS pin is unique)
- Must initialize AFTER display to avoid SPI conflicts
- Add 10kΩ pull-up resistor on MISO line (optional but recommended)
- Use proper voltage levels (3.3V logic)

---

### 3. I2C Devices

**Bus Speed:** 100kHz (standard mode)
**Voltage:** 3.3V logic

| Pin | GPIO | Function | Connection |
|-----|------|----------|------------|
| SDA | 3 | I2C Data | All I2C devices |
| SCL | 4 | I2C Clock | All I2C devices |

**Connected Devices:**

| Device | I2C Address | Function |
|--------|-------------|----------|
| CardKB Keyboard | 0x5F | Primary input device |
| MAX17048 (or LC709203F) | 0x36 | Battery fuel gauge |

**Notes:**
- Use STEMMA QT / Qwiic connector for easy connection
- 4.7kΩ pull-up resistors on SDA and SCL (usually included on modules)
- Battery monitor auto-detected at startup

---

### 4. Audio System (MAX98357A I2S Amplifier)

**Interface:** I2S Digital Audio
**Sample Rate:** 44.1kHz
**Output:** Class-D speaker amplifier

| Pin | GPIO | Function | Connection |
|-----|------|----------|------------|
| BCK | 14 | Bit Clock | MAX98357A BCLK pin |
| LRC | 15 | Word Select | MAX98357A LRC/WS pin |
| DIN | 16 | Data Output | MAX98357A DIN pin |

**MAX98357A Additional Pins:**

| MAX98357A Pin | Connection | Function |
|---------------|------------|----------|
| GAIN | Float (9dB) or GND (12dB) or VIN (6dB) | Gain control (recommend float for 9dB) |
| SD | Float | Shutdown control (float = always on) |
| VIN | 3.3V or 5V | Power supply |
| GND | Ground | Ground connection |
| OUT+ / OUT- | Speaker | 3W speaker (4Ω or 8Ω) |

**Notes:**
- **CRITICAL:** Initialize I2S BEFORE display initialization (I2S needs higher DMA priority)
- Software volume control implemented in firmware (0-100%)
- Do NOT connect GPIO 5 for audio (legacy pin, now used for capacitive touch)

---

### 5. Paddle Input (Iambic Keyer) - 3.5mm Jack

**Jack Type:** 3.5mm TRS (Tip-Ring-Sleeve)
**Logic:** Active LOW (internal pullups enabled)

| Jack Pin | GPIO | Function | Connection |
|----------|------|----------|------------|
| Tip | 6 | Dit Paddle | Connect to dit paddle contact |
| Ring | 9 | Dah Paddle | Connect to dah paddle contact |
| Sleeve | GND | Ground | Common ground |

**Circuit:**
```
ESP32 GPIO 6 (DIT_PIN) ----[Internal Pullup]---- 3.3V
                      |
                      +---- Tip (3.5mm jack) ---- Paddle Dit ---- GND

ESP32 GPIO 9 (DAH_PIN) ----[Internal Pullup]---- 3.3V
                      |
                      +---- Ring (3.5mm jack) ---- Paddle Dah ---- GND
```

**Notes:**
- Internal pullups enabled in firmware (`INPUT_PULLUP`)
- Active LOW detection (paddle closes to ground when pressed)
- Supports iambic Mode A and Mode B keying
- No external resistors required

---

### 6. Capacitive Touch Pads (Built-in Keys)

**Sensor Type:** ESP32-S3 capacitive touch sensors
**Threshold:** 40,000 (values rise when touched on S3)

| Pad | GPIO | Function | Connection |
|-----|------|----------|------------|
| T8 | 8 | Dit Touch | Copper touch pad or wire antenna |
| T5 | 5 | Dah Touch | Copper touch pad or wire antenna |

**Notes:**
- Use GPIO numbers for `touchRead()`, not T-constants
- GPIO 13 conflicts with I2S/touch shield (do not use)
- Recommended pad size: 10mm × 10mm copper area
- Add ground ring around pads for better sensitivity
- Calibrate threshold in firmware if needed

---

### 7. Radio Keying Output - 3.5mm Jack

**Jack Type:** 3.5mm TRS (Tip-Ring-Sleeve)
**Logic:** Output HIGH to key radio (3.3V logic)
**Modes:** Summit Keyer (tip only) or Radio Keyer (tip+ring together)

| Jack Pin | GPIO | Function | Connection |
|----------|------|----------|------------|
| Tip | 18 (A0) | Dit Output | Radio key input (dit) |
| Ring | 17 (A1) | Dah Output | Radio key input (dah) |
| Sleeve | GND | Ground | Common ground with radio |

**Circuit (per output):**
```
ESP32 GPIO 18/17 ----[1kΩ]----+---- Optocoupler LED+ (e.g., 4N25)
                               |
                              GND ---- Optocoupler LED-

Optocoupler Output ---- Radio Key Line (isolated)
```

**Notes:**
- Recommend optocoupler isolation for safety (protect ESP32 from radio voltages)
- 1kΩ current-limiting resistor for LED
- Summit Keyer mode: Only tip (GPIO 18) used
- Radio Keyer mode: Both tip and ring used together
- Maximum current: ~20mA per pin (safe for direct GPIO if no isolation)

---

### 8. Battery Monitoring

**Fuel Gauge IC:** MAX17048 or LC709203F
**Interface:** I2C (address 0x36)
**Battery:** LiPo 3.7V single cell

**I2C Connection:** See I2C section above (GPIO 3/4)

**Battery Voltage Thresholds:**
- Full: 4.2V
- Empty: 3.3V (cutoff)

**Notes:**
- Auto-detected at startup (firmware tries MAX17048 first, then LC709203F)
- **DO NOT use analog pin A3 (GPIO 15)** - conflicts with I2S_LCK pin!
- USB detection disabled in firmware due to pin conflict
- Battery percentage displayed in status bar

---

### 9. Power Supply

**Input Voltage:** 5V USB or 3.7V LiPo battery
**Operating Voltage:** 3.3V (ESP32-S3 internal regulator)

**Power Distribution:**

| Component | Voltage | Current (typ) | Notes |
|-----------|---------|---------------|-------|
| ESP32-S3 | 3.3V | ~200mA | Peak current during WiFi TX |
| ST7796S Display | 3.3V | ~100mA | Backlight on |
| MAX98357A Amp | 3.3V or 5V | ~500mA | 3W speaker output |
| CardKB | 3.3V | ~5mA | Low power |
| Battery Monitor | 3.3V | ~50µA | Very low power |

**Total System Current:**
- Idle (display on, no audio): ~300mA @ 3.3V
- Active (audio + WiFi): ~800mA @ 3.3V
- Peak (WiFi TX + audio): ~1A @ 3.3V

**Battery Life Estimate (1000mAh LiPo):**
- Typical use: 2-3 hours
- Idle: 3-4 hours
- Deep sleep: 100+ hours

**Notes:**
- Use 3.3V LDO regulator with ≥1A output if powering from 5V
- Recommend separate 5V supply for MAX98357A for better audio quality
- Add bulk capacitors: 100µF on 3.3V rail, 220µF on 5V rail (if used)

---

## GPIO Summary by Function Type

### Inputs
- **GPIO 6:** Dit paddle (pullup)
- **GPIO 9:** Dah paddle (pullup)
- **GPIO 8:** Capacitive touch dit
- **GPIO 5:** Capacitive touch dah
- **GPIO 37:** SPI MISO (display + SD)

### Outputs
- **GPIO 10:** Display CS
- **GPIO 11:** Display RST
- **GPIO 12:** Display DC
- **GPIO 14:** I2S BCK
- **GPIO 15:** I2S LRC
- **GPIO 16:** I2S Data
- **GPIO 17:** Radio dah output
- **GPIO 18:** Radio dit output
- **GPIO 35:** SPI MOSI (shared)
- **GPIO 36:** SPI SCK (shared)
- **GPIO 38:** SD Card CS
- **GPIO 39:** Display backlight control

### Bidirectional
- **GPIO 3:** I2C SDA
- **GPIO 4:** I2C SCL

---

## Unused / Reserved Pins

The following ESP32-S3 Feather pins are **NOT used** and available for expansion:

- **GPIO 1** (A0) - ADC/GPIO
- **GPIO 2** (A1) - ADC/GPIO
- **GPIO 7** - GPIO
- **GPIO 13** - GPIO (avoid - conflicts with touch)
- **GPIO 21** - GPIO
- **GPIO 33** - GPIO
- **GPIO 34** - GPIO
- **GPIO 40** - GPIO
- **GPIO 41** - GPIO
- **GPIO 42** - GPIO

**Note:** GPIO 15 (A3) is used for I2S and **must NOT** be used for analog reads (breaks audio).

---

## PCB Layout Recommendations

### 1. **SPI Bus (Display + SD Card)**
- Keep traces short (<50mm if possible)
- Route CLK and MOSI as differential pair or matched length
- Add series resistors (22Ω-33Ω) on CLK and MOSI for signal integrity
- Add 0.1µF decoupling caps near each CS pin
- Route CS signals separately (don't share CS inadvertently)

### 2. **I2C Bus**
- Use 4.7kΩ pull-up resistors on SDA and SCL
- Keep traces short and away from high-speed signals (SPI, I2S)
- Add 0.1µF decoupling caps on each I2C device VCC pin

### 3. **I2S Audio**
- Route BCK, LRC, and DIN as matched-length traces if possible
- Keep away from digital switching signals (SPI)
- Add ferrite bead on MAX98357A VIN (reduces noise)
- Use wide traces for speaker output (≥20mil)

### 4. **Power Supply**
- Use separate 3.3V and GND planes if possible
- Add bulk capacitors near power input (100µF-220µF)
- Add 0.1µF ceramic caps near each IC VCC pin
- Use thick traces for power distribution (≥40mil for 1A)

### 5. **Touch Pads**
- Use 10mm × 10mm copper pads (no solder mask)
- Add ground ring (2mm width, 1mm spacing) around each pad
- Keep touch traces away from high-frequency signals

### 6. **Mechanical**
- 3.5mm jack footprints: Use PJ-320A or similar (common TRS jack)
- CardKB connector: STEMMA QT / Qwiic 4-pin JST-SH
- SD card slot: Push-push micro SD card holder
- Display mounting: M2.5 or M3 standoffs (check display module dimensions)

---

## Connector Summary

| Connector | Type | Pins | Purpose |
|-----------|------|------|---------|
| Paddle Input | 3.5mm TRS Jack | 3 | Iambic paddle (tip=dit, ring=dah, sleeve=GND) |
| Radio Output | 3.5mm TRS Jack | 3 | Radio keying (tip=dit, ring=dah, sleeve=GND) |
| I2C Keyboard | STEMMA QT / Qwiic | 4 | CardKB connection (SDA, SCL, 3.3V, GND) |
| Speaker | Screw Terminal or JST | 2 | 4Ω or 8Ω speaker (3W max) |
| Display | 40-pin FPC | 40 | ST7796S LCD module |
| USB-C | USB Type-C | - | Power + programming (ESP32-S3 built-in) |
| Battery | JST PH 2-pin | 2 | LiPo battery (3.7V) |

---

## BOM (Bill of Materials) - Key Components

| Component | Part Number / Type | Qty | Notes |
|-----------|-------------------|-----|-------|
| MCU Board | Adafruit ESP32-S3 Feather | 1 | Main microcontroller |
| Display | ST7796S 4.0" LCD (480×320) | 1 | SPI interface |
| Keyboard | M5Stack CardKB | 1 | I2C keyboard |
| Audio Amp | MAX98357A breakout | 1 | I2S Class-D amplifier |
| Speaker | 8Ω 3W | 1 | 40mm diameter recommended |
| Battery Monitor | MAX17048 or LC709203F | 1 | I2C fuel gauge |
| 3.5mm Jack | PJ-320A (TRS) | 2 | Paddle input + radio output |
| SD Card Holder | Micro SD push-push | 1 | Integrated on display or separate |
| Resistor 4.7kΩ | 0805 SMD or 1/4W TH | 2 | I2C pull-ups |
| Resistor 1kΩ | 0805 SMD or 1/4W TH | 2 | Radio output (if using optocouplers) |
| Capacitor 0.1µF | 0805 SMD or ceramic | 10+ | Decoupling caps |
| Capacitor 100µF | Electrolytic | 2 | Bulk power caps |
| LiPo Battery | 3.7V 1000-2000mAh | 1 | JST connector |
| Optocoupler (optional) | 4N25 or similar | 2 | Radio output isolation |

---

## Design Checklist

- [ ] All SPI signals (MOSI, SCK, MISO) shared correctly between display and SD card
- [ ] Separate CS pins for display (GPIO 10) and SD card (GPIO 38)
- [ ] I2C pull-up resistors (4.7kΩ) on SDA and SCL
- [ ] I2S pins (14, 15, 16) routed to MAX98357A
- [ ] Paddle inputs (6, 9) have internal pullups enabled in firmware (no external needed)
- [ ] Radio outputs (17, 18) have current limiting (1kΩ) if using optocouplers
- [ ] Touch pads (5, 8) have proper copper area and ground ring
- [ ] Battery monitor connected to I2C bus (address 0x36)
- [ ] Display backlight controlled via GPIO 39 (active LOW)
- [ ] Power supply rated for ≥1A continuous current
- [ ] Decoupling capacitors on all IC power pins
- [ ] USB-C connector for programming and power
- [ ] Deep sleep current minimized (disconnect unnecessary pullups/LEDs)

---

## References

- **Main Firmware:** [vail-summit.ino](../vail-summit.ino)
- **Hardware Config:** [src/core/config.h](../src/core/config.h)
- **Hardware Init:** [src/core/hardware_init.h](../src/core/hardware_init.h)
- **Full Documentation:** [CLAUDE.md](../CLAUDE.md)
- **Architecture:** [docs/ARCHITECTURE.md](ARCHITECTURE.md)
- **Hardware Details:** [docs/HARDWARE.md](HARDWARE.md)

---

**Document Version:** 1.0
**Created:** 2025-01-22
**License:** Proprietary (VAIL SUMMIT Contributors)
