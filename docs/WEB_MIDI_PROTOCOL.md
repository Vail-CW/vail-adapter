# Web MIDI Protocol for Vail Adapter Integration

This document describes the future implementation of Web MIDI API integration for the VAIL SUMMIT web practice mode, allowing direct MIDI communication with the Vail USB adapter.

## Overview

The Web MIDI API provides a JavaScript interface for sending and receiving MIDI messages from USB MIDI devices. This allows the browser to communicate directly with the Vail adapter without relying on keyboard emulation mode.

### Why MIDI Over Keyboard Mode?

**Advantages:**
- **Sub-millisecond timing precision** - MIDI provides accurate timestamps
- **No keyboard focus required** - Works even when browser window is in background
- **Direct adapter control** - Can configure Vail adapter settings from browser
- **Native event protocol** - No key repeat filtering needed
- **Channel-aware** - Can distinguish multiple MIDI devices

**Disadvantages:**
- **Limited browser support** - Chrome, Edge, Opera, Brave only (no Firefox/Safari)
- **Requires user permission** - Browser must prompt for MIDI access
- **More complex setup** - Device enumeration and connection management

## Browser Support (2025)

- ✅ **Chrome/Chromium** - Full support
- ✅ **Edge** - Full support
- ✅ **Opera** - Full support
- ✅ **Brave** - Full support
- ❌ **Firefox** - Not supported
- ❌ **Safari** - Not supported

**Market Share:** ~70% of desktop browsers support Web MIDI API

## Vail Adapter MIDI Specification

The Vail USB adapter communicates using standard MIDI messages when in MIDI mode.

### MIDI Note Mapping

