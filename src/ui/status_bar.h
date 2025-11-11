/*
 * Status Bar Module
 * Handles battery and WiFi status monitoring and display
 */

#ifndef STATUS_BAR_H
#define STATUS_BAR_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_LC709203F.h>
#include <Adafruit_MAX1704X.h>
#include <WiFi.h>
#include "../core/config.h"

// Forward declarations from main file
extern Adafruit_ST7789 tft;
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
 * Draw battery icon with charge level and charging indicator
 */
void drawBatteryIcon(int x, int y) {
  // Battery outline (24x14 pixels - larger for better visibility)
  tft.drawRect(x, y, 24, 14, ST77XX_WHITE);
  tft.fillRect(x + 24, y + 4, 2, 6, ST77XX_WHITE); // Battery nub

  // Determine battery color based on percentage
  uint16_t fillColor;
  if (batteryPercent > 60) {
    fillColor = ST77XX_GREEN;
  } else if (batteryPercent > 20) {
    fillColor = ST77XX_YELLOW;
  } else {
    fillColor = ST77XX_RED;
  }

  // Fill battery based on percentage
  int fillWidth = (batteryPercent * 20) / 100; // 20 pixels max fill

  if (DEBUG_ENABLED) {
    Serial.print("Drawing battery: ");
    Serial.print(batteryPercent);
    Serial.print("% fillWidth=");
    Serial.print(fillWidth);
    Serial.print(" charging=");
    Serial.println(isCharging ? "YES" : "NO");
  }

  if (fillWidth > 0) {
    tft.fillRect(x + 2, y + 2, fillWidth, 10, fillColor);
  }

  // Draw charging indicator (white lightning bolt with black outline for contrast)
  if (isCharging) {
    // Draw black outline first for better visibility
    tft.fillTriangle(x + 13, y + 3, x + 10, y + 8, x + 16, y + 8, ST77XX_BLACK);
    tft.fillTriangle(x + 10, y + 8, x + 13, y + 13, x + 9, y + 8, ST77XX_BLACK);
    // Draw white lightning bolt on top
    tft.fillTriangle(x + 14, y + 4, x + 11, y + 8, x + 15, y + 8, ST77XX_WHITE);
    tft.fillTriangle(x + 11, y + 8, x + 14, y + 12, x + 10, y + 8, ST77XX_WHITE);
  }
}

/*
 * Draw WiFi icon with signal strength bars
 */
void drawWiFiIcon(int x, int y) {
  // WiFi icon (signal bars)
  uint16_t wifiColor = wifiConnected ? ST77XX_GREEN : ST77XX_RED;

  // Draw signal strength bars (4 bars, increasing height)
  tft.fillRect(x, y + 8, 2, 2, wifiColor);      // Shortest bar
  tft.fillRect(x + 4, y + 6, 2, 4, wifiColor);  // Medium-short bar
  tft.fillRect(x + 8, y + 4, 2, 6, wifiColor);  // Medium-tall bar
  tft.fillRect(x + 12, y + 2, 2, 8, wifiColor); // Tallest bar
}

/*
 * Draw all status icons (WiFi and battery)
 */
void drawStatusIcons() {
  int iconX = SCREEN_WIDTH - 10; // Start from right edge
  int iconY = 13; // Vertically centered in 40px header

  // Draw battery icon (now larger - 26px wide including nub)
  iconX -= 30;
  drawBatteryIcon(iconX, iconY);

  // Draw WiFi icon
  iconX -= 25;
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
