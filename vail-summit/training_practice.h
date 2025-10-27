/*
 * Practice Oscillator Mode
 * Allows free-form morse code practice with paddle/key
 */

#ifndef TRAINING_PRACTICE_H
#define TRAINING_PRACTICE_H

#include "config.h"
#include "settings_cw.h"

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

// Forward declarations
void startPracticeMode(Adafruit_ST7789 &display);
void drawPracticeUI(Adafruit_ST7789 &display);
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

  drawPracticeUI(display);

  Serial.println("Practice mode started");
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
  display.setCursor(80, 60);
  display.print("PRACTICE");

  // Display current settings
  display.setTextSize(1);
  display.setTextColor(ST77XX_WHITE);

  int yPos = 95;
  display.setCursor(60, yPos);
  display.print("Speed: ");
  display.setTextColor(ST77XX_GREEN);
  display.print(cwSpeed);
  display.setTextColor(ST77XX_WHITE);
  display.print(" WPM");

  yPos += 20;
  display.setCursor(60, yPos);
  display.print("Tone: ");
  display.setTextColor(ST77XX_GREEN);
  display.print(cwTone);
  display.setTextColor(ST77XX_WHITE);
  display.print(" Hz");

  yPos += 20;
  display.setCursor(60, yPos);
  display.print("Key: ");
  display.setTextColor(ST77XX_GREEN);
  if (cwKeyType == KEY_STRAIGHT) {
    display.print("Straight");
  } else if (cwKeyType == KEY_IAMBIC_A) {
    display.print("Iambic A");
  } else {
    display.print("Iambic B");
  }

  // No visual indicators - keeps display static for best audio performance
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(30, 165);
  display.print("Key to practice - ESC to exit");

  // Draw footer instructions
  display.setTextSize(1);
  display.setTextColor(COLOR_WARNING);
  String footerText = "ESC: Exit to Training Menu";

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(footerText, 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, SCREEN_HEIGHT - 12);
  display.print(footerText);
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
    return -1;  // Exit practice mode
  }

  return 0;
}

// Update practice oscillator (called in main loop)
void updatePracticeOscillator() {
  if (!practiceActive) return;

  // Read paddle/key inputs
  ditPressed = (digitalRead(DIT_PIN) == PADDLE_ACTIVE);
  dahPressed = (digitalRead(DAH_PIN) == PADDLE_ACTIVE);

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
  // Use DIT pin as straight key
  if (ditPressed) {
    if (!isTonePlaying()) {
      startTone(cwTone);
      Serial.println("Started tone");
    }
    continueTone(cwTone);
  } else {
    if (isTonePlaying()) {
      stopTone();
      Serial.println("Stopped tone");
    }
  }
}

// Iambic keyer handler (Mode A or B)
void iambicKeyerHandler() {
  unsigned long currentTime = millis();

  // If not actively sending or spacing, check for new input
  if (!keyerActive && !inSpacing) {
    if (ditPressed || ditMemory) {
      // Start sending dit
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
