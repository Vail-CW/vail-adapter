/*
 * Radio Output Mode
 * Allows keying external radios via 3.5mm jack outputs
 * Supports Summit Keyer mode (internal keying) and Radio Keyer mode (passthrough)
 */

#ifndef RADIO_OUTPUT_H
#define RADIO_OUTPUT_H

#include "../core/config.h"
#include "../settings/settings_cw.h"
#include <Preferences.h>

// Radio keyer modes
enum RadioMode {
  RADIO_MODE_SUMMIT_KEYER,   // Summit does the keying logic, outputs straight key format
  RADIO_MODE_RADIO_KEYER     // Passthrough dit/dah contacts to radio's internal keyer
};

// Radio output state
bool radioOutputActive = false;
RadioMode radioMode = RADIO_MODE_SUMMIT_KEYER;
int radioSettingSelection = 0;  // 0=Speed, 1=Key Type, 2=Radio Mode
#define RADIO_SETTINGS_COUNT 3

// Memory selector state
bool memorySelectorActive = false;
int memorySelectorSelection = 0;  // Selected memory slot (0-9)

// Message queue for web-based transmission
#define RADIO_MESSAGE_QUEUE_SIZE 5
#define RADIO_MESSAGE_MAX_LENGTH 200
struct RadioMessageQueue {
  char messages[RADIO_MESSAGE_QUEUE_SIZE][RADIO_MESSAGE_MAX_LENGTH];
  int readIndex;
  int writeIndex;
  int count;
} radioMessageQueue = {
  .readIndex = 0,
  .writeIndex = 0,
  .count = 0
};

// Current message transmission state
bool isTransmittingMessage = false;
int messageCharIndex = 0;
unsigned long messageTransmissionTimer = 0;
char currentTransmittingMessage[RADIO_MESSAGE_MAX_LENGTH] = "";

// Iambic keyer state for Summit Keyer mode
bool radioKeyerActive = false;
bool radioSendingDit = false;
bool radioSendingDah = false;
bool radioInSpacing = false;
bool radioDitMemory = false;
bool radioDahMemory = false;
unsigned long radioDitDahTimer = 0;
unsigned long radioElementStartTime = 0;
int radioDitDuration = 0;

// Preferences for radio mode
Preferences radioPrefs;

// Forward declarations
void startRadioOutput(LGFX &display);
void drawRadioOutputUI(LGFX &display);
int handleRadioOutputInput(char key, LGFX &display);
void updateRadioOutput();
void saveRadioSettings();
void loadRadioSettings();
void radioStraightKeyHandler();
void radioIambicKeyerHandler();
bool queueRadioMessage(const char* message);
void processRadioMessageQueue();
void playMorseCharViaRadio(char c);

// Load radio settings from flash
void loadRadioSettings() {
  radioPrefs.begin("radio", true);
  radioMode = (RadioMode)radioPrefs.getInt("mode", RADIO_MODE_SUMMIT_KEYER);
  radioPrefs.end();

  Serial.print("Radio settings loaded: Mode = ");
  Serial.println(radioMode == RADIO_MODE_SUMMIT_KEYER ? "Summit Keyer" : "Radio Keyer");
}

// Save radio settings to flash
void saveRadioSettings() {
  radioPrefs.begin("radio", false);
  radioPrefs.putInt("mode", (int)radioMode);
  radioPrefs.end();

  Serial.println("Radio settings saved");
}

// Start radio output mode
void startRadioOutput(LGFX &display) {
  radioOutputActive = true;
  radioSettingSelection = 0;
  radioKeyerActive = false;
  radioInSpacing = false;
  radioDitMemory = false;
  radioDahMemory = false;

  // Load radio settings
  loadRadioSettings();

  // Calculate dit duration from current CW speed setting
  radioDitDuration = DIT_DURATION(cwSpeed);

  // Set radio output pins as outputs
  pinMode(RADIO_KEY_DIT_PIN, OUTPUT);
  pinMode(RADIO_KEY_DAH_PIN, OUTPUT);
  digitalWrite(RADIO_KEY_DIT_PIN, LOW);  // Not keyed
  digitalWrite(RADIO_KEY_DAH_PIN, LOW);  // Not keyed

  drawRadioOutputUI(display);
}

