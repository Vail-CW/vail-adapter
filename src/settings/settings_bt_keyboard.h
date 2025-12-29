/*
 * Bluetooth Keyboard Settings Module
 * Handles BLE keyboard scanning, pairing, and connection management
 */

#ifndef SETTINGS_BT_KEYBOARD_H
#define SETTINGS_BT_KEYBOARD_H

#include "../core/config.h"
#include "../bluetooth/ble_keyboard_host.h"

// Bluetooth keyboard settings state machine
enum BTKeyboardSettingsState {
  BTKB_UI_STATUS,         // Show current status and paired device
  BTKB_UI_SCANNING,       // Actively scanning for keyboards
  BTKB_UI_DEVICE_LIST,    // Show list of found devices
  BTKB_UI_CONNECTING,     // Connecting to selected device
  BTKB_UI_FORGET_CONFIRM  // Confirm forget pairing
};

// UI state
BTKeyboardSettingsState btkbUIState = BTKB_UI_STATUS;
int btkbSelectedDevice = 0;
unsigned long btkbLastUIUpdate = 0;
bool btkbForgetConfirm = false;

// LVGL mode flag - when true, skip legacy draw functions (LVGL handles display)
bool btKeyboardUseLVGL = true;  // Default to LVGL mode

// Forward declarations
void startBTKeyboardSettings(LGFX &display);
void drawBTKeyboardSettingsUI(LGFX &display);
int handleBTKeyboardSettingsInput(char key, LGFX &display);
void drawBTKBStatusScreen(LGFX &display);
void drawBTKBScanningScreen(LGFX &display);
void drawBTKBDeviceList(LGFX &display);
void drawBTKBConnectingScreen(LGFX &display);
void drawBTKBForgetConfirm(LGFX &display);
void updateBTKeyboardSettingsUI(LGFX &display);

// Start Bluetooth keyboard settings mode
void startBTKeyboardSettings(LGFX &display) {
  Serial.println("Starting BT Keyboard Settings");

  btkbSelectedDevice = 0;
  btkbForgetConfirm = false;
  btkbLastUIUpdate = millis();

  // Initialize BLE keyboard host if not already
  if (bleKBHost.state == BLEKB_STATE_IDLE) {
    loadBLEKeyboardSettings();
    initBLEKeyboardHost();
  }

  // Determine initial UI state based on BLE state
  if (bleKBHost.state == BLEKB_STATE_CONNECTED) {
    btkbUIState = BTKB_UI_STATUS;
  } else if (bleKBHost.state == BLEKB_STATE_SCANNING) {
    btkbUIState = BTKB_UI_SCANNING;
  } else if (bleKBHost.state == BLEKB_STATE_SCAN_COMPLETE && bleKBHost.foundCount > 0) {
    btkbUIState = BTKB_UI_DEVICE_LIST;
  } else {
    btkbUIState = BTKB_UI_STATUS;
  }

  display.fillScreen(COLOR_BACKGROUND);

  // Draw header
  display.setFont(&FreeSansBold12pt7b);
  display.setTextSize(1);
  display.setTextColor(COLOR_TITLE);
  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(display, "BLUETOOTH KEYBOARD", 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, 30);
  display.print("BLUETOOTH KEYBOARD");

  drawBTKeyboardSettingsUI(display);
}

// Main UI draw function
void drawBTKeyboardSettingsUI(LGFX &display) {
  if (btKeyboardUseLVGL) return;  // LVGL handles display
  // Clear content area (preserve header)
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  switch (btkbUIState) {
    case BTKB_UI_STATUS:
      drawBTKBStatusScreen(display);
      break;
    case BTKB_UI_SCANNING:
      drawBTKBScanningScreen(display);
      break;
    case BTKB_UI_DEVICE_LIST:
      drawBTKBDeviceList(display);
      break;
    case BTKB_UI_CONNECTING:
      drawBTKBConnectingScreen(display);
      break;
    case BTKB_UI_FORGET_CONFIRM:
      drawBTKBForgetConfirm(display);
      break;
  }
}

