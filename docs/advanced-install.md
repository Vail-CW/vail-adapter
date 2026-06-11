# Advanced Install

In the Arduino IDE, edit [vail-adapter.ino](../vail-adapter.ino) with the pins
you want to use on your device. 

You will need the MidiUSB and Keyboard libraries installed.
You can do this through the Library manager.

Then compile and upload the sketch.


## Works with no source code changes

* Seeeduino Xiao SAMD21
* Adafruit Qt Py SAMD21
* Arduino Micro (select the `ARDUINO_MICRO_BOARD` config — see below)

## Known to work with source code changes

* KeeYees Pro Micro
* Arduino Leonardo
* Arduino Zero
* Adafruit Trinket M0
* Adafruit GEMMA M0
* Adafruit Feather M0

## Arduino Micro — Notes and Build

The Arduino Micro is supported as an experimental DIY/breadboard-only target (there's no Micro PCB variant). It uses 5V logic instead of the SAMD21's 3.3V, and has a micro-USB connector.

### Build

1. In `config.h`, uncomment `#define ARDUINO_MICRO_BOARD` and comment out all other board configs.
2. Build with:
   ```
   arduino-cli compile --fqbn arduino:avr:micro --output-dir build_output --export-binaries .
   ```
3. Flash with:
   ```
   arduino-cli upload --fqbn arduino:avr:micro -p <PORT> build_output
   ```
   …or use the web updater at [vailadapter.com](https://vailadapter.com) (activate the 🧪 Test channel and pick **DIY No PCB → Arduino Micro** in the wizard — it speaks the Caterina/AVR109 protocol via WebSerial).

### Micro pinout & wiring

Full pinout diagram and datasheet: [docs.arduino.cc/hardware/micro](https://docs.arduino.cc/hardware/micro).

Required connections:

| Function | Micro pin | Notes |
|---|---|---|
| Ground | `GND` | Common for paddle/key, piezo, radio |
| Dit (paddle) | `D2` | Tie paddle dit contact to D2, ring it to GND through the switch |
| Dah (paddle) | `D1` | Same pattern as dit |
| Straight key | `D0` | For a straight key or mono-TRS jack tip |
| Piezo buzzer | `D10` → piezo `+`, `GND` → piezo `−` | Passive piezo; polarity doesn't matter for tone |
| Radio output dit (optional) | `A3` | **5V logic** — verify radio tolerance or use a level shifter/transistor |
| Radio output dah (optional) | `A2` | Same 5V caveat |

The internal pull-ups are enabled, so each input pin (D0/D1/D2) is HIGH when the contact is open and pulled LOW when the paddle/key closes. No external resistors needed on the paddle side.

### Using a 3.5mm TRS headphone jack (straight key or paddle)

Wire the jack exactly like the XIAO version, using the Micro's equivalent pins:

```
    o  --- D2  (tip   — dit, or straight-key contact)
   |_| --- D1  (ring  — dah)
   | | --- GND (sleeve)
   | |
```

For a simple mono straight key, wire only tip (`D0`) and sleeve (`GND`).

### Entering the bootloader

The web updater handles this automatically (1200-baud touch), but if you need to do it manually:

- **Double-tap the reset button** on the Micro within ~1 second. A new COM port will appear for ~8 seconds — that's the Caterina bootloader port.

### Micro limitations

* **No capacitive touch** — ATmega32U4 lacks the FreeTouch hardware. All paddle/key inputs must use physical switches.
* **No button menu** — the resistor-ladder menu is SAMD-only. Change settings via MIDI (vailmorse.com) instead.
* **No LED status indicators** — the onboard LED is not driven by the firmware (RAM is tight; skipped for the AVR port).
* **CW memory** — 3 slots × ~12 seconds each (vs. 25 seconds on SAMD21), due to the 1024-byte EEPROM and 2.5 KB RAM budget.
* **5V radio output** — the optional radio pins swing to 5V, not 3.3V. Verify your radio's keying line can accept 5V, or use a transistor/level shifter.
* **Flashing** — no UF2 drag-and-drop. Use the web updater's test channel, Arduino IDE, or `arduino-cli upload`.

## Will Not Work!

The RP2040 chip will not work, because it lacks a USB MCU,
needed by the MIDIUSB library. 
I'm listing specific devices here
in the hopes that seeing them crossed out will prevent people from
making a purchasing mistake!

* ~~Seeeduino Xiao RP2040~~ Will not work!
* ~~Adafruit Qt Pi RP2040~~ Will not work!
* ~~Any RP2040 Device~~ Will not work!


# Advanced Wiring

![XIAO Pinout](https://files.seeedstudio.com/wiki/Seeeduino-XIAO/img/Seeeduino-XIAO-pinout-1.jpg)

* GND: Ground
* D2: Dit
* D1: Dah
* D0: Straight Key
* D10: Speaker or Passive piezo buzzer
* A6: Capacative Dit
* A7: Capacative Dah
* A8: Capacative Straight Key


## Using a headphone jack

You can wire a headphone jack up to GND, D1, and D2.
GND should be the sleeve, D1 the ring, and D2 the tip.

     o  --- D2 (tip, dit)
    |_| --- D1 (ring, dah)
    | | --- GND (sleeve)
    | |

## Sidetone generator

If you connect a buzzer or speaker to pin 10 on one leg,
and ground on the other,
the adapter will beep when you press the straight key.

This will help a lot if there is a noticeable delay between when you press the key
and when your computer starts making a local beeping sound.

If you feel like no matter what you do,
you're always getting DAH with your straight key,
you should try this.


## Capacative Touch

The adapter works as a capacative touch sensor,
like a touch lamp.

You might wire these pins to screws or conductive pads. 
These can be used instead of, or in addition to, the normal pins D0, D1, and D2.

You do not need a ground wire with capacative touch!

* Pin A6: Dit capacative touch
* Pin A7: Dah capacative touch
* Pin A8: Straight key capacative touch
