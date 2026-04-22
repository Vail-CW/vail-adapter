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

The Arduino Micro is supported as an experimental target. To build for it:

1. In `config.h`, uncomment `#define ARDUINO_MICRO_BOARD` and comment out all other board configs.
2. Build with:
   ```
   arduino-cli compile --fqbn arduino:avr:micro --output-dir build_output --export-binaries .
   ```
3. Flash with:
   ```
   arduino-cli upload --fqbn arduino:avr:micro -p <PORT> build_output
   ```
   …or use the web updater at [vailadapter.com](https://vailadapter.com), which will speak the Caterina/AVR109 protocol via WebSerial.

### Micro pin map
* D0: Straight Key
* D1: Dah
* D2: Dit
* D10: Piezo/buzzer
* A2/A3: optional radio output (uncomment `HAS_RADIO_OUTPUT` in config.h — 5V logic!)

### Micro limitations
* No capacitive touch (ATmega32U4 lacks FreeTouch hardware)
* No button menu (no resistor ladder implementation for AVR)
* No LED state indicators
* CW memory: 3 slots × ~12 seconds each (vs. 25 seconds on SAMD21), due to the 1024-byte EEPROM and 2.5 KB RAM budget
* Radio output pins are 5V — verify your radio's keying line can accept 5V, or use a transistor/level shifter

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