// Draw status screen (main screen)
void drawBTKBStatusScreen(LGFX &display) {
  if (btKeyboardUseLVGL) return;  // LVGL handles display
  // Status card
  int cardX = 20;
  int cardY = 55;
  int cardW = SCREEN_WIDTH - 40;
  int cardH = 120;

  display.fillRoundRect(cardX, cardY, cardW, cardH, 12, 0x1082);
  display.drawRoundRect(cardX, cardY, cardW, cardH, 12, 0x34BF);

  // Connection status
  display.setFont(&FreeSansBold12pt7b);
  display.setTextSize(1);
  int yPos = cardY + 30;
  display.setCursor(cardX + 15, yPos);

  if (isBLEKeyboardConnected()) {
    display.setTextColor(ST77XX_GREEN);
    display.print("Connected");
  } else if (bleKBHost.state == BLEKB_STATE_CONNECTING) {
    display.setTextColor(ST77XX_YELLOW);
    display.print("Connecting...");
  } else if (bleKBHost.pairedDevice.valid) {
    display.setTextColor(ST77XX_YELLOW);
    display.print("Disconnected");
  } else {
    display.setTextColor(0x7BEF);  // Gray
    display.print("No Keyboard Paired");
  }

  // Paired device info
  display.setFont(nullptr);
  display.setTextSize(1);
  display.setTextColor(0x7BEF);  // Light gray
  yPos += 25;
  display.setCursor(cardX + 15, yPos);
  display.print("Paired Device:");

  display.setTextSize(2);
  display.setTextColor(ST77XX_CYAN);
  yPos += 18;
  display.setCursor(cardX + 15, yPos);

  if (bleKBHost.pairedDevice.valid) {
    String name = bleKBHost.pairedDevice.name;
    if (name.length() > 20) {
      name = name.substring(0, 17) + "...";
    }
    display.print(name);
  } else {
    display.setTextColor(0x7BEF);
    display.print("None");
  }

  // Auto-reconnect status
  display.setTextSize(1);
  display.setTextColor(0x7BEF);
  yPos += 30;
  display.setCursor(cardX + 15, yPos);
  display.print("Auto-Reconnect: ");
  display.setTextColor(bleKBHost.autoReconnect ? ST77XX_GREEN : ST77XX_RED);
  display.print(bleKBHost.autoReconnect ? "ON" : "OFF");

  // Instructions card
  cardY = 190;
  cardH = 70;
  display.fillRoundRect(cardX, cardY, cardW, cardH, 12, 0x1082);
  display.drawRoundRect(cardX, cardY, cardW, cardH, 12, 0x34BF);

  display.setFont(nullptr);
  display.setTextSize(1);
  display.setTextColor(ST77XX_WHITE);
  yPos = cardY + 15;
  display.setCursor(cardX + 15, yPos);
  display.print("S = Scan for keyboards");

  yPos += 18;
  display.setCursor(cardX + 15, yPos);
  if (bleKBHost.pairedDevice.valid) {
    display.print("F = Forget pairing");
  } else {
    display.setTextColor(0x5AEB);  // Dimmed
    display.print("F = Forget pairing (N/A)");
  }

  yPos += 18;
  display.setCursor(cardX + 15, yPos);
  display.setTextColor(ST77XX_WHITE);
  display.print("A = Toggle auto-reconnect");

  // Footer
  display.setTextSize(1);
  display.setTextColor(ST77XX_YELLOW);
  int16_t x1, y1;
  uint16_t w, h;
  String footerText = "ESC Back";
  getTextBounds_compat(display, footerText.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT - 12);
  display.print(footerText);
}

// Draw scanning screen
void drawBTKBScanningScreen(LGFX &display) {
  if (btKeyboardUseLVGL) return;  // LVGL handles display
  // Scanning card
  int cardX = 40;
  int cardY = 90;
  int cardW = SCREEN_WIDTH - 80;
  int cardH = 100;

  display.fillRoundRect(cardX, cardY, cardW, cardH, 12, 0x1082);
  display.drawRoundRect(cardX, cardY, cardW, cardH, 12, 0x34BF);

  // Scanning text
  display.setFont(&FreeSansBold12pt7b);
  display.setTextSize(1);
  display.setTextColor(ST77XX_CYAN);

  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(display, "Scanning...", 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, cardY + 40);
  display.print("Scanning...");

  // Animated dots based on time
  int dots = ((millis() / 500) % 4);
  display.setFont(nullptr);
  display.setTextSize(2);
  display.setTextColor(ST77XX_WHITE);
  centerX = (SCREEN_WIDTH - 40) / 2;
  display.setCursor(centerX, cardY + 65);
  for (int i = 0; i < dots; i++) {
    display.print(".");
  }

  // Found count
  display.setTextSize(1);
  display.setTextColor(0x7BEF);
  String foundText = "Found: " + String(bleKBHost.foundCount) + " devices";
  getTextBounds_compat(display, foundText.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, cardY + 85);
  display.print(foundText);

  // Footer
  display.setTextColor(ST77XX_YELLOW);
  String footerText = "Press ESC to cancel";
  getTextBounds_compat(display, footerText.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT - 12);
  display.print(footerText);
}

