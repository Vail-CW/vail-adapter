# Hardware Interfaces

This document covers all hardware interfaces, pin assignments, and peripheral configurations for the VAIL SUMMIT device.

## Display (ST7789V via SPI)

- **Resolution:** 240×320 pixels rotated to 320×240 landscape (`SCREEN_ROTATION = 1`)
- **Chip Select:** GPIO 10 (`TFT_CS`)
- **Data/Command:** GPIO 12 (`TFT_DC`)
- **Reset:** GPIO 11 (`TFT_RST`)
- **MOSI:** GPIO 35 (Hardware SPI)
- **SCK:** GPIO 36 (Hardware SPI)
- **Backlight:** Hardwired to 3.3V (always on)

**Configuration:**
- Hardware SPI used for maximum performance
- Display updates disabled during audio-critical operations (practice, games, radio)
- Partial screen updates used where possible to minimize glitches

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
| 35 | SPI MOSI | Output | Display data (hardware SPI) |
| 36 | SPI SCK | Output | Display clock (hardware SPI) |

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
