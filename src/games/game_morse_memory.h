/*
 * Morse Memory Game (Memory Chain)
 * Progressive memory game where players must remember and reproduce
 * increasingly long sequences of Morse code characters
 */

#ifndef GAME_MORSE_MEMORY_H
#define GAME_MORSE_MEMORY_H

#include <Preferences.h>
#include "../core/config.h"
#include "../core/morse_code.h"
#include "../audio/i2s_audio.h"
#include "../audio/morse_decoder_adaptive.h"

// ============================================
// Game Constants
// ============================================

#define MEMORY_MAX_SEQUENCE 99        // Maximum chain length
#define MEMORY_CHAR_TIMEOUT 2000      // ms to wait for player to finish character
#define MEMORY_ROUND_PAUSE 1500       // ms pause between rounds
#define MEMORY_FEEDBACK_DURATION 800  // ms to show correct/wrong feedback

// Character sets for different difficulty levels
const char MEMORY_CHARSET_BEGINNER[] = "ETIANMSURWDKGOHVFLPJBXCYZQ";
const char MEMORY_CHARSET_INTERMEDIATE[] = "ETIANMSURWDKGOHVFLPJBXCYZQ0123456789";
const char MEMORY_CHARSET_ADVANCED[] = "ETIANMSURWDKGOHVFLPJBXCYZQ0123456789";  // Will add prosigns in code

// ============================================
// Game State Enums
// ============================================

enum MemoryGameState {
  MEMORY_STATE_READY,      // Waiting to start
  MEMORY_STATE_PLAYING,    // Game sends sequence
  MEMORY_STATE_LISTENING,  // Player's turn to reproduce
  MEMORY_STATE_FEEDBACK,   // Showing correct/wrong
  MEMORY_STATE_GAME_OVER   // Game ended
};

enum MemoryDifficulty {
  MEMORY_DIFF_BEGINNER,      // Letters only
  MEMORY_DIFF_INTERMEDIATE,  // Letters + Numbers
  MEMORY_DIFF_ADVANCED       // Letters + Numbers + Prosigns
};

enum MemoryGameMode {
  MEMORY_MODE_STANDARD,  // One mistake = game over
  MEMORY_MODE_PRACTICE,  // 3 lives
  MEMORY_MODE_TIMED      // 60 second challenge
};

// ============================================
// Game State Structures
// ============================================

struct MemoryGameSettings {
  MemoryDifficulty difficulty;
  MemoryGameMode mode;
  int wpm;
  bool soundEnabled;
  bool showHints;  // Show character sequence on screen
};

struct MemoryGameData {
  char sequence[MEMORY_MAX_SEQUENCE];  // The sequence to remember
  int sequenceLength;                  // Current length of sequence
  int playerPosition;                  // Where player is in reproduction
  int lives;                           // Remaining lives (practice mode)
  int score;                           // Current score (chain length)
  int highScore;                       // Best score this session
  int allTimeBest;                     // All-time high score
  unsigned long roundStartTime;        // Timer for timed mode
  unsigned long stateStartTime;        // When current state began
  unsigned long lastInputTime;         // Last time player sent something
  MemoryGameState state;
  bool waitingForInput;
  bool sequenceCorrect;
  String lastDecodedChar;              // Last character player sent
};

// ============================================
// Global State Variables
// ============================================

MemoryGameSettings memorySettings = {
  MEMORY_DIFF_BEGINNER,
  MEMORY_MODE_STANDARD,
  15,    // 15 WPM default
  true,  // Sound on
  false  // Hints off (true memory training)
};

MemoryGameData memoryGame;
MorseDecoderAdaptive memoryDecoder(15, 20, 30);  // WPM, buffer size 30
bool memoryLastToneState = false;
unsigned long memoryLastStateChangeTime = 0;
bool memoryNeedsUIUpdate = false;  // Flag to request UI redraw

// Iambic keyer state variables (same as practice mode)
bool memoryKeyerActive = false;
bool memorySendingDit = false;
bool memorySendingDah = false;
bool memoryInSpacing = false;
bool memoryDitMemory = false;
bool memoryDahMemory = false;
unsigned long memoryElementStartTime = 0;

// Settings menu state
bool inMemorySettings = false;
int memorySettingsSelection = 0;  // 0=Difficulty, 1=Mode, 2=Speed, 3=Sound, 4=Hints, 5=Back

// Preferences for persistent storage
Preferences memoryPrefs;

// ============================================
// Character Set Functions
// ============================================

/*
 * Get the appropriate character set based on difficulty
 */
const char* getMemoryCharset() {
  switch (memorySettings.difficulty) {
    case MEMORY_DIFF_BEGINNER:
      return MEMORY_CHARSET_BEGINNER;
    case MEMORY_DIFF_INTERMEDIATE:
      return MEMORY_CHARSET_INTERMEDIATE;
    case MEMORY_DIFF_ADVANCED:
      return MEMORY_CHARSET_ADVANCED;
    default:
      return MEMORY_CHARSET_BEGINNER;
  }
}

