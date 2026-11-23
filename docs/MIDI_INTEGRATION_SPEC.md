# Vail Adapter MIDI Integration Specification

## Overview

The Vail Adapter is an open-source Morse code keyer that provides both USB HID keyboard functionality and USB MIDI control. This document specifies how external applications can integrate with the MIDI portion of the adapter to control keyer behavior, tone generation, and timing parameters.

> All multi-byte examples below are raw MIDI bytes in hex, e.g. `B0 00 00`.

## Device Information

- **Device Type**: USB MIDI Class-Compliant Device
- **MIDI Channel**: 1 (Channel 0 in zero-indexed systems) — all status bytes are `B0/C0/90/80`
- **Vendor**: Open Source Hardware
- **Compatible with**: Any MIDI-capable application or DAW

## MIDI Message Types

The Vail Adapter responds to the following MIDI message types:

### Control Change Messages (0xBn)

#### CC0 - Mode Control
**Purpose**: Switch between Keyboard mode and MIDI mode

- **Message**: `B0 00 xx`
- **Values** (the value byte selects the mode by threshold, `value > 0x3F` = Keyboard):
  - `00-3F` (0-63): Enable **MIDI mode** (the adapter sends MIDI note events)
  - `40-7F` (64-127): Enable **Keyboard mode** (the adapter sends USB HID keyboard events)
- **Default**: Keyboard mode on startup
- **Examples**:
  - `B0 00 00` → enable MIDI mode
  - `B0 00 7F` → enable Keyboard mode

> Note: CC0 selects **only** the Keyboard-vs-MIDI output mode. Sending other
> MIDI messages (CC1/CC2, Program Change, Note On/Off) does **not** change it —
> issue a CC0 with a value of `00-3F` to put the adapter into MIDI mode first.
> Radio Mode is a separate output path controlled by CC4 (see below).

#### CC1 - Dit Duration (Speed Control)
**Purpose**: Control keying speed by setting dit duration

- **Message**: `B0 01 xx`
- **Formula**: Dit duration = `xx × 2` milliseconds; WPM = `1200 / dit_ms`
- **Range**: value `01`-`7F` → 2 ms to 254 ms (≈ 600 WPM down to ≈ 4.7 WPM)
- **Default**: value 50 (100 ms dit duration ≈ 12 WPM)
- **Example**: `B0 01 32` sets dit duration to 100 ms (~12 WPM)

#### CC2 - Sidetone Note
**Purpose**: Set the MIDI note number for the sidetone frequency

- **Message**: `B0 02 xx`
- **Range**: 0-127 (standard MIDI note numbers)
- **Default**: 69 (A4 = 440 Hz)
- **Tuning**: equal temperament
- **Example**: `B0 02 2D` sets sidetone to note 45 = A2 (110 Hz)

#### CC3 - Paddle Swap (Dit/Dah Inversion)
**Purpose**: Swap which input is treated as dit vs. dah. The swap can apply to **all inputs** (physical paddle pins DIT_PIN / DAH_PIN plus the capacitive touch pads QT_DIT_PIN / QT_DAH_PIN) or to the **capacitive touch pads only**, leaving the physical paddle jack alone. The straight-key input (PADDLE_STRAIGHT) and the radio output pin assignments (RADIO_DIT_PIN / RADIO_DAH_PIN) are intentionally **not** affected — only the input mapping is inverted.

- **Message**: `B0 03 xx`
- **Values**:
  - `00-1F` (0-31): Swap **OFF** (normal mapping everywhere)
  - `20-3F` (32-63): Swap **touch pads only** (physical paddle stays normal)
  - `40-7E` (64-126): Swap **all inputs** (physical paddle pins and touch pads)
  - `7F` (127): **Toggle** between OFF and all-inputs swap (legacy toggle behavior)
- **Default**: OFF
- **Persistence**: Saved to EEPROM and restored on power-up (`0`=off, `1`=all, `2`=touch only)
- **Audible feedback**: Whenever the state actually changes, the adapter announces the new state in Morse on the sidetone buzzer at the user's current WPM and tone: "INV" (all inputs swapped), "INV C" (touch pads only), "INV OFF" (back to normal). No tone is played if the command does not change the current state.
- **Side effects on change**: Any in-flight transmission is ended and all keyboard/MIDI keys are released cleanly before the new mapping takes effect.
- **Backward compatibility**: The first beta used `00-3F` = off and `40-7E` = on. The `40-7E` = full swap meaning is unchanged; values `20-3F` (previously "off") now select the touch-only swap.
- **Examples**:
  - `B0 03 00` → swap off ("INV OFF")
  - `B0 03 20` → swap touch pads only ("INV C")
  - `B0 03 40` → swap everything ("INV")
  - `B0 03 7F` → toggle off/all ("INV" or "INV OFF")
