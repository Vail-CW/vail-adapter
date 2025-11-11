/*
 * CW Academy Training - QSO Practice Mode
 * Sessions 11-13: Realistic QSO exchanges with personalization
 */

#ifndef TRAINING_CWA_QSO_PRACTICE_H
#define TRAINING_CWA_QSO_PRACTICE_H

#include "training_cwa_core.h"  // Same folder

// ============================================
// QSO Personalization Data
// ============================================

struct QSOPersonalization {
  String callsign;
  String name;
  String city;
  String state;
  String power;     // e.g., "100"
  String antenna;   // e.g., "VERT", "DIPOLE"
  int age;
};

QSOPersonalization qsoPersonal = {
  "W1AW",           // Default callsign
  "JOHN",           // Default name
  "NEWINGTON",      // Default city
  "CT",             // Default state
  "100",            // Default power
  "DIPOLE",         // Default antenna
  25                // Default age
};

// ============================================
// QSO Exchange Structures
// ============================================

struct QSOExchange {
  const char* otherCallsign;  // Callsign of other station
  const char* otherName;      // Name of other operator
  const char* otherLocation;  // Location (city, state)
  const char* rst;            // Signal report
  const char* messageTemplate; // Full exchange with placeholders
};

// ============================================
// Session 11: Basic QSO Exchanges
// ============================================

const QSOExchange session11_exchanges[] = {
  {
    "K6RB",
    "ROB",
    "SF CA",
    "579",
    "[call] DE K6RB UR RST 579 IN SF CA <BT> NAME ROB HW? [call] DE K6RB K"
  },
  {
    "N3JT",
    "JIM",
    "MCLEAN VA",
    "5NN",
    "[call] DE N3JT UR RST IS 5NN <BT> QTH MCLEAN VA <BT> OP IS JIM DE N3JT K"
  },
  {
    "W1RM",
    "PETE",
    "CT",
    "569",
    "[call] DE W1RM <BT> UR RST 569 IN CT NAME IS PETE <BK>"
  },
  {nullptr, nullptr, nullptr, nullptr, nullptr} // Terminator
};

const char* session11_responses[] = {
  "K6RB DE [call] UR RST 57N IN [city] [st] <BT> NAME IS [name] BTU K6RB DE [call] K",
  "N3JT DE [call] UR RST 56N IN [city] [st] <BT> NAME IS [name] N3JT DE [call] K",
  "W1RM DE [call] UR RST 45N WID QRN IN [city] [st] <BT> NAME IS [name] W1RM DE [call] K",
  nullptr
};

// ============================================
// Session 12: Weather & Equipment
// ============================================

const QSOExchange session12_exchanges[] = {
  {
    "K6RB",
    "ROB",
    "SF CA",
    "579",
    "[call] DE K6RB WX CLDY TEMP 58 <BT> RIG RUNS 100 W TO VERT <BT> AGE IS 66 SO HW? [call] DE K6RB K"
  },
  {
    "N3JT",
    "JIM",
    "MCLEAN VA",
    "5NN",
    "[call] DE N3JT <BT> WX RAIN TEMP 42 <BT> RIG IS K3 ES ANT IS 4 EL YAGI <BT> AGE IS 65 OK? DE N3JT K"
  },
  {
    "W1RM",
    "PETE",
    "CT",
    "569",
    "[call] DE W1RM WX SNOW TEMP 24 <BT> RIG IS IC 7700 PWR IS KW ES ANT IS DIPOLE <BT> AGE IS 70 HW? <BK>"
  },
  {nullptr, nullptr, nullptr, nullptr, nullptr}
};

const char* session12_responses[] = {
  "K6RB DE [call] WX SUNNY TEMP 82 RIG RUNS [pwr] W TO [ant] AGE IS [age] HW? K6RB DE [call] K",
  "N3JT DE [call] WX RAIN TEMP 54 RIG RUNS [pwr] W TO [ant] AGE IS [age] HW? N3JT DE [call] K",
  "W1RM DE [call] WX OC TEMP 70 RIG RUNS [pwr] W TO [ant] AGE IS [age] HW? W1RM DE [call] K",
  nullptr
};

// ============================================
// Session 13: QSO Closing
// ============================================

const QSOExchange session13_exchanges[] = {
  {
    "K6RB",
    "ROB",
    "SF CA",
    "579",
    "[call] DE K6RB TNX FER QSO ES HPE CU AGN 73 <SK> [call] DE K6RB E E"
  },
  {
    "N3JT",
    "JIM",
    "MCLEAN VA",
    "5NN",
    "[call] DE N3JT NICE QSO TNX CUL 73 <SK> [call] DE N3JT GN"
  },
  {
    "W1RM",
    "PETE",
    "CT",
    "569",
    "[call] DE W1RM ENJOYED QSO 73 <SK> [call] DE W1RM E E"
  },
  {nullptr, nullptr, nullptr, nullptr, nullptr}
};