/*
 * Get random character from current character set
 */
char getRandomMemoryChar() {
  const char* charset = getMemoryCharset();
  int len = strlen(charset);

  // For advanced mode, occasionally add prosigns
  if (memorySettings.difficulty == MEMORY_DIFF_ADVANCED && random(100) < 10) {
    // 10% chance of prosign
    const char* prosigns[] = {"<AR>", "<SK>", "<BK>", "<BT>"};
    int idx = random(4);
    // Return first char of prosign name (we'll handle display specially)
    // For now, just return regular chars - prosign support can be added later
    return charset[random(len)];
  }

  return charset[random(len)];
}

// ============================================
// Preferences Management
// ============================================

/*
 * Load game settings and high scores from flash
 */
void loadMemorySettings() {
  memoryPrefs.begin("memory", false);

  memorySettings.difficulty = (MemoryDifficulty)memoryPrefs.getInt("difficulty", MEMORY_DIFF_BEGINNER);
  memorySettings.mode = (MemoryGameMode)memoryPrefs.getInt("mode", MEMORY_MODE_STANDARD);
  memorySettings.wpm = memoryPrefs.getInt("wpm", 15);
  memorySettings.soundEnabled = memoryPrefs.getBool("sound", true);
  memorySettings.showHints = memoryPrefs.getBool("hints", false);  // Default to OFF for true memory training

  memoryGame.allTimeBest = memoryPrefs.getInt("highscore", 0);

  memoryPrefs.end();
}

/*
 * Save game settings to flash
 */
void saveMemorySettings() {
  memoryPrefs.begin("memory", false);

  memoryPrefs.putInt("difficulty", (int)memorySettings.difficulty);
  memoryPrefs.putInt("mode", (int)memorySettings.mode);
  memoryPrefs.putInt("wpm", memorySettings.wpm);
  memoryPrefs.putBool("sound", memorySettings.soundEnabled);
  memoryPrefs.putBool("hints", memorySettings.showHints);

  memoryPrefs.end();
}

/*
 * Save high score to flash
 */
void saveMemoryHighScore() {
  memoryPrefs.begin("memory", false);
  memoryPrefs.putInt("highscore", memoryGame.allTimeBest);
  memoryPrefs.end();
}

// ============================================
// Game Logic Functions
// ============================================

/*
 * Initialize/reset the game
 */
void resetMemoryGame() {
  memset(memoryGame.sequence, 0, sizeof(memoryGame.sequence));
  memoryGame.sequenceLength = 0;
  memoryGame.playerPosition = 0;
  memoryGame.score = 0;
  memoryGame.highScore = 0;
  memoryGame.state = MEMORY_STATE_READY;
  memoryGame.waitingForInput = false;
  memoryGame.sequenceCorrect = true;
  memoryGame.lastDecodedChar = "";
  memoryGame.roundStartTime = millis();
  memoryGame.stateStartTime = millis();
  memoryGame.lastInputTime = millis();

  // Set lives based on mode
  if (memorySettings.mode == MEMORY_MODE_PRACTICE) {
    memoryGame.lives = 3;
  } else {
    memoryGame.lives = 1;
  }

  // Reset decoder
  memoryDecoder.reset();
  memoryDecoder.flush();
  memoryDecoder.setWPM(memorySettings.wpm);
  memoryLastToneState = false;
  memoryLastStateChangeTime = 0;
}

/*
 * Add a new character to the sequence
 */
void addMemorySequenceChar() {
  if (memoryGame.sequenceLength < MEMORY_MAX_SEQUENCE) {
    memoryGame.sequence[memoryGame.sequenceLength] = getRandomMemoryChar();
    memoryGame.sequenceLength++;
    memoryGame.sequence[memoryGame.sequenceLength] = '\0';  // Null terminate
  }
}

/*
 * Play the current sequence through the buzzer
 */
void playMemorySequence() {
  if (!memorySettings.soundEnabled) return;

  // Use external cwTone variable from settings_cw.h
  extern int cwTone;

  MorseTiming timing(memorySettings.wpm);

  for (int i = 0; i < memoryGame.sequenceLength; i++) {
    char c = memoryGame.sequence[i];

    // Use the proper playMorseChar function for smooth audio
    playMorseChar(c, memorySettings.wpm, cwTone);

    // Gap between letters (unless last character)
    if (i < memoryGame.sequenceLength - 1) {
      delay(timing.letterGap);
    }
  }
}

/*
 * Check if player's input matches the expected sequence
 */
