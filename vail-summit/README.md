# VAIL SUMMIT - Morse Code Training Device

## Project Overview

VAIL SUMMIT is a portable morse code training device built on the ESP32-S3 Feather platform. It features a modern touchscreen UI, battery monitoring, WiFi/Bluetooth connectivity, and an iambic paddle interface for morse code practice.

---

## Hardware Components

### Main Controller
- **Adafruit ESP32-S3 Feather V2**
  - 4MB Flash, 2MB PSRAM
  - Built-in battery charging (MCP73831)
  - I2C battery fuel gauge (MAX17048 at address 0x36)
  - USB Type-C connector

### Display
- **Waveshare 2" LCD (240x320)**
  - Controller: ST7789V
  - Interface: SPI
  - Running in landscape mode (320x240)

### Input Devices
- **M5Stack CardKB**
  - Interface: I2C (address 0x5F)
  - Mini QWERTY keyboard with arrow keys
  - Connected via STEMMA QT connector

- **Iambic Paddle**
  - Dual-lever morse code key
  - Connected via 3.5mm TRS jack
  - Dit (tip) and Dah (ring) inputs

- **Capacitive Touch Pads**
  - Built-in touch-sensitive key
  - Two touch points for dit and dah
  - Uses ESP32-S3 internal capacitive sensing
  - No external components required (optional 1MΩ pulldown resistors recommended)

### Output Devices

- **Radio Keying Output**
  - 3.5mm TRS jack for keying external ham radios
  - Digital outputs for dit and dah keying
  - Tip = Dit, Ring = Dah, Sleeve = GND
  - Requires transistor/relay driver circuit for radio compatibility
  - Compatible with external keyers, transmitters, and transceivers

### Audio Output
- **MAX98357A I2S Class-D Amplifier**
  - 3W mono amplifier
  - High-quality digital audio via I2S interface
  - Software volume control (0-100%)
  - Speaker and headphone output with automatic switching
  - Multiple tone frequencies for different UI events

### Power
- **350mAh LiPo Battery**
  - Rechargeable via USB-C
  - Monitored by MAX17048 fuel gauge chip

---

## Pin Assignments

### LCD Display (SPI)
```
TFT_CS      = 10    // Chip Select
TFT_DC      = 11    // Data/Command
TFT_RST     = 12    // Reset
TFT_BL      = N/A   // Backlight (hardwired to 3.3V for always-on)
TFT_MOSI    = 35    // SPI Data (hardware SPI)
TFT_SCK     = 36    // SPI Clock (hardware SPI)
```

### CardKB Keyboard (I2C)
```
I2C_SDA     = 3     // I2C Data (STEMMA QT)
I2C_SCL     = 4     // I2C Clock (STEMMA QT)
CARDKB_ADDR = 0x5F  // I2C Address
```

### Iambic Paddle (Digital Input)
```
DIT_PIN     = 6     // Dit paddle (active LOW with pullup)
DAH_PIN     = 9     // Dah paddle (active LOW with pullup)
```

### Capacitive Touch Pads (Built-in Key)
```
TOUCH_DIT_PIN   = 5     // T5 - Capacitive touch dit pad
TOUCH_DAH_PIN   = 13    // T13 - Capacitive touch dah pad
TOUCH_THRESHOLD = 40    // Touch sensitivity (20-80 typical range)
```

### Radio Keying Output (Digital Output)
```
RADIO_KEY_DIT_PIN = 18  // A0 - Dit output for keying ham radio
RADIO_KEY_DAH_PIN = 17  // A1 - Dah output for keying ham radio
                        // Tip = Dit, Ring = Dah, Sleeve = GND
```

### Audio (MAX98357A I2S Amplifier)
```
I2S_BCK_PIN  = 14   // I2S Bit Clock (BCLK)
I2S_LCK_PIN  = 15   // I2S Left/Right Clock (LRC/WS)
I2S_DATA_PIN = 16   // I2S Data Output (DIN)
```
Note: Legacy BUZZER_PIN (GPIO 5) has been repurposed for capacitive touch.

**MAX98357A Connections:**
- BCLK → GPIO 14
- LRC → GPIO 15
- DIN → GPIO 16
- VIN → 3.3V
- GND → GND
- GAIN → Float (9dB default) or GND (12dB) or VIN (6dB)
- SD → Float (always on)

**Speaker/Headphone Output:**
- Use switched 3.5mm stereo jack (PJ-307 or similar)
- 100Ω resistor in series with headphone output for volume limiting
- Speaker connects via normally-closed contacts (auto-mutes when headphones plugged in)