- **Special-feature gestures**: Gesture detection (5-second dit hold for buzzer disable, capacitive dah taps for radio mode, etc.) keys off the **post-swap** dit/dah roles, so documented gestures follow whichever input currently acts as dit or dah.

#### CC4 - Radio Mode On/Off
**Purpose**: Enter or exit Radio Mode from software. Radio Mode routes keying to the radio output pins (RADIO_DIT_PIN / RADIO_DAH_PIN) instead of USB and mutes the sidetone.

- **Message**: `B0 04 xx`
- **Values** (threshold, `value > 0x3F` = on):
  - `00-3F` (0-63): Radio Mode **OFF**
  - `40-7F` (64-127): Radio Mode **ON**
- **Availability**: Advanced PCB only (firmware built with `HAS_RADIO_OUTPUT`). On other boards CC4 is ignored.
- **Works while in Radio Mode**: The adapter keeps parsing MIDI in every mode, so `B0 04 00` is the software way to **exit** Radio Mode even though keying output is diverted to the radio pins. (Previously Radio Mode could only be toggled by tapping the capacitive dah pad 10x within 500 ms.)
- **No reset**: Exiting Radio Mode is a clean, in-place transition — the USB connection stays up and the host MIDI port is **not** dropped. (The capacitive-dah gesture now exits the same clean way.)
- **Persistence**: Not saved to EEPROM — the adapter always boots out of Radio Mode.
- **Audible feedback**: Ascending 3-note cue on entering, descending 3-note cue on exiting. Sending the state the adapter is already in does nothing (no cue).
- **Side effects on change**: Any in-flight transmission is ended on the path it started on, and the keyer, host keys/notes, hold timers and the Morse decoder are cleared, so nothing keyed before the switch leaks out after it.
- **Examples**:
  - `B0 04 7F` → enter Radio Mode
  - `B0 04 00` → exit Radio Mode

#### CC5 - Buzzer Enable/Disable
**Purpose**: Turn the sidetone buzzer on or off from software. (Pitch is set separately by CC2.)

- **Message**: `B0 05 xx`
- **Values** (threshold, `value > 0x3F` = on):
  - `00-3F` (0-63): Buzzer **disabled** (descending cue, then silent)
  - `40-7F` (64-127): Buzzer **enabled** (ascending cue)
- **Note**: This is the only way to **re-enable** the buzzer short of a reboot — the physical gesture (holding the key 5-6 s) can only disable it.
- **Persistence**: Not saved to EEPROM — the buzzer always boots enabled.
- **Repeats**: Sending the state the buzzer is already in does nothing (no cue), so a host can safely re-send CC5.
- **Held key**: Enabling while a key is held does not get undone by the hold-to-mute timer. That only re-arms on the next press.
- **Radio Mode**: The sidetone is suppressed in Radio Mode regardless of this setting; CC5 still updates the stored state for when Radio Mode is exited.
- **Memory recording**: While a CW memory is being recorded the sidetone always plays as feedback, even with the buzzer disabled.
- **Examples**:
  - `B0 05 00` → disable buzzer
  - `B0 05 7F` → enable buzzer

#### CC6 - Software Reset
**Purpose**: Reboot the adapter on demand (equivalent to unplugging/replugging) without touching the USB cable.

- **Message**: `B0 06 xx`
- **Values**: Only `7F` (127) triggers the reset. All other values are ignored, so a stray CC6 cannot accidentally reboot the device.
- **Availability**: SAMD boards only (uses `NVIC_SystemReset()`); ignored on other architectures.
- **USB re-enumeration**: A reset drops and re-enumerates USB by nature, so the host application **will lose the MIDI port and must reconnect** after sending this. Unlike CC4's Radio Mode exit, this is a true reboot, not a seamless transition.
- **Example**: `B0 06 7F` → reboot the adapter

### Program Change Messages (0xCn)

#### Keyer Mode Selection
**Purpose**: Select the keyer algorithm/behavior