const char* session13_responses[] = {
  "K6RB DE [call] TU FER QSO CU AGN 73 <SK> K6RB DE [call]",
  "N3JT DE [call] CUL ES NICE QSO 73 <SK> N3JT DE [call] E E",
  "W1RM DE [call] ENJOYED QSO ALSO 73 <SK> W1RM DE [call] CU E E",
  nullptr
};

// ============================================
// QSO Lookup Tables
// ============================================

const QSOExchange* qso_exchanges[] = {
  session11_exchanges,
  session12_exchanges,
  session13_exchanges
};

const char** qso_responses[] = {
  session11_responses,
  session12_responses,
  session13_responses
};

// ============================================
// Helper Functions
// ============================================

/*
 * Replace placeholders in QSO template with personalized data
 */
String personalizeQSOText(const char* template_text) {
  String result = String(template_text);

  // Replace placeholders
  result.replace("[call]", qsoPersonal.callsign);
  result.replace("[name]", qsoPersonal.name);
  result.replace("[city]", qsoPersonal.city);
  result.replace("[st]", qsoPersonal.state);
  result.replace("[pwr]", qsoPersonal.power);
  result.replace("[ant]", qsoPersonal.antenna);
  result.replace("[age]", String(qsoPersonal.age));

  return result;
}

/*
 * Load personalization data from Preferences
 * Falls back to QSO logger operator info if available
 */
void loadQSOPersonalization() {
  Preferences prefs;
  prefs.begin("qso_personal", false);

  // Try to load from QSO personalization namespace first
  qsoPersonal.callsign = prefs.getString("callsign", "");

  // If not set, try QSO operator namespace
  if (qsoPersonal.callsign.length() == 0) {
    prefs.end();
    prefs.begin("qso_operator", false);
    qsoPersonal.callsign = prefs.getString("callsign", "W1AW");
    qsoPersonal.name = prefs.getString("name", "JOHN");
    qsoPersonal.city = prefs.getString("city", "NEWINGTON");
    qsoPersonal.state = prefs.getString("state", "CT");
  } else {
    qsoPersonal.name = prefs.getString("name", "JOHN");
    qsoPersonal.city = prefs.getString("city", "NEWINGTON");
    qsoPersonal.state = prefs.getString("state", "CT");
  }

  qsoPersonal.power = prefs.getString("power", "100");
  qsoPersonal.antenna = prefs.getString("antenna", "DIPOLE");
  qsoPersonal.age = prefs.getInt("age", 25);

  prefs.end();
}

/*
 * Save personalization data to Preferences
 */
void saveQSOPersonalization() {
  Preferences prefs;
  prefs.begin("qso_personal", false);
  prefs.putString("callsign", qsoPersonal.callsign);
  prefs.putString("name", qsoPersonal.name);
  prefs.putString("city", qsoPersonal.city);
  prefs.putString("state", qsoPersonal.state);
  prefs.putString("power", qsoPersonal.power);
  prefs.putString("antenna", qsoPersonal.antenna);
  prefs.putInt("age", qsoPersonal.age);
  prefs.end();
}

// ============================================
// QSO Practice State
// ============================================

int qsoCurrentExchange = 0;
bool qsoPracticeActive = false;
String qsoReceivedText = "";
int qsoRound = 0;
int qsoPlaybackSpeed = 15;  // Start at 15 WPM, adjustable
bool qsoWaitingForInput = false;
String qsoUserInput = "";
String qsoExpectedResponse = "";
enum QSOState {
  QSO_READY,           // Ready to start
  QSO_PLAYING,         // Playing other station's message
  QSO_WAITING_INPUT,   // Waiting for user to type response
  QSO_SHOWING_FEEDBACK // Showing correct/incorrect feedback
};
QSOState qsoState = QSO_READY;

/*
 * Draw QSO Practice UI (Modern, preserving header)
 */