### Battery Monitoring
```
MAX17048 I2C Address: 0x36  // Battery fuel gauge
```
Note: USB_DETECT_PIN (A3/GPIO 15) disabled - conflicts with I2S_LCK_PIN.

---

## I2C Device Map

| Address | Device          | Purpose                           |
|---------|-----------------|-----------------------------------|
| 0x36    | MAX17048        | Battery fuel gauge (voltage & %)  |
| 0x5F    | CardKB          | I2C keyboard                      |

---

## Software Architecture

### Libraries Used
- `Adafruit_GFX` - Graphics primitives
- `Adafruit_ST7789` - Display driver
- `Adafruit_MAX1704X` - Battery monitoring
- `Wire` - I2C communication
- `WiFi` - WiFi functionality
- `WiFiClientSecure` - Secure WiFi connections
- `SPI` - SPI communication
- `Preferences` - Non-volatile storage
- `WebSockets` by Markus Sattler - WebSocket client
- `ArduinoJson` by Benoit Blanchon - JSON parsing

### Key Features Implemented

#### 1. Menu System
- **Modern carousel/stack UI design**
- Four menu options:
  1. Training (submenu with Hear It Type It, Practice)
  2. Settings (submenu with WiFi Setup, CW Settings)
  3. WiFi (Vail CW Repeater)
  4. Bluetooth (coming soon)
- Arrow key navigation (up/down)
- Enter key to select
- ESC key to go back
- Visual feedback with card-based layout
- Multi-level menu navigation

#### 2. Status Bar
- **WiFi indicator**: Green (connected) / Red (disconnected)
- **Battery indicator**:
  - Color-coded by charge level (green >60%, yellow >20%, red ≤20%)
  - Shows charge percentage as fill level
  - Updates every 5 seconds
  - Note: USB charging detection disabled due to I2S pin conflict

#### 3. Battery Management
- **MAX17048 fuel gauge integration**
  - Accurate voltage reading (0.01V precision)
  - State of charge percentage
  - Auto-calibrates over charge cycles

#### 4. Display Management
- **Backlight**: Hardwired to 3.3V (always on at full brightness)
- **Landscape orientation** (320x240)
- **Custom fonts**: FreeSansBold12pt7b, FreeSans9pt7b

#### 5. Audio Feedback
Different tones for different actions:
```
TONE_SIDETONE   = 700 Hz   // Morse code audio
TONE_MENU_NAV   = 800 Hz   // Menu navigation
TONE_SELECT     = 1200 Hz  // Selection confirmation
TONE_ERROR      = 400 Hz   // Error/invalid action
TONE_STARTUP    = 1000 Hz  // Device startup
```

### CardKB Key Mapping
```
KEY_UP        = 0xB5  // Navigate up in menu
KEY_DOWN      = 0xB6  // Navigate down in menu
KEY_LEFT      = 0xB4  // Left arrow
KEY_RIGHT     = 0xB7  // Right arrow
KEY_ENTER     = 0x0D  // Select menu item
KEY_ENTER_ALT = 0x0A  // Alternate enter
KEY_BACKSPACE = 0x08  // Backspace (Fn+X)
KEY_ESC       = 0x1B  // Escape (Fn+Z)
KEY_TAB       = 0x09  // Tab (Fn+Space)
```

---

## UI Design

### Color Scheme
```
Background:      Black
Title:           Cyan
Text:            White
Highlight BG:    Blue
Highlight FG:    White
Success:         Green
Error:           Red
Warning:         Yellow
```

### Layout
```
┌─────────────────────────────────────┐
│ VAIL SUMMIT          [WiFi] [Batt]  │ ← Header (Cyan title + status icons)
├─────────────────────────────────────┤
│                                     │
│      ┌────────────┐                 │
│      │  ○ T       │  ← Stack card   │
│      └────────────┘                 │
│   ┌─────────────────┐               │
│   │   ◉ Training    │  ← Main card  │
│   └─────────────────┘               │
│      ┌────────────┐                 │
│      │  ○ S       │  ← Stack card   │
│      └────────────┘                 │
│                                     │
├─────────────────────────────────────┤
│     ↑/↓ Navigate  ENTER Select      │ ← Footer help text
└─────────────────────────────────────┘
```

### Menu Cards
- **Main card**: Selected item, large with full text
- **Stack cards**: Adjacent items above/below, abbreviated with icon only
- Smooth scrolling feel through card positioning

---

## Configuration

All hardware settings are centralized in `config.h`:
- Pin definitions
- Display settings
- I2C addresses
- Key codes
- Audio tone frequencies
- Morse code timing (WPM settings)
- UI color scheme
- Menu layout constants