// Draw device list
void drawBTKBDeviceList(LGFX &display) {
  if (btKeyboardUseLVGL) return;  // LVGL handles display
  if (bleKBHost.foundCount == 0) {
    // No devices found
    display.setFont(&FreeSansBold12pt7b);
    display.setTextSize(1);
    display.setTextColor(ST77XX_RED);

    int16_t x1, y1;
    uint16_t w, h;
    getTextBounds_compat(display, "No Keyboards Found", 0, 0, &x1, &y1, &w, &h);
    int centerX = (SCREEN_WIDTH - w) / 2;
    display.setCursor(centerX, 120);
    display.print("No Keyboards Found");

    display.setFont(nullptr);
    display.setTextSize(1);
    display.setTextColor(0x7BEF);
    getTextBounds_compat(display, "Put keyboard in pairing mode", 0, 0, &x1, &y1, &w, &h);
    centerX = (SCREEN_WIDTH - w) / 2;
    display.setCursor(centerX, 150);
    display.print("Put keyboard in pairing mode");

    display.setTextColor(ST77XX_YELLOW);
    String footerText = "S = Scan again   ESC = Back";
    getTextBounds_compat(display, footerText.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT - 12);
    display.print(footerText);
    return;
  }

  // Header
  display.setFont(nullptr);
  display.setTextSize(1);
  display.setTextColor(0x7BEF);
  display.setCursor(20, 50);
  display.print("Select a keyboard to pair:");

  // Device list
  int startY = 70;
  int itemHeight = 35;
  int maxVisible = 5;  // Show up to 5 devices

  // Calculate scroll offset
  int scrollOffset = 0;
  if (btkbSelectedDevice >= maxVisible) {
    scrollOffset = btkbSelectedDevice - maxVisible + 1;
  }

  for (int i = 0; i < min(bleKBHost.foundCount, maxVisible); i++) {
    int deviceIndex = i + scrollOffset;
    if (deviceIndex >= bleKBHost.foundCount) break;

    int itemY = startY + i * itemHeight;
    bool isSelected = (deviceIndex == btkbSelectedDevice);

    // Selection highlight
    if (isSelected) {
      display.fillRoundRect(15, itemY, SCREEN_WIDTH - 30, itemHeight - 2, 8, 0x249F);
    } else {
      display.fillRoundRect(15, itemY, SCREEN_WIDTH - 30, itemHeight - 2, 8, 0x1082);
    }
    display.drawRoundRect(15, itemY, SCREEN_WIDTH - 30, itemHeight - 2, 8, 0x34BF);

    // Device name
    display.setTextSize(2);
    display.setTextColor(isSelected ? ST77XX_WHITE : ST77XX_CYAN);
    display.setCursor(25, itemY + 8);

    String name = bleKBHost.foundDevices[deviceIndex].name;
    if (name.length() > 22) {
      name = name.substring(0, 19) + "...";
    }
    display.print(name);

    // RSSI indicator
    display.setTextSize(1);
    display.setTextColor(0x7BEF);

    int rssi = bleKBHost.foundDevices[deviceIndex].rssi;
    int bars = map(rssi, -100, -40, 1, 4);
    bars = constrain(bars, 1, 4);

    // Draw signal bars
    int barX = SCREEN_WIDTH - 55;
    for (int b = 0; b < 4; b++) {
      int barHeight = (b + 1) * 4;
      int bX = barX + b * 6;
      if (b < bars) {
        display.fillRect(bX, itemY + 25 - barHeight, 5, barHeight, ST77XX_GREEN);
      } else {
        display.drawRect(bX, itemY + 25 - barHeight, 5, barHeight, 0x4208);
      }
    }
  }

  // Scroll indicators
  if (scrollOffset > 0) {
    display.setTextColor(ST77XX_CYAN);
    display.setCursor(SCREEN_WIDTH / 2 - 5, startY - 12);
    display.print("\x18");  // Up arrow
  }
  if (scrollOffset + maxVisible < bleKBHost.foundCount) {
    display.setTextColor(ST77XX_CYAN);
    display.setCursor(SCREEN_WIDTH / 2 - 5, startY + maxVisible * itemHeight - 5);
    display.print("\x19");  // Down arrow
  }

  // Footer
  display.setTextSize(1);
  display.setTextColor(ST77XX_YELLOW);
  int16_t x1, y1;
  uint16_t w, h;
  String footerText = "\x18\x19 Select   ENTER Connect   ESC Back";
  getTextBounds_compat(display, footerText.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT - 12);
  display.print(footerText);
}

