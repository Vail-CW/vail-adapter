/*
 * Hardware Initialization Module
 * Handles initialization of all hardware peripherals
 */

#ifndef HARDWARE_INIT_H
#define HARDWARE_INIT_H

#include <Wire.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_LC709203F.h>
#include <Adafruit_MAX1704X.h>
#include "config.h"  // Same folder, no path change needed

// Forward declarations from main file
extern Adafruit_ST7789 tft;
extern Adafruit_LC709203F lc;
extern Adafruit_MAX17048 maxlipo;
extern bool hasLC709203;
extern bool hasMAX17048;
extern bool hasBatteryMonitor;

/*
 * Run I2C bus scan for debugging
 */
void runI2CScan() {
  Serial.println("Scanning I2C bus...");
  for (byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at 0x");
      Serial.println(i, HEX);
    }
  }
}

/*
 * Initialize battery monitor (MAX17048 or LC709203F)
 */
void initBatteryMonitor() {
  Serial.println("Initializing battery monitor...");

  // Try MAX17048 first (address 0x36) - like Adafruit example
  if (maxlipo.begin()) {
    Serial.print("Found MAX17048 with Chip ID: 0x");
    Serial.println(maxlipo.getChipID(), HEX);
    hasMAX17048 = true;
    hasBatteryMonitor = true;
  }
  // Try LC709203F if MAX not found (address 0x0B)
  else if (lc.begin()) {
    Serial.println("Found LC709203F battery monitor");
    Serial.print("Version: 0x");
    Serial.println(lc.getICversion(), HEX);

    lc.setThermistorB(3950);
    lc.setPackSize(LC709203F_APA_500MAH); // Closest to 350mAh
    lc.setAlarmVoltage(3.8);

    hasLC709203 = true;
    hasBatteryMonitor = true;
  }
  else {
    Serial.println("Could not find MAX17048 or LC709203F battery monitor!");
    runI2CScan();
  }
}

/*
 * Initialize display
 */
void initDisplay() {
  Serial.println("Initializing display...");
  tft.init(240, 320);  // Initialize with hardware dimensions
  tft.setRotation(SCREEN_ROTATION);  // Then rotate to landscape
  tft.fillScreen(COLOR_BACKGROUND);
  Serial.println("Display initialized");
}

/*
 * Initialize GPIO pins
 */
void initPins() {
  // DO NOT initialize buzzer pin - conflicts with I2S
  // pinMode(BUZZER_PIN, OUTPUT);

  // Initialize Paddle
  pinMode(DIT_PIN, INPUT_PULLUP);
  pinMode(DAH_PIN, INPUT_PULLUP);

  // USB detection disabled - A3 conflicts with I2S_LCK_PIN
  // pinMode(USB_DETECT_PIN, INPUT);
}

#endif // HARDWARE_INIT_H