---

## Development Progress

### ✅ Completed Features

#### Core System
- [x] Basic menu navigation system
- [x] Landscape display orientation (320x240)
- [x] Modern carousel UI design with card-based layout
- [x] WiFi status indicator
- [x] Battery voltage and percentage monitoring
- [x] USB/charging detection via analog pin
- [x] PWM backlight control
- [x] I2C battery fuel gauge integration (MAX17048)
- [x] Status icon updates (every 5 seconds)
- [x] Audio feedback for navigation
- [x] Modular configuration system (config.h)
- [x] Multi-level menu system with mode switching
- [x] Deep sleep power management with wake-on-paddle

#### Morse Code Engine
- [x] Complete morse code lookup table (A-Z, 0-9, punctuation)
- [x] WPM-based timing calculations (PARIS method)
- [x] Morse code playback through buzzer
- [x] Variable speed support (5-40 WPM)

#### Training Modes
- [x] **"Hear It Type It" Training Mode**
  - Random US ham radio callsign generator (format: `^[AKNW][A-Z]{0,2}[0-9][A-Z]{1,3}$`)
  - Variable speed practice (12-20 WPM, randomized per callsign)
  - Real-time text input with modern UI
  - Optimized input box rendering (only redraws box on keypress)
  - Correct/incorrect feedback with visual and audio cues
  - Replay function (ESC key)
  - Skip function (TAB key)
  - Attempt counter
  - Serial debug output for troubleshooting

- [x] **Practice Oscillator Mode**
  - Free-form morse code practice with paddle/key
  - Uses configurable CW settings (speed, tone, key type)
  - Real-time visual feedback showing paddle state
  - Supports straight key, Iambic A, and Iambic B modes
  - Proper inter-element spacing
  - Local sidetone feedback

#### Settings
- [x] **WiFi Setup**
  - Scan for available WiFi networks
  - Connect to selected network with password
  - Save WiFi credentials to flash memory
  - Auto-connect on startup
  - Display signal strength and encryption status

- [x] **CW Settings**
  - Adjustable speed (5-40 WPM)
  - Adjustable tone frequency (400-1200 Hz in 50 Hz steps)
  - Tone preview: hear frequency as you adjust it
  - Key type selection (Straight Key, Iambic A, Iambic B)
  - Settings saved to flash memory
  - Persistent across reboots
  - Modern card-based UI with rounded corners

- [x] **Volume Control**
  - Software volume control (0-100%)
  - Adjustable with up/down arrows in 5% increments
  - Visual volume bar with color coding (red/yellow/green)
  - Settings saved to flash memory
  - Persistent across reboots
  - Real-time preview when adjusting

