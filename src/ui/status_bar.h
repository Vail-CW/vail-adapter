/*
 * Status Bar Module
 * Handles battery and WiFi status monitoring and display
 */

#ifndef STATUS_BAR_H
#define STATUS_BAR_H

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <Adafruit_LC709203F.h>
#include <Adafruit_MAX1704X.h>
#include <WiFi.h>
#include "../core/config.h"
#include "../network/internet_check.h"

// Forward declarations from main file
extern LGFX tft;
extern Adafruit_LC709203F lc;
extern Adafruit_MAX17048 maxlipo;
extern bool hasLC709203;
extern bool hasMAX17048;
extern bool hasBatteryMonitor;

// Status tracking
bool wifiConnected = false;
int batteryPercent = 100;
bool isCharging = false;

/*
 * Draw battery icon with charge level and charging indicator (clean minimal style)
 */
void drawBatteryIcon(int x, int y) {
  // Clean battery outline (smaller, 24x12)
  int battW = 24;
  int battH = 12;

  tft.drawRoundRect(x, y, battW, battH, 2, COLOR_BORDER_LIGHT);

  // Battery nub (small terminal)
  tft.fillRect(x + battW, y + 4, 2, 4, COLOR_BORDER_LIGHT);

  // Determine fill color based on level
  uint16_t fillColor;
  if (batteryPercent > 60) {
    fillColor = COLOR_SUCCESS_PASTEL;  // Soft green for high
  } else if (batteryPercent > 20) {
    fillColor = COLOR_ACCENT_CYAN;     // Soft cyan for medium
  } else {
    fillColor = COLOR_ERROR_PASTEL;    // Soft red for low
  }

  // Solid fill based on percentage
  int fillWidth = (batteryPercent * (battW - 4)) / 100;

  if (DEBUG_ENABLED) {
    Serial.print("Drawing battery: ");
    Serial.print(batteryPercent);
    Serial.print("% fillWidth=");
    Serial.print(fillWidth);
    Serial.print(" charging=");
    Serial.println(isCharging ? "YES" : "NO");
  }

  // Solid color fill (no banding)
  if (fillWidth > 0) {
    tft.fillRect(x + 2, y + 2, fillWidth, battH - 4, fillColor);
  }

  // Simple charging indicator
  if (isCharging) {
    // Small white lightning bolt
    tft.fillTriangle(x + battW/2 + 2, y + 2, x + battW/2 - 1, y + battH/2, x + battW/2 + 1, y + battH/2, ST77XX_WHITE);
    tft.fillTriangle(x + battW/2 - 1, y + battH/2, x + battW/2 + 2, y + battH - 2, x + battW/2, y + battH/2, ST77XX_WHITE);
  }
}

/*
 * Draw WiFi icon with signal strength bars (clean minimal style)
 * Color indicates connectivity state:
 *   - Cyan: Full internet connectivity
 *   - Orange: WiFi connected but no internet
 *   - Gray: Disconnected
 */
void drawWiFiIcon(int x, int y) {
  // Determine color based on internet connectivity state
  uint16_t barColor;
  InternetStatus inetStatus = getInternetStatus();

  if (inetStatus == INET_CONNECTED) {
    barColor = COLOR_ACCENT_CYAN;      // Full connectivity
  } else if (inetStatus == INET_WIFI_ONLY) {
    barColor = COLOR_WARNING_PASTEL;   // WiFi but no internet (orange)
  } else {
    barColor = COLOR_TEXT_DISABLED;    // Disconnected (gray)
  }

  // Draw 4 signal bars (increasing height)
  tft.fillRect(x, y + 8, 3, 4, barColor);
  tft.fillRect(x + 5, y + 5, 3, 7, barColor);
  tft.fillRect(x + 10, y + 2, 3, 10, barColor);
  tft.fillRect(x + 15, y, 3, 12, barColor);
}

/*
 * Draw all status icons (WiFi and battery) - clean minimal layout
 */
void drawStatusIcons() {
  int iconX = SCREEN_WIDTH - 10; // Start from right edge
  int iconY = (HEADER_HEIGHT - 12) / 2; // Vertically center 12px icons

  // Draw battery icon (24px wide + 2px nub = 26px total)
  iconX -= 28;
  drawBatteryIcon(iconX, iconY);

  // Draw WiFi icon (18px wide)
  iconX -= 24;
  drawWiFiIcon(iconX, iconY);
}

/*
 * Update WiFi and battery status from hardware
 */
void updateStatus() {
  // Update internet connectivity status (handles timing internally)
  updateInternetStatus();

  // Update WiFi status based on internet check result
  wifiConnected = (getInternetStatus() == INET_CONNECTED);

  // Read battery voltage and percentage from I2C battery monitor
  float voltage = 3.7; // Default
  batteryPercent = 50;

  if (hasLC709203) {
    voltage = lc.cellVoltage();
    batteryPercent = (int)lc.cellPercent();
  }
  else if (hasMAX17048) {
    voltage = maxlipo.cellVoltage();
    batteryPercent = (int)maxlipo.cellPercent();
  }
  else {
    // No battery monitor - show placeholder values
    voltage = 3.7;
    batteryPercent = 50;
  }

  // Validate readings
  if (voltage < 2.5 || voltage > 5.0) {
    voltage = 3.7;
  }

  // Constrain to 0-100%
  if (batteryPercent > 100) batteryPercent = 100;
  if (batteryPercent < 0) batteryPercent = 0;

  // USB detection DISABLED - A3 conflicts with I2S_LCK_PIN (GPIO 15)
  // Cannot use analogRead on GPIO 15 or it breaks I2S audio!
  // Assume not charging for now (could use battery voltage trend instead)
  isCharging = false;

  // Debug output
  if (DEBUG_ENABLED) {
    Serial.print("Battery: ");
    Serial.print(voltage);
    Serial.print("V (");
    Serial.print(batteryPercent);
    Serial.print("%) ");
    Serial.print(isCharging ? "CHARGING" : "");
    Serial.print(" | WiFi: ");
    Serial.println(wifiConnected ? "Connected" : "Disconnected");
  }
}

#endif // STATUS_BAR_H
