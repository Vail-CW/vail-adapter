/*
 * HID Key Code Translation
 * Converts USB HID key codes to ASCII/CardKB format
 * Used by BLE keyboard host to translate external keyboard input
 */

#ifndef HID_KEYCODES_H
#define HID_KEYCODES_H

#include <Arduino.h>
#include "../core/config.h"

// HID Keyboard modifier bits (byte 0 of report)
#define HID_MOD_LCTRL   0x01
#define HID_MOD_LSHIFT  0x02
#define HID_MOD_LALT    0x04
#define HID_MOD_LGUI    0x08
#define HID_MOD_RCTRL   0x10
#define HID_MOD_RSHIFT  0x20
#define HID_MOD_RALT    0x40
#define HID_MOD_RGUI    0x80

// Combined shift mask
#define HID_MOD_SHIFT   (HID_MOD_LSHIFT | HID_MOD_RSHIFT)

// HID Key codes (standard USB HID usage table)
#define HID_KEY_A       0x04
#define HID_KEY_B       0x05
#define HID_KEY_C       0x06
#define HID_KEY_D       0x07
#define HID_KEY_E       0x08
#define HID_KEY_F       0x09
#define HID_KEY_G       0x0A
#define HID_KEY_H       0x0B
#define HID_KEY_I       0x0C
#define HID_KEY_J       0x0D
#define HID_KEY_K       0x0E
#define HID_KEY_L       0x0F
#define HID_KEY_M       0x10
#define HID_KEY_N       0x11
#define HID_KEY_O       0x12
#define HID_KEY_P       0x13
#define HID_KEY_Q       0x14
#define HID_KEY_R       0x15
#define HID_KEY_S       0x16
#define HID_KEY_T       0x17
#define HID_KEY_U       0x18
#define HID_KEY_V       0x19
#define HID_KEY_W       0x1A
#define HID_KEY_X       0x1B
#define HID_KEY_Y       0x1C
#define HID_KEY_Z       0x1D
#define HID_KEY_1       0x1E
#define HID_KEY_2       0x1F
#define HID_KEY_3       0x20
#define HID_KEY_4       0x21
#define HID_KEY_5       0x22
#define HID_KEY_6       0x23
#define HID_KEY_7       0x24
#define HID_KEY_8       0x25
#define HID_KEY_9       0x26
#define HID_KEY_0       0x27
#define HID_KEY_ENTER   0x28
#define HID_KEY_ESC     0x29
#define HID_KEY_BACKSPACE 0x2A
#define HID_KEY_TAB     0x2B
#define HID_KEY_SPACE   0x2C
#define HID_KEY_MINUS   0x2D
#define HID_KEY_EQUAL   0x2E
#define HID_KEY_LBRACKET 0x2F
#define HID_KEY_RBRACKET 0x30
#define HID_KEY_BACKSLASH 0x31
#define HID_KEY_SEMICOLON 0x33
#define HID_KEY_QUOTE   0x34
#define HID_KEY_GRAVE   0x35
#define HID_KEY_COMMA   0x36
#define HID_KEY_PERIOD  0x37
#define HID_KEY_SLASH   0x38
#define HID_KEY_CAPSLOCK 0x39
#define HID_KEY_F1      0x3A
#define HID_KEY_F2      0x3B
#define HID_KEY_F3      0x3C
#define HID_KEY_F4      0x3D
#define HID_KEY_F5      0x3E
#define HID_KEY_F6      0x3F
#define HID_KEY_F7      0x40
#define HID_KEY_F8      0x41
#define HID_KEY_F9      0x42
#define HID_KEY_F10     0x43
#define HID_KEY_F11     0x44
#define HID_KEY_F12     0x45
#define HID_KEY_INSERT  0x49
#define HID_KEY_HOME    0x4A
#define HID_KEY_PAGEUP  0x4B
#define HID_KEY_DELETE  0x4C
#define HID_KEY_END     0x4D
#define HID_KEY_PAGEDOWN 0x4E
#define HID_KEY_RIGHT   0x4F
#define HID_KEY_LEFT    0x50
#define HID_KEY_DOWN    0x51
#define HID_KEY_UP      0x52

// HID to ASCII lookup table (unshifted)
// Index = HID key code, Value = ASCII character (0 = no mapping)
static const char hidToAsciiUnshifted[128] = {
  0, 0, 0, 0,                     // 0x00-0x03: Reserved
  'a', 'b', 'c', 'd',             // 0x04-0x07: A-D
  'e', 'f', 'g', 'h',             // 0x08-0x0B: E-H
  'i', 'j', 'k', 'l',             // 0x0C-0x0F: I-L
  'm', 'n', 'o', 'p',             // 0x10-0x13: M-P
  'q', 'r', 's', 't',             // 0x14-0x17: Q-T
  'u', 'v', 'w', 'x',             // 0x18-0x1B: U-X
  'y', 'z',                       // 0x1C-0x1D: Y-Z
  '1', '2', '3', '4',             // 0x1E-0x21: 1-4
  '5', '6', '7', '8',             // 0x22-0x25: 5-8
  '9', '0',                       // 0x26-0x27: 9-0
  KEY_ENTER,                      // 0x28: Enter -> 0x0D
  KEY_ESC,                        // 0x29: Escape -> 0x1B
  KEY_BACKSPACE,                  // 0x2A: Backspace -> 0x08
  KEY_TAB,                        // 0x2B: Tab -> 0x09
  ' ',                            // 0x2C: Space
  '-', '=', '[', ']',             // 0x2D-0x30: - = [ ]
  '\\',                           // 0x31: Backslash
  0,                              // 0x32: Non-US # (not used)
  ';', '\'', '`',                 // 0x33-0x35: ; ' `
  ',', '.', '/',                  // 0x36-0x38: , . /
  0,                              // 0x39: Caps Lock (no output)
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 0x3A-0x43: F1-F10 (no ASCII)
  0, 0,                           // 0x44-0x45: F11-F12 (no ASCII)
  0, 0, 0,                        // 0x46-0x48: PrintScreen, ScrollLock, Pause
  0,                              // 0x49: Insert
  0,                              // 0x4A: Home
  0,                              // 0x4B: Page Up
  KEY_BACKSPACE,                  // 0x4C: Delete -> Backspace (for convenience)
  0,                              // 0x4D: End
  0,                              // 0x4E: Page Down
  KEY_RIGHT,                      // 0x4F: Right Arrow -> 0xB7
  KEY_LEFT,                       // 0x50: Left Arrow -> 0xB4
  KEY_DOWN,                       // 0x51: Down Arrow -> 0xB6
  KEY_UP,                         // 0x52: Up Arrow -> 0xB5
  // 0x53-0x7F: Numpad and other keys (not mapped)
};