#### Connectivity
- [x] **Vail Chat - Internet CW Repeater**
  - WebSocket connection to vail.woozle.org
  - Real-time morse code transmission to internet
  - Receive and playback morse code from other operators
  - JSON protocol with clock synchronization
  - Default channel: "General"
  - Live channel switching (General, 1-10) with up/down arrows
  - Live speed adjustment (5-40 WPM) with left/right arrows
  - Echo filtering (don't play back own transmissions)
  - Non-blocking playback state machine
  - Immediate transmission (sends each tone as it's generated)
  - Accurate timing with tone start timestamps
  - Modern UI showing channel, status, speed, and operator count

### 🚧 Pending Features
- [ ] Additional training modes (Koch method, character drills, etc.)
- [ ] Bluetooth connectivity
- [ ] Character recognition/decoding (decode mode)
- [ ] Progress tracking and statistics
- [ ] Volume control

---

## Power Management

### Deep Sleep Mode

**How to enter sleep:**
1. From the main menu, press ESC three times quickly (within 2 seconds)
2. Device will beep and show "Going to Sleep..." message
3. Screen turns off, device enters ultra-low power mode (~20µA)

**How to wake:**
- Press the DIT paddle
- Device performs full restart from setup()

**Battery Life Estimates (350mAh battery):**
- Active Vail Chat (WiFi): 1.5-2 hours
- Practice Mode (no WiFi): 2-2.5 hours
- Deep Sleep: ~750 hours (31 days)

**With 18650 battery (3000mAh):**
- Active Vail Chat (WiFi): 10-15 hours
- Practice Mode (no WiFi): 20-23 hours
- Deep Sleep: ~6400 hours (267 days / 8.9 months)

---

## Known Issues & Notes

### Battery Calibration
The MAX17048 fuel gauge requires several charge/discharge cycles to accurately learn battery capacity. Initial readings may show lower percentages (e.g., 90% for a fully charged battery at 4.11V). This is normal and will improve with use.

### CardKB LED Flash
The CardKB keyboard flashes a white LED 3 times during navigation input. This is a hardware feature and cannot be disabled in software.

### Pin Repurposing Notes
**GPIO 5**: Originally assigned to BUZZER_PIN (legacy PWM buzzer), now repurposed for capacitive touch dit pad since I2S audio replaced the PWM buzzer.

**GPIO 13**: Originally assigned to TFT_BL (backlight PWM control), now repurposed for capacitive touch dah pad. Display backlight is hardwired to 3.3V for always-on operation.

**I2S Audio Pin Conflict**: Pin A3 (GPIO 15 on ESP32-S3 Feather) is used by I2S for the LRC clock signal. Using `analogRead(A3)` will reconfigure the pin as an analog input and completely break I2S audio, causing severe distortion. USB charging detection has been disabled to prevent this conflict.

---

## Morse Code Timing

Uses the standard **PARIS method**:
- 50 dit units per word
- Dit duration (ms) = 1200 / WPM
- Default: 20 WPM
- Range: 5-40 WPM

**Timing relationships:**
- Dah = 3 × dit
- Inter-element gap = 1 × dit
- Letter gap = 3 × dit
- Word gap = 7 × dit

---

## Serial Debug Output

Baud rate: **115200**

Example output:
```
=== VAIL SUMMIT STARTING ===
Initializing backlight...
Backlight ON
Initializing display...
Display initialized
Initializing I2C...
Initializing battery monitor...
Found MAX17048 with Chip ID: 0x12
Battery: 4.11V, 90%
USB detected (ADC: 2845)
Setup complete!
```

---

## Build Instructions

### Required Arduino Libraries
1. Adafruit GFX Library
2. Adafruit ST7735 and ST7789 Library
3. Adafruit MAX1704X
4. Adafruit LC709203F (backup battery monitor support)
5. WebSockets by Markus Sattler
6. ArduinoJson by Benoit Blanchon

### Board Configuration
- Board: **ESP32S3 Dev Module** or **Adafruit Feather ESP32-S3**
- USB CDC On Boot: **Enabled**
- Flash Size: **4MB**
- PSRAM: **OPI PSRAM**
- Upload Speed: **921600**

### Compilation
1. Install required libraries via Arduino Library Manager
2. Select correct board and port
3. Upload `morse_trainer_menu.ino`

---

## File Structure

```
Project Jupiter/
├── morse_trainer_menu/
│   ├── morse_trainer_menu.ino        # Main program with menu system
│   ├── config.h                      # Hardware configuration
│   ├── morse_code.h                  # Morse code engine and lookup tables
│   ├── training_hear_it_type_it.h    # "Hear It Type It" training mode
│   ├── training_practice.h           # Practice oscillator mode
│   ├── settings_wifi.h               # WiFi configuration and management
│   ├── settings_cw.h                 # CW settings (speed, tone, key type)
│   └── vail_repeater.h               # Vail CW repeater WebSocket client
├── vail_web_repeater/                # Cloned Vail repeater source (reference)
├── ESP32-S3 Project Hardware Documentation.pdf
└── README.md                         # This file
```

---

## Training Modes

### Hear It Type It

**Purpose**: Practice receiving morse code by listening to random callsigns and typing what you hear.

**How it works:**
1. Navigate to Training → Hear It Type It from main menu
2. A random US ham radio callsign is generated (e.g., W4ABC, KA2XYZ)
3. The callsign is sent via morse code through the buzzer at a random speed (12-20 WPM)
4. Type what you heard using the CardKB keyboard
5. Press ENTER to submit your answer
6. Receive immediate feedback (CORRECT/INCORRECT)
7. If incorrect, the callsign replays for another attempt
8. If correct, a new callsign is automatically generated

**Controls:**
- **Type A-Z, 0-9**: Enter characters (auto-uppercase)
- **BACKSPACE**: Delete last character
- **ENTER**: Submit answer
- **ESC**: Replay current callsign
- **TAB**: Skip to next callsign

**Features:**
- Callsigns follow official US format: `^[AKNW][A-Z]{0,2}[0-9][A-Z]{1,3}$`
- Speed varies randomly (12-20 WPM) but remains constant per callsign
- Attempt counter shows how many tries on current callsign
- Modern UI with large input box and blinking cursor
- Optimized rendering (only input box redraws during typing)
- Debug output to serial monitor shows answer (for practice/troubleshooting)

**UI Elements:**
```
┌─────────────────────────────────────┐
│ TRAINING             [WiFi] [Batt]  │
├─────────────────────────────────────┤
│        HEAR IT TYPE IT              │
│             15 WPM                  │
│                                     │
│      Type what you heard:           │
│   ┌─────────────────────────────┐   │
│   │  W4ABC_                     │   │ ← Input box
│   └─────────────────────────────┘   │
│                                     │
│         Attempt 2                   │
├─────────────────────────────────────┤
│ ENTER Submit  ESC Replay  TAB Skip  │
└─────────────────────────────────────┘
```

---

## Practice Mode

**Purpose**: Free-form morse code practice using your iambic paddle or straight key.

**How it works:**
1. Navigate to Training → Practice from main menu
2. Use your paddle/key to send morse code
3. Hear local sidetone through the buzzer
4. Practice at your configured speed and tone settings

**Features:**
- Uses saved CW settings (Settings → CW Settings)
- Real-time visual feedback (green circle lights up when keying)
- Supports three key types:
  - **Straight Key**: Simple on/off keying using DIT pin
  - **Iambic A**: Memory mode with manual alternation
  - **Iambic B**: Automatic alternation with squeeze keying
- Proper timing: 1 dit inter-element spacing
- ESC to exit back to Training menu

---

## Vail Chat (Internet CW Repeater)

**Purpose**: Practice morse code with other operators around the world via the internet.

**How it works:**
1. Connect to WiFi (Settings → WiFi Setup)
2. Navigate to WiFi from main menu
3. Automatically connects to Vail "General" channel
4. Use your paddle to transmit - other operators hear you in real-time
5. Hear other operators' transmissions through your buzzer
6. Change channels and speed on-the-fly with arrow keys

**Features:**
- Secure WebSocket connection (WSS) to vail.woozle.org
- Real-time bidirectional morse code
- Clock synchronization with server
- 500ms playback delay to handle network jitter
- Immediate transmission (each tone sent as it's generated for real-time performance)
- Accurate timing using tone start timestamps (matches web client behavior)
- Echo filtering (your own transmissions aren't played back)
- Shows connection status, current channel, speed, and operator count
- Non-blocking playback (can transmit while receiving)
- Live channel switching (General, 1-10)
- Live speed adjustment (5-40 WPM)
- Modern UI with rounded card design

**Controls:**
- **↑/↓ Arrows**: Change channel (General → 1 → 2 → ... → 10 → General)
- **←/→ Arrows**: Adjust speed (5-40 WPM)
- **Paddle**: Transmit morse code
- **ESC**: Disconnect and exit

**UI Display:**
```
┌─────────────────────────────────────┐
│ VAIL CHAT            [WiFi] [Batt]  │
├─────────────────────────────────────┤
│  ╔═══════════════════════════════╗  │
│  ║ Channel                       ║  │
│  ║ General                       ║  │
│  ║                               ║  │
│  ║ Status                        ║  │
│  ║ Connected                     ║  │
│  ║                               ║  │
│  ║ Speed      15 WPM    Ops   2  ║  │
│  ╚═══════════════════════════════╝  │
│                                     │
│  Use paddle to transmit             │
├─────────────────────────────────────┤
│  ↑↓ Chan  ←→ Spd  ESC Exit          │
└─────────────────────────────────────┘
```

**Protocol:**
- Based on the Vail protocol (https://github.com/Vail-CW/vail_web_repeater)
- JSON format over WebSocket with `json.vail.woozle.org` subprotocol
- Transmission example: `{"Timestamp":1759710473428,"Clients":0,"Duration":[198]}`
  - Timestamp: Unix epoch milliseconds (when tone started)
  - Clients: Number of connected operators (0 when sending, server fills in)
  - Duration: Array containing tone duration in milliseconds
  - Each tone sent immediately as a separate message
  - Silences are implicit (gaps between tones)
- Reception messages contain alternating tone/silence arrays
  - Even indices (0, 2, 4...) = tone durations
  - Odd indices (1, 3, 5...) = silence durations

---

## Future Enhancements

### Training Features
- Koch method training
- Farnsworth spacing
- Random character drills
- Word practice
- QSO simulation
- Speed ramping

### UI Improvements
- Settings persistence (EEPROM/preferences)
- Brightness control
- Volume control
- Theme selection

### Connectivity
- WiFi-based morse practice (internet)
- Bluetooth keyboard mode
- Web-based configuration interface
- OTA firmware updates

### Power Management
- Auto-sleep timer
- Deep sleep with wake-on-key
- Battery life optimization

---

## Credits

**Author**: Brett Hollifield KE9BOS
**Email**: ke9bos@pigletradio.org

---

## License

This project is for educational and personal use.

---

*Last Updated: 2025-10-05*
