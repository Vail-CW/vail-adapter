# Hardware Interfaces

This document covers all hardware interfaces, pin assignments, and peripheral configurations for the VAIL SUMMIT device.

## Display (ST7796S 4.0" via SPI)

- **Model:** 4.0" TFT ST7796S LCD
- **Resolution:** 480×320 pixels (landscape orientation, `SCREEN_ROTATION = 1`)
- **Library:** LovyanGFX (replaces Adafruit_ST7789)
- **Chip Select:** GPIO 10 (`TFT_CS`)
- **Reset:** GPIO 11 (`TFT_RST`)
- **Data/Command:** GPIO 12 (`TFT_DC`)
- **MOSI:** GPIO 35 (Hardware SPI)
- **SCK:** GPIO 36 (Hardware SPI)
- **MISO:** GPIO 37 (Hardware SPI - required for ST7796S)
- **Backlight:** Hardwired to 3.3V (always on)

**Configuration:**
- Hardware SPI used for maximum performance
- Display updates disabled during audio-critical operations (practice, games, radio)
- Partial screen updates used where possible to minimize glitches

## SD Card (via SPI - shares bus with display)

- **Chip Select:** GPIO 38 (`SD_CS`)
- **MOSI:** GPIO 35 (Shared with display - Hardware SPI)
- **SCK:** GPIO 36 (Shared with display - Hardware SPI)
- **MISO:** GPIO 37 (Shared with display - Hardware SPI)

**Configuration:**
- SD card shares the same SPI bus as the display
- Separate chip select pins allow independent device control (Display: GPIO 10, SD: GPIO 38)
- Only one device can be active at a time (controlled by CS pin)
- SD card slot is integrated on the back of the ST7796S display board
- Requires separate wiring of all 4 SD card pins to ESP32-S3 Feather

**Library Support:**
- Standard Arduino SD library (FAT32 file system)
- SPI bus must be properly shared between display and SD card operations
- SD card initialized on-demand (first access to storage page) to avoid boot-time SPI conflicts

**Card Requirements:**
- Format: FAT32 (required)
- Recommended size: 4-32 GB SDHC
- Speed class: Class 10 or higher recommended
- Cards larger than 32 GB must be reformatted from exFAT to FAT32

## Keyboard (CardKB via I2C)

- **I2C Address:** `0x5F`
- **SDA:** GPIO 3
- **SCL:** GPIO 4

### Special Key Codes

- `0xB5` - UP arrow
- `0xB6` - DOWN arrow
- `0xB4` - LEFT arrow
- `0xB7` - RIGHT arrow
- `0x0D` - ENTER
- `0x1B` - ESC

### Polling Behavior

- **Normal modes:** 10ms intervals
- **Practice/Game modes:** 50ms intervals (audio priority)

## Paddle Input

**Physical Paddle Pins:**
- **DIT:** GPIO 6 (`PADDLE_DIT_PIN`)
- **DAH:** GPIO 9 (`PADDLE_DAH_PIN`)
- **Configuration:** Active LOW with internal pullups

**Supported Key Types:**
- Straight key (DIT paddle only)
- Iambic A (alternating priority)
- Iambic B (last-contact priority with memory)

**Implementation:**
- Iambic logic in `training_practice.h`
- Implements memory modes and squeeze keying
- State machine handles timing for WPM accuracy

## Capacitive Touch

**Touch Pad Pins:**
- **DIT:** GPIO 8 (T8) - `TOUCH_DIT_PIN`
- **DAH:** GPIO 5 (T5) - `TOUCH_DAH_PIN`
- **Threshold:** Configured in `config.h` (`TOUCH_THRESHOLD = 40000`)

### Critical Notes

**CRITICAL BUG WORKAROUNDS:**
1. **Must use GPIO numbers directly in `touchRead()`**, not T-constants (ESP32-S3 bug)
   - Correct: `touchRead(8)` and `touchRead(5)`
   - Incorrect: `touchRead(T8)` and `touchRead(T5)`

2. **Touch values RISE when touched on ESP32-S3**
   - Check `> threshold`, not `< threshold`
   - Opposite of ESP32 classic behavior

3. **GPIO 13 conflicts with GPIO 14** (I2S/touch shield channel)
   - Causes peripheral freeze
   - Avoid using GPIO 13 for touch sensing

4. **GPIO 8 and GPIO 5 work reliably together** without conflicts

### Hardware Requirements

- Uses ESP32-S3 internal capacitive sensing
- No external components required
- Conductive pads or bare PCB traces sufficient

## Radio Keying Output

**Output Pins:**
- **DIT output:** GPIO 18 (A0) - `RADIO_KEY_DIT_PIN`
- **DAH output:** GPIO 17 (A1) - `RADIO_KEY_DAH_PIN`
- **Format:** 3.5mm TRS jack (Tip = Dit, Ring = Dah, Sleeve = GND)
- **Logic:** Active HIGH (pin goes HIGH when keying)

### Hardware Interface Circuit

**CRITICAL:** Direct GPIO-to-radio connection may damage equipment. Always use driver circuit:

```
GPIO → 1kΩ resistor → NPN transistor base
Transistor collector → Radio keying input
Transistor emitter → GND
```

Recommended transistors: 2N2222, 2N3904, or similar NPN types

### Compatibility

- Tested and working with external ham radios
- Same design as standard Vail adapters
- Transistors pull radio keying inputs to ground when activated