**Keying Events (Input):**
- **Note 0 (C-1)**: Straight Key
- **Note 1 (C#-1)**: Dit Paddle
- **Note 2 (D-1)**: Dah Paddle

**Message Format:**
- **Note On** (0x90 | channel): Key pressed
- **Note Off** (0x80 | channel): Key released
- **Velocity**: 0-127 (typically 64 for keying)

**Example Messages:**
```
0x90 0x01 0x40  // Dit paddle pressed (Note On, Note 1, velocity 64)
0x80 0x01 0x00  // Dit paddle released (Note Off, Note 1, velocity 0)
0x90 0x02 0x40  // Dah paddle pressed (Note On, Note 2, velocity 64)
0x80 0x02 0x00  // Dah paddle released (Note Off, Note 2, velocity 0)
```

### Configuration Messages (SysEx)

Future implementation will include System Exclusive (SysEx) messages for configuring Vail adapter settings:

**Manufacturer ID:** TBD (waiting for Vail adapter documentation)

**Configuration Parameters:**
- **Key Type**: Straight Key, Iambic A, Iambic B, Ultimatic
- **WPM Speed**: 5-60 WPM
- **Tone Frequency**: 400-1200 Hz
- **Sidetone Volume**: 0-100%
- **Dit/Dah Polarity**: Normal/Reversed

**SysEx Format (proposed):**
```
F0 <Manufacturer ID> <Command> <Parameter> <Value> F7

Examples:
F0 7D 01 00 02 F7  // Set key type to Iambic B (command=01, param=00, value=02)
F0 7D 01 01 14 F7  // Set WPM to 20 (command=01, param=01, value=0x14=20)
```

**Note:** Exact SysEx specification requires Vail adapter firmware documentation. This is a proposed format for future implementation.

## Web MIDI API Implementation

### Feature Detection

```javascript
if (navigator.requestMIDIAccess) {
  console.log('Web MIDI API supported');
  initMIDI();
} else {
  console.log('Web MIDI API not supported - falling back to keyboard mode');
  initKeyboardMode();
}
```

### Requesting MIDI Access

```javascript
async function initMIDI() {
  try {
    const midiAccess = await navigator.requestMIDIAccess();
    console.log('MIDI access granted');

    // List available inputs
    for (let input of midiAccess.inputs.values()) {
      console.log('MIDI input:', input.name, input.id);

      // Find Vail adapter
      if (input.name.includes('Vail') || input.name.includes('vail')) {
        connectVailAdapter(input);
        return;
      }
    }

    console.error('Vail adapter not found');
  } catch (err) {
    console.error('MIDI access denied:', err);
  }
}
```

### Connecting to Vail Adapter

```javascript
let vailInput = null;
let keyDownTime = { 1: null, 2: null };  // Track dit/dah timing

function connectVailAdapter(input) {
  vailInput = input;
  vailInput.onmidimessage = handleMIDIMessage;
  console.log('Connected to Vail adapter:', input.name);
}
```

### Handling MIDI Messages

```javascript
function handleMIDIMessage(event) {
  const [status, noteNumber, velocity] = event.data;
  const timestamp = event.timeStamp;  // DOMHighResTimeStamp (milliseconds)

  // Parse status byte
  const messageType = status & 0xF0;
  const channel = status & 0x0F;

  // Check for Note On/Off
  const isNoteOn = messageType === 0x90 && velocity > 0;
  const isNoteOff = messageType === 0x80 || (messageType === 0x90 && velocity === 0);

  if (noteNumber === 1) {
    // Dit paddle
    if (isNoteOn && keyDownTime[1] === null) {
      keyDownTime[1] = timestamp;
      console.log('Dit pressed at', timestamp);
    } else if (isNoteOff && keyDownTime[1] !== null) {
      const duration = timestamp - keyDownTime[1];
      console.log('Dit released, duration:', duration, 'ms');

      // Send to Summit device via WebSocket
      sendToSummit({
        type: 'timing',
        duration: duration,
        positive: true,
        key: 'dit'
      });

      keyDownTime[1] = null;
    }
  } else if (noteNumber === 2) {
    // Dah paddle
    if (isNoteOn && keyDownTime[2] === null) {
      keyDownTime[2] = timestamp;
      console.log('Dah pressed at', timestamp);
    } else if (isNoteOff && keyDownTime[2] !== null) {
      const duration = timestamp - keyDownTime[2];
      console.log('Dah released, duration:', duration, 'ms');

      sendToSummit({
        type: 'timing',
        duration: duration,
        positive: true,
        key: 'dah'
      });

      keyDownTime[2] = null;
    }
  }
}
```

### Calculating Gap Timing

For accurate decoder operation, silences (gaps) between keying elements must also be sent:

```javascript
let lastReleaseTime = null;

function handleMIDIMessage(event) {
  const [status, noteNumber, velocity] = event.data;
  const timestamp = event.timeStamp;

  const isNoteOn = (status & 0xF0) === 0x90 && velocity > 0;
  const isNoteOff = (status & 0xF0) === 0x80 || ((status & 0xF0) === 0x90 && velocity === 0);

  if (isNoteOn && lastReleaseTime !== null) {
    // Calculate gap since last key release
    const gapDuration = timestamp - lastReleaseTime;
    sendToSummit({
      type: 'timing',
      duration: gapDuration,
      positive: false,  // Silence
      key: 'gap'
    });
  }

  if (noteNumber === 1 || noteNumber === 2) {
    const keyName = noteNumber === 1 ? 'dit' : 'dah';

    if (isNoteOn && keyDownTime[noteNumber] === null) {
      keyDownTime[noteNumber] = timestamp;
    } else if (isNoteOff && keyDownTime[noteNumber] !== null) {
      const duration = timestamp - keyDownTime[noteNumber];
      sendToSummit({
        type: 'timing',
        duration: duration,
        positive: true,  // Tone
        key: keyName
      });

      keyDownTime[noteNumber] = null;
      lastReleaseTime = timestamp;
    }
  }
}
```

## Sending SysEx Configuration (Future)

```javascript
function setVailKeyerMode(mode) {
  // mode: 0=Straight, 1=Iambic A, 2=Iambic B, 3=Ultimatic
  const sysex = [
    0xF0,              // SysEx start
    0x7D,              // Manufacturer ID (proposed)
    0x01,              // Command: Set Parameter
    0x00,              // Parameter: Key Type
    mode,              // Value
    0xF7               // SysEx end
  ];

  if (vailOutput) {
    vailOutput.send(sysex);
    console.log('Sent keyer mode:', mode);
  }
}

function setVailWPM(wpm) {
  // wpm: 5-60
  const sysex = [0xF0, 0x7D, 0x01, 0x01, wpm, 0xF7];

  if (vailOutput) {
    vailOutput.send(sysex);
    console.log('Sent WPM:', wpm);
  }
}
```

**Note:** Requires MIDI Output permission and Vail adapter to support bidirectional MIDI.

## Integration with Practice Mode

### Hybrid Input Detection

The practice mode should support both keyboard and MIDI input simultaneously:

```javascript
// Try MIDI first
if (await tryInitMIDI()) {
  console.log('Using MIDI input mode');
  showMIDIStatus('Connected');
} else {
  console.log('MIDI not available, using keyboard mode');
  showMIDIStatus('Keyboard Mode (Ctrl keys)');
  initKeyboardMode();
}
```

### Settings UI

Add settings panel to practice mode page:

```html
<div class="settings-panel">
  <h3>Vail Adapter Settings</h3>

  <label>Input Mode:</label>
  <select id="inputMode" onchange="switchInputMode()">
    <option value="midi">MIDI (Recommended)</option>
    <option value="keyboard">Keyboard (Ctrl keys)</option>
  </select>

  <label>Keyer Type:</label>
  <select id="keyerType" onchange="setKeyerType()">
    <option value="0">Straight Key</option>
    <option value="1">Iambic A</option>
    <option value="2">Iambic B</option>
    <option value="3">Ultimatic</option>
  </select>

  <label>WPM: <span id="wpmValue">20</span></label>
  <input type="range" id="wpm" min="5" max="60" value="20"
         oninput="updateWPM(this.value)">
</div>
```

### Graceful Degradation

```javascript
// Feature detection and fallback
const inputModes = {
  midi: false,
  keyboard: true  // Always available
};

async function detectAvailableInputs() {
  // Check MIDI support
  if (navigator.requestMIDIAccess) {
    try {
      const access = await navigator.requestMIDIAccess();
      inputModes.midi = access.inputs.size > 0;
    } catch (err) {
      console.warn('MIDI access denied');
    }
  }

  // Update UI to show available modes
  updateInputModeSelector(inputModes);
}
```

## Performance Considerations

### Timing Precision

**Web MIDI API:**
- Timestamp resolution: `DOMHighResTimeStamp` (microsecond precision)
- Typical latency: < 1ms
- Jitter: < 0.5ms

**Keyboard Events API:**
- Timestamp resolution: `event.timeStamp` (millisecond precision)
- Typical latency: 1-4ms
- Jitter: 1-2ms

**Decoder Impact:**
- Summit's adaptive decoder has 10ms noise threshold
- Both input methods provide sufficient precision
- MIDI preferred for professional use, keyboard adequate for practice

### WebSocket Throughput

**Message Rate:**
- Typical morse: 3-10 events/second (at 20 WPM)
- Maximum rate: ~50 events/second (very fast code)
- WebSocket can handle 1000+ messages/second easily

**Bandwidth:**
- Each timing message: ~50 bytes JSON
- Typical bandwidth: < 1 KB/second
- Network latency more important than bandwidth

## Testing and Debugging

### MIDI Monitor Console

Add debug panel to practice page:

```javascript
function logMIDIEvent(event) {
  const [status, data1, data2] = event.data;
  const time = event.timeStamp.toFixed(2);

  console.log(`[${time}] Status: 0x${status.toString(16)}, ` +
              `Note: ${data1}, Velocity: ${data2}`);

  // Display in debug panel
  const debugPanel = document.getElementById('midiDebug');
  if (debugPanel) {
    const entry = document.createElement('div');
    entry.textContent = `${time}ms - ${getMIDIMessageName(status, data1, data2)}`;
    debugPanel.appendChild(entry);
    debugPanel.scrollTop = debugPanel.scrollHeight;
  }
}

function getMIDIMessageName(status, note, velocity) {
  const type = status & 0xF0;
  if (type === 0x90 && velocity > 0) {
    return `Note On: ${['Straight', 'Dit', 'Dah'][note]}`;
  } else if (type === 0x80 || (type === 0x90 && velocity === 0)) {
    return `Note Off: ${['Straight', 'Dit', 'Dah'][note]}`;
  }
  return `Unknown: 0x${status.toString(16)}`;
}
```

### Latency Measurement

```javascript
function measureLatency() {
  const startTime = performance.now();

  ws.send(JSON.stringify({ type: 'ping' }));

  ws.addEventListener('message', function handler(event) {
    const data = JSON.parse(event.data);
    if (data.type === 'pong') {
      const latency = performance.now() - startTime;
      console.log(`WebSocket round-trip latency: ${latency.toFixed(2)}ms`);
      document.getElementById('latency').textContent = `${latency.toFixed(0)}ms`;
      ws.removeEventListener('message', handler);
    }
  });
}

// Measure latency every 10 seconds
setInterval(measureLatency, 10000);
```

## Security Considerations

### MIDI Access Permission

- Browser prompts user for permission to access MIDI devices
- Permission persists per-origin (user doesn't need to approve every time)
- Users can revoke permission in browser settings
- HTTPS required for permission API to work

### Input Validation

```javascript
function validateMIDIMessage(event) {
  const [status, note, velocity] = event.data;

  // Only accept Note On/Off messages
  const type = status & 0xF0;
  if (type !== 0x80 && type !== 0x90) {
    console.warn('Ignoring non-note MIDI message');
    return false;
  }

  // Only accept notes 0-2 (Vail adapter range)
  if (note > 2) {
    console.warn('Ignoring out-of-range note:', note);
    return false;
  }

  // Validate velocity range
  if (velocity > 127) {
    console.warn('Invalid velocity:', velocity);
    return false;
  }

  return true;
}
```

## Future Roadmap

### Phase 1: Basic MIDI Input (Current Implementation)
- ✅ Keyboard mode working
- ⬜ Add MIDI detection and connection
- ⬜ Implement Note On/Off parsing
- ⬜ Test with Vail adapter

### Phase 2: Enhanced UI
- ⬜ Input mode selector (MIDI vs Keyboard)
- ⬜ Device connection indicator
- ⬜ MIDI debug panel
- ⬜ Latency monitoring

### Phase 3: Vail Configuration
- ⬜ Document Vail SysEx protocol
- ⬜ Implement keyer type switching
- ⬜ Implement WPM adjustment
- ⬜ Add tone frequency control

### Phase 4: Advanced Features
- ⬜ Session recording (store MIDI events)
- ⬜ Playback mode (replay recorded sessions)
- ⬜ Speed challenge mode (progressive WPM increase)
- ⬜ Multi-device support (practice with multiple paddles)

## References

- **Web MIDI API Specification:** https://www.w3.org/TR/webmidi/
- **MIDI 1.0 Specification:** https://www.midi.org/specifications
- **Vail Adapter Documentation:** (awaiting official documentation)
- **Browser Compatibility:** https://caniuse.com/midi

## License

This documentation is part of the VAIL SUMMIT project and follows the same license as the main project. The Web MIDI integration is proposed for future implementation and does not currently exist in the codebase.