// Draw connecting screen
void drawBTKBConnectingScreen(LGFX &display) {
  if (btKeyboardUseLVGL) return;  // LVGL handles display
  int cardX = 40;
  int cardY = 80;
  int cardW = SCREEN_WIDTH - 80;
  int cardH = 120;

  display.fillRoundRect(cardX, cardY, cardW, cardH, 12, 0x1082);
  display.drawRoundRect(cardX, cardY, cardW, cardH, 12, 0x34BF);

  // Connecting text
  display.setFont(&FreeSansBold12pt7b);
  display.setTextSize(1);
  display.setTextColor(ST77XX_CYAN);

  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(display, "Connecting...", 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, cardY + 35);
  display.print("Connecting...");

  // Device name
  display.setFont(nullptr);
  display.setTextSize(2);
  display.setTextColor(ST77XX_WHITE);

  String name = "Unknown";
  if (btkbSelectedDevice >= 0 && btkbSelectedDevice < bleKBHost.foundCount) {
    name = bleKBHost.foundDevices[btkbSelectedDevice].name;
    if (name.length() > 18) {
      name = name.substring(0, 15) + "...";
    }
  }
  getTextBounds_compat(display, name.c_str(), 0, 0, &x1, &y1, &w, &h);
  centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, cardY + 70);
  display.print(name);

  // Animated dots
  int dots = ((millis() / 500) % 4);
  display.setTextSize(1);
  centerX = (SCREEN_WIDTH - 40) / 2;
  display.setCursor(centerX, cardY + 100);
  for (int i = 0; i < dots; i++) {
    display.print(".");
  }
}

// Draw forget confirmation
void drawBTKBForgetConfirm(LGFX &display) {
  if (btKeyboardUseLVGL) return;  // LVGL handles display
  int cardX = 30;
  int cardY = 80;
  int cardW = SCREEN_WIDTH - 60;
  int cardH = 120;

  display.fillRoundRect(cardX, cardY, cardW, cardH, 12, 0x1082);
  display.drawRoundRect(cardX, cardY, cardW, cardH, 12, ST77XX_RED);

  // Title
  display.setFont(&FreeSansBold12pt7b);
  display.setTextSize(1);
  display.setTextColor(ST77XX_RED);

  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(display, "Forget Pairing?", 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, cardY + 35);
  display.print("Forget Pairing?");

  // Device name
  display.setFont(nullptr);
  display.setTextSize(2);
  display.setTextColor(ST77XX_WHITE);

  String name = bleKBHost.pairedDevice.name;
  if (name.length() > 18) {
    name = name.substring(0, 15) + "...";
  }
  getTextBounds_compat(display, name.c_str(), 0, 0, &x1, &y1, &w, &h);
  centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, cardY + 70);
  display.print(name);

  // Buttons
  display.setTextSize(1);
  display.setTextColor(ST77XX_YELLOW);
  String footerText = "Y = Yes, Forget   N/ESC = Cancel";
  getTextBounds_compat(display, footerText.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT - 12);
  display.print(footerText);
}

// Update UI (call periodically for animations and state changes)
void updateBTKeyboardSettingsUI(LGFX &display) {
  // Update BLE keyboard host
  updateBLEKeyboardHost();

  // Check for state changes
  static BLEKBHostState lastBLEState = BLEKB_STATE_IDLE;
  bool stateChanged = (bleKBHost.state != lastBLEState);
  lastBLEState = bleKBHost.state;

  // Handle BLE state changes
  if (stateChanged) {
    switch (bleKBHost.state) {
      case BLEKB_STATE_SCAN_COMPLETE:
        if (btkbUIState == BTKB_UI_SCANNING) {
          if (bleKBHost.foundCount > 0) {
            btkbUIState = BTKB_UI_DEVICE_LIST;
            btkbSelectedDevice = 0;
          } else {
            btkbUIState = BTKB_UI_DEVICE_LIST;  // Show "no devices" message
          }
          drawBTKeyboardSettingsUI(display);
        }
        break;

      case BLEKB_STATE_CONNECTED:
        if (btkbUIState == BTKB_UI_CONNECTING) {
          btkbUIState = BTKB_UI_STATUS;
          drawBTKeyboardSettingsUI(display);
          beep(TONE_SELECT, BEEP_MEDIUM);  // Success feedback
        }
        break;

      case BLEKB_STATE_ERROR:
      case BLEKB_STATE_DISCONNECTED:
        if (btkbUIState == BTKB_UI_CONNECTING) {
          // Connection failed, go back to device list
          btkbUIState = BTKB_UI_DEVICE_LIST;
          drawBTKeyboardSettingsUI(display);
          beep(TONE_ERROR, 300);  // Error feedback
        }
        break;

      default:
        break;
    }
  }

  // Periodic UI updates for animations
  unsigned long now = millis();
  if (now - btkbLastUIUpdate >= 200) {
    btkbLastUIUpdate = now;

    if (btkbUIState == BTKB_UI_SCANNING || btkbUIState == BTKB_UI_CONNECTING) {
      // Redraw for animation updates
      drawBTKeyboardSettingsUI(display);
    }
  }
}

