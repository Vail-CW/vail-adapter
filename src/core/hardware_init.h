/*
 * Hardware Initialization Module
 * Handles initialization of all hardware peripherals
 */

#ifndef HARDWARE_INIT_H
#define HARDWARE_INIT_H

#include <Wire.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <Adafruit_LC709203F.h>
#include <Adafruit_MAX1704X.h>
#include "config.h"  // Same folder, no path change needed

// LovyanGFX configuration for ST7796S 4.0" display
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7796 _panel_instance;
  lgfx::Bus_SPI _bus_instance;

public:
  LGFX(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;  // 40MHz write speed
      cfg.freq_read = 16000000;   // 16MHz read speed
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = TFT_SCK;
      cfg.pin_mosi = TFT_MOSI;
      cfg.pin_miso = TFT_MISO;
      cfg.pin_dc = TFT_DC;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs = TFT_CS;
      cfg.pin_rst = TFT_RST;
      cfg.pin_busy = -1;
      cfg.memory_width = 320;
      cfg.memory_height = 480;
      cfg.panel_width = 320;
      cfg.panel_height = 480;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = true;
      cfg.invert = false;       // Color inversion disabled
      cfg.rgb_order = false;    // BGR color order (swaps red and blue)
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;    // Shared SPI bus (for future SD card support)
      _panel_instance.config(cfg);
    }

    setPanel(&_panel_instance);
  }
};

// Forward declarations from main file
extern LGFX tft;
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
 * Initialize display (LovyanGFX ST7796S)
 */
void initDisplay() {
  Serial.println("Initializing 4.0\" ST7796S display...");
  tft.init();
  tft.setRotation(SCREEN_ROTATION);  // Set landscape orientation
  tft.fillScreen(COLOR_BACKGROUND);
  Serial.print("Display initialized: ");
  Serial.print(tft.width());
  Serial.print("×");
  Serial.println(tft.height());
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