bool checkPlayerSequence(char inputChar) {
  if (memoryGame.playerPosition >= memoryGame.sequenceLength) {
    return false;  // Too many characters
  }

  char expected = memoryGame.sequence[memoryGame.playerPosition];
  memoryGame.playerPosition++;

  return (inputChar == expected);
}

/*
 * Handle correct answer
 */
void handleCorrectAnswer() {
  memoryGame.sequenceCorrect = true;
  memoryGame.state = MEMORY_STATE_FEEDBACK;
  memoryGame.stateStartTime = millis();
  memoryGame.score = memoryGame.sequenceLength;

  // Update high scores
  if (memoryGame.score > memoryGame.highScore) {
    memoryGame.highScore = memoryGame.score;
  }
  if (memoryGame.score > memoryGame.allTimeBest) {
    memoryGame.allTimeBest = memoryGame.score;
    saveMemoryHighScore();
  }

  // Request UI update to show "CORRECT!" feedback
  memoryNeedsUIUpdate = true;
}

/*
 * Handle wrong answer
 */
void handleWrongAnswer() {
  memoryGame.sequenceCorrect = false;
  memoryGame.state = MEMORY_STATE_FEEDBACK;
  memoryGame.stateStartTime = millis();

  // Request UI update to show "WRONG!" feedback
  memoryNeedsUIUpdate = true;

  // Deduct a life
  memoryGame.lives--;

  if (memoryGame.lives <= 0) {
    memoryGame.state = MEMORY_STATE_GAME_OVER;
  }
}

/*
 * Start next round
 */
void startNextRound() {
  addMemorySequenceChar();
  memoryGame.playerPosition = 0;
  memoryGame.state = MEMORY_STATE_PLAYING;
  memoryGame.stateStartTime = millis();
  memoryGame.lastDecodedChar = "";

  // Reset decoder for clean input
  memoryDecoder.reset();
  memoryDecoder.flush();
}

// ============================================
// Display Functions
// ============================================

/*
 * Draw the game header
 */