// Handle input
int handleBTKeyboardSettingsInput(char key, LGFX &display) {
  // Update UI first (handles BLE state changes)
  updateBTKeyboardSettingsUI(display);

  switch (btkbUIState) {
    case BTKB_UI_STATUS:
      if (key == 'S' || key == 's') {
        // Start scanning
        btkbUIState = BTKB_UI_SCANNING;
        drawBTKeyboardSettingsUI(display);
        startBLEKeyboardScan();
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 0;
      }
      if ((key == 'F' || key == 'f') && bleKBHost.pairedDevice.valid) {
        // Forget pairing
        btkbUIState = BTKB_UI_FORGET_CONFIRM;
        drawBTKeyboardSettingsUI(display);
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 0;
      }
      if (key == 'A' || key == 'a') {
        // Toggle auto-reconnect
        bleKBHost.autoReconnect = !bleKBHost.autoReconnect;
        saveBLEKeyboardSettings();
        drawBTKeyboardSettingsUI(display);
        beep(TONE_SELECT, BEEP_SHORT);
        return 0;
      }
      if (key == KEY_ESC) {
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return -1;  // Exit
      }
      break;

    case BTKB_UI_SCANNING:
      if (key == KEY_ESC) {
        // Cancel scan
        stopBLEKeyboardScan();
        btkbUIState = BTKB_UI_STATUS;
        drawBTKeyboardSettingsUI(display);
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 0;
      }
      break;

    case BTKB_UI_DEVICE_LIST:
      if (key == KEY_UP) {
        if (btkbSelectedDevice > 0) {
          btkbSelectedDevice--;
          drawBTKeyboardSettingsUI(display);
          beep(TONE_MENU_NAV, BEEP_SHORT);
        }
        return 0;
      }
      if (key == KEY_DOWN) {
        if (btkbSelectedDevice < bleKBHost.foundCount - 1) {
          btkbSelectedDevice++;
          drawBTKeyboardSettingsUI(display);
          beep(TONE_MENU_NAV, BEEP_SHORT);
        }
        return 0;
      }
      if (key == KEY_ENTER || key == KEY_ENTER_ALT || key == KEY_RIGHT) {
        if (bleKBHost.foundCount > 0 && btkbSelectedDevice < bleKBHost.foundCount) {
          // Connect to selected device
          btkbUIState = BTKB_UI_CONNECTING;
          drawBTKeyboardSettingsUI(display);
          beep(TONE_SELECT, BEEP_MEDIUM);

          // Start connection (non-blocking)
          connectToBLEKeyboard(btkbSelectedDevice);
        }
        return 0;
      }
      if (key == 'S' || key == 's') {
        // Re-scan
        btkbUIState = BTKB_UI_SCANNING;
        drawBTKeyboardSettingsUI(display);
        startBLEKeyboardScan();
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 0;
      }
      if (key == KEY_ESC) {
        btkbUIState = BTKB_UI_STATUS;
        drawBTKeyboardSettingsUI(display);
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 0;
      }
      break;

    case BTKB_UI_CONNECTING:
      if (key == KEY_ESC) {
        // Cancel connection (disconnect if in progress)
        disconnectBLEKeyboard();
        btkbUIState = BTKB_UI_DEVICE_LIST;
        drawBTKeyboardSettingsUI(display);
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 0;
      }
      break;

    case BTKB_UI_FORGET_CONFIRM:
      if (key == 'Y' || key == 'y') {
        // Confirm forget
        forgetBLEKeyboardPairing();
        btkbUIState = BTKB_UI_STATUS;
        drawBTKeyboardSettingsUI(display);
        beep(TONE_SELECT, BEEP_MEDIUM);
        return 0;
      }
      if (key == 'N' || key == 'n' || key == KEY_ESC) {
        // Cancel forget
        btkbUIState = BTKB_UI_STATUS;
        drawBTKeyboardSettingsUI(display);
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 0;
      }
      break;
  }

  return 0;
}

#endif // SETTINGS_BT_KEYBOARD_H