## I2S Audio (MAX98357A)

**I2S Pins:**
- **BCK (Bit Clock):** GPIO 14 (`I2S_BCK_PIN`)
- **LRC (Word Select):** GPIO 15 (`I2S_LRC_PIN`)
- **DIN (Data In):** GPIO 16 (`I2S_DIN_PIN`)

**Audio Configuration:**
- **Sample Rate:** 44.1kHz
- **Bit Depth:** 16-bit
- **Channels:** Stereo (mono signal duplicated to both channels)
- **Software Volume Control:** 0-100%
- **Hardware Gain:** Set by GAIN pin (float=9dB, GND=12dB, VIN=6dB)

### Critical Initialization

1. **I2S must be initialized BEFORE display** - I2S needs higher DMA priority
2. **I2S DMA has highest interrupt priority** (`ESP_INTR_FLAG_LEVEL3`)
3. **Audio buffers filled in interrupt context** - tone generation must be fast

### Volume Control

- Software attenuation applied during sample generation in `i2s_audio.h`
- Settings persisted in Preferences namespace "audio"
- Hardware gain on MAX98357A is fixed (GAIN pin configuration)

## Battery Monitor

Two battery monitor chips supported (I2C auto-detection on startup):

### MAX17048 (Primary)

- **I2C Address:** `0x36`
- **Used on:** Adafruit ESP32-S3 Feather V2
- **Features:** Voltage and state-of-charge percentage
- **Library:** Adafruit MAX1704X

### LC709203F (Backup/Alternative)

- **I2C Address:** `0x0B`
- **Features:** Voltage and state-of-charge percentage
- **Library:** Adafruit LC709203F

### Calibration

- Fuel gauge chips provide voltage and SoC percentage
- Calibration improves over several charge/discharge cycles
- Initial readings may be lower than actual charge

### USB Charging Detection

**USB charging detection is DISABLED** because GPIO 15 (A3) is used by I2S for the LRC clock signal.

**Why:**
- Using `analogRead(A3)` reconfigures the pin
- This completely breaks I2S audio output
- No workaround available - I2S LRC cannot be moved

## Pin Repurposing Notes

### GPIO 5 (Originally BUZZER_PIN)

**Old usage:** PWM buzzer
**New usage:** Capacitive touch dit pad (`TOUCH_DIT_PIN`)
**Reason:** PWM buzzer replaced by I2S audio system

### GPIO 13 (Originally TFT_BL)

**Old usage:** Display backlight PWM
**New usage:** Capacitive touch dah pad (`TOUCH_DAH_PIN`)
**Reason:** Display backlight hardwired to 3.3V for always-on operation

**NOTE:** GPIO 13 was later changed to GPIO 5 due to conflicts with GPIO 14 (I2S)

### GPIO 15 (A3, USB_DETECT_PIN)

**Cannot be used for:** Analog input (USB charging detection)
**Current usage:** I2S LRC clock signal
**Reason:** Any `analogRead(A3)` call breaks audio

### GPIO 38 (SD Card CS)

**Current usage:** SD card chip select
**Notes:** One of the few remaining free GPIO pins on the ESP32-S3 Feather
**Functionality:** Web-based file management (upload, download, delete), data logging support
**Initialization:** On-demand (lazy init when storage management page is accessed)

## I2C Devices

**I2C Bus:**
- **SDA:** GPIO 3
- **SCL:** GPIO 4

**Devices:**
- `0x5F` - CardKB keyboard
- `0x36` - MAX17048 battery monitor (primary)
- `0x0B` - LC709203F battery monitor (backup)

**Auto-Detection:**
- I2C scan performed at startup
- Battery monitor type auto-detected
- Debug output on serial monitor

## GPIO Pin Summary

| GPIO | Function | Direction | Notes |
|------|----------|-----------|-------|
| 3 | I2C SDA | Bidirectional | CardKB, battery monitor |
| 4 | I2C SCL | Output | CardKB, battery monitor |
| 5 | Touch DIT | Input | Capacitive touch pad T5 |
| 6 | Paddle DIT | Input | Active LOW with pullup |
| 8 | Touch DAH | Input | Capacitive touch pad T8 |
| 9 | Paddle DAH | Input | Active LOW with pullup |
| 10 | TFT CS | Output | Display chip select |
| 11 | TFT RST | Output | Display reset |
| 12 | TFT DC | Output | Display data/command |
| 14 | I2S BCK | Output | Audio bit clock |
| 15 | I2S LRC | Output | Audio word select |
| 16 | I2S DIN | Output | Audio data |
| 17 | Radio DAH | Output | Radio keying (A1) |
| 18 | Radio DIT | Output | Radio keying (A0) |
| 35 | SPI MOSI | Output | Display/SD card data (hardware SPI) |
| 36 | SPI SCK | Output | Display/SD card clock (hardware SPI) |
| 37 | SPI MISO | Input | Display/SD card data in (hardware SPI) |
| 38 | SD CS | Output | SD card chip select |

## Power Considerations

**Active Power Consumption:**
- WiFi on: ~200mA
- WiFi off: ~100mA
- Deep sleep: ~20µA

**Wake Source:**
- DIT paddle press (GPIO 6)
- Triple-tap ESC in main menu to enter deep sleep

**Battery Life Estimates:**
- 500mAh battery: ~2-5 hours (WiFi dependent)
- Deep sleep: Several days to weeks