void drawCWAQSOPracticeUI(Adafruit_ST7789& tft) {
  // Get session index (11-13 maps to 0-2)
  int sessionIndex = cwaSelectedSession - 11;
  if (sessionIndex < 0 || sessionIndex > 2) {
    sessionIndex = 0; // Default to session 11
  }

  // Clear content area only (preserve header at top)
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Info bar below header - speed and round
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 48);
  tft.print("Speed: ");
  tft.setTextColor(ST77XX_CYAN);
  tft.print(qsoPlaybackSpeed);
  tft.setTextColor(ST77XX_WHITE);
  tft.print(" WPM");

  // Round indicator (right side)
  tft.setCursor(180, 48);
  tft.print("Round ");
  tft.setTextColor(ST77XX_CYAN);
  tft.print(qsoRound + 1);

  // Session info
  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);
  tft.setCursor(5, 63);
  tft.print("Session ");
  tft.print(cwaSelectedSession);
  tft.print(": ");
  if (sessionIndex == 0) {
    tft.print("Basic Exchange");
  } else if (sessionIndex == 1) {
    tft.print("Weather & Equipment");
  } else {
    tft.print("QSO Closing");
  }

  // Divider line
  tft.drawFastHLine(0, 78, SCREEN_WIDTH, 0x4208);

  // State-dependent display
  if (qsoState == QSO_READY) {
    // Ready state - show instructions
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(10, 50);
    tft.print("Your Station:");

    tft.setTextColor(ST77XX_CYAN);
    tft.setCursor(10, 65);
    tft.print(qsoPersonal.callsign);
    tft.print(" - ");
    tft.print(qsoPersonal.name);

    tft.setTextColor(0x7BEF);
    tft.setCursor(10, 80);
    tft.print(qsoPersonal.city);
    tft.print(", ");
    tft.print(qsoPersonal.state);

    tft.setTextSize(1);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(10, 110);
    tft.print("Listen to the other station");
    tft.setCursor(10, 125);
    tft.print("and type your response");

    tft.setTextColor(COLOR_WARNING);
    tft.setCursor(10, 150);
    tft.print("Press ENTER to start");

  } else if (qsoState == QSO_PLAYING) {
    // Playing state
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(60, 80);
    tft.print("LISTENING...");

    tft.setTextSize(1);
    tft.setTextColor(0x7BEF);
    tft.setCursor(40, 110);
    tft.print("Copying transmission");

  } else if (qsoState == QSO_WAITING_INPUT) {
    // Input state - show what was heard and prompt for response
    tft.setTextSize(1);
    tft.setTextColor(0x7BEF);
    tft.setCursor(10, 45);
    tft.print("Type your response:");

    // Input box (similar to practice mode)
    int boxY = 60;
    int boxH = 80;
    tft.fillRoundRect(10, boxY, SCREEN_WIDTH - 20, boxH, 8, 0x1082);
    tft.drawRoundRect(10, boxY, SCREEN_WIDTH - 20, boxH, 8, ST77XX_CYAN);

    // Display user input (word-wrapped)
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(20, boxY + 10);

    // Simple word wrap
    String displayText = qsoUserInput;
    if (displayText.length() > 0) {
      int lineWidth = 0;
      int yPos = boxY + 10;
      String currentLine = "";

      for (unsigned int i = 0; i < displayText.length(); i++) {
        currentLine += displayText[i];
        lineWidth += 12; // Approximate character width

        if (lineWidth > (SCREEN_WIDTH - 40) || displayText[i] == ' ') {
          tft.setCursor(20, yPos);
          tft.print(currentLine);
          currentLine = "";
          lineWidth = 0;
          yPos += 20;
          if (yPos > boxY + boxH - 20) break;
        }
      }
      if (currentLine.length() > 0 && yPos < boxY + boxH - 10) {
        tft.setCursor(20, yPos);
        tft.print(currentLine);
      }
    }

    // Cursor
    if (millis() % 1000 < 500) {
      int cursorX = 20 + (qsoUserInput.length() % 15) * 12;
      int cursorY = boxY + 10 + ((qsoUserInput.length() / 15) * 20);
      if (cursorY < boxY + boxH - 10) {
        tft.fillRect(cursorX, cursorY + 15, 10, 2, ST77XX_CYAN);
      }
    }

    tft.setTextSize(1);
    tft.setTextColor(COLOR_WARNING);
    tft.setCursor(10, 150);
    tft.print("ENTER=Submit  BKSP=Delete");

  } else if (qsoState == QSO_SHOWING_FEEDBACK) {
    // Feedback state
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(10, 50);
    tft.print("Expected Response:");

    // Show expected response in box
    int boxY = 65;
    tft.fillRoundRect(10, boxY, SCREEN_WIDTH - 20, 60, 8, 0x1082);
    tft.drawRoundRect(10, boxY, SCREEN_WIDTH - 20, 60, 8, ST77XX_GREEN);

    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(20, boxY + 10);

    // Word wrap expected response
    String expected = qsoExpectedResponse;
    int yPos = boxY + 10;
    int lineStart = 0;

    for (unsigned int i = 0; i < expected.length(); i++) {
      if (expected[i] == ' ' || i == expected.length() - 1) {
        String word = expected.substring(lineStart, i + 1);
        int16_t x1, y1;
        uint16_t w, h;
        tft.getTextBounds(word, 0, 0, &x1, &y1, &w, &h);

        if (w > SCREEN_WIDTH - 40) {
          yPos += 12;
          tft.setCursor(20, yPos);
        }
        tft.print(word);
        lineStart = i + 1;

        if (yPos > boxY + 45) break;
      }
    }

    tft.setTextSize(1);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(10, 140);
    tft.print("Press ENTER for next round");
  }

  // Footer help text
  tft.setTextSize(1);
  tft.setTextColor(0x4208);
  tft.setCursor(5, SCREEN_HEIGHT - 12);
  tft.print("\x18\x19 Speed  S=Settings  ESC=Exit");
}

