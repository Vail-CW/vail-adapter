/*
 * Practice Oscillator Mode
 * Allows free-form morse code practice with paddle/key
 * Includes real-time morse decoding with adaptive speed tracking
 */

#ifndef TRAINING_PRACTICE_H
#define TRAINING_PRACTICE_H

#include "../core/config.h"
#include "../settings/settings_cw.h"
#include "../audio/morse_decoder_adaptive.h"

// Forward declaration of drawHeader (defined in menu_ui.h)
void drawHeader();

// Practice mode state
bool practiceActive = false;
bool ditPressed = false;
bool dahPressed = false;
bool lastDitPressed = false;
bool lastDahPressed = false;
unsigned long practiceStartupTime = 0;  // Track startup time for input delay

// Iambic keyer state
unsigned long ditDahTimer = 0;
bool keyerActive = false;
bool sendingDit = false;
bool sendingDah = false;
bool inSpacing = false;  // True when in inter-element gap
bool ditMemory = false;
bool dahMemory = false;
unsigned long elementStartTime = 0;
int ditDuration = 0;

// Statistics
unsigned long practiceStartTime = 0;
int ditCount = 0;
int dahCount = 0;

// Decoder state
MorseDecoderAdaptive decoder(20, 20, 30);  // Initial 20 WPM, buffer size 30
String decodedText = "";
String decodedMorse = "";
bool showDecoding = true;
bool needsUIUpdate = false;

// Timing capture for decoder
unsigned long lastStateChangeTime = 0;
bool lastToneState = false;
unsigned long lastElementTime = 0;  // Track last element for timeout flush

// Forward declarations
void startPracticeMode(LGFX &display);
void drawPracticeUI(LGFX &display);
void drawDecodedTextOnly(LGFX &display);
int handlePracticeInput(char key, LGFX &display);
void updatePracticeOscillator();
void drawPracticeStats(LGFX &display);
void straightKeyHandler();
void iambicKeyerHandler();

// Start practice mode
void startPracticeMode(LGFX &display) {
  practiceActive = true;
  ditPressed = false;
  dahPressed = false;
  keyerActive = false;
  inSpacing = false;
  ditMemory = false;
  dahMemory = false;
  practiceStartupTime = millis();  // Record startup time for input delay

  // Clear any lingering touch sensor state
  touchRead(TOUCH_DIT_PIN);
  touchRead(TOUCH_DAH_PIN);
  delay(50);

  // Reinitialize I2S to ensure clean state
  Serial.println("Reinitializing I2S for practice mode...");
  i2s_zero_dma_buffer(I2S_NUM_0);
  delay(50);

  // Calculate dit duration from current speed setting
  ditDuration = DIT_DURATION(cwSpeed);

  // Reset statistics
  practiceStartTime = millis();
  ditCount = 0;
  dahCount = 0;

  // Reset decoder and clear any buffered state
  decoder.reset();
  decoder.flush();  // Clear any pending timings
  decoder.setWPM(cwSpeed);
  decodedText = "";
  decodedMorse = "";
  lastStateChangeTime = 0;  // Don't initialize until first key press
  lastToneState = false;
  lastElementTime = 0;  // Reset element timeout tracker
  showDecoding = true;
  needsUIUpdate = false;

  // Setup decoder callbacks
  decoder.messageCallback = [](String morse, String text) {
    // Process each character in the decoded text individually
    for (int i = 0; i < text.length(); i++) {
      // Check if adding this character would exceed our 32-char limit
      if (decodedText.length() >= 32) {
        // Clear everything and start fresh
        decodedText = "";
        decodedMorse = "";
      }

      // Add the character
      decodedText += text[i];
    }

    // Also track morse pattern
    if (decodedMorse.length() + morse.length() > 100) {
      decodedMorse = "";  // Clear morse if it gets too long
    }
    decodedMorse += morse;

    needsUIUpdate = true;

    Serial.print("Decoded: ");
    Serial.print(text);
    Serial.print(" (");
    Serial.print(morse);
    Serial.print(") -> Total length: ");
    Serial.println(decodedText.length());
  };

  decoder.speedCallback = [](float wpm, float fwpm) {
    Serial.print("Speed detected: ");
    Serial.print(wpm);
    Serial.println(" WPM");
  };

  // Draw header with correct "PRACTICE" title
  drawHeader();

  // Draw practice UI
  drawPracticeUI(display);

  Serial.println("Practice mode started with decoding enabled");
  Serial.print("Speed: ");
  Serial.print(cwSpeed);
  Serial.print(" WPM, Tone: ");
  Serial.print(cwTone);
  Serial.print(" Hz, Key type: ");
  if (cwKeyType == KEY_STRAIGHT) {
    Serial.println("Straight");
  } else if (cwKeyType == KEY_IAMBIC_A) {
    Serial.println("Iambic A");
  } else {
    Serial.println("Iambic B");
  }
}

