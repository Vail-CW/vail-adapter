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

> Note: the mode is controlled **only** by CC0. Sending other MIDI messages
> (CC1/CC2, Program Change, Note On/Off) does **not** change the mode — issue a
> CC0 with a value of `00-3F` to put the adapter into MIDI mode first.

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

#### CC3 - Enable sending MIDI event time delta
**Purpose**: Switch between sending normal MIDI notes and notes with event time deltas

- **Message**: `B0 03 xx`
- **Values**:
  - `00-3F` (0-63): Disable sending of event time deltas
  - `40-7F` (64-127): Enable sending of event time deltas
- **Default**: Sending of time deltas is disabled
- **Example**: `B0 03 7F` enables sending of event time deltas

### Program Change Messages (0xCn)

#### Keyer Mode Selection
**Purpose**: Select the keyer algorithm/behavior

- **Message**: `C0 xx`
- **Keyer Types**:
  - `00`: Passthrough (manual control — routes paddle/key directly; in MIDI mode sends notes C#/D for dit/dah, C for a straight key)
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

### Note On/Off Messages (0x9n/0x8n) — received by the adapter
**Purpose**: Manual sidetone generation (works like a standard MIDI synthesizer)

- **Note On**: `90 xx yy` (start playing note `xx`; velocity `yy` is accepted but ignored)
- **Note Off**: `80 xx yy` (stop playing note `xx`)
- **Range**: 0-127 (standard MIDI notes)
- **Behavior**: Drives the built-in buzzer only — it does not affect keyer logic. No tone is produced while the buzzer is disabled or while radio mode is active.

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

1. **Mode switching**: The mode is set exclusively by CC0 (`00-3F` = MIDI, `40-7F` = Keyboard). The adapter does **not** auto-switch on other messages.
2. **Settings persistence**: Keyer type, dit duration, and sidetone note are saved to EEPROM and restored on power-up. (Output mode from CC0 is **not** persisted — the adapter always boots in Keyboard mode.)
3. **Real-time response**: All MIDI commands take effect immediately.
4. **Keyboard-mode output**: In Keyboard mode, dit sends Left Ctrl and dah sends Right Ctrl as USB HID key events.

## Technical Specifications

- **USB MIDI Class**: 1.0 compliant
- **Latency**: <1 ms for MIDI command processing
- **Concurrent Notes**: Single note playback (monophonic)
- **Power Requirements**: USB bus powered
- **Operating System**: Windows, macOS, Linux (class-compliant)
