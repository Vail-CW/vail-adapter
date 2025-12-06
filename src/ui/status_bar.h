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
 * Draw battery icon with charge level and charging indicator (scaled for 4" display)
 */
void drawBatteryIcon(int x, int y) {
  // Battery outline (scaled to 36x20 pixels from 24x14)
  tft.drawRect(x, y, STATUS_ICON_SIZE, 20, ST77XX_WHITE);
  tft.fillRect(x + STATUS_ICON_SIZE, y + 6, 3, 8, ST77XX_WHITE); // Battery nub (scaled)

  // Determine battery color based on percentage
  uint16_t fillColor;
  if (batteryPercent > 60) {
    fillColor = ST77XX_GREEN;
  } else if (batteryPercent > 20) {
    fillColor = ST77XX_YELLOW;
  } else {
    fillColor = ST77XX_RED;
  }

  // Fill battery based on percentage (30 pixels max fill, scaled from 20)
  int fillWidth = (batteryPercent * 30) / 100;

  if (DEBUG_ENABLED) {
    Serial.print("Drawing battery: ");
    Serial.print(batteryPercent);
    Serial.print("% fillWidth=");
    Serial.print(fillWidth);
    Serial.print(" charging=");
    Serial.println(isCharging ? "YES" : "NO");
  }

  if (fillWidth > 0) {
    tft.fillRect(x + 3, y + 3, fillWidth, 14, fillColor); // Scaled fill area
  }

  // Draw charging indicator (white lightning bolt with black outline for contrast) - scaled
  if (isCharging) {
    // Draw black outline first for better visibility (scaled positions)
    tft.fillTriangle(x + 19, y + 5, x + 15, y + 12, x + 24, y + 12, ST77XX_BLACK);
    tft.fillTriangle(x + 15, y + 12, x + 19, y + 19, x + 14, y + 12, ST77XX_BLACK);
    // Draw white lightning bolt on top (scaled positions)
    tft.fillTriangle(x + 20, y + 6, x + 16, y + 12, x + 23, y + 12, ST77XX_WHITE);
    tft.fillTriangle(x + 16, y + 12, x + 20, y + 18, x + 15, y + 12, ST77XX_WHITE);
  }
}

/*
 * Draw WiFi icon with signal strength bars (scaled for 4" display)
 */
void drawWiFiIcon(int x, int y) {
  // WiFi icon (signal bars) - 25% larger than previous
  uint16_t wifiColor = wifiConnected ? ST77XX_GREEN : ST77XX_RED;

  // Draw signal strength bars (4 bars, increasing height) - enlarged for better visibility
  tft.fillRect(x, y + 11, 4, 4, wifiColor);       // Shortest bar (was 3x3)
  tft.fillRect(x + 7, y + 7, 4, 8, wifiColor);    // Medium-short bar (was 3x6)
  tft.fillRect(x + 14, y + 3, 4, 12, wifiColor);  // Medium-tall bar (was 3x9)
  tft.fillRect(x + 21, y - 1, 4, 16, wifiColor);  // Tallest bar (was 3x12)
}

/*
 * Draw all status icons (WiFi and battery) - scaled for 4" display
 */
void drawStatusIcons() {
  int iconX = SCREEN_WIDTH - 15; // Start from right edge (scaled)
  int iconY = 20; // Vertically centered in 60px header (HEADER_HEIGHT)

  // Draw battery icon (scaled - 39px wide including nub)
  iconX -= 45; // Scaled from 30
  drawBatteryIcon(iconX, iconY);

  // Draw WiFi icon (enlarged - now ~27px wide)
  iconX -= 40; // Increased from 35 to accommodate larger icon
  drawWiFiIcon(iconX, iconY);
}

/*
 * Update WiFi and battery status from hardware
 */
void updateStatus() {
  // Update WiFi status
  wifiConnected = WiFi.status() == WL_CONNECTED;

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
