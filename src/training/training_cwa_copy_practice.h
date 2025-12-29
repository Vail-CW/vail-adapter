/*
 * CW Academy Training - Copy Practice Mode
 * Receive morse code and type what you hear
 */

#ifndef TRAINING_CWA_COPY_PRACTICE_H
#define TRAINING_CWA_COPY_PRACTICE_H

#include "training_cwa_core.h"  // Same folder
#include "training_cwa_data.h"  // Same folder

// Forward declaration for header drawing
extern void drawHeader();

// ============================================
// Copy Practice State
// ============================================

String cwaCopyTarget = "";              // What was sent (correct answer)
String cwaCopyInput = "";               // What user typed
int cwaCopyRound = 0;                   // Current round number
int cwaCopyCorrect = 0;                 // Number correct this session
int cwaCopyTotal = 0;                   // Total attempts this session
int cwaCopyCharCount = 1;               // Number of characters per round (1-10, adjustable)
bool cwaCopyWaitingForInput = false;    // Waiting for user to type
bool cwaCopyShowingFeedback = false;    // Showing correct/incorrect feedback

// ============================================
// Content Generation
// ============================================

/*
 * Generate random content based on message type and session
 */
String generateCWAContent() {
  int sessionIndex = cwaSelectedSession - 1;

  if (sessionIndex < 0 || sessionIndex >= 10) {
    // Sessions 11+ use all characters
    sessionIndex = 9; // Use session 10 data
  }

  switch (cwaSelectedMessageType) {
    case MESSAGE_CHARACTERS: {
      // Generate random characters from session's character set
      String charSet = String(cwaSessionCharSets[sessionIndex]);
      String result = "";
      for (int i = 0; i < cwaCopyCharCount; i++) {
        int index = random(charSet.length());
        result += charSet[index];
      }
      return result;
    }

    case MESSAGE_WORDS: {
      // Select random words from the session's word list
      const char** words = session_words[sessionIndex];
      int numWords = max(1, cwaCopyCharCount / 4); // Roughly 1 word per 4 chars
      return selectRandomItems(words, numWords);
    }

    case MESSAGE_ABBREVIATIONS: {
      // Select random abbreviations from the session's abbreviation list
      const char** abbrevs = session_abbrev[sessionIndex];
      int numAbbrevs = max(1, cwaCopyCharCount / 3); // Roughly 1 abbrev per 3 chars
      return selectRandomItems(abbrevs, numAbbrevs);
    }

    case MESSAGE_NUMBERS: {
      // Select random numbers from the session's number list
      const char** numbers = session_numbers[sessionIndex];
      if (numbers == nullptr) {
        // Fall back to random characters if no numbers available
        String charSet = String(cwaSessionCharSets[sessionIndex]);
        String result = "";
        for (int i = 0; i < cwaCopyCharCount; i++) {
          int index = random(charSet.length());
          result += charSet[index];
        }
        return result;
      }
      int numNumbers = max(1, cwaCopyCharCount / 4); // Roughly 1 number per 4 chars
      return selectRandomItems(numbers, numNumbers);
    }

    case MESSAGE_CALLSIGNS: {
      // Select random callsigns from the session's callsign list
      const char** callsigns = session_callsigns[sessionIndex];
      if (callsigns == nullptr) {
        // Fall back to random characters if no callsigns available
        String charSet = String(cwaSessionCharSets[sessionIndex]);
        String result = "";
        for (int i = 0; i < cwaCopyCharCount; i++) {
          int index = random(charSet.length());
          result += charSet[index];
        }
        return result;
      }
      int numCallsigns = max(1, cwaCopyCharCount / 5); // Roughly 1 callsign per 5 chars
      return selectRandomItems(callsigns, numCallsigns);
    }

    case MESSAGE_PHRASES: {
      // Select one random phrase from the session's phrase list
      const char** phrases = session_phrases[sessionIndex];
      int arraySize = countArrayItems(phrases);
      if (arraySize == 0) {
        // Fall back to random characters
        String charSet = String(cwaSessionCharSets[sessionIndex]);
        String result = "";
        for (int i = 0; i < cwaCopyCharCount; i++) {
          int index = random(charSet.length());
          result += charSet[index];
        }
        return result;
      }
      int index = random(arraySize);
      return String(phrases[index]);
    }

    default:
      return "";
  }
}