// Draw radio output UI
void drawRadioOutputUI(LGFX &display) {
  // Clear screen (preserve header)
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Modern card container
  int cardX = 20;
  int cardY = 55;
  int cardW = SCREEN_WIDTH - 40;
  int cardH = 150;

  display.fillRoundRect(cardX, cardY, cardW, cardH, 12, 0x1082); // Dark blue fill
  display.drawRoundRect(cardX, cardY, cardW, cardH, 12, 0x34BF); // Light blue outline

  // Setting 0: Speed (WPM)
  int yPos = cardY + 15;
  bool isSelected = (radioSettingSelection == 0);

  if (isSelected) {
    display.fillRoundRect(cardX + 8, yPos, cardW - 16, 38, 8, 0x249F); // Blue highlight
  }

  display.setTextSize(1);
  display.setTextColor(isSelected ? ST77XX_WHITE : 0x7BEF); // Light gray
  display.setCursor(cardX + 15, yPos + 8);
  display.print("Speed");

  display.setTextSize(2);
  display.setTextColor(isSelected ? ST77XX_WHITE : ST77XX_CYAN);
  display.setCursor(cardX + 15, yPos + 20);
  display.print(cwSpeed);
  display.print(" WPM");

  // Setting 1: Key Type
  yPos += 45;
  isSelected = (radioSettingSelection == 1);

  if (isSelected) {
    display.fillRoundRect(cardX + 8, yPos, cardW - 16, 38, 8, 0x249F); // Blue highlight
  }

  display.setTextSize(1);
  display.setTextColor(isSelected ? ST77XX_WHITE : 0x7BEF); // Light gray
  display.setCursor(cardX + 15, yPos + 8);
  display.print("Key Type");

  display.setTextSize(2);
  display.setTextColor(isSelected ? ST77XX_WHITE : ST77XX_CYAN);
  display.setCursor(cardX + 15, yPos + 20);

  if (cwKeyType == KEY_STRAIGHT) {
    display.print("Straight");
  } else if (cwKeyType == KEY_IAMBIC_A) {
    display.print("Iambic A");
  } else {
    display.print("Iambic B");
  }

  // Setting 2: Radio Mode
  yPos += 45;
  isSelected = (radioSettingSelection == 2);

  if (isSelected) {
    display.fillRoundRect(cardX + 8, yPos, cardW - 16, 38, 8, 0x249F); // Blue highlight
  }

  display.setTextSize(1);
  display.setTextColor(isSelected ? ST77XX_WHITE : 0x7BEF); // Light gray
  display.setCursor(cardX + 15, yPos + 8);
  display.print("Radio Mode");

  display.setTextSize(2);
  display.setTextColor(isSelected ? ST77XX_WHITE : ST77XX_CYAN);
  display.setCursor(cardX + 15, yPos + 20);

  if (radioMode == RADIO_MODE_SUMMIT_KEYER) {
    display.print("Summit Keyer");
  } else {
    display.print("Radio Keyer");
  }

  // Footer with instructions
  display.setTextSize(1);
  display.setTextColor(COLOR_WARNING);
  String helpText = "\x18\x19 Select  \x1B\x1A Adjust  M Memories  ESC Back";
  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(display, helpText.c_str(), 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, SCREEN_HEIGHT - 12);
  display.print(helpText);
}

