/*
 * Web Practice Mode Module
 * Handles web-based practice mode where browser sends keying events
 * and device runs decoder, returning decoded text
 */

#ifndef WEB_PRACTICE_MODE_H
#define WEB_PRACTICE_MODE_H

#include <Adafruit_ST7789.h>
#include "../../audio/morse_decoder_adaptive.h"
#include "../../audio/morse_wpm.h"
#include "../../core/config.h"

// Web practice decoder instance (separate from device practice mode)
MorseDecoderAdaptive webPracticeDecoder(20.0f);  // Default 20 WPM

// Forward declarations from web_server.h
extern void sendPracticeDecoded(String morse, String text);
extern void sendPracticeWPM(float wpm);
extern bool webPracticeModeActive;

/*
 * Decoder callback: character decoded
 */
void onWebPracticeDecoded(String morse, String text) {
  Serial.print("Web Practice Decoded: ");
  Serial.print(morse);
  Serial.print(" = ");
  Serial.println(text);

  // Send to browser via WebSocket
  sendPracticeDecoded(morse, text);
}

/*
 * Decoder callback: speed detected
 */
void onWebPracticeSpeed(float wpm, float fwpm) {
  Serial.print("Web Practice Speed: ");
  Serial.print(wpm);
  Serial.println(" WPM");

  // Send to browser via WebSocket
  sendPracticeWPM(wpm);
}

/*
 * Initialize web practice mode
 */
void startWebPracticeMode(Adafruit_ST7789& tft) {
  Serial.println("Starting web practice mode");

  // Clear screen
  tft.fillScreen(COLOR_BACKGROUND);

  // Set up decoder callbacks
  webPracticeDecoder.messageCallback = onWebPracticeDecoded;
  webPracticeDecoder.speedCallback = onWebPracticeSpeed;

  // Reset decoder
  webPracticeDecoder.reset();

  // Draw static UI
  drawWebPracticeUI(tft);
}

/*
 * Draw web practice mode UI (static display)
 */
void drawWebPracticeUI(Adafruit_ST7789& tft) {
  tft.fillScreen(COLOR_BACKGROUND);

  // Header
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(30, 40);
  tft.print("Web Practice");

  // Subtitle
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(50, 70);
  tft.print("Mode Active");

  // Instructions box
  tft.drawRect(20, 100, 280, 80, ST77XX_GREEN);
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(30, 115);
  tft.println("Keying from web browser");
  tft.setCursor(30, 130);
  tft.println("Decoded text shows in");
  tft.setCursor(30, 145);
  tft.println("browser window");

  // Exit instruction
  tft.setTextColor(0x7BEF);  // Gray
  tft.setCursor(60, 200);
  tft.print("Press ESC to exit");
}

/*
 * Handle web practice mode input
 * Returns: -1 to exit mode, 0 otherwise
 */
int handleWebPracticeInput(char key, Adafruit_ST7789& tft) {
  if (key == 0x1B) {  // ESC key
    Serial.println("Exiting web practice mode");
    webPracticeDecoder.reset();
    return -1;  // Exit mode
  }

  return 0;  // Stay in mode
}

/*
 * Update function (called every loop iteration)
 * Web practice mode is mostly passive - decoder is fed via WebSocket
 */
void updateWebPracticeMode() {
  // No continuous updates needed - decoder is fed by WebSocket handler
  // This function exists for consistency with other modes
}

#endif // WEB_PRACTICE_MODE_H