/*
 * Start QSO Practice mode
 */
void startCWAQSOPractice(Adafruit_ST7789& tft) {
  // Load personalization
  loadQSOPersonalization();

  qsoPracticeActive = true;
  qsoCurrentExchange = 0;
  qsoRound = 0;
  qsoPlaybackSpeed = 15;  // Start at 15 WPM
  qsoState = QSO_READY;
  qsoUserInput = "";
  qsoExpectedResponse = "";

  // Draw initial UI
  drawCWAQSOPracticeUI(tft);

  Serial.println("QSO Practice mode started");
  Serial.print("Callsign: ");
  Serial.println(qsoPersonal.callsign);
  Serial.print("Speed: ");
  Serial.print(qsoPlaybackSpeed);
  Serial.println(" WPM");
}

/*
 * Play QSO exchange at current playback speed
 */
void playQSOExchange(int sessionIndex, int exchangeIndex, Adafruit_ST7789& tft) {
  const QSOExchange* exchange = &qso_exchanges[sessionIndex][exchangeIndex];

  if (exchange->messageTemplate == nullptr) {
    return; // Invalid exchange
  }

  // Personalize the text
  String qsoText = personalizeQSOText(exchange->messageTemplate);

  // Set state to playing and update UI
  qsoState = QSO_PLAYING;
  drawCWAQSOPracticeUI(tft);

  // Play at current playback speed
  playMorseString(qsoText.c_str(), qsoPlaybackSpeed, cwTone);

  // After playing, prepare expected response and move to input state
  const char** responses = qso_responses[sessionIndex];
  if (responses[exchangeIndex] != nullptr) {
    qsoExpectedResponse = personalizeQSOText(responses[exchangeIndex]);
  } else {
    qsoExpectedResponse = ""; // No response expected (shouldn't happen)
  }

  qsoUserInput = "";
  qsoState = QSO_WAITING_INPUT;
  drawCWAQSOPracticeUI(tft);
}

/*
 * Handle QSO practice input - Interactive copy practice
 */
int handleCWAQSOPracticeInput(char key, Adafruit_ST7789& tft) {
  int sessionIndex = cwaSelectedSession - 11;
  if (sessionIndex < 0 || sessionIndex > 2) {
    sessionIndex = 0;
  }

  if (key == KEY_ESC) {
    return -1; // Exit
  }

  // Speed control (always available)
  if (key == KEY_UP) {
    if (qsoPlaybackSpeed < 40) {
      qsoPlaybackSpeed++;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2; // Redraw
    }
  } else if (key == KEY_DOWN) {
    if (qsoPlaybackSpeed > 5) {
      qsoPlaybackSpeed--;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2; // Redraw
    }
  }

  // State-dependent input handling
  if (qsoState == QSO_READY) {
    // Ready state - ENTER starts the round
    if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
      beep(TONE_SELECT, BEEP_MEDIUM);
      playQSOExchange(sessionIndex, qsoCurrentExchange, tft);
      return 0;
    }

    // 'S' for station settings
    if (key == 'S' || key == 's') {
      // TODO: Show station info edit screen
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 0;
    }

  } else if (qsoState == QSO_WAITING_INPUT) {
    // Input state - collect user's response

    if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
      // Submit response and show feedback
      qsoState = QSO_SHOWING_FEEDBACK;
      beep(TONE_SELECT, BEEP_MEDIUM);
      return 2; // Redraw to show feedback
    }

    else if (key == KEY_BACKSPACE || key == 0x08) {
      // Delete last character
      if (qsoUserInput.length() > 0) {
        qsoUserInput.remove(qsoUserInput.length() - 1);
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 2; // Redraw
      }
    }

    else if (key >= 32 && key <= 126) {
      // Printable character - add to input
      qsoUserInput += (char)toupper(key);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2; // Redraw
    }

  } else if (qsoState == QSO_SHOWING_FEEDBACK) {
    // Feedback state - ENTER advances to next round
    if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
      // Move to next exchange
      qsoCurrentExchange++;
      if (qso_exchanges[sessionIndex][qsoCurrentExchange].messageTemplate == nullptr) {
        qsoCurrentExchange = 0; // Loop back to first exchange
        qsoRound++; // Increment round counter
      }

      // Reset to ready state
      qsoState = QSO_READY;
      qsoUserInput = "";
      qsoExpectedResponse = "";
      beep(TONE_SELECT, BEEP_MEDIUM);
      return 2; // Redraw
    }
  }

  return 0;
}

#endif // TRAINING_CWA_QSO_PRACTICE_H