// ============================================
// UI Functions
// ============================================

/*
 * Draw copy practice UI
 */
void drawCWACopyPracticeUI(LGFX& tft) {
  if (cwaUseLVGL) return;  // LVGL handles display
  tft.fillScreen(COLOR_BACKGROUND);
  drawHeader();

  // Score display and settings
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(10, 50);
  tft.print("Round: ");
  tft.print(cwaCopyRound);
  tft.print("/10");

  tft.setCursor(SCREEN_WIDTH - 100, 50);
  tft.print("Score: ");
  tft.print(cwaCopyCorrect);
  tft.print("/");
  tft.print(cwaCopyTotal);

  // Character count setting (in middle)
  tft.setCursor(130, 50);
  tft.print("Chars: ");
  tft.print(cwaCopyCharCount);

  if (cwaCopyShowingFeedback) {
    // Show what was sent and what was typed
    tft.setTextSize(1);
    tft.setTextColor(0x7BEF);
    tft.setCursor(20, 80);
    tft.print("Sent:");

    tft.setTextSize(3);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(20, 100);
    tft.print(cwaCopyTarget);

    tft.setTextSize(1);
    tft.setTextColor(0x7BEF);
    tft.setCursor(20, 140);
    tft.print("You typed:");

    tft.setTextSize(3);
    bool correct = (cwaCopyInput.equalsIgnoreCase(cwaCopyTarget));
    tft.setTextColor(correct ? ST77XX_GREEN : ST77XX_RED);
    tft.setCursor(20, 160);
    tft.print(cwaCopyInput);

    // Feedback message
    tft.setTextSize(2);
    tft.setTextColor(correct ? ST77XX_GREEN : ST77XX_RED);
    tft.setCursor(20, 195);
    tft.print(correct ? "CORRECT!" : "INCORRECT");

  } else if (cwaCopyWaitingForInput) {
    // Show input prompt
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_CYAN);
    tft.setCursor(20, 90);
    tft.print("Type what you heard:");

    // Show current input
    tft.fillRect(20, 120, 440, 50, 0x1082);  // Input box (scaled for 480px width)
    tft.drawRect(20, 120, 440, 50, 0x34BF);

    tft.setTextSize(3);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(25, 135);
    tft.print(cwaCopyInput);

  } else {
    // Show "Get ready" message before playing morse
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_CYAN);
    tft.setCursor(30, 90);
    tft.print("Type what you hear:");

    // Show empty input box ready for typing
    tft.fillRect(20, 120, 440, 50, 0x1082);  // Input box (scaled for 480px width)
    tft.drawRect(20, 120, 440, 50, 0x34BF);

    tft.setTextSize(1);
    tft.setTextColor(0x7BEF);
    tft.setCursor(80, 180);
    tft.print("Listening...");
  }

  // Footer
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING);
  String helpText;
  if (cwaCopyShowingFeedback) {
    helpText = "Any key: Continue  \x18\x19 Chars  ESC Exit";
  } else if (cwaCopyWaitingForInput) {
    helpText = "SPACE Replay  ENTER Submit  \x18\x19 Chars  ESC";
  } else {
    helpText = "\x18\x19 Adjust chars  ESC Exit";
  }
  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(tft, helpText.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT - 12);
  tft.print(helpText);
}

// ============================================
// Round Management
// ============================================

/*
 * Start a new round of practice
 */
void startCWACopyRound(LGFX& tft) {
  cwaCopyRound++;
  cwaCopyInput = "";
  cwaCopyTarget = generateCWAContent();
  cwaCopyWaitingForInput = false;
  cwaCopyShowingFeedback = false;

  // Draw the UI FIRST showing the ready state
  drawCWACopyPracticeUI(tft);

  // Then play the morse code after a brief delay
  delay(1000);
  playMorseString(cwaCopyTarget.c_str(), cwSpeed, cwTone);

  // Now ready for input
  cwaCopyWaitingForInput = true;
  drawCWACopyPracticeUI(tft);  // Redraw to show input prompt
}

