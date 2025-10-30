/*
 * Practice Oscillator Mode
 * Allows free-form morse code practice with paddle/key
 * Includes real-time morse decoding with adaptive speed tracking
 */

#ifndef TRAINING_PRACTICE_H
#define TRAINING_PRACTICE_H

#include "config.h"
#include "settings_cw.h"
#include "morse_decoder_adaptive.h"

// Practice mode state
bool practiceActive = false;
bool ditPressed = false;
bool dahPressed = false;
bool lastDitPressed = false;
bool lastDahPressed = false;

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
void startPracticeMode(Adafruit_ST7789 &display);
void drawPracticeUI(Adafruit_ST7789 &display);
void drawDecodedTextOnly(Adafruit_ST7789 &display);
int handlePracticeInput(char key, Adafruit_ST7789 &display);
void updatePracticeOscillator();
void drawPracticeStats(Adafruit_ST7789 &display);
void straightKeyHandler();
void iambicKeyerHandler();

// Start practice mode
void startPracticeMode(Adafruit_ST7789 &display) {
  practiceActive = true;
  ditPressed = false;
  dahPressed = false;
  keyerActive = false;
  inSpacing = false;
  ditMemory = false;
  dahMemory = false;

  // Disable WiFi to prevent audio interference
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Disabling WiFi for clean audio in practice mode");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
  }

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

  // Reset decoder
  decoder.reset();
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
    decodedMorse += morse;
    decodedText += text;

    // Limit text length (keep last 200 characters)
    if (decodedText.length() > 200) {
      decodedText = decodedText.substring(decodedText.length() - 200);
    }
    if (decodedMorse.length() > 300) {
      decodedMorse = decodedMorse.substring(decodedMorse.length() - 300);
    }

    needsUIUpdate = true;

    Serial.print("Decoded: ");
    Serial.print(text);
    Serial.print(" (");
    Serial.print(morse);
    Serial.println(")");
  };

  decoder.speedCallback = [](float wpm, float fwpm) {
    Serial.print("Speed detected: ");
    Serial.print(wpm);
    Serial.println(" WPM");
  };

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
void drawPracticeUI(Adafruit_ST7789 &display) {
  // Clear screen (preserve header)
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Title
  display.setTextSize(2);
  display.setTextColor(ST77XX_CYAN);
  display.setCursor(55, 50);
  display.print("PRACTICE");

  // Display current settings (compact row)
  display.setTextSize(1);
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(10, 75);
  display.print("Speed:");
  display.setTextColor(ST77XX_GREEN);
  display.print(cwSpeed);
  display.setTextColor(0x7BEF);  // Gray
  display.print(" WPM");

  // Show detected speed (always show if decoding enabled)
  float detectedWPM = decoder.getWPM();
  if (showDecoding && detectedWPM > 0) {
    if (abs(detectedWPM - cwSpeed) > 1.0f) {
      display.setTextColor(ST77XX_YELLOW);  // Yellow if different
    } else {
      display.setTextColor(ST77XX_GREEN);  // Green if same
    }
    display.print(" -> ");
    display.print(detectedWPM, 1);
  }

  display.setTextColor(ST77XX_WHITE);
  display.setCursor(200, 75);
  display.print("Tone:");
  display.setTextColor(ST77XX_GREEN);
  display.print(cwTone);

  // Decoded text area (if enabled)
  if (showDecoding) {
    display.setTextSize(1);
    display.setTextColor(0x7BEF);  // Gray
    display.setCursor(10, 95);
    display.print("Decoded Text:");

    // Show decoded text (larger area now, 4-5 lines)
    display.setTextSize(2);
    display.setTextColor(ST77XX_WHITE);

    // Calculate how much text fits (approx 26 chars per line at size 2)
    int charsPerLine = 26;
    int maxLines = 5;  // More lines available without morse display
    int maxChars = charsPerLine * maxLines;

    int textStartIdx = max(0, (int)decodedText.length() - maxChars);
    String displayText = decodedText.substring(textStartIdx);

    // Word wrap: split into lines
    int yPos = 110;  // Start higher without morse display
    int xPos = 10;
    String currentLine = "";

    for (int i = 0; i < (int)displayText.length() && yPos < 195; i++) {
      char c = displayText[i];
      currentLine += c;

      if (c == ' ' || i == (int)displayText.length() - 1) {
        // Check if line exceeds width
        if (currentLine.length() >= charsPerLine) {
          display.setCursor(xPos, yPos);
          display.print(currentLine);
          currentLine = "";
          yPos += 20;
        }
      }
    }

    // Print remaining
    if (currentLine.length() > 0 && yPos < 195) {
      display.setCursor(xPos, yPos);
      display.print(currentLine);
    }
  } else {
    // Decoding disabled message
    display.setTextSize(1);
    display.setTextColor(0x7BEF);  // Gray
    display.setCursor(50, 125);
    display.print("Press D to enable decoding");
  }

  // Draw footer instructions
  display.setTextSize(1);
  display.setTextColor(COLOR_WARNING);
  String footerText = showDecoding ? "D:Hide Decode  ESC:Exit" : "D:Show Decode  ESC:Exit";

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(footerText, 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, SCREEN_HEIGHT - 12);
  display.print(footerText);
}

