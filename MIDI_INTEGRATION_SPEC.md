# Vail Adapter MIDI Integration Specification

## Overview

The Vail Adapter is an open-source Morse code keyer that provides both USB HID keyboard functionality and USB MIDI control. This document specifies how external applications can integrate with the MIDI portion of the adapter to control keyer behavior, tone generation, and timing parameters.

## Device Information

- **Device Type**: USB MIDI Class-Compliant Device
- **MIDI Channel**: 1 (Channel 0 in zero-indexed systems)
- **Vendor**: Open Source Hardware
- **Compatible with**: Any MIDI-capable application or DAW

## MIDI Message Types

The Vail Adapter responds to the following MIDI message types:

### Control Change Messages (0xBn)

#### CC0 - Mode Control
**Purpose**: Switch between Keyboard mode and MIDI mode

- **Message**: `B0 00 xx`
- **Values**:
  - `00-3F` (0-63): Enable MIDI mode (sends MIDI note events)
  - `40-7F` (64-127): Enable Keyboard mode (sends USB HID keyboard events)
- **Default**: Keyboard mode on startup
- **Example**: `B0 00 7F` enables MIDI mode

#### CC1 - Dit Duration (Speed Control)
**Purpose**: Control keying speed by setting dit duration

- **Message**: `B0 01 xx`
- **Formula**: Dit duration = `xx × 2` milliseconds
- **Range**: 2ms to 254ms (corresponds to ~600 WPM down to ~2.4 WPM)
- **Default**: 50 (100ms dit duration ≈ 12 WPM)
- **Example**: `B0 01 32` sets dit duration to 100ms (~12 WPM)

#### CC2 - Sidetone Note
**Purpose**: Set the MIDI note number for the sidetone frequency

- **Message**: `B0 02 xx`
- **Range**: 0-127 (standard MIDI note range)
- **Default**: 69 (A4 = 440Hz)
- **Notes**: Uses equal temperament tuning
- **Example**: `B0 02 45` sets sidetone to A2 (110Hz)

#### CC3 - Paddle Swap (Dit/Dah Inversion)
**Purpose**: Swap which input pad is treated as dit vs. dah. Affects both physical paddle pins (DIT_PIN / DAH_PIN) and the capacitive touch pads (QT_DIT_PIN / QT_DAH_PIN). The straight-key input (PADDLE_STRAIGHT) and the radio output pin assignments (RADIO_DIT_PIN / RADIO_DAH_PIN) are intentionally **not** affected — only the input mapping is inverted.

- **Message**: `B0 03 xx`
- **Values**:
  - `00-3F` (0-63): Set paddle swap **OFF** (normal mapping)
  - `40-7E` (64-126): Set paddle swap **ON** (dit and dah inputs swapped)
  - `7F` (127): **Toggle** the current swap state
- **Default**: OFF
- **Persistence**: Saved to EEPROM and restored on power-up
- **Audible feedback**: Whenever the state actually changes (in either direction), the adapter plays "INVERT" in Morse code on the sidetone buzzer at the user's current WPM and tone. No tone is played if the command does not change the current state.
- **Side effects on change**: Any in-flight transmission is ended and all keyboard/MIDI keys are released cleanly before the new mapping takes effect.
- **Examples**:
  - `B0 03 7F` → toggle swap (announces "INVERT" if state changed)
  - `B0 03 00` → force swap OFF
  - `B0 03 40` → force swap ON

### Program Change Messages (0xCn)

#### Keyer Mode Selection
**Purpose**: Select the keyer algorithm/behavior

- **Message**: `C0 xx`
- **Keyer Types**:
  - `00`: Passthrough (manual control - sends MIDI notes C# and D for dit/dah)
  - `01`: Straight Key (cootie)
  - `02`: Bug (semi-automatic)
  - `03`: Electric Bug
  - `04`: Single Dot
  - `05`: Ultimatic
  - `06`: Plain Iambic
  - `07`: Iambic A
  - `08`: Iambic B
  - `09`: Keyahead
- **Default**: 1 (Straight Key)
- **Invalid values**: Default to passthrough (0)

### Note On/Off Messages (0x9n/0x8n)

#### Sidetone Control
**Purpose**: Manual sidetone generation (works like a standard MIDI synthesizer)

- **Note On**: `90 xx yy` (start playing note xx at velocity yy)
- **Note Off**: `80 xx yy` (stop playing note xx)
- **Range**: 0-127 (standard MIDI notes)
- **Behavior**: Only affects built-in buzzer, not keyer logic

## Integration Example

Here's a sample sequence for an integration to configure the adapter:

```
# Set to MIDI mode
B0 00 7F

# Set medium speed (120ms dit = ~10 WPM)
B0 01 3C

# Set sidetone to middle C (261.6Hz)
B0 02 3C

# Enable Iambic B keyer mode
C0 08
```



### Implementation Notes

1. **Mode Priority**: The adapter automatically prioritizes MIDI mode when receiving MIDI commands, eliminating the need for manual mode switching
2. **Settings Persistence**: All settings (keyer type, dit duration, sidetone note) are saved to EEPROM and restored on power-up
3. **Real-time Response**: All MIDI commands take effect immediately
4. **Thread Safety**: MIDI processing is handled in the main loop with proper state management

## Technical Specifications

- **USB MIDI Class**: 1.0 compliant
- **Latency**: <1ms for MIDI command processing
- **Concurrent Notes**: Single note playback (monophonic)
- **Power Requirements**: USB bus powered
- **Operating System**: Windows, macOS, Linux (class-compliant)