// HID to ASCII lookup table (shifted)
static const char hidToAsciiShifted[128] = {
  0, 0, 0, 0,                     // 0x00-0x03: Reserved
  'A', 'B', 'C', 'D',             // 0x04-0x07: A-D
  'E', 'F', 'G', 'H',             // 0x08-0x0B: E-H
  'I', 'J', 'K', 'L',             // 0x0C-0x0F: I-L
  'M', 'N', 'O', 'P',             // 0x10-0x13: M-P
  'Q', 'R', 'S', 'T',             // 0x14-0x17: Q-T
  'U', 'V', 'W', 'X',             // 0x18-0x1B: U-X
  'Y', 'Z',                       // 0x1C-0x1D: Y-Z
  '!', '@', '#', '$',             // 0x1E-0x21: Shift+1-4
  '%', '^', '&', '*',             // 0x22-0x25: Shift+5-8
  '(', ')',                       // 0x26-0x27: Shift+9-0
  KEY_ENTER,                      // 0x28: Enter (same)
  KEY_ESC,                        // 0x29: Escape (same)
  KEY_BACKSPACE,                  // 0x2A: Backspace (same)
  KEY_TAB,                        // 0x2B: Tab (same)
  ' ',                            // 0x2C: Space (same)
  '_', '+', '{', '}',             // 0x2D-0x30: Shift+- = [ ]
  '|',                            // 0x31: Shift+Backslash
  0,                              // 0x32: Non-US # (not used)
  ':', '"', '~',                  // 0x33-0x35: Shift+; ' `
  '<', '>', '?',                  // 0x36-0x38: Shift+, . /
  0,                              // 0x39: Caps Lock (no output)
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 0x3A-0x43: F1-F10 (no ASCII)
  0, 0,                           // 0x44-0x45: F11-F12 (no ASCII)
  0, 0, 0,                        // 0x46-0x48: PrintScreen, ScrollLock, Pause
  0,                              // 0x49: Insert
  0,                              // 0x4A: Home
  0,                              // 0x4B: Page Up
  KEY_BACKSPACE,                  // 0x4C: Delete -> Backspace
  0,                              // 0x4D: End
  0,                              // 0x4E: Page Down
  KEY_RIGHT,                      // 0x4F: Right Arrow (same)
  KEY_LEFT,                       // 0x50: Left Arrow (same)
  KEY_DOWN,                       // 0x51: Down Arrow (same)
  KEY_UP,                         // 0x52: Up Arrow (same)
  // 0x53-0x7F: Numpad and other keys (not mapped)
};

/**
 * Convert HID key code to ASCII/CardKB character
 * @param hidKeyCode The USB HID key code (0x04-0x52 typical)
 * @param modifiers The modifier byte from HID report
 * @return ASCII character or CardKB special key code, 0 if no mapping
 */
char hidKeyCodeToChar(uint8_t hidKeyCode, uint8_t modifiers) {
  // Validate key code range
  if (hidKeyCode >= 128) {
    return 0;
  }

  // Check if shift is pressed
  bool shifted = (modifiers & HID_MOD_SHIFT) != 0;

  // Look up in appropriate table
  if (shifted) {
    return hidToAsciiShifted[hidKeyCode];
  } else {
    return hidToAsciiUnshifted[hidKeyCode];
  }
}

/**
 * Check if a HID key code is a printable character
 * @param hidKeyCode The USB HID key code
 * @return true if key produces printable output
 */
bool isHidKeyPrintable(uint8_t hidKeyCode) {
  // Letters (A-Z)
  if (hidKeyCode >= HID_KEY_A && hidKeyCode <= HID_KEY_Z) return true;
  // Numbers (1-0)
  if (hidKeyCode >= HID_KEY_1 && hidKeyCode <= HID_KEY_0) return true;
  // Space and punctuation
  if (hidKeyCode >= HID_KEY_SPACE && hidKeyCode <= HID_KEY_SLASH) return true;

  return false;
}

/**
 * Check if a HID key code is a navigation key
 * @param hidKeyCode The USB HID key code
 * @return true if key is arrow, enter, escape, etc.
 */
bool isHidKeyNavigation(uint8_t hidKeyCode) {
  switch (hidKeyCode) {
    case HID_KEY_ENTER:
    case HID_KEY_ESC:
    case HID_KEY_BACKSPACE:
    case HID_KEY_TAB:
    case HID_KEY_RIGHT:
    case HID_KEY_LEFT:
    case HID_KEY_DOWN:
    case HID_KEY_UP:
    case HID_KEY_DELETE:
      return true;
    default:
      return false;
  }
}

#endif // HID_KEYCODES_H
