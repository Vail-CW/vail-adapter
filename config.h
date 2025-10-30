/*
 * Morse Code Trainer - Hardware Configuration
 * ESP32-S3 Feather Pin Definitions and Settings
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// Firmware Version Information
// ============================================
#define FIRMWARE_VERSION "0.1"
#define FIRMWARE_DATE "2025-10-30"  // Update this date each time you build new firmware
#define FIRMWARE_NAME "VAIL SUMMIT"

// ============================================
// LCD Display (ST7789V) - SPI Interface
// ============================================
#define TFT_CS      10    // Chip Select
#define TFT_DC      11    // Data/Command
#define TFT_RST     12    // Reset
// #define TFT_BL      13    // Backlight (hardwired to 3.3V - GPIO 13 now used for capacitive touch)
#define TFT_MOSI    35    // SPI Data (hardware SPI)
#define TFT_SCK     36    // SPI Clock (hardware SPI)

// Display Settings
#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240
#define SCREEN_ROTATION 1     // 0=Portrait, 1=Landscape, 2=Portrait flipped, 3=Landscape flipped

// ============================================
// CardKB Keyboard - I2C Interface
// ============================================
#define CARDKB_ADDR 0x5F  // I2C Address
#define I2C_SDA     3     // I2C Data (STEMMA QT)
#define I2C_SCL     4     // I2C Clock (STEMMA QT)

// CardKB Special Key Codes
#define KEY_UP      0xB5  // Up arrow
#define KEY_DOWN    0xB6  // Down arrow
#define KEY_LEFT    0xB4  // Left arrow
#define KEY_RIGHT   0xB7  // Right arrow
#define KEY_ENTER   0x0D  // Enter/Return
#define KEY_ENTER_ALT 0x0A // Alternate Enter
#define KEY_BACKSPACE 0x08 // Backspace (Fn+X)
#define KEY_ESC     0x1B  // ESC (Fn+Z)
#define KEY_TAB     0x09  // Tab (Fn+Space)

// ============================================
// Buzzer - PWM Output (Legacy - now using I2S amplifier)
// ============================================
// #define BUZZER_PIN  5     // PWM output for buzzer (deprecated - GPIO 5 now used for capacitive touch)

// ============================================
// I2S Audio - MAX98357A Class-D Amplifier
// ============================================
#define I2S_BCK_PIN     14    // I2S Bit Clock (BCLK)
#define I2S_LCK_PIN     15    // I2S Left/Right Clock (LRC/WS)
#define I2S_DATA_PIN    16    // I2S Data Output (DIN)
// MAX98357A GAIN pin: Float for 9dB (default), GND for 12dB, VIN for 6dB
// SD (shutdown) pin: Leave floating for always-on

// Audio Settings
#define TONE_SIDETONE   700   // Hz - Morse code audio tone
#define TONE_MENU_NAV   800   // Hz - Menu navigation beep
#define TONE_SELECT     1200  // Hz - Selection confirmation
#define TONE_ERROR      400   // Hz - Error/invalid beep
#define TONE_STARTUP    1000  // Hz - Startup beep

#define BEEP_SHORT      30    // ms - Short beep duration
#define BEEP_MEDIUM     100   // ms - Medium beep duration
#define BEEP_LONG       200   // ms - Long beep duration

// I2S Audio Configuration
#define I2S_SAMPLE_RATE 44100 // 44.1kHz sample rate
#define I2S_BUFFER_SIZE 256   // Total samples (128 stereo pairs = 256 int16 values)

// Volume Control
#define DEFAULT_VOLUME  50    // Default volume (0-100%)
#define VOLUME_MIN      0     // Minimum volume
#define VOLUME_MAX      100   // Maximum volume

// ============================================
// Iambic Paddle Key - Digital Inputs
// ============================================
#define DIT_PIN     6     // Dit paddle (tip on 3.5mm jack)
#define DAH_PIN     9     // Dah paddle (ring on 3.5mm jack)
                          // Sleeve = GND

// Paddle Settings
#define PADDLE_ACTIVE   LOW   // Paddles are active LOW (pullup enabled)

// ============================================
// Capacitive Touch Pads - Built-in Key
// ============================================
// IMPORTANT: ESP32-S3 touchRead() requires GPIO numbers (not T-constants!)
// GPIO 8 and 5 work together; GPIO 13 conflicts with I2S/touch shield
#define TOUCH_DIT_PIN   8     // GPIO 8 (T8) - Capacitive touch dit pad
#define TOUCH_DAH_PIN   5     // GPIO 5 (T5) - Capacitive touch dah pad
#define TOUCH_THRESHOLD 40000 // Touch threshold (values rise when touched on ESP32-S3)

// ============================================
// Radio Keying Output - 3.5mm Jack
// ============================================
#define RADIO_KEY_DIT_PIN   18    // A0 - Dit output for keying ham radio
#define RADIO_KEY_DAH_PIN   17    // A1 - Dah output for keying ham radio
                                   // Tip = Dit, Ring = Dah, Sleeve = GND

// ============================================
// Battery Monitoring
// ============================================
// ESP32-S3 Feather V2: Uses MAX17048 I2C fuel gauge at 0x36
// USB detection - DISABLED because A3 conflicts with I2S_LCK_PIN (GPIO 15)
// #define USB_DETECT_PIN  A3    // CONFLICT WITH I2S! Do not use!

// Battery voltage thresholds (for LiPo)
#define VBAT_FULL   4.2   // Fully charged voltage
#define VBAT_EMPTY  3.3   // Empty voltage (cutoff)

// ============================================
// Morse Code Timing Settings
// ============================================
#define DEFAULT_WPM     20    // Words per minute
#define WPM_MIN         5     // Minimum WPM
#define WPM_MAX         40    // Maximum WPM

// Calculate dit duration in milliseconds from WPM
// Standard: PARIS method (50 dit units per word)
#define DIT_DURATION(wpm) (1200 / wpm)

// ============================================
// Serial Debug
// ============================================
#define SERIAL_BAUD 115200
#define DEBUG_ENABLED true

// ============================================
// UI Color Scheme
// ============================================
#define COLOR_BACKGROUND    ST77XX_BLACK
#define COLOR_TITLE         ST77XX_CYAN
#define COLOR_TEXT          ST77XX_WHITE
#define COLOR_HIGHLIGHT_BG  ST77XX_BLUE
#define COLOR_HIGHLIGHT_FG  ST77XX_WHITE
#define COLOR_SUCCESS       ST77XX_GREEN
#define COLOR_ERROR         ST77XX_RED
#define COLOR_WARNING       ST77XX_YELLOW
#define COLOR_SEPARATOR     ST77XX_WHITE

// ============================================
// Menu Configuration
// ============================================
#define MENU_ITEMS      6
#define MENU_START_Y    55
#define MENU_ITEM_HEIGHT 35
#define MENU_TEXT_SIZE  2

#endif // CONFIG_H