// Draw memory selector overlay
void drawMemorySelector(LGFX &display) {
  // Semi-transparent overlay effect (just draw over existing content)
  display.fillRoundRect(30, 50, SCREEN_WIDTH - 60, 160, 12, 0x0841);  // Very dark blue
  display.drawRoundRect(30, 50, SCREEN_WIDTH - 60, 160, 12, 0x34BF);  // Light blue outline

  // Title
  display.setTextSize(1);
  display.setTextColor(ST77XX_CYAN);
  int16_t x1, y1;
  uint16_t w, h;
  String title = "CW MEMORIES";
  getTextBounds_compat(display, title.c_str(), 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, 65);
  display.print(title);

  // Draw memory list (show 5 at a time, scrollable)
  int startY = 85;
  int itemHeight = 22;
  int visibleItems = 5;
  int scrollOffset = (memorySelectorSelection > 4) ? (memorySelectorSelection - 4) : 0;

  for (int i = 0; i < visibleItems && (scrollOffset + i) < CW_MEMORY_MAX_SLOTS; i++) {
    int slot = scrollOffset + i;
    int yPos = startY + (i * itemHeight);

    bool isSelected = (slot == memorySelectorSelection);

    // Highlight selected item
    if (isSelected) {
      display.fillRoundRect(40, yPos - 2, SCREEN_WIDTH - 80, itemHeight - 3, 6, 0x249F);
    }

    // Draw slot number and label
    display.setTextSize(1);
    display.setTextColor(isSelected ? ST77XX_WHITE : ST77XX_CYAN);
    display.setCursor(50, yPos + 6);
    display.print(slot + 1);
    display.print(". ");

    if (cwMemories[slot].isEmpty) {
      display.setTextColor(isSelected ? 0xC618 : 0x7BEF);  // Gray
      display.print("(empty)");
    } else {
      display.setTextColor(isSelected ? ST77XX_WHITE : ST77XX_CYAN);
      // Truncate label if too long
      if (strlen(cwMemories[slot].label) > 20) {
        char truncated[21];
        strlcpy(truncated, cwMemories[slot].label, 18);
        strcat(truncated, "...");
        display.print(truncated);
      } else {
        display.print(cwMemories[slot].label);
      }
    }
  }

  // Footer instructions
  display.setTextSize(1);
  display.setTextColor(COLOR_WARNING);
  String helpText = "\x18\x19 Select  ENTER Send  ESC Cancel";
  getTextBounds_compat(display, helpText.c_str(), 0, 0, &x1, &y1, &w, &h);
  centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, 195);
  display.print(helpText);
}

// Handle radio output input
int handleRadioOutputInput(char key, LGFX &display) {
  // Handle memory selector if active
  if (memorySelectorActive) {
    if (key == KEY_UP) {
      if (memorySelectorSelection > 0) {
        memorySelectorSelection--;
        drawMemorySelector(display);
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 0;  // Don't request full redraw - just redrew overlay
      }
    }
    else if (key == KEY_DOWN) {
      if (memorySelectorSelection < CW_MEMORY_MAX_SLOTS - 1) {
        memorySelectorSelection++;
        drawMemorySelector(display);
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 0;  // Don't request full redraw - just redrew overlay
      }
    }
    else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
      // Send selected memory
      if (!cwMemories[memorySelectorSelection].isEmpty) {
        bool success = queueRadioMessage(cwMemories[memorySelectorSelection].message);
        memorySelectorActive = false;
        drawRadioOutputUI(display);

        if (success) {
          beep(TONE_SUCCESS, BEEP_MEDIUM);
        } else {
          beep(TONE_ERROR, BEEP_LONG);
        }
        return 2;
      } else {
        // Empty slot - show error
        beep(TONE_ERROR, BEEP_SHORT);
        return 0;
      }
    }
    else if (key == KEY_ESC) {
      // Cancel memory selector
      memorySelectorActive = false;
      drawRadioOutputUI(display);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;
    }
    return 0;
  }

  // Check for 'M' key to open memory selector
  if (key == 'm' || key == 'M') {
    memorySelectorActive = true;
    memorySelectorSelection = 0;
    drawMemorySelector(display);
    beep(TONE_SELECT, BEEP_SHORT);
    return 0;  // Don't request redraw - overlay is already drawn
  }

  // Navigation
  if (key == KEY_UP) {
    if (radioSettingSelection > 0) {
      radioSettingSelection--;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2; // Redraw requested
    }
  }
  else if (key == KEY_DOWN) {
    if (radioSettingSelection < RADIO_SETTINGS_COUNT - 1) {
      radioSettingSelection++;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2; // Redraw requested
    }
  }
  // Adjust values
  else if (key == KEY_LEFT || key == KEY_RIGHT) {
    bool increase = (key == KEY_RIGHT);
    bool changed = false;

    if (radioSettingSelection == 0) {
      // Adjust speed
      if (increase && cwSpeed < WPM_MAX) {
        cwSpeed++;
        changed = true;
      } else if (!increase && cwSpeed > WPM_MIN) {
        cwSpeed--;
        changed = true;
      }
      if (changed) {
        radioDitDuration = DIT_DURATION(cwSpeed);
        saveCWSettings();
      }
    }
    else if (radioSettingSelection == 1) {
      // Cycle key type
      if (increase) {
        if (cwKeyType == KEY_STRAIGHT) cwKeyType = KEY_IAMBIC_A;
        else if (cwKeyType == KEY_IAMBIC_A) cwKeyType = KEY_IAMBIC_B;
        else cwKeyType = KEY_STRAIGHT;
      } else {
        if (cwKeyType == KEY_STRAIGHT) cwKeyType = KEY_IAMBIC_B;
        else if (cwKeyType == KEY_IAMBIC_A) cwKeyType = KEY_STRAIGHT;
        else cwKeyType = KEY_IAMBIC_A;
      }
      saveCWSettings();
      changed = true;
    }
    else if (radioSettingSelection == 2) {
      // Toggle radio mode
      if (radioMode == RADIO_MODE_SUMMIT_KEYER) {
        radioMode = RADIO_MODE_RADIO_KEYER;
      } else {
        radioMode = RADIO_MODE_SUMMIT_KEYER;
      }
      saveRadioSettings();
      changed = true;
    }

    if (changed) {
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2; // Redraw requested
    }
  }
  else if (key == KEY_ESC) {
    // Exit radio output mode
    radioOutputActive = false;

    // Release radio keying outputs
    digitalWrite(RADIO_KEY_DIT_PIN, LOW);
    digitalWrite(RADIO_KEY_DAH_PIN, LOW);

    return -1; // Exit to radio menu
  }

  return 0; // Normal input processed
}

