/*
 * Morse Code Trainer - Hardware Configuration
 * ESP32-S3 Feather Pin Definitions and Settings
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// Firmware Version Information
// ============================================
#define FIRMWARE_VERSION "0.27"
#define FIRMWARE_DATE "2026-01-04"  // Update this date each time you build new firmware
#define FIRMWARE_NAME "VAIL SUMMIT"

// ============================================
// LCD Display (ST7796S 4.0") - SPI Interface
// ============================================
#define TFT_CS      10    // Chip Select
#define TFT_RST     11    // Hardware Reset
#define TFT_DC      12    // Data/Command
#define TFT_MOSI    35    // SPI Data Out (hardware SPI)
#define TFT_SCK     36    // SPI Clock (hardware SPI)
#define TFT_MISO    37    // SPI Data In (required for ST7796S)
#define TFT_BL      39    // Backlight control (GPIO 39)

// Display Settings
#define SCREEN_WIDTH    480
#define SCREEN_HEIGHT   320
#define SCREEN_ROTATION 3     // 0=Portrait, 1=Landscape, 2=Portrait flipped, 3=Landscape flipped

// ============================================
// SD Card - SPI Interface (shares SPI bus with display)
// ============================================
#define SD_CS       38    // SD Card Chip Select
#define SD_MOSI     35    // SPI Data Out (shared with display)
#define SD_SCK      36    // SPI Clock (shared with display)
#define SD_MISO     37    // SPI Data In (shared with display)
// Note: SD card shares hardware SPI bus with display
// Only CS pin is unique; MOSI/SCK/MISO are shared with TFT

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
#define TONE_SUCCESS    1000  // Hz - Success confirmation
#define TONE_ERROR      400   // Hz - Error/invalid beep
#define TONE_STARTUP    1000  // Hz - Startup beep

#define BEEP_SHORT      30    // ms - Short beep duration
#define BEEP_MEDIUM     100   // ms - Medium beep duration
#define BEEP_LONG       200   // ms - Long beep duration

// I2S Audio Configuration
#define I2S_SAMPLE_RATE 22050 // 22.05kHz sample rate (reduced for less I2S bus noise)
#define I2S_BUFFER_SIZE 256   // Total samples (128 stereo pairs = 256 int16 values)

// Volume Control
#define DEFAULT_VOLUME  50    // Default volume (0-100%)
#define VOLUME_MIN      0     // Minimum volume
#define VOLUME_MAX      100   // Maximum volume

// Brightness Control (PWM for LCD backlight)
#define DEFAULT_BRIGHTNESS      100   // Default brightness (0-100%)
#define BRIGHTNESS_MIN          10    // Minimum brightness (prevent completely dark)
#define BRIGHTNESS_MAX          100   // Maximum brightness
#define BRIGHTNESS_PWM_CHANNEL  0     // ESP32 LEDC channel for backlight
#define BRIGHTNESS_PWM_FREQ     5000  // PWM frequency in Hz
#define BRIGHTNESS_PWM_RESOLUTION 8   // 8-bit resolution (0-255)

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
// Web Files Download Configuration
// ============================================
// GitHub raw URL for downloading web interface files to SD card
// Update OWNER/REPO/BRANCH when repository is public
#define WEB_FILES_BASE_URL "https://raw.githubusercontent.com/OWNER/REPO/BRANCH/web/www/"
#define WEB_FILES_MANIFEST "manifest.json"
#define WEB_FILES_PATH "/www/"  // SD card path for web files

// ============================================
// Color Definitions (LovyanGFX compatible - 16-bit RGB565)
// ============================================
// LovyanGFX uses different color names than Adafruit, so define ST77XX_* constants
#define ST77XX_BLACK       0x0000  // Black
#define ST77XX_WHITE       0xFFFF  // White
#define ST77XX_RED         0xF800  // Red
#define ST77XX_GREEN       0x07E0  // Green
#define ST77XX_BLUE        0x001F  // Blue
#define ST77XX_CYAN        0x07FF  // Cyan
#define ST77XX_MAGENTA     0xF81F  // Magenta
#define ST77XX_YELLOW      0xFFE0  // Yellow
#define ST77XX_ORANGE      0xFC00  // Orange

// ============================================
// UI Color Scheme - Modern Minimal Design
// ============================================

// Background & Base Colors (Dark, Subtle)
#define COLOR_BG_DEEP           0x0841  // RGB(15, 20, 35) - Deep dark background
#define COLOR_BG_LAYER2         0x1082  // RGB(20, 30, 50) - Subtle raised layer

// Card & Surface Colors (Pastel Blue-Green)
#define COLOR_CARD_BLUE         0x32AB  // RGB(50, 85, 90) - Soft muted blue
#define COLOR_CARD_TEAL         0x2B0E  // RGB(40, 95, 115) - Soft teal
#define COLOR_CARD_CYAN         0x3471  // RGB(50, 140, 140) - Muted cyan
#define COLOR_CARD_MINT         0x3D53  // RGB(60, 170, 155) - Soft mint

// Accent Colors (Subtle Highlights)
#define COLOR_ACCENT_BLUE       0x4D5F  // RGB(75, 170, 255) - Soft bright blue
#define COLOR_ACCENT_CYAN       0x5EB9  // RGB(90, 215, 210) - Soft cyan accent
#define COLOR_ACCENT_GREEN      0x5712  // RGB(85, 225, 150) - Soft green accent

// Status Colors (Pastels)
#define COLOR_SUCCESS_PASTEL    0x4E91  // RGB(75, 210, 140) - Soft pastel green
#define COLOR_WARNING_PASTEL    0xFE27  // RGB(255, 195, 60) - Soft pastel orange
#define COLOR_ERROR_PASTEL      0xFACB  // RGB(250, 90, 90) - Soft pastel red

// Border & Outline Colors
#define COLOR_BORDER_SUBTLE     0x39EC  // RGB(55, 65, 100) - Very subtle border
#define COLOR_BORDER_LIGHT      0x632C  // RGB(100, 100, 100) - Light neutral border
#define COLOR_BORDER_ACCENT     0x6D7F  // RGB(110, 170, 255) - Soft blue border

// Text Hierarchy Colors
#define COLOR_TEXT_PRIMARY      0xFFFF  // RGB(255, 255, 255) - White (main text)
#define COLOR_TEXT_SECONDARY    0xBDF7  // RGB(190, 190, 190) - Light gray (labels)
#define COLOR_TEXT_TERTIARY     0x8410  // RGB(130, 130, 130) - Medium gray (hints)
#define COLOR_TEXT_DISABLED     0x5ACB  // RGB(90, 90, 90) - Dark gray (inactive)

// Legacy Color Aliases (for backward compatibility during transition)
#define COLOR_BACKGROUND    COLOR_BG_DEEP
#define COLOR_TITLE         COLOR_ACCENT_CYAN
#define COLOR_TEXT          COLOR_TEXT_PRIMARY
#define COLOR_HIGHLIGHT_BG  COLOR_CARD_CYAN
#define COLOR_HIGHLIGHT_FG  COLOR_TEXT_PRIMARY
#define COLOR_SUCCESS       COLOR_SUCCESS_PASTEL
#define COLOR_ERROR         COLOR_ERROR_PASTEL
#define COLOR_WARNING       COLOR_WARNING_PASTEL
#define COLOR_SEPARATOR     COLOR_BORDER_SUBTLE

// ============================================
// UI Layout Constants (scaled for 480×320 display)
// ============================================
#define HEADER_HEIGHT       45    // Top status bar height (reduced for better proportions)
#define FOOTER_HEIGHT       30    // Bottom instruction bar height
#define CARD_MAIN_WIDTH     450   // Main menu card width
#define CARD_MAIN_HEIGHT    80    // Main menu card height
#define CARD_STACK_WIDTH_1  405   // First stacked card width
#define CARD_STACK_WIDTH_2  375   // Second stacked card width
#define ICON_RADIUS         30    // Menu card icon circle radius
#define STATUS_ICON_SIZE    36    // Status bar icon size (battery, WiFi)
#define GROUND_Y            300   // Ground position for Morse Shooter game

// ============================================
// Menu Configuration
// ============================================
// MENU_ITEMS is deprecated - use MAIN_MENU_ITEMS from menu_ui.h instead
// Keeping for backwards compatibility during transition
#define MENU_ITEMS      4
#define MENU_START_Y    55
#define MENU_ITEM_HEIGHT 35
#define MENU_TEXT_SIZE  3     // Increased from 2 for better readability on larger display

// ============================================
// LovyanGFX Compatibility Helpers
// ============================================
// LovyanGFX doesn't have getTextBounds() like Adafruit_GFX
// This inline function provides compatibility by using textWidth() and fontHeight()
template<typename T>
inline void getTextBounds_compat(T& display, const char* text, int16_t x, int16_t y,
                                  int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
  *w = display.textWidth(text);
  *h = display.fontHeight();
  *x1 = x;
  *y1 = y - *h;  // LovyanGFX draws from baseline, Adafruit from top-left
}

#endif // CONFIG_H
