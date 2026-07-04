# TRRS Trinkey Build Guide

## Overview

The Adafruit TRRS Trinkey is a tiny USB key-sized SAMD21 board with a built-in TRRS (3.5mm audio) jack, making it ideal for a minimal Vail adapter build.

## Hardware Setup

### TRRS Jack Connections

The Trinkey's TRRS jack pins are mapped as follows:

- **Tip (Pin 0)** → DIT input
- **Ring 1 (Pin 2)** → DAH input
- **Ring 2 (Pin 4)** → Not used
- **Sleeve (Pin 5)** → Ground

**For paddle operation:**
- Use a stereo (TRS) cable from your paddle
- Tip connects to dit paddle contact
- Ring connects to dah paddle contact
- Sleeve is ground

**For straight key operation:**
- Use a mono (TS) cable from your straight key
- Tip connects to key contact
- Sleeve is ground
- The firmware will use the dit input (Pin 0) for straight key

### Buzzer Connection

Connect a piezo buzzer through the STEMMA QT connector:

- **Positive (+)** → SCL (Pin 9 / PA09)
- **Negative (-)** → GND

The STEMMA QT connector has four pins (in order):
1. GND (Black wire)
2. 3V (Red wire)
3. SDA (Blue wire / Pin 8)
4. SCL (Yellow wire / Pin 9)

Connect your buzzer between **GND (pin 1)** and **SCL (pin 4)**.

## Software Setup

### Prerequisites

Install arduino-cli and configure it for Adafruit boards:

```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
arduino-cli core update-index
arduino-cli core install adafruit:samd
```

Install required libraries:

```bash
arduino-cli lib install MIDIUSB "Adafruit FreeTouch Library" FlashStorage_SAMD Keyboard
```

### Configuration

1. Edit `config.h` and ensure **only** the TRRS_TRINKEY configuration is enabled:

```cpp
// #define V1_Basic_PCB
// #define V2_Basic_PCB
// #define Advanced_PCB
// #define NO_PCB_GITHUB_SPECS
#define TRRS_TRINKEY
```

2. All other configurations should be commented out.

### Building

Build the firmware with the Trinkey M0 board profile:

```bash
arduino-cli compile --fqbn adafruit:samd:adafruit_trinkey_m0 --output-dir build_output --export-binaries .
```

### Converting to UF2 (for drag-and-drop flashing)

Convert the .bin file to .uf2 format:

```bash
python uf2conv.py -c -f 0x68ED2B88 -b 0x2000 build_output/*.bin -o trinkey_vail_adapter.uf2
```

The family ID `0x68ED2B88` is for SAMD21 devices.

### Flashing

1. **Enter bootloader mode:**
   - Press the reset button on the Trinkey **twice quickly**
   - The NeoPixel LED will turn green/blue and a `TRINKETBOOT` drive will appear

2. **Flash the firmware:**
   - Drag `trinkey_vail_adapter.uf2` onto the `TRINKETBOOT` drive
   - The board will automatically reset and run the new firmware

## Pin Configuration Summary

| Function | Pin | Notes |
|----------|-----|-------|
| DIT Input | 0 (TIP) | TRRS tip contact |
| DAH Input | 2 (RING1) | TRRS ring 1 contact |
| KEY Input | 0 (TIP) | Same as DIT for straight key mode |
| Piezo Buzzer | 9 (SCL) | Via STEMMA QT connector |
| NeoPixel LED | 1 | Built-in status LED |

## Limitations

**No Button Menu:**
The Trinkey has no physical buttons, so the button menu system is not available. Settings must be changed via:
- MIDI control messages (CC0, CC1, CC2, PC)
- Modifying default values in code before building

**No Capacitive Touch:**
The Trinkey does not have capacitive touch capability.

**No Radio Output:**
This configuration does not include radio keying output pins.

## Default Settings

When flashed with default firmware:
- **Keyer Mode:** Iambic B (mode 8)
- **Speed:** 20 WPM (100ms dit duration)
- **Sidetone:** A4 (MIDI note 69, 440 Hz)
- **Output Mode:** USB HID Keyboard

## MIDI Control

To change settings without buttons, use MIDI control messages:

- **CC0** (value 0-1): Switch between keyboard (0) and MIDI (1) output mode
- **CC1** (value 10-200): Set dit duration (value × 2 milliseconds)
  - Example: Value 50 = 100ms dit = 20 WPM
- **CC2** (value 43-85): Set sidetone frequency (MIDI note number)
  - Example: 69 = A4 = 440 Hz
- **Program Change (PC)** (value 0-9): Select keyer mode
  - 0 = Passthrough
  - 1 = Straight Key
  - 2 = Bug
  - 3 = ElBug
  - 4 = Single Dot
  - 5 = Ultimatic
  - 6 = Plain
  - 7 = Iambic A
  - 8 = Iambic B
  - 9 = Keyahead

## Testing

After flashing:

1. **Test buzzer:** Power on the Trinkey - it should play "VAIL" in Morse code
2. **Test paddle/key:** Plug in your paddle/key cable and tap the contacts
3. **Test keyboard output:** Open a text editor and verify Ctrl key presses
4. **Test MIDI:** Send MIDI commands to verify mode switching and settings

## Board Information

- **MCU:** ATSAMD21E18 (32-bit Cortex M0+)
- **Clock:** 48 MHz
- **Flash:** 256 KB
- **RAM:** 32 KB
- **USB:** Native USB HID + MIDI
- **Board Package:** `adafruit:samd:adafruit_trinkey_m0`
- **Bootloader:** UF2 (double-tap reset to enter)

## Troubleshooting

**Trinkey not recognized:**
- Try a different USB port or cable
- Ensure the cable supports data, not just power

**No bootloader drive appears:**
- Reset timing is critical - try the double-tap slower or faster
- Try unplugging and re-plugging while holding reset

**Buzzer not working:**
- Verify connections to GND and SCL (pin 9) on STEMMA QT
- Check buzzer polarity (some piezo buzzers are polarized)
- Test with a multimeter in continuity mode

**No keyboard output:**
- Check that paddle cable is properly inserted
- Try switching keyer modes via MIDI
- Verify the device shows up as a USB keyboard in system settings

## References

- [Adafruit TRRS Trinkey Product Page](https://www.adafruit.com/product/5954)
- [Adafruit TRRS Trinkey Pinout Guide](https://learn.adafruit.com/adafruit-trrs-trinkey/pinouts)
- [Main Vail Adapter Documentation](README.md)
- [MIDI Integration Spec](MIDI_INTEGRATION_SPEC.md)