- **Message**: `C0 xx`
- **Keyer Types**:
  - `00`: Passthrough. The adapter does no keying of its own. In MIDI mode it sends note 1 (C#) for the dit paddle and note 2 (D) for the dah paddle, raw, for as long as each paddle is held, and plays no sidetone for them. A straight key still sends note 0 (C) with sidetone. See [Passthrough mode](#passthrough-mode) below.
  - `01`: Straight Key (cootie)
  - `02`: Bug (semi-automatic)
  - `03`: Electric Bug
  - `04`: Single Dot
  - `05`: Ultimatic
  - `06`: Plain Iambic
  - `07`: Iambic A
  - `08`: Iambic B
  - `09`: Keyahead
- **Default**: 8 (Iambic B) — the value written when the EEPROM is first initialized
- **Invalid values**: numbers outside `00-09` fall back to passthrough (0)
- **Persistence**: Saved to EEPROM, including passthrough, so the adapter comes back up in the same keyer type after a replug

### Note On/Off Messages (0x9n/0x8n) — received by the adapter
**Purpose**: Manual sidetone generation (works like a standard MIDI synthesizer)

- **Note On**: `90 xx yy` (start playing note `xx`; velocity `yy` is accepted but ignored)
- **Note Off**: `80 xx yy` (stop playing note `xx`)
- **Range**: 0-127 (standard MIDI notes)
- **Behavior**: Drives the built-in buzzer only — it does not affect keyer logic. No tone is produced while the buzzer is disabled or while radio mode is active.
- **Stopping a note**: Always use `80 xx yy`. The velocity is ignored, so `90 xx 00` starts the note instead of stopping it.
- **Typical use**: An app running the adapter in passthrough can send these back, timed to its own keyed output, so the sidetone comes out of the adapter's buzzer.

## MIDI Output — notes the adapter sends

When in **MIDI mode**, keying produces Note On (`90 nn 7F`) / Note Off (`80 nn 00`)
events on channel 1, where the note number `nn` is:

| Note number | Pitch | Sent for |
|---|---|---|
| `0` | C  | Straight key, and any element from keyer modes 1-9 (already-timed key-down) |
| `1` | C# | Dit (passthrough mode 0, or direct paddle routing) |
| `2` | D  | Dah (passthrough mode 0, or direct paddle routing) |

In practice, set the keyer to **Passthrough (PC 0)** when you want the host to
receive distinct dit (`1`/C#) and dah (`2`/D) events and do its own timing — this
is how the Vail web repeater drives the adapter. With any onboard keyer (PC 1-9),
the adapter performs the timing itself and emits note `0` (C) for each element.

## Passthrough mode

In passthrough (`C0 00`) the adapter just reports the paddles and the host does
all of the timing.

| Paddle | Pressed | Released |
|---|---|---|
| Dit | `90 01 7F` | `80 01 00` |
| Dah | `90 02 7F` | `80 02 00` |

The adapter doesn't know what the host ends up sending, so it plays no sidetone
for the paddles in this mode. The host has two ways to give the operator a tone:

1. **Play it on the computer.** Generate the tone in the app while it's keying. This is what Vail web does.
2. **Play it on the adapter.** Send `90 nn 7F` when the app's keyed output goes down and `80 nn 00` when it comes back up, where `nn` is the pitch as a MIDI note (`45`, which is 69, for 440 Hz). See the Note On/Off section above.

In Keyboard mode, passthrough sends Left Ctrl for the dit paddle and Right Ctrl
for the dah paddle instead of notes.

## Integration Example

A sample sequence to configure the adapter:

```
# Enable MIDI mode
B0 00 00

# Set medium speed (120 ms dit = ~10 WPM)
B0 01 3C

# Set sidetone to middle C (C4, 261.6 Hz)
B0 02 3C

# Select Passthrough so the host receives dit/dah notes and does its own timing
C0 00
```

### Implementation Notes

1. **Mode switching**: Keyboard-vs-MIDI output is set exclusively by CC0 (`00-3F` = MIDI, `40-7F` = Keyboard). Radio Mode is set by CC4 (`40-7F` = on, `00-3F` = off) on Advanced-PCB firmware, and works even while Radio Mode is active. The adapter does **not** auto-switch on other messages.
2. **Settings persistence**: Keyer type, dit duration, sidetone note, and paddle swap are saved to EEPROM and restored on power-up. (CC0 output mode, CC4 Radio Mode, and CC5 buzzer state are **not** persisted — the adapter always boots in Keyboard mode, out of Radio Mode, with the buzzer enabled.)
3. **Real-time response**: All MIDI commands take effect immediately.
4. **Keyboard-mode output**: In Keyboard mode, dit sends Left Ctrl and dah sends Right Ctrl as USB HID key events.
5. **Keyboard Sim mode**: Keying the prosign KSKS as its own word puts the adapter in Keyboard Sim mode. It then decodes the operator's Morse and types the letters over USB instead of sending Ctrl keys or notes. BT is a space, BK is Enter, and 8 or more dits is a backspace. It stays on until the adapter is unplugged. This is turned on from the key, not over MIDI.

## Technical Specifications

- **USB MIDI Class**: 1.0 compliant
- **Latency**: <1 ms for MIDI command processing
- **Concurrent Notes**: Single note playback (monophonic)
- **Power Requirements**: USB bus powered
- **Operating System**: Windows, macOS, Linux (class-compliant)