// Update radio output (called from main loop)
void updateRadioOutput() {
  if (!radioOutputActive) return;

  // Process message queue first (for web-based transmission)
  processRadioMessageQueue();

  // Read paddle/key inputs (physical and capacitive touch)
  bool ditPressed = (digitalRead(DIT_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DIT_PIN) > TOUCH_THRESHOLD);
  bool dahPressed = (digitalRead(DAH_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DAH_PIN) > TOUCH_THRESHOLD);

  if (radioMode == RADIO_MODE_SUMMIT_KEYER) {
    // Summit Keyer mode: Do keying logic on Summit, output straight key format

    if (cwKeyType == KEY_STRAIGHT) {
      radioStraightKeyHandler();
    } else {
      radioIambicKeyerHandler();
    }

  } else {
    // Radio Keyer mode: Passthrough contacts to radio

    if (cwKeyType == KEY_STRAIGHT) {
      // Straight key: output on DIT pin
      digitalWrite(RADIO_KEY_DIT_PIN, ditPressed ? HIGH : LOW);
      digitalWrite(RADIO_KEY_DAH_PIN, LOW);
    } else {
      // Iambic: pass both dit and dah to radio
      digitalWrite(RADIO_KEY_DIT_PIN, ditPressed ? HIGH : LOW);
      digitalWrite(RADIO_KEY_DAH_PIN, dahPressed ? HIGH : LOW);
    }
  }
}

// Straight key handler for Summit Keyer mode
void radioStraightKeyHandler() {
  bool ditPressed = (digitalRead(DIT_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DIT_PIN) > TOUCH_THRESHOLD);

  // Output straight key format on DIT pin
  digitalWrite(RADIO_KEY_DIT_PIN, ditPressed ? HIGH : LOW);
  digitalWrite(RADIO_KEY_DAH_PIN, LOW);
}