/*
 * Start copy practice mode
 */
void startCWACopyPractice(LGFX& tft) {
  cwaCopyRound = 0;
  cwaCopyCorrect = 0;
  cwaCopyTotal = 0;
  cwaCopyInput = "";
  cwaCopyWaitingForInput = false;
  cwaCopyShowingFeedback = false;

  randomSeed(analogRead(0));  // Seed random number generator

  startCWACopyRound(tft);
}

// ============================================
// Input Handler
// ============================================

/*
 * Handle input for copy practice mode
 * Returns: -1 to exit, 0 for normal input, 2 for redraw
 */
int handleCWACopyPracticeInput(char key, LGFX& tft) {
  // ESC always exits
  if (key == KEY_ESC) {
    return -1;  // Return to message type selection
  }

  // Handle character count adjustment with UP/DOWN arrows (1-10)
  if (key == KEY_UP) {
    if (cwaCopyCharCount < 10) {
      cwaCopyCharCount++;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;  // Redraw
    }
  } else if (key == KEY_DOWN) {
    if (cwaCopyCharCount > 1) {
      cwaCopyCharCount--;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;  // Redraw
    }
  }

  // If showing feedback, any key starts next round
  if (cwaCopyShowingFeedback) {
    if (cwaCopyRound >= 10) {
      // Finished all rounds, show final score
      tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);
      tft.setTextSize(2);
      tft.setTextColor(ST77XX_CYAN);
      tft.setCursor(40, 80);
      tft.print("Practice Complete!");

      tft.setTextSize(3);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(60, 120);
      tft.print("Score: ");
      tft.print(cwaCopyCorrect);
      tft.print("/");
      tft.print(cwaCopyTotal);

      int percentage = (cwaCopyTotal > 0) ? (cwaCopyCorrect * 100 / cwaCopyTotal) : 0;
      tft.setTextSize(2);
      tft.setTextColor(percentage >= 70 ? ST77XX_GREEN : ST77XX_YELLOW);
      tft.setCursor(90, 160);
      tft.print(percentage);
      tft.print("%");

      tft.setTextSize(1);
      tft.setTextColor(0x7BEF);
      tft.setCursor(60, 200);
      tft.print("Press any key to exit...");

      delay(3000);
      return -1;  // Exit to message type selection
    } else {
      startCWACopyRound(tft);  // This handles its own UI drawing
      return 0;  // Already handled, no redraw needed
    }
  }

  // Waiting for input
  if (cwaCopyWaitingForInput) {
    if (key == ' ') {  // Space bar for replay
      // Replay the morse code
      playMorseString(cwaCopyTarget.c_str(), cwSpeed, cwTone);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 0;  // No redraw needed

    } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
      // Submit answer
      cwaCopyTotal++;
      if (cwaCopyInput.equalsIgnoreCase(cwaCopyTarget)) {
        cwaCopyCorrect++;
        beep(1000, 200);  // Success beep
      } else {
        beep(400, 300);  // Error beep
      }
      cwaCopyShowingFeedback = true;
      cwaCopyWaitingForInput = false;
      return 2;  // Redraw to show feedback

    } else if (key == 0x08 || key == 0x7F) {  // Backspace or DEL
      if (cwaCopyInput.length() > 0) {
        cwaCopyInput.remove(cwaCopyInput.length() - 1);
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 2;  // Redraw
      }
    } else if (key >= 33 && key <= 126) {  // Printable character (excluding space)
      if (cwaCopyInput.length() < 20) {  // Limit input length
        cwaCopyInput += (char)toupper(key);
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 2;  // Redraw
      }
    }
  }

  return 0;
}

#endif // TRAINING_CWA_COPY_PRACTICE_H