void drawMemoryHeader(LGFX& tft) {
  tft.fillRect(0, 0, SCREEN_WIDTH, 40, COLOR_TITLE);
  tft.setFont(nullptr);  // Use default font
  tft.setTextColor(COLOR_BACKGROUND);
  tft.setTextSize(2);  // Larger size for better readability

  tft.setCursor(10, 28);
  tft.print("MEMORY CHAIN");

  // Show chain length on right
  String chainStr = "Chain: " + String(memoryGame.sequenceLength);
  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(tft, chainStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(SCREEN_WIDTH - w - 10, 28);
  tft.print(chainStr);
}

/*
 * Draw lives indicator (practice mode)
 */
void drawMemoryLives(LGFX& tft, int y) {
  if (memorySettings.mode != MEMORY_MODE_PRACTICE) return;

  tft.setFont(nullptr);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_TEXT);

  String livesStr = "";
  for (int i = 0; i < memoryGame.lives; i++) {
    livesStr += "♥ ";
  }

  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(tft, livesStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;

  tft.setCursor(x, y);
  tft.print(livesStr);
}

/*
 * Draw the main game UI
 */
void drawMemoryGameUI(LGFX& tft) {
  tft.fillScreen(COLOR_BACKGROUND);
  drawMemoryHeader(tft);

  tft.setFont(nullptr);
  tft.setTextSize(2);

  // State indicator
  int centerY = 80;
  String stateText = "";
  uint16_t stateColor = COLOR_TEXT;

  switch (memoryGame.state) {
    case MEMORY_STATE_READY:
      stateText = "READY";
      stateColor = COLOR_SUCCESS;
      break;
    case MEMORY_STATE_PLAYING:
      stateText = "LISTEN";
      stateColor = COLOR_TITLE;
      break;
    case MEMORY_STATE_LISTENING:
      stateText = "YOUR TURN";
      stateColor = COLOR_WARNING;
      break;
    case MEMORY_STATE_FEEDBACK:
      if (memoryGame.sequenceCorrect) {
        stateText = "CORRECT!";
        stateColor = COLOR_SUCCESS;
      } else {
        stateText = "WRONG!";
        stateColor = COLOR_ERROR;
      }
      break;
    case MEMORY_STATE_GAME_OVER:
      stateText = "GAME OVER";
      stateColor = COLOR_ERROR;
      break;
  }

  // Center the state text
  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(tft, stateText.c_str(), 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;

  tft.fillRect(0, centerY - 5, SCREEN_WIDTH, h + 10, stateColor);
  tft.setTextColor(COLOR_BACKGROUND);
  tft.setCursor(x, centerY);
  tft.print(stateText);

  // Show sequence during playback (only if hints enabled)
  if (memorySettings.showHints && (memoryGame.state == MEMORY_STATE_PLAYING || memoryGame.state == MEMORY_STATE_LISTENING)) {
    tft.setTextColor(ST77XX_CYAN);  // Use lighter color for visual aid
    tft.setTextSize(1);
    String seqStr = "";
    for (int i = 0; i < memoryGame.sequenceLength; i++) {
      seqStr += memoryGame.sequence[i];
      seqStr += " ";
    }
    getTextBounds_compat(tft, seqStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    x = (SCREEN_WIDTH - w) / 2;
    tft.setCursor(x, centerY + 40);
    tft.print(seqStr);

    // Show player progress
    if (memoryGame.state == MEMORY_STATE_LISTENING && memoryGame.playerPosition > 0) {
      tft.setTextColor(COLOR_SUCCESS);
      String progressStr = "Sent: " + String(memoryGame.playerPosition) + "/" + String(memoryGame.sequenceLength);
      getTextBounds_compat(tft, progressStr.c_str(), 0, 0, &x1, &y1, &w, &h);
      x = (SCREEN_WIDTH - w) / 2;
      tft.setCursor(x, centerY + 60);
      tft.print(progressStr);
    }
  }

  // Show lives (practice mode)
  if (memorySettings.mode == MEMORY_MODE_PRACTICE) {
    drawMemoryLives(tft, 140);
  }

  // Show scores at bottom
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);

  String scoreStr = "Score: " + String(memoryGame.score);
  tft.setCursor(10, 180);
  tft.print(scoreStr);

  String highStr = "Best: " + String(memoryGame.allTimeBest);
  getTextBounds_compat(tft, highStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(SCREEN_WIDTH - w - 10, 180);
  tft.print(highStr);

  // Instructions
  tft.setTextColor(ST77XX_CYAN);
  String instrStr = "ESC=Menu  S=Settings";
  getTextBounds_compat(tft, instrStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  x = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(x, 210);
  tft.print(instrStr);
}

/*
 * Draw game over screen
 */
void drawMemoryGameOver(LGFX& tft) {
  tft.fillScreen(COLOR_BACKGROUND);
  drawMemoryHeader(tft);

  tft.setFont(nullptr);
  tft.setTextSize(3);
  tft.setTextColor(COLOR_ERROR);

  String gameOverStr = "GAME OVER";
  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(tft, gameOverStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(x, 70);
  tft.print(gameOverStr);

  // Final score
  tft.setTextSize(2);
  tft.setTextColor(COLOR_TEXT);

  String finalStr = "Final Chain: " + String(memoryGame.score);
  getTextBounds_compat(tft, finalStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  x = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(x, 110);
  tft.print(finalStr);

  // High scores
  tft.setTextSize(1);

  String sessionStr = "Session Best: " + String(memoryGame.highScore);
  getTextBounds_compat(tft, sessionStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  x = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(x, 140);
  tft.print(sessionStr);

  String allTimeStr = "All-Time Best: " + String(memoryGame.allTimeBest);
  getTextBounds_compat(tft, allTimeStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  x = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(x, 160);
  tft.print(allTimeStr);

  // Options
  tft.setTextColor(COLOR_TITLE);
  tft.setTextSize(1);

  String playAgainStr = "ENTER = Play Again";
  getTextBounds_compat(tft, playAgainStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  x = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(x, 190);
  tft.print(playAgainStr);

  String menuStr = "ESC = Main Menu";
  getTextBounds_compat(tft, menuStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  x = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(x, 210);
  tft.print(menuStr);
}

/*
 * Draw settings menu
 */
void drawMemorySettings(LGFX& tft) {
  tft.fillScreen(COLOR_BACKGROUND);

  // Header
  tft.fillRect(0, 0, SCREEN_WIDTH, 40, COLOR_TITLE);
  tft.setFont(nullptr);  // Use default font
  tft.setTextColor(COLOR_BACKGROUND);
  tft.setTextSize(2);  // Larger size for better readability
  tft.setCursor(10, 28);
  tft.print("GAME SETTINGS");

  tft.setFont(nullptr);
  tft.setTextSize(1);

  int y = 60;
  int lineHeight = 25;

  // Difficulty
  tft.setTextColor(memorySettingsSelection == 0 ? COLOR_TITLE : COLOR_TEXT);
  tft.setCursor(20, y);
  tft.print("Difficulty: ");
  tft.setTextColor(memorySettingsSelection == 0 ? COLOR_WARNING : ST77XX_CYAN);
  String diffStr = "";
  switch (memorySettings.difficulty) {
    case MEMORY_DIFF_BEGINNER: diffStr = "Beginner"; break;
    case MEMORY_DIFF_INTERMEDIATE: diffStr = "Intermediate"; break;
    case MEMORY_DIFF_ADVANCED: diffStr = "Advanced"; break;
  }
  tft.print(diffStr);

  y += lineHeight;

  // Mode
  tft.setTextColor(memorySettingsSelection == 1 ? COLOR_TITLE : COLOR_TEXT);
  tft.setCursor(20, y);
  tft.print("Mode: ");
  tft.setTextColor(memorySettingsSelection == 1 ? COLOR_WARNING : ST77XX_CYAN);
  String modeStr = "";
  switch (memorySettings.mode) {
    case MEMORY_MODE_STANDARD: modeStr = "Standard"; break;
    case MEMORY_MODE_PRACTICE: modeStr = "Practice (3 Lives)"; break;
    case MEMORY_MODE_TIMED: modeStr = "Timed (60s)"; break;
  }
  tft.print(modeStr);

  y += lineHeight;

  // Speed
  tft.setTextColor(memorySettingsSelection == 2 ? COLOR_TITLE : COLOR_TEXT);
  tft.setCursor(20, y);
  tft.print("Speed: ");
  tft.setTextColor(memorySettingsSelection == 2 ? COLOR_WARNING : ST77XX_CYAN);
  tft.print(String(memorySettings.wpm) + " WPM");

  y += lineHeight;

  // Sound
  tft.setTextColor(memorySettingsSelection == 3 ? COLOR_TITLE : COLOR_TEXT);
  tft.setCursor(20, y);
  tft.print("Sound: ");
  tft.setTextColor(memorySettingsSelection == 3 ? COLOR_WARNING : ST77XX_CYAN);
  tft.print(memorySettings.soundEnabled ? "ON" : "OFF");

  y += lineHeight;

  // Show Hints
  tft.setTextColor(memorySettingsSelection == 4 ? COLOR_TITLE : COLOR_TEXT);
  tft.setCursor(20, y);
  tft.print("Show Hints: ");
  tft.setTextColor(memorySettingsSelection == 4 ? COLOR_WARNING : ST77XX_CYAN);
  tft.print(memorySettings.showHints ? "ON" : "OFF");

  y += lineHeight + 10;

  // Save & Return
  tft.setTextColor(memorySettingsSelection == 5 ? COLOR_SUCCESS : COLOR_TEXT);
  int16_t x1, y1;
  uint16_t w, h;
  String saveStr = "< Save & Return >";
  getTextBounds_compat(tft, saveStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(x, y);
  tft.print(saveStr);

  // Instructions
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(1);
  tft.setCursor(20, 200);
  tft.print("Up/Down = Navigate");
  tft.setCursor(20, 215);
  tft.print("Left/Right = Change");
}

// ============================================
// Settings Input Handler
// ============================================

int handleMemorySettingsInput(char key, LGFX& tft) {
  if (key == 0x1B) {  // ESC - save and exit
    saveMemorySettings();
    inMemorySettings = false;
    drawMemoryGameUI(tft);
    return 0;
  }

  if (key == 0xB5) {  // Up arrow
    memorySettingsSelection--;
    if (memorySettingsSelection < 0) memorySettingsSelection = 5;
    drawMemorySettings(tft);
    return 0;
  }

  if (key == 0xB6) {  // Down arrow
    memorySettingsSelection++;
    if (memorySettingsSelection > 5) memorySettingsSelection = 0;
    drawMemorySettings(tft);
    return 0;
  }

  if (key == 0xB4 || key == 0x08) {  // Left arrow or backspace
    switch (memorySettingsSelection) {
      case 0:  // Difficulty
        if (memorySettings.difficulty > MEMORY_DIFF_BEGINNER) {
          memorySettings.difficulty = (MemoryDifficulty)(memorySettings.difficulty - 1);
        }
        break;
      case 1:  // Mode
        if (memorySettings.mode > MEMORY_MODE_STANDARD) {
          memorySettings.mode = (MemoryGameMode)(memorySettings.mode - 1);
        }
        break;
      case 2:  // Speed
        if (memorySettings.wpm > 5) {
          memorySettings.wpm -= 5;
        }
        break;
      case 3:  // Sound
        memorySettings.soundEnabled = !memorySettings.soundEnabled;
        break;
      case 4:  // Show Hints
        memorySettings.showHints = !memorySettings.showHints;
        break;
    }
    drawMemorySettings(tft);
    return 0;
  }

  if (key == 0xB7 || key == ' ') {  // Right arrow or space
    switch (memorySettingsSelection) {
      case 0:  // Difficulty
        if (memorySettings.difficulty < MEMORY_DIFF_ADVANCED) {
          memorySettings.difficulty = (MemoryDifficulty)(memorySettings.difficulty + 1);
        }
        break;
      case 1:  // Mode
        if (memorySettings.mode < MEMORY_MODE_TIMED) {
          memorySettings.mode = (MemoryGameMode)(memorySettings.mode + 1);
        }
        break;
      case 2:  // Speed
        if (memorySettings.wpm < 40) {
          memorySettings.wpm += 5;
        }
        break;
      case 3:  // Sound
        memorySettings.soundEnabled = !memorySettings.soundEnabled;
        break;
      case 4:  // Show Hints
        memorySettings.showHints = !memorySettings.showHints;
        break;
    }
    drawMemorySettings(tft);
    return 0;
  }

  if (key == 0x0D || key == 0x0A) {  // Enter - select
    if (memorySettingsSelection == 5) {  // Save & Return
      saveMemorySettings();
      inMemorySettings = false;
      drawMemoryGameUI(tft);
    }
    return 0;
  }

  return 0;
}

// ============================================
// Main Game Functions
// ============================================

/*
 * Reset keyer state variables
 */
void resetMemoryKeyerState() {
  memoryKeyerActive = false;
  memorySendingDit = false;
  memorySendingDah = false;
  memoryInSpacing = false;
  memoryDitMemory = false;
  memoryDahMemory = false;
  memoryLastToneState = false;
  memoryLastStateChangeTime = 0;
}

/*
 * Start the Memory game
 */
void startMemoryGame(LGFX& tft) {
  loadMemorySettings();
  resetMemoryGame();
  resetMemoryKeyerState();
  inMemorySettings = false;
  memorySettingsSelection = 0;

  // UI is now handled by LVGL - see lv_game_screens.h
}

/*
 * Update game state (call every loop)
 */
void updateMemoryGame() {
  unsigned long now = millis();

  // State machine
  switch (memoryGame.state) {
    case MEMORY_STATE_READY:
      // Waiting for first round to start
      if (now - memoryGame.stateStartTime > 1000) {
        startNextRound();
        playMemorySequence();
        delay(500);  // Brief pause after playback
        memoryGame.state = MEMORY_STATE_LISTENING;
        memoryGame.stateStartTime = now;
        memoryGame.lastInputTime = now;
        resetMemoryKeyerState();  // Reset keyer state for clean input
        memoryNeedsUIUpdate = true;  // Request UI update to show "YOUR TURN"
      }
      break;

    case MEMORY_STATE_PLAYING:
      // Sequence is playing, handled by playMemorySequence
      break;

    case MEMORY_STATE_LISTENING:
      // Flush decoder after character gap to finalize decoding
      if (memoryLastStateChangeTime > 0 && !isTonePlaying()) {
        unsigned long timeSinceLastInput = now - memoryLastStateChangeTime;
        // Use 3x dit duration as character gap (approx 700ms at 15 WPM)
        extern int cwSpeed;
        MorseTiming timing(cwSpeed);
        float charGapDuration = timing.ditDuration * 5;  // Conservative character gap

        if (timeSinceLastInput > charGapDuration) {
          // Flush decoder to finalize any pending character
          memoryDecoder.flush();
          memoryLastStateChangeTime = 0;  // Prevent repeated flushes
        }
      }

      // Player is sending - timeout check for complete sequence
      if (memoryGame.playerPosition > 0) {
        if (now - memoryGame.lastInputTime > MEMORY_CHAR_TIMEOUT) {
          // Timeout - assume sequence is done
          if (memoryGame.playerPosition == memoryGame.sequenceLength) {
            handleCorrectAnswer();
            memoryNeedsUIUpdate = true;
          } else {
            handleWrongAnswer();
            memoryNeedsUIUpdate = true;
          }
        }
      }
      break;

    case MEMORY_STATE_FEEDBACK:
      // Show feedback briefly, then continue or end
      {
        static bool feedbackSoundPlayed = false;

        // Play sound once when entering feedback state
        if (!feedbackSoundPlayed) {
          if (memorySettings.soundEnabled) {
            if (memoryGame.sequenceCorrect) {
              // Check for new high score
              if (memoryGame.score > memoryGame.allTimeBest) {
                // Play victory fanfare for new all-time best
                beep(800, 100);
                delay(100);
                beep(1000, 100);
                delay(100);
                beep(1200, 200);
              } else {
                // Play correct beep
                beep(1000, 200);
              }
            } else {
              // Play wrong buzz
              beep(200, 300);
            }
          }
          feedbackSoundPlayed = true;
        }

        // After feedback duration, move to next state
        if (now - memoryGame.stateStartTime > MEMORY_FEEDBACK_DURATION) {
          feedbackSoundPlayed = false;  // Reset for next time

          if (memoryGame.sequenceCorrect) {
            // Next round - add pause before playing sequence
            delay(MEMORY_ROUND_PAUSE);
            startNextRound();
            playMemorySequence();
            delay(500);
            memoryGame.state = MEMORY_STATE_LISTENING;
            memoryGame.stateStartTime = now;
            memoryGame.lastInputTime = now;
            resetMemoryKeyerState();  // Reset keyer state for clean input
            memoryNeedsUIUpdate = true;  // Request UI update
          } else {
            // Wrong answer - check if game over
            if (memoryGame.state == MEMORY_STATE_GAME_OVER) {
              memoryNeedsUIUpdate = true;  // Show game over screen
            } else {
              // Continue in practice mode - replay same sequence
              delay(MEMORY_ROUND_PAUSE);
              playMemorySequence();
              delay(500);
              memoryGame.state = MEMORY_STATE_LISTENING;
              memoryGame.playerPosition = 0;  // Retry same sequence
              memoryGame.stateStartTime = now;
              memoryGame.lastInputTime = now;
              resetMemoryKeyerState();  // Reset keyer state for clean input
              memoryNeedsUIUpdate = true;  // Request UI update
            }
          }
        }
      }
      break;

    case MEMORY_STATE_GAME_OVER:
      // Nothing to update, waiting for player input
      break;
  }
}

/*
 * Handle keyboard input during game
 */
int handleMemoryGameInput(char key, LGFX& tft) {
  // Settings menu
  if (inMemorySettings) {
    return handleMemorySettingsInput(key, tft);
  }

  // Game over state
  if (memoryGame.state == MEMORY_STATE_GAME_OVER) {
    if (key == 0x0D || key == 0x0A) {  // Enter - play again
      resetMemoryGame();
      drawMemoryGameUI(tft);
      startNextRound();
      playMemorySequence();
      delay(500);
      memoryGame.state = MEMORY_STATE_LISTENING;
      memoryGame.stateStartTime = millis();
      memoryGame.lastInputTime = millis();
      return 0;
    } else if (key == 0x1B) {  // ESC - exit to menu
      return -1;
    }
    return 0;
  }

  // Settings key
  if (key == 's' || key == 'S') {
    inMemorySettings = true;
    memorySettingsSelection = 0;
    drawMemorySettings(tft);
    return 0;
  }

  // ESC - exit to menu
  if (key == 0x1B) {
    return -1;
  }

  return 0;
}

/*
 * Straight key handler for Memory Chain game
 * Same pattern as practice mode's straightKeyHandler
 */
void memoryStraightKeyHandler(bool ditPressed, bool dahPressed) {
  // Use external cwTone variable from settings_cw.h
  extern int cwTone;

  unsigned long currentTime = millis();
  bool toneOn = isTonePlaying();

  // Use DIT pin as straight key
  if (ditPressed && !toneOn) {
    // Tone starting
    if (memoryLastToneState == false) {
      // Send silence duration to decoder (negative)
      if (memoryLastStateChangeTime > 0) {
        float silenceDuration = currentTime - memoryLastStateChangeTime;
        if (silenceDuration > 0) {
          memoryDecoder.addTiming(-silenceDuration);
        }
      }
      memoryLastStateChangeTime = currentTime;
      memoryLastToneState = true;
    }
    startTone(cwTone);
  }
  else if (ditPressed && toneOn) {
    // Tone continuing
    continueTone(cwTone);
  }
  else if (!ditPressed && toneOn) {
    // Tone stopping
    if (memoryLastToneState == true) {
      // Send tone duration to decoder (positive)
      float toneDuration = currentTime - memoryLastStateChangeTime;
      if (toneDuration > 0) {
        memoryDecoder.addTiming(toneDuration);
      }
      memoryLastStateChangeTime = currentTime;
      memoryLastToneState = false;
    }
    stopTone();
  }
}

/*
 * Iambic keyer handler for Memory Chain game
 * Same pattern as practice mode's iambicKeyerHandler
 */
void memoryIambicKeyerHandler(bool ditPressed, bool dahPressed) {
  // Use external cwTone variable from settings_cw.h
  extern int cwTone;
  extern int cwSpeed;

  unsigned long currentTime = millis();
  MorseTiming timing(cwSpeed);
  int ditDuration = timing.ditDuration;

  // If not actively sending or spacing, check for new input
  if (!memoryKeyerActive && !memoryInSpacing) {
    if (ditPressed || memoryDitMemory) {
      // Start sending dit
      if (memoryLastToneState == false) {
        if (memoryLastStateChangeTime > 0) {
          float silenceDuration = currentTime - memoryLastStateChangeTime;
          if (silenceDuration > 0) {
            memoryDecoder.addTiming(-silenceDuration);
          }
        }
        memoryLastStateChangeTime = currentTime;
        memoryLastToneState = true;
      }

      memoryKeyerActive = true;
      memorySendingDit = true;
      memorySendingDah = false;
      memoryInSpacing = false;
      memoryElementStartTime = currentTime;
      startTone(cwTone);

      // Clear dit memory
      memoryDitMemory = false;
    }
    else if (dahPressed || memoryDahMemory) {
      // Start sending dah
      if (memoryLastToneState == false) {
        if (memoryLastStateChangeTime > 0) {
          float silenceDuration = currentTime - memoryLastStateChangeTime;
          if (silenceDuration > 0) {
            memoryDecoder.addTiming(-silenceDuration);
          }
        }
        memoryLastStateChangeTime = currentTime;
        memoryLastToneState = true;
      }

      memoryKeyerActive = true;
      memorySendingDit = false;
      memorySendingDah = true;
      memoryInSpacing = false;
      memoryElementStartTime = currentTime;
      startTone(cwTone);

      // Clear dah memory
      memoryDahMemory = false;
    }
  }
  // Currently sending an element
  else if (memoryKeyerActive && !memoryInSpacing) {
    unsigned long elementDuration = memorySendingDit ? ditDuration : (ditDuration * 3);

    // Keep tone playing
    continueTone(cwTone);

    // Continuously check for paddle input during element send
    if (ditPressed && dahPressed) {
      // Both pressed (squeeze) - remember opposite paddle
      if (memorySendingDit) {
        memoryDahMemory = true;
      } else {
        memoryDitMemory = true;
      }
    }
    else if (memorySendingDit && dahPressed) {
      // Sending dit, dah pressed
      memoryDahMemory = true;
    }
    else if (memorySendingDah && ditPressed) {
      // Sending dah, dit pressed
      memoryDitMemory = true;
    }

    // Check if element is complete
    if (currentTime - memoryElementStartTime >= elementDuration) {
      // Element complete, turn off tone and start spacing
      if (memoryLastToneState == true) {
        // Send tone duration to decoder
        float toneDuration = currentTime - memoryLastStateChangeTime;
        if (toneDuration > 0) {
          memoryDecoder.addTiming(toneDuration);
        }
        memoryLastStateChangeTime = currentTime;
        memoryLastToneState = false;
      }

      stopTone();
      memoryKeyerActive = false;
      memorySendingDit = false;
      memorySendingDah = false;
      memoryInSpacing = true;
      memoryElementStartTime = currentTime;  // Reset timer for spacing
    }
  }
  // In inter-element spacing
  else if (memoryInSpacing) {
    // Continue checking paddles during spacing to catch input
    if (ditPressed && dahPressed) {
      memoryDitMemory = true;
      memoryDahMemory = true;
    }
    else if (ditPressed && !memoryDitMemory) {
      memoryDitMemory = true;
    }
    else if (dahPressed && !memoryDahMemory) {
      memoryDahMemory = true;
    }

    // Wait for 1 dit duration (inter-element gap)
    if (currentTime - memoryElementStartTime >= ditDuration) {
      memoryInSpacing = false;
      // Now ready to send next element if memory is set or paddle still pressed
    }
  }
}

/*
 * Handle paddle input (dit/dah) during gameplay
 * Routes to appropriate handler based on key type setting
 */
void handleMemoryPaddleInput(bool ditPressed, bool dahPressed) {
  if (memoryGame.state != MEMORY_STATE_LISTENING) {
    return;  // Only accept input during listening state
  }

  // Use external key type setting from settings_cw.h
  extern KeyType cwKeyType;

  // Set up callback to handle decoded messages (one-time setup)
  static bool callbackSetup = false;
  if (!callbackSetup) {
    memoryDecoder.messageCallback = [](String morse, String text) {
      // Only process first character of decoded text
      if (text.length() > 0) {
        char decoded = text[0];
        Serial.printf("Memory: Decoded character: '%c' from morse: %s\n", decoded, morse.c_str());
        memoryGame.lastDecodedChar = String(decoded);
        memoryGame.lastInputTime = millis();

        // Check if it matches expected character
        bool correct = checkPlayerSequence(decoded);

        if (!correct) {
          Serial.println("Memory: WRONG answer");
          handleWrongAnswer();
        } else if (memoryGame.playerPosition >= memoryGame.sequenceLength) {
          // Completed the sequence!
          Serial.println("Memory: Sequence COMPLETE!");
          handleCorrectAnswer();
        } else {
          Serial.printf("Memory: Correct so far (%d/%d)\n", memoryGame.playerPosition, memoryGame.sequenceLength);
        }
      }
    };
    callbackSetup = true;
  }

  // Route to appropriate handler based on key type
  if (cwKeyType == KEY_STRAIGHT) {
    memoryStraightKeyHandler(ditPressed, dahPressed);
  } else {
    memoryIambicKeyerHandler(ditPressed, dahPressed);
  }
}

/*
 * Draw the game UI (called from main loop)
 */
void drawMemoryUI(LGFX& tft) {
  if (inMemorySettings) {
    drawMemorySettings(tft);
  } else if (memoryGame.state == MEMORY_STATE_GAME_OVER) {
    drawMemoryGameOver(tft);
  } else {
    drawMemoryGameUI(tft);
  }
}

#endif // GAME_MORSE_MEMORY_H