// Iambic keyer handler for Summit Keyer mode
void radioIambicKeyerHandler() {
  bool ditPressed = (digitalRead(DIT_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DIT_PIN) > TOUCH_THRESHOLD);
  bool dahPressed = (digitalRead(DAH_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DAH_PIN) > TOUCH_THRESHOLD);

  unsigned long currentTime = millis();

  if (!radioKeyerActive && !radioInSpacing) {
    // IDLE state - check for paddle presses
    if (ditPressed || dahPressed) {
      // Start sending element
      if (ditPressed) {
        radioSendingDit = true;
        radioSendingDah = false;
      } else {
        radioSendingDit = false;
        radioSendingDah = true;
      }

      radioKeyerActive = true;
      radioDitDahTimer = currentTime + (radioSendingDit ? radioDitDuration : radioDitDuration * 3);
      radioElementStartTime = currentTime;

      // Debug
      Serial.print("Keyer: Starting ");
      Serial.print(radioSendingDit ? "DIT" : "DAH");
      Serial.print(" duration=");
      Serial.println(radioSendingDit ? radioDitDuration : radioDitDuration * 3);

      // Key radio output (straight key format on DIT pin)
      digitalWrite(RADIO_KEY_DIT_PIN, HIGH);
      digitalWrite(RADIO_KEY_DAH_PIN, LOW);
    }
  }
  else if (radioKeyerActive) {
    // SENDING state - outputting dit or dah

    // Check for memory paddle presses
    if (ditPressed && !radioSendingDit) radioDitMemory = true;
    if (dahPressed && !radioSendingDah) radioDahMemory = true;

    // Check if element duration completed
    if (currentTime >= radioDitDahTimer) {
      // Release key output
      digitalWrite(RADIO_KEY_DIT_PIN, LOW);
      digitalWrite(RADIO_KEY_DAH_PIN, LOW);

      // Enter spacing state
      radioKeyerActive = false;
      radioInSpacing = true;
      radioDitDahTimer = currentTime + radioDitDuration; // Element gap
    }
  }
  else if (radioInSpacing) {
    // SPACING state - inter-element gap

    // Check for memory paddle presses
    if (ditPressed && !radioSendingDit) radioDitMemory = true;
    if (dahPressed && !radioSendingDah) radioDahMemory = true;

    // Check if spacing completed
    if (currentTime >= radioDitDahTimer) {
      radioInSpacing = false;

      // Check for queued element (memory or opposite paddle for iambic B)
      bool sendNextElement = false;
      bool nextIsDit = false;

      if (cwKeyType == KEY_IAMBIC_B) {
        // Iambic B: alternate on squeeze
        if (radioDitMemory && radioDahMemory) {
          // Both paddles - send opposite of what we just sent
          nextIsDit = !radioSendingDit;
          sendNextElement = true;
        } else if (radioDitMemory) {
          nextIsDit = true;
          sendNextElement = true;
        } else if (radioDahMemory) {
          nextIsDit = false;
          sendNextElement = true;
        }
      } else {
        // Iambic A: memory only
        if (radioDitMemory) {
          nextIsDit = true;
          sendNextElement = true;
        } else if (radioDahMemory) {
          nextIsDit = false;
          sendNextElement = true;
        }
      }

      if (sendNextElement) {
        // Start next element
        radioSendingDit = nextIsDit;
        radioSendingDah = !nextIsDit;
        radioKeyerActive = true;
        radioDitDahTimer = currentTime + (nextIsDit ? radioDitDuration : radioDitDuration * 3);
        radioElementStartTime = currentTime;

        // Clear memory for sent element
        if (nextIsDit) radioDitMemory = false;
        else radioDahMemory = false;

        // Key radio output
        digitalWrite(RADIO_KEY_DIT_PIN, HIGH);
        digitalWrite(RADIO_KEY_DAH_PIN, LOW);
      } else {
        // No queued element - return to idle
        radioDitMemory = false;
        radioDahMemory = false;
      }
    }
  }
}

// Queue a message for radio transmission
bool queueRadioMessage(const char* message) {
  if (radioMessageQueue.count >= RADIO_MESSAGE_QUEUE_SIZE) {
    return false; // Queue is full
  }

  // Copy message to queue
  strlcpy(radioMessageQueue.messages[radioMessageQueue.writeIndex], message, RADIO_MESSAGE_MAX_LENGTH);

  // Advance write index (circular buffer)
  radioMessageQueue.writeIndex = (radioMessageQueue.writeIndex + 1) % RADIO_MESSAGE_QUEUE_SIZE;
  radioMessageQueue.count++;

  Serial.print("Message queued (");
  Serial.print(radioMessageQueue.count);
  Serial.print(" in queue): '");
  Serial.print(message);
  Serial.print("' Length=");
  Serial.println(strlen(message));

  return true;
}