// Draw only the decoded text area (for real-time updates without full redraw)
void drawDecodedTextOnly(Adafruit_ST7789 &display) {
  if (!showDecoding) return;

  // Also update the WPM display
  display.fillRect(95, 75, 100, 10, COLOR_BACKGROUND);  // Clear WPM area
  float detectedWPM = decoder.getWPM();
  if (detectedWPM > 0) {
    display.setTextSize(1);
    if (abs(detectedWPM - cwSpeed) > 1.0f) {
      display.setTextColor(ST77XX_YELLOW);  // Yellow if different
    } else {
      display.setTextColor(ST77XX_GREEN);  // Green if same
    }
    display.setCursor(95, 75);
    display.print(" -> ");
    display.print(detectedWPM, 1);
  }

  // Clear decoded text area only (from y=95 to y=200)
  display.fillRect(0, 95, SCREEN_WIDTH, 105, COLOR_BACKGROUND);

  display.setTextSize(1);
  display.setTextColor(0x7BEF);  // Gray
  display.setCursor(10, 95);
  display.print("Decoded Text:");

  // Show decoded text (larger area now, 4-5 lines)
  display.setTextSize(2);
  display.setTextColor(ST77XX_WHITE);

  // Calculate how much text fits (approx 26 chars per line at size 2)
  int charsPerLine = 26;
  int maxLines = 5;  // More lines available without morse display
  int maxChars = charsPerLine * maxLines;

  int textStartIdx = max(0, (int)decodedText.length() - maxChars);
  String displayText = decodedText.substring(textStartIdx);

  // Word wrap: split into lines
  int yPos = 110;  // Start higher without morse display
  int xPos = 10;
  String currentLine = "";

  for (int i = 0; i < (int)displayText.length() && yPos < 195; i++) {
    char c = displayText[i];
    currentLine += c;

    if (c == ' ' || i == (int)displayText.length() - 1) {
      // Check if line exceeds width
      if (currentLine.length() >= charsPerLine) {
        display.setCursor(xPos, yPos);
        display.print(currentLine);
        currentLine = "";
        yPos += 20;
      }
    }
  }

  // Print remaining
  if (currentLine.length() > 0 && yPos < 195) {
    display.setCursor(xPos, yPos);
    display.print(currentLine);
  }
}

// Draw practice statistics and visual feedback
void drawPracticeStats(Adafruit_ST7789 &display) {
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
int handlePracticeInput(char key, Adafruit_ST7789 &display) {
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

  return 0;
}

// Update practice oscillator (called in main loop)
void updatePracticeOscillator() {
  if (!practiceActive) return;

  // Check for decoder timeout (flush if no activity for word gap duration)
  // The decoder auto-flushes on character gaps (2.5 dits), but we need a backup
  // timeout to flush if the user stops keying mid-character or after a character
  // without enough silence to trigger the auto-flush (e.g., released paddles but
  // silence duration not yet captured). Use word gap (7 dits) as safety timeout.
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