// Draw practice UI
void drawPracticeUI(LGFX &display) {
  // Clear screen but preserve header bar (0-HEADER_HEIGHT)
  display.fillRect(0, HEADER_HEIGHT + 2, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT - 2, COLOR_BACKGROUND);

  // Modern card-style info display (3 equal cards)
  const int CARD_Y = 75;  // Positioned below 60px header
  const int CARD_HEIGHT = 50;
  const int CARD_SPACING = 4;
  const int CARD_WIDTH = (SCREEN_WIDTH - (4 * CARD_SPACING)) / 3;

  // Variables for text bounds
  int16_t x1, y1;
  uint16_t w, h;

  // Get detected WPM for card 2
  float detectedWPM = decoder.getWPM();

  // Card 1: Set Speed
  int card1X = CARD_SPACING;

  // Card background (draw first)
  display.fillRoundRect(card1X, CARD_Y, CARD_WIDTH, CARD_HEIGHT, 6, 0x2104);  // Dark gray
  display.drawRoundRect(card1X, CARD_Y, CARD_WIDTH, CARD_HEIGHT, 6, 0x4A49);  // Light border

  // Title badge for Card 1 (draw on top to hover over card)
  display.fillRoundRect(card1X + 5, CARD_Y - 7, 60, 14, 4, ST77XX_CYAN);
  display.setFont(&FreeSansBold9pt7b);
  display.setTextSize(1);
  display.setTextColor(ST77XX_BLACK);
  display.setCursor(card1X + 8, CARD_Y - 7);
  display.print("SET WPM");
  display.setFont(nullptr);

  // Speed value (centered) - use smooth font
  display.setFont(&FreeSansBold18pt7b);
  display.setTextSize(1);
  display.setTextColor(ST77XX_CYAN);
  String speedStr = String(cwSpeed);
  getTextBounds_compat(display, speedStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor(card1X + (CARD_WIDTH - w) / 2, CARD_Y + 15);
  display.print(speedStr);
  display.setFont(nullptr);

  // Card 2: Detected Speed
  int card2X = card1X + CARD_WIDTH + CARD_SPACING;

  // Clear the entire card area first to remove old overlapping text
  display.fillRect(card2X, CARD_Y, CARD_WIDTH, CARD_HEIGHT, COLOR_BACKGROUND);

  // Card background (draw first)
  display.fillRoundRect(card2X, CARD_Y, CARD_WIDTH, CARD_HEIGHT, 6, 0x2104);
  display.drawRoundRect(card2X, CARD_Y, CARD_WIDTH, CARD_HEIGHT, 6, 0x4A49);

  // Title badge for Card 2 (draw on top to hover over card)
  display.fillRoundRect(card2X + 5, CARD_Y - 7, 55, 14, 4, ST77XX_GREEN);
  display.setFont(&FreeSansBold9pt7b);
  display.setTextSize(1);
  display.setTextColor(ST77XX_BLACK);
  display.setCursor(card2X + 8, CARD_Y - 7);
  display.print("ACTUAL");
  display.setFont(nullptr);

  // WPM value (centered, color-coded) - use smooth font
  display.setFont(&FreeSansBold18pt7b);
  display.setTextSize(1);
  if (detectedWPM > 0) {
    if (abs(detectedWPM - cwSpeed) > 2) {
      display.setTextColor(ST77XX_YELLOW);  // Yellow if different
    } else {
      display.setTextColor(ST77XX_GREEN);  // Green if same
    }
    String detStr = String(detectedWPM, 1);
    getTextBounds_compat(display, detStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(card2X + (CARD_WIDTH - w) / 2, CARD_Y + 15);
    display.print(detStr);
  } else {
    display.setTextColor(0x7BEF);
    display.setCursor(card2X + (CARD_WIDTH - 24) / 2, CARD_Y + 15);
    display.print("--");
  }
  display.setFont(nullptr);

  // Card 3: Key Type
  int card3X = card2X + CARD_WIDTH + CARD_SPACING;

  // Card background (draw first)
  display.fillRoundRect(card3X, CARD_Y, CARD_WIDTH, CARD_HEIGHT, 6, 0x2104);
  display.drawRoundRect(card3X, CARD_Y, CARD_WIDTH, CARD_HEIGHT, 6, 0x4A49);

  // Title badge for Card 3 (draw on top to hover over card)
  display.fillRoundRect(card3X + 5, CARD_Y - 7, 65, 14, 4, ST77XX_YELLOW);
  display.setFont(&FreeSansBold9pt7b);
  display.setTextSize(1);
  display.setTextColor(ST77XX_BLACK);
  display.setCursor(card3X + 8, CARD_Y - 7);
  display.print("KEY TYPE");
  display.setFont(nullptr);

  // Key type value (centered) - use smooth font
  display.setFont(&FreeSansBold12pt7b);
  display.setTextSize(1);
  display.setTextColor(ST77XX_YELLOW);
  String keyStr;
  if (cwKeyType == KEY_STRAIGHT) {
    keyStr = "Straight";
  } else if (cwKeyType == KEY_IAMBIC_A) {
    keyStr = "Iambic A";
  } else {
    keyStr = "Iambic B";
  }
  getTextBounds_compat(display, keyStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor(card3X + (CARD_WIDTH - w) / 2, CARD_Y + 12);
  display.print(keyStr);
  display.setFont(nullptr);

  // Decoded text area (if enabled) - modernized 2-line display
  if (showDecoding) {
    // Add space between cards and decoder box
    const int DECODER_Y = 135;  // Positioned below cards (CARD_Y=75 + CARD_HEIGHT=50 + spacing)
    const int DECODER_HEIGHT = 70;

    // Clear any potential stray characters from previous renders first
    display.fillRect(0, DECODER_Y, SCREEN_WIDTH, DECODER_HEIGHT, COLOR_BACKGROUND);

    // Draw rounded rect background for decoder
    display.fillRoundRect(5, DECODER_Y, SCREEN_WIDTH - 10, DECODER_HEIGHT, 8, 0x1082);  // Dark gray background
    display.drawRoundRect(5, DECODER_Y, SCREEN_WIDTH - 10, DECODER_HEIGHT, 8, 0x4A49);  // Light gray border

    // Title badge
    display.fillRoundRect(15, DECODER_Y - 7, 80, 14, 4, ST77XX_CYAN);
    display.setFont(&FreeSansBold9pt7b);
    display.setTextSize(1);
    display.setTextColor(ST77XX_BLACK);
    display.setCursor(18, DECODER_Y - 7);
    display.print("DECODER");
    display.setFont(nullptr);

    // Show decoded text (2 lines, size 3 for modern look)
    display.setTextSize(3);
    display.setTextColor(ST77XX_WHITE);
    display.setTextWrap(false);  // Disable automatic text wrapping

    // STRICT: 16 chars per line, 2 lines max = 32 chars total
    const int CHARS_PER_LINE = 16;
    const int MAX_LINES = 2;
    const int MAX_TOTAL_CHARS = 32;  // Absolute maximum
    const int LINE1_Y = DECODER_Y + 17;  // Adjusted for new decoder position
    const int LINE2_Y = DECODER_Y + 43;  // Adjusted for new decoder position
    const int TEXT_X = 15;

    // Enforce absolute maximum - truncate if needed
    String safeText = decodedText;
    if (safeText.length() > MAX_TOTAL_CHARS) {
      safeText = safeText.substring(safeText.length() - MAX_TOTAL_CHARS);
    }

    // Extract exactly the last 32 chars (or less if shorter)
    int textLen = safeText.length();

    // Line 1: Show last N characters that fit (up to 16)
    if (textLen > 0) {
      String line1;
      if (textLen <= CHARS_PER_LINE) {
        // All text fits on line 1
        line1 = safeText;
      } else {
        // Text spans both lines - line 1 gets chars 0-15 of the last 32
        line1 = safeText.substring(0, CHARS_PER_LINE);
      }

      Serial.print("Line 1 [");
      Serial.print(TEXT_X);
      Serial.print(",");
      Serial.print(LINE1_Y);
      Serial.print("]: '");
      Serial.print(line1);
      Serial.print("' (");
      Serial.print(line1.length());
      Serial.println(" chars)");

      display.setCursor(TEXT_X, LINE1_Y);
      display.print(line1);
    }

    // Line 2: Show overflow (chars 16-31)
    if (textLen > CHARS_PER_LINE) {
      String line2 = safeText.substring(CHARS_PER_LINE);  // Everything after char 16

      Serial.print("Line 2: '");
      Serial.print(line2);
      Serial.print("' (");
      Serial.print(line2.length());
      Serial.println(" chars)");

      // Clear the line 2 area specifically before rendering
      // Line 2 is at y=138, text size 3 = ~24 pixels high
      display.fillRect(6, LINE2_Y - 2, 308, 24, 0x1082);

      // Render line 2 at fixed position - use multiple setCursor calls
      display.setCursor(TEXT_X, LINE2_Y);
      delay(1);  // Small delay to ensure cursor position takes
      display.setCursor(TEXT_X, LINE2_Y);
      display.print(line2);
    }
  } else {
    // Decoding disabled message
    const int DECODER_Y = 135;
    display.setFont(&FreeSansBold12pt7b);
    display.setTextSize(1);
    display.setTextColor(0x7BEF);  // Gray
    const char* disabledMsg = "Press D to enable decoding";
    int16_t dx1, dy1;
    uint16_t dw, dh;
    getTextBounds_compat(display, disabledMsg, 0, 0, &dx1, &dy1, &dw, &dh);
    display.setCursor((SCREEN_WIDTH - dw) / 2, DECODER_Y + 30);
    display.print(disabledMsg);
    display.setFont(nullptr);
  }

  // Draw footer instructions (updated with arrow key hints) - use smooth font
  display.setFont(&FreeSansBold9pt7b);
  display.setTextSize(1);
  display.setTextColor(COLOR_WARNING);

  // Split into two lines for better readability
  if (showDecoding) {
    String line1 = "UP/DN:Speed  L/R:Key";
    String line2 = "C:Clear  D:Hide  ESC:Exit";

    int16_t fx1, fy1;
    uint16_t fw, fh;
    getTextBounds_compat(display, line1.c_str(), 0, 0, &fx1, &fy1, &fw, &fh);
    int fcenterX = (SCREEN_WIDTH - fw) / 2;
    display.setCursor(fcenterX, SCREEN_HEIGHT - 32);
    display.print(line1);

    getTextBounds_compat(display, line2.c_str(), 0, 0, &fx1, &fy1, &fw, &fh);
    fcenterX = (SCREEN_WIDTH - fw) / 2;
    display.setCursor(fcenterX, SCREEN_HEIGHT - 16);
    display.print(line2);
  } else {
    // When decoder is hidden, show single line
    String footerText = "D:Show  ESC:Exit";
    int16_t fx1, fy1;
    uint16_t fw, fh;
    getTextBounds_compat(display, footerText.c_str(), 0, 0, &fx1, &fy1, &fw, &fh);
    int fcenterX = (SCREEN_WIDTH - fw) / 2;
    display.setCursor(fcenterX, SCREEN_HEIGHT - 22);
    display.print(footerText);
  }
  display.setFont(nullptr);
}

// Draw only the decoded text area (for real-time updates without full redraw)
void drawDecodedTextOnly(LGFX &display) {
  if (!showDecoding) return;

  // Update the ACTUAL WPM card (Card 2)
  const int CARD_Y = 75;  // Match updated position
  const int CARD_HEIGHT = 50;
  const int CARD_SPACING = 4;
  const int CARD_WIDTH = (SCREEN_WIDTH - (4 * CARD_SPACING)) / 3;
  int card2X = CARD_SPACING + CARD_WIDTH + CARD_SPACING;

  float detectedWPM = decoder.getWPM();

  // Clear just the number area of card 2
  display.fillRect(card2X + 5, CARD_Y + 10, CARD_WIDTH - 10, 35, 0x2104);

  // Use smooth font for WPM display
  display.setFont(&FreeSansBold18pt7b);
  display.setTextSize(1);
  if (detectedWPM > 0) {
    if (abs(detectedWPM - cwSpeed) > 2) {
      display.setTextColor(ST77XX_YELLOW);  // Yellow if different
    } else {
      display.setTextColor(ST77XX_GREEN);  // Green if same
    }
    String detStr = String(detectedWPM, 1);
    int16_t x1, y1;
    uint16_t w, h;
    getTextBounds_compat(display, detStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(card2X + (CARD_WIDTH - w) / 2, CARD_Y + 15);
    display.print(detStr);
  } else {
    display.setTextColor(0x7BEF);
    display.setCursor(card2X + (CARD_WIDTH - 24) / 2, CARD_Y + 15);
    display.print("--");
  }
  display.setFont(nullptr);

  // Decoder box positioning (matches drawPracticeUI)
  const int DECODER_Y = 135;  // Match updated position
  const int DECODER_HEIGHT = 70;

  // Clear the entire content area inside the decoder box
  display.fillRect(6, DECODER_Y + 10, 308, 58, 0x1082);  // Clear entire text area

  // Show decoded text (2 lines, size 3 for modern look)
  display.setTextSize(3);
  display.setTextColor(ST77XX_WHITE);
  display.setTextWrap(false);  // Disable automatic text wrapping

  // STRICT: 16 chars per line, 2 lines max = 32 chars total
  const int CHARS_PER_LINE = 16;
  const int MAX_TOTAL_CHARS = 32;  // Absolute maximum
  const int LINE1_Y = DECODER_Y + 17;  // Match drawPracticeUI
  const int LINE2_Y = DECODER_Y + 43;  // Match drawPracticeUI
  const int TEXT_X = 15;

  // Enforce absolute maximum - truncate if needed
  String safeText = decodedText;
  if (safeText.length() > MAX_TOTAL_CHARS) {
    safeText = safeText.substring(safeText.length() - MAX_TOTAL_CHARS);
  }

  // Extract exactly the last 32 chars (or less if shorter)
  int textLen = safeText.length();

  // Line 1: Show last N characters that fit (up to 16)
  if (textLen > 0) {
    String line1;
    if (textLen <= CHARS_PER_LINE) {
      // All text fits on line 1
      line1 = safeText;
    } else {
      // Text spans both lines - line 1 gets chars 0-15 of the last 32
      line1 = safeText.substring(0, CHARS_PER_LINE);
    }

    display.setCursor(TEXT_X, LINE1_Y);
    display.print(line1);

    // CRITICAL: Reset text wrapping state after line 1
    // The display library may have advanced the cursor, so we need to
    // ensure a clean state before attempting line 2
    display.setTextWrap(false);
  }

  // Line 2: Show overflow (chars 16-31)
  if (textLen > CHARS_PER_LINE) {
    String line2 = safeText.substring(CHARS_PER_LINE);  // Everything after char 16

    // Clear the line 2 area specifically before rendering
    // Line 2 is at y=138, text size 3 = ~24 pixels high
    display.fillRect(6, LINE2_Y - 2, 308, 24, 0x1082);

    // Render line 2 at fixed position - use multiple setCursor calls
    display.setCursor(TEXT_X, LINE2_Y);
    delay(1);  // Small delay to ensure cursor position takes
    display.setCursor(TEXT_X, LINE2_Y);
    display.print(line2);
  }
}

// Draw practice statistics and visual feedback
void drawPracticeStats(LGFX &display) {
  // Clear indicator area
  display.fillRect(0, 155, SCREEN_WIDTH, 35, COLOR_BACKGROUND);

  // Draw visual indicator (large dot that lights up)
  int centerX = SCREEN_WIDTH / 2;
  int centerY = 170;

  if (ditPressed || dahPressed) {
    // Active - filled circle
    display.fillCircle(centerX, centerY, 15, ST77XX_GREEN);
    display.drawCircle(centerX, centerY, 15, ST77XX_WHITE);

    // Show which paddle
    display.setTextSize(1);
    display.setTextColor(ST77XX_BLACK);
    display.setCursor(centerX - 10, centerY - 4);
    if (ditPressed && dahPressed) {
      display.print("BOTH");
    } else if (ditPressed) {
      display.print("DIT");
    } else {
      display.print("DAH");
    }
  } else {
    // Inactive - outline only
    display.drawCircle(centerX, centerY, 15, 0x4208);
  }
}

// Handle practice mode input (keyboard)
int handlePracticeInput(char key, LGFX &display) {
  if (key == KEY_ESC) {
    practiceActive = false;
    stopTone();
    decoder.flush();  // Decode any remaining buffered timings
    return -1;  // Exit practice mode
  }
  else if (key == 'd' || key == 'D') {
    // Toggle decoding display
    showDecoding = !showDecoding;
    drawPracticeUI(display);
    beep(TONE_MENU_NAV, BEEP_SHORT);
    return 1;
  }
  else if (key == KEY_UP) {
    // Increase speed
    if (cwSpeed < WPM_MAX) {
      cwSpeed++;
      ditDuration = DIT_DURATION(cwSpeed);
      decoder.setWPM(cwSpeed);
      saveCWSettings();  // Save to preferences
      drawPracticeUI(display);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 1;
    }
  }
  else if (key == KEY_DOWN) {
    // Decrease speed
    if (cwSpeed > WPM_MIN) {
      cwSpeed--;
      ditDuration = DIT_DURATION(cwSpeed);
      decoder.setWPM(cwSpeed);
      saveCWSettings();  // Save to preferences
      drawPracticeUI(display);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 1;
    }
  }
  else if (key == KEY_LEFT) {
    // Cycle key type backward: Iambic B -> Iambic A -> Straight -> Iambic B
    if (cwKeyType == KEY_IAMBIC_B) {
      cwKeyType = KEY_IAMBIC_A;
    } else if (cwKeyType == KEY_IAMBIC_A) {
      cwKeyType = KEY_STRAIGHT;
    } else {
      cwKeyType = KEY_IAMBIC_B;
    }
    saveCWSettings();  // Save to preferences
    drawPracticeUI(display);
    beep(TONE_MENU_NAV, BEEP_SHORT);
    return 1;
  }
  else if (key == KEY_RIGHT) {
    // Cycle key type forward: Straight -> Iambic A -> Iambic B -> Straight
    if (cwKeyType == KEY_STRAIGHT) {
      cwKeyType = KEY_IAMBIC_A;
    } else if (cwKeyType == KEY_IAMBIC_A) {
      cwKeyType = KEY_IAMBIC_B;
    } else {
      cwKeyType = KEY_STRAIGHT;
    }
    saveCWSettings();  // Save to preferences
    drawPracticeUI(display);
    beep(TONE_MENU_NAV, BEEP_SHORT);
    return 1;
  }
  else if (key == 'c' || key == 'C') {
    // Clear decoder text
    decodedText = "";
    decodedMorse = "";
    decoder.reset();
    decoder.flush();
    drawDecodedTextOnly(display);
    beep(TONE_MENU_NAV, BEEP_SHORT);
    return 1;
  }

  return 0;
}

// Update practice oscillator (called in main loop)
void updatePracticeOscillator() {
  if (!practiceActive) return;

  // Ignore all input for first 1000ms to prevent startup glitches
  if (millis() - practiceStartupTime < 1000) {
    return;
  }

  // Check for decoder timeout (flush if no activity for word gap duration)
  // This is a backup to flush any buffered data if user stops keying mid-character
  if (showDecoding && lastElementTime > 0 && !ditPressed && !dahPressed) {
    unsigned long timeSinceLastElement = millis() - lastElementTime;
    float wordGapDuration = MorseWPM::wordGap(decoder.getWPM());

    // Flush buffered data after word gap silence (backup timeout)
    if (timeSinceLastElement > wordGapDuration) {
      decoder.flush();
      lastElementTime = 0;  // Reset timeout
    }
  }

  // Read paddle/key inputs (physical + capacitive touch)
  // ESP32-S3: Use GPIO numbers directly, check > threshold (values rise when touched)
  ditPressed = (digitalRead(DIT_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DIT_PIN) > TOUCH_THRESHOLD);
  dahPressed = (digitalRead(DAH_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DAH_PIN) > TOUCH_THRESHOLD);

  // Handle based on key type
  if (cwKeyType == KEY_STRAIGHT) {
    straightKeyHandler();
  } else {
    iambicKeyerHandler();
  }

  // Update visual feedback if state changed
  if (ditPressed != lastDitPressed || dahPressed != lastDahPressed) {
    // Will be redrawn in main loop
    lastDitPressed = ditPressed;
    lastDahPressed = dahPressed;
  }
}

// Straight key handler (simple on/off)
void straightKeyHandler() {
  unsigned long currentTime = millis();
  bool toneOn = isTonePlaying();

  // Use DIT pin as straight key
  if (ditPressed && !toneOn) {
    // Tone starting
    if (showDecoding && lastToneState == false) {
      // Send silence duration to decoder (negative)
      // Only if we have a valid previous state change time
      if (lastStateChangeTime > 0) {
        float silenceDuration = currentTime - lastStateChangeTime;
        if (silenceDuration > 0) {
          decoder.addTiming(-silenceDuration);
        }
      }
      lastStateChangeTime = currentTime;
      lastToneState = true;
    }

    startTone(cwTone);
  }
  else if (ditPressed && toneOn) {
    // Tone continuing
    continueTone(cwTone);
  }
  else if (!ditPressed && toneOn) {
    // Tone stopping
    if (showDecoding && lastToneState == true) {
      // Send tone duration to decoder (positive)
      float toneDuration = currentTime - lastStateChangeTime;
      if (toneDuration > 0) {
        decoder.addTiming(toneDuration);
        lastElementTime = currentTime;  // Update timeout tracker
      }
      lastStateChangeTime = currentTime;
      lastToneState = false;
    }

    stopTone();
  }
}

// Iambic keyer handler (Mode A or B)
void iambicKeyerHandler() {
  unsigned long currentTime = millis();

  // If not actively sending or spacing, check for new input
  if (!keyerActive && !inSpacing) {
    if (ditPressed || ditMemory) {
      // Start sending dit
      if (showDecoding && lastToneState == false) {
        // Send silence duration to decoder - let decoder filter inter-element gaps
        // Only if we have a valid previous state change time
        if (lastStateChangeTime > 0) {
          float silenceDuration = currentTime - lastStateChangeTime;
          if (silenceDuration > 0) {
            decoder.addTiming(-silenceDuration);
          }
        }
        lastStateChangeTime = currentTime;
        lastToneState = true;
      }

      keyerActive = true;
      sendingDit = true;
      sendingDah = false;
      inSpacing = false;
      elementStartTime = currentTime;
      ditCount++;
      startTone(cwTone);

      // Clear dit memory
      ditMemory = false;
    }
    else if (dahPressed || dahMemory) {
      // Start sending dah
      if (showDecoding && lastToneState == false) {
        // Send silence duration to decoder - let decoder filter inter-element gaps
        // Only if we have a valid previous state change time
        if (lastStateChangeTime > 0) {
          float silenceDuration = currentTime - lastStateChangeTime;
          if (silenceDuration > 0) {
            decoder.addTiming(-silenceDuration);
          }
        }
        lastStateChangeTime = currentTime;
        lastToneState = true;
      }

      keyerActive = true;
      sendingDit = false;
      sendingDah = true;
      inSpacing = false;
      elementStartTime = currentTime;
      dahCount++;
      startTone(cwTone);

      // Clear dah memory
      dahMemory = false;
    }
  }
  // Currently sending an element
  else if (keyerActive && !inSpacing) {
    unsigned long elementDuration = sendingDit ? ditDuration : (ditDuration * 3);

    // Keep tone playing
    continueTone(cwTone);

    // Continuously check for paddle input during element send
    if (ditPressed && dahPressed) {
      // Both pressed (squeeze) - remember opposite paddle
      if (sendingDit) {
        dahMemory = true;
      } else {
        ditMemory = true;
      }
    }
    else if (sendingDit && dahPressed) {
      // Sending dit, dah pressed
      dahMemory = true;
    }
    else if (sendingDah && ditPressed) {
      // Sending dah, dit pressed
      ditMemory = true;
    }

    // Check if element is complete
    if (currentTime - elementStartTime >= elementDuration) {
      // Element complete, turn off tone and start spacing
      if (showDecoding && lastToneState == true) {
        // Send tone duration to decoder
        float toneDuration = currentTime - lastStateChangeTime;
        if (toneDuration > 0) {
          decoder.addTiming(toneDuration);
          lastElementTime = currentTime;  // Update timeout tracker
        }
        lastStateChangeTime = currentTime;
        lastToneState = false;
      }

      stopTone();
      keyerActive = false;
      sendingDit = false;
      sendingDah = false;
      inSpacing = true;
      elementStartTime = currentTime;  // Reset timer for spacing
    }
  }
  // In inter-element spacing
  else if (inSpacing) {
    // Continue checking paddles during spacing to catch input
    if (ditPressed && dahPressed) {
      // Both pressed - if we just sent dit, remember dah (and vice versa)
      // Can't determine what we just sent, so set both memories
      ditMemory = true;
      dahMemory = true;
    }
    else if (ditPressed && !ditMemory) {
      ditMemory = true;
    }
    else if (dahPressed && !dahMemory) {
      dahMemory = true;
    }

    // Wait for 1 dit duration (inter-element gap)
    if (currentTime - elementStartTime >= ditDuration) {
      inSpacing = false;
      // Now ready to send next element if memory is set or paddle still pressed
    }
  }
}

#endif // TRAINING_PRACTICE_H