// Play a single morse character via radio output
void playMorseCharViaRadio(char c) {
  const char* pattern = getMorseCode(c);
  if (pattern == nullptr) {
    return; // Skip unknown characters
  }

  MorseTiming timing(cwSpeed);

  // Play each element in the pattern
  for (int i = 0; pattern[i] != '\0'; i++) {
    int duration;
    if (pattern[i] == '.') {
      duration = timing.ditDuration;
    } else if (pattern[i] == '-') {
      duration = timing.dahDuration;
    } else {
      continue;
    }

    // Key the radio output (DIT pin for straight key format)
    digitalWrite(RADIO_KEY_DIT_PIN, HIGH);
    delay(duration);
    digitalWrite(RADIO_KEY_DIT_PIN, LOW);

    // Gap between elements (unless last element)
    if (pattern[i + 1] != '\0') {
      delay(timing.elementGap);
    }
  }
}

// Process radio message queue (called from updateRadioOutput)
void processRadioMessageQueue() {
  // Only process queue if we're in radio output mode
  if (!radioOutputActive) {
    return;
  }

  // Don't start new message if user is keying OR if keyer state machine is active
  bool ditPressed = (digitalRead(DIT_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DIT_PIN) > TOUCH_THRESHOLD);
  bool dahPressed = (digitalRead(DAH_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DAH_PIN) > TOUCH_THRESHOLD);

  // In Summit Keyer mode, also check if keyer state machine is busy
  if (radioMode == RADIO_MODE_SUMMIT_KEYER) {
    if (ditPressed || dahPressed || radioKeyerActive || radioInSpacing) {
      return; // Wait for keyer to be completely idle
    }
  } else {
    // In Radio Keyer mode, just check paddle states
    if (ditPressed || dahPressed) {
      return; // Wait for user to finish keying
    }
  }

  // If not currently transmitting, check if there's a message in the queue
  if (!isTransmittingMessage) {
    if (radioMessageQueue.count > 0) {
      // Dequeue message
      strlcpy(currentTransmittingMessage, radioMessageQueue.messages[radioMessageQueue.readIndex], RADIO_MESSAGE_MAX_LENGTH);
      radioMessageQueue.readIndex = (radioMessageQueue.readIndex + 1) % RADIO_MESSAGE_QUEUE_SIZE;
      radioMessageQueue.count--;

      // Start transmission
      isTransmittingMessage = true;
      messageCharIndex = 0;
      messageTransmissionTimer = millis(); // Start immediately

      Serial.print("Starting transmission: '");
      Serial.print(currentTransmittingMessage);
      Serial.print("' Length=");
      Serial.println(strlen(currentTransmittingMessage));
    }
  } else {
    // Continue transmitting current message
    unsigned long currentTime = millis();

    // Check if it's time to send the next character
    if (currentTime >= messageTransmissionTimer) {
      if (messageCharIndex < strlen(currentTransmittingMessage)) {
        char c = currentTransmittingMessage[messageCharIndex];

        if (c == ' ') {
          // Word gap (7 dits total, but we already have letter gap from previous character, so add 4 more dits)
          MorseTiming timing(cwSpeed);
          messageTransmissionTimer = currentTime + (timing.ditDuration * 4);
        } else {
          // Play morse character (this blocks during transmission)
          unsigned long charStartTime = millis();
          playMorseCharViaRadio(c);
          unsigned long charEndTime = millis();

          // Calculate how long the character took to send
          unsigned long charDuration = charEndTime - charStartTime;

          // Add letter gap after the character
          MorseTiming timing(cwSpeed);
          messageTransmissionTimer = charEndTime + timing.letterGap;

          Serial.print("Sent: ");
          Serial.print(c);
          Serial.print(" (took ");
          Serial.print(charDuration);
          Serial.print("ms, next at ");
          Serial.print(messageTransmissionTimer);
          Serial.println("ms)");
        }

        messageCharIndex++;
      } else {
        // Message complete
        isTransmittingMessage = false;
        Serial.println("Transmission complete");

        // Add extra delay before starting next message
        messageTransmissionTimer = currentTime + (DIT_DURATION(cwSpeed) * 7);
      }
    }
  }
}

#endif // RADIO_OUTPUT_H
