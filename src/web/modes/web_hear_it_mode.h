/*
 * Web Hear It Type It Mode Module
 * Handles web-based "Hear It Type It" training where device plays callsigns
 * and browser collects user answers
 */

#ifndef WEB_HEAR_IT_MODE_H
#define WEB_HEAR_IT_MODE_H

#include "../../core/morse_code.h"
#include "../../core/config.h"

// Web hear it mode state
bool webHearItModeActive = false;
String webCurrentCallsign = "";
int webCurrentWPM = 0;
int webAttempts = 0;
bool webWaitingForInput = false;
bool webReplayRequested = false;
bool webSkipRequested = false;
unsigned long webLastActionTime = 0;

// Forward declarations from web_hear_it_socket.h
extern void sendHearItNewCallsign(String callsign, int wpm);
extern void sendHearItPlaying();
extern void sendHearItReadyForInput();

// Forward declaration from training_hear_it_type_it.h
extern String generateCallsign();

// Forward declarations (defined later in this file)
void drawWebHearItUI(LGFX& tft);
void webPlayCurrentCallsign();
void webGenerateNewCallsign();

/*
 * Generate and send new callsign to browser
 */
void webGenerateNewCallsign() {
  webCurrentCallsign = generateCallsign();
  webCurrentWPM = random(12, 21); // Random speed between 12-20 WPM
  webAttempts = 0;

  Serial.print("Web mode: New callsign: ");
  Serial.print(webCurrentCallsign);
  Serial.print(" at ");
  Serial.print(webCurrentWPM);
  Serial.println(" WPM");

  // Send to browser
  sendHearItNewCallsign(webCurrentCallsign, webCurrentWPM);

  // Small delay before playing
  delay(500);

  // Play the callsign
  webPlayCurrentCallsign();
}

/*
 * Play the current callsign
 */
void webPlayCurrentCallsign() {
  webWaitingForInput = false;

  Serial.print("Web mode: Triggering browser playback for callsign: ");
  Serial.print(webCurrentCallsign);
  Serial.print(" @ ");
  Serial.print(webCurrentWPM);
  Serial.println(" WPM");

  // Notify browser that audio is playing (browser will handle audio playback)
  // The browser will play the morse code and then notify when ready for input
  sendHearItPlaying();

  // NOTE: Do NOT play audio on device - that caused crashes
  // Audio playback is handled entirely in the browser

  Serial.println("Web mode: Audio playback delegated to browser");
}

/*
 * Initialize web hear it mode
 */
void startWebHearItMode(LGFX& tft) {
  Serial.println("Starting web Hear It Type It mode");

  // Reset state
  webHearItModeActive = true;
  webCurrentCallsign = "";
  webCurrentWPM = 0;
  webAttempts = 0;
  webWaitingForInput = false;
  webReplayRequested = false;
  webSkipRequested = false;
  webLastActionTime = millis();

  // Clear screen
  tft.fillScreen(COLOR_BACKGROUND);

  // Draw static UI
  drawWebHearItUI(tft);

  // Generate first callsign (with small delay)
  delay(1000);
  webGenerateNewCallsign();
}

/*
 * Draw web hear it mode UI (static display)
 */
void drawWebHearItUI(LGFX& tft) {
  tft.fillScreen(COLOR_BACKGROUND);

  // Header
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(10, 40);
  tft.print("Web Hear It");

  // Subtitle
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(50, 70);
  tft.print("Mode Active");

  // Instructions box
  tft.drawRect(20, 100, 280, 80, ST77XX_GREEN);
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(30, 115);
  tft.println("Playing callsigns");
  tft.setCursor(30, 130);
  tft.println("Type answers in");
  tft.setCursor(30, 145);
  tft.println("browser window");

  // Exit instruction
  tft.setTextColor(0x7BEF);  // Gray
  tft.setCursor(60, 200);
  tft.print("Press ESC to exit");
}

/*
 * Handle web hear it mode input
 * Returns: -1 to exit mode, 0 otherwise
 */
int handleWebHearItInput(char key, LGFX& tft) {
  if (key == 0x1B) {  // ESC key
    Serial.println("Exiting web Hear It Type It mode");
    webHearItModeActive = false;
    return -1;  // Exit mode
  }

  return 0;  // Stay in mode
}

/*
 * Update function (called every loop iteration)
 * Handles replay and skip requests from browser
 */
void updateWebHearItMode() {
  // Handle replay request
  if (webReplayRequested) {
    webReplayRequested = false;
    Serial.println("Web mode: Handling replay request");
    delay(2000);  // Give time to see feedback
    webPlayCurrentCallsign();
  }

  // Handle skip request (also used for new callsign after correct answer)
  if (webSkipRequested) {
    webSkipRequested = false;
    Serial.println("Web mode: Handling skip/next request");
    delay(2000);  // Give time to see feedback
    webGenerateNewCallsign();
  }
}

#endif // WEB_HEAR_IT_MODE_H
