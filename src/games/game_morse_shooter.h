/*
 * Morse Shooter Game
 * Classic arcade-style game where players shoot falling letters using morse code
 */

#ifndef GAME_MORSE_SHOOTER_H
#define GAME_MORSE_SHOOTER_H

#include "../core/config.h"
#include "../core/morse_code.h"
#include "../audio/i2s_audio.h"
#include "../audio/morse_decoder_adaptive.h"
#include <Preferences.h>

// ============================================
// Game Constants
// ============================================

#define MAX_FALLING_LETTERS 5
#define GAME_UPDATE_INTERVAL 1000  // ms between game updates (1 second)
// Game ground level for LVGL layout (canvas is 240px tall, starts at y=40)
// Letters hit ground when y >= 200 in game coords (y=240 on screen, above bottom HUD)
#define GAME_GROUND_Y 200
#define MAX_LIVES 3               // Lives (letters that can hit ground)

// ============================================
// Difficulty System
// ============================================

enum ShooterDifficulty {
    SHOOTER_EASY = 0,
    SHOOTER_MEDIUM = 1,
    SHOOTER_HARD = 2
};

struct DifficultyParams {
    int spawnInterval;      // ms between spawns
    float fallSpeed;        // pixels per update
    const char* charset;    // available characters
    int charsetSize;
    int startLives;
    int scoreMultiplier;
    const char* name;       // display name
};

// Character sets for each difficulty
static const char CHARSET_EASY[] = "ETIANMS";
static const char CHARSET_MEDIUM[] = "ETIANMSURWDKGOHVFLPJBXCYZQ";
static const char CHARSET_HARD[] = "ETIANMSURWDKGOHVFLPJBXCYZQ0123456789";

// Difficulty parameters table
static const DifficultyParams DIFF_PARAMS[] = {
    { 4000, 0.5f, CHARSET_EASY,   7,  3, 1, "Easy" },
    { 3000, 1.0f, CHARSET_MEDIUM, 26, 3, 2, "Medium" },
    { 2000, 1.5f, CHARSET_HARD,   36, 3, 3, "Hard" }
};

// Current difficulty setting
static ShooterDifficulty shooterDifficulty = SHOOTER_MEDIUM;
static int shooterHighScores[3] = {0, 0, 0};  // Per difficulty

// ============================================
// Game State Structures
// ============================================

struct FallingLetter {
  char letter;
  float x;
  float y;
  bool active;
};

struct MorseInputBuffer {
  bool ditPressed;
  bool dahPressed;

  // Iambic keyer state
  bool keyerActive;        // Currently sending an element
  bool inSpacing;          // In the gap between elements
  bool sendingDit;         // Currently sending a dit
  bool sendingDah;         // Currently sending a dah
  bool ditMemory;          // Dit paddle was pressed (memory)
  bool dahMemory;          // Dah paddle was pressed (memory)
  unsigned long elementStartTime; // When current element/gap started
};

// ============================================
// Game State Variables
// ============================================

FallingLetter fallingLetters[MAX_FALLING_LETTERS];
MorseInputBuffer morseInput;
int gameScore = 0;
int gameLives = MAX_LIVES;
unsigned long lastSpawnTime = 0;
unsigned long lastGameUpdate = 0;
unsigned long gameStartTime = 0;
bool gameOver = false;
bool gamePaused = false;
// Note: highScore moved to shooterHighScores[] array per difficulty

// Decoder state
MorseDecoderAdaptive shooterDecoder(20, 20, 30);  // Initial 20 WPM, buffer size 30
String shooterDecodedText = "";
unsigned long shooterLastStateChangeTime = 0;
bool shooterLastToneState = false;
unsigned long shooterLastElementTime = 0;  // Track last element for timeout flush

// Settings mode state (legacy - will be removed)
bool inShooterSettings = false;
int shooterSettingsSelection = 0;  // 0=Speed, 1=Tone, 2=Key Type, 3=Save & Return

// LVGL mode flag - when true, skip legacy draw functions (LVGL handles display)
bool shooterUseLVGL = true;  // Default to LVGL mode

// Forward declarations for settings
void drawShooterSettings(LGFX& tft);
int handleShooterSettingsInput(char key, LGFX& tft);

// ============================================
// Preferences Functions
// ============================================

void loadShooterPrefs() {
    Preferences prefs;
    prefs.begin("shooter", true);  // read-only
    shooterDifficulty = (ShooterDifficulty)prefs.getInt("difficulty", SHOOTER_MEDIUM);
    if (shooterDifficulty > SHOOTER_HARD) shooterDifficulty = SHOOTER_MEDIUM;
    shooterHighScores[0] = prefs.getInt("hs_easy", 0);
    shooterHighScores[1] = prefs.getInt("hs_medium", 0);
    shooterHighScores[2] = prefs.getInt("hs_hard", 0);
    prefs.end();
    Serial.printf("[Shooter] Loaded prefs: difficulty=%d, HS=[%d,%d,%d]\n",
                  shooterDifficulty, shooterHighScores[0], shooterHighScores[1], shooterHighScores[2]);
}

void saveShooterPrefs() {
    Preferences prefs;
    prefs.begin("shooter", false);  // read-write
    prefs.putInt("difficulty", (int)shooterDifficulty);
    prefs.end();
    Serial.printf("[Shooter] Saved difficulty: %d\n", shooterDifficulty);
}

void saveShooterHighScore() {
    Preferences prefs;
    prefs.begin("shooter", false);  // read-write
    const char* keys[] = {"hs_easy", "hs_medium", "hs_hard"};
    prefs.putInt(keys[shooterDifficulty], shooterHighScores[shooterDifficulty]);
    prefs.end();
    Serial.printf("[Shooter] Saved high score for %s: %d\n",
                  DIFF_PARAMS[shooterDifficulty].name,
                  shooterHighScores[shooterDifficulty]);
}

// ============================================
// LVGL Display Update Functions (defined in lv_game_screens.h)
// ============================================

extern void updateShooterScore(int score);
extern void updateShooterLives(int lives);
extern void updateShooterDecoded(const char* text);
extern void updateShooterLetter(int index, char letter, int x, int y, bool visible);
extern void showShooterHitEffect(int x, int y);
extern void showShooterGameOver();

/*
 * Initialize a falling letter (with collision avoidance)
 */
void initFallingLetter(int index) {
  const DifficultyParams& params = DIFF_PARAMS[shooterDifficulty];
  fallingLetters[index].letter = params.charset[random(params.charsetSize)];

  // Try to find a spawn position that doesn't overlap with existing letters
  int attempts = 0;
  bool positionOk = false;
  int newX;

  // Letters spawn at top of play area (just below HUD at y=40 in LVGL coords)
  // Using game coords: y=5 + offset 40 = 45 on screen (just below HUD)
  const int SPAWN_Y = 5;

  while (!positionOk && attempts < 20) {
    newX = random(20, SCREEN_WIDTH - 40);
    positionOk = true;

    // Check if this position is too close to any active letter
    for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
      if (i != index && fallingLetters[i].active) {
        // Letters are about 20 pixels wide, so check for 30 pixel spacing
        if (abs(newX - (int)fallingLetters[i].x) < 30 &&
            abs(SPAWN_Y - (int)fallingLetters[i].y) < 40) {
          positionOk = false;
          break;
        }
      }
    }
    attempts++;
  }

  fallingLetters[index].x = newX;
  fallingLetters[index].y = SPAWN_Y;  // Start at top of play area
  fallingLetters[index].active = true;

  // Update LVGL display (y+40 for header offset)
  updateShooterLetter(index, fallingLetters[index].letter,
                     (int)fallingLetters[index].x,
                     (int)fallingLetters[index].y + 40, true);
}

/*
 * Reset game state
 */
void resetGame() {
  const DifficultyParams& params = DIFF_PARAMS[shooterDifficulty];

  // Clear all falling letters
  for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
    fallingLetters[i].active = false;
  }

  // Reset morse input
  morseInput.ditPressed = false;
  morseInput.dahPressed = false;
  morseInput.keyerActive = false;
  morseInput.inSpacing = false;
  morseInput.sendingDit = false;
  morseInput.sendingDah = false;
  morseInput.ditMemory = false;
  morseInput.dahMemory = false;
  morseInput.elementStartTime = 0;

  // Reset decoder state
  shooterDecoder.reset();
  shooterDecoder.flush();
  shooterDecoder.setWPM(cwSpeed);
  shooterDecodedText = "";
  shooterLastStateChangeTime = 0;
  shooterLastToneState = false;
  shooterLastElementTime = 0;

  // Reset game variables using difficulty params
  gameScore = 0;
  gameLives = params.startLives;
  lastSpawnTime = millis();
  lastGameUpdate = millis();
  gameStartTime = millis();
  gameOver = false;
  gamePaused = false;

  // Update LVGL display
  updateShooterScore(0);
  updateShooterLives(gameLives);
  updateShooterDecoded("");
  for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
    updateShooterLetter(i, ' ', 0, 0, false);  // Hide all letters
  }

  Serial.printf("[Shooter] Game reset: difficulty=%s, lives=%d\n",
                params.name, gameLives);
}

/*
 * Draw old-school ground scenery
 */
void drawGroundScenery(LGFX& tft) {
  // Skip legacy drawing when using LVGL
  if (shooterUseLVGL) return;

  // Ground line
  tft.drawFastHLine(0, GROUND_Y, SCREEN_WIDTH, ST77XX_GREEN);
  tft.drawFastHLine(0, GROUND_Y + 1, SCREEN_WIDTH, 0x05E0);

  // Houses (simple rectangles with roofs)
  // House 1 (left edge)
  tft.fillRect(5, GROUND_Y - 25, 30, 25, 0x4208);  // Dark gray house
  tft.fillTriangle(5, GROUND_Y - 25, 35, GROUND_Y - 25, 20, GROUND_Y - 35, ST77XX_RED);  // Red roof
  tft.fillRect(13, GROUND_Y - 12, 8, 12, 0x0861);  // Dark window

  // House 2
  tft.fillRect(90, GROUND_Y - 30, 35, 30, 0x52AA);  // Blue-gray house
  tft.fillTriangle(90, GROUND_Y - 30, 125, GROUND_Y - 30, 107, GROUND_Y - 42, 0xC618);  // Orange roof
  tft.fillRect(100, GROUND_Y - 15, 10, 15, 0x2104);  // Dark window

  // House 3
  tft.fillRect(195, GROUND_Y - 28, 32, 28, 0x6B4D);  // Tan house
  tft.fillTriangle(195, GROUND_Y - 28, 227, GROUND_Y - 28, 211, GROUND_Y - 38, 0x7800);  // Brown roof
  tft.fillRect(203, GROUND_Y - 14, 8, 14, 0x18C3);  // Door

  // House 4 (right side)
  tft.fillRect(270, GROUND_Y - 27, 30, 27, 0x39C7);  // Purple-gray house
  tft.fillTriangle(270, GROUND_Y - 27, 300, GROUND_Y - 27, 285, GROUND_Y - 37, 0xF800);  // Red roof
  tft.fillRect(278, GROUND_Y - 13, 8, 13, 0x18C3);  // Window

  // Trees (simple triangles)
  // Tree 1
  tft.fillRect(55, GROUND_Y - 15, 6, 15, 0x4A00);  // Brown trunk
  tft.fillTriangle(52, GROUND_Y - 15, 64, GROUND_Y - 15, 58, GROUND_Y - 28, 0x0400);  // Dark green
  tft.fillTriangle(53, GROUND_Y - 20, 63, GROUND_Y - 20, 58, GROUND_Y - 32, 0x05E0);  // Green

  // Tree 2
  tft.fillRect(165, GROUND_Y - 18, 6, 18, 0x4A00);  // Brown trunk
  tft.fillTriangle(162, GROUND_Y - 18, 174, GROUND_Y - 18, 168, GROUND_Y - 32, 0x0400);  // Dark green
  tft.fillTriangle(163, GROUND_Y - 24, 173, GROUND_Y - 24, 168, GROUND_Y - 36, 0x05E0);  // Green

  // Tree 3 (right side)
  tft.fillRect(245, GROUND_Y - 16, 6, 16, 0x4A00);  // Brown trunk
  tft.fillTriangle(242, GROUND_Y - 16, 254, GROUND_Y - 16, 248, GROUND_Y - 30, 0x0400);  // Dark green
  tft.fillTriangle(243, GROUND_Y - 22, 253, GROUND_Y - 22, 248, GROUND_Y - 34, 0x05E0);  // Green

  // Tree 4 (far right)
  tft.fillRect(310, GROUND_Y - 14, 5, 14, 0x4A00);  // Brown trunk
  tft.fillTriangle(308, GROUND_Y - 14, 318, GROUND_Y - 14, 313, GROUND_Y - 26, 0x0400);  // Dark green
  tft.fillTriangle(309, GROUND_Y - 19, 317, GROUND_Y - 19, 313, GROUND_Y - 30, 0x05E0);  // Green

  // Turret at bottom center (simple tank-like shape)
  tft.fillRect(150, GROUND_Y - 20, 20, 12, 0x7BEF);  // Gray base
  tft.fillRect(157, GROUND_Y - 26, 6, 10, 0x4208);   // Dark gray barrel
  tft.drawCircle(160, GROUND_Y - 14, 3, ST77XX_CYAN); // Turret circle accent
}

/*
 * Draw falling letters (with background clearing for current position)
 */
void drawFallingLetters(LGFX& tft, bool clearOld = false) {
  // Skip legacy drawing when using LVGL
  if (shooterUseLVGL) return;

  static int lastY[MAX_FALLING_LETTERS] = {0};

  tft.setTextSize(3);
  for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
    if (fallingLetters[i].active) {
      // Clear old position if requested (but only if it's below the header!)
      if (clearOld && lastY[i] != (int)fallingLetters[i].y && lastY[i] > 42) {
        tft.fillRect((int)fallingLetters[i].x - 2, lastY[i] - 2, 24, 28, COLOR_BACKGROUND);
      }

      // Draw at new position (only if below header)
      if (fallingLetters[i].y > 42) {
        tft.setTextColor(ST77XX_YELLOW, COLOR_BACKGROUND);
        tft.setCursor((int)fallingLetters[i].x, (int)fallingLetters[i].y);
        tft.print(fallingLetters[i].letter);
        lastY[i] = (int)fallingLetters[i].y;
      }
    } else if (clearOld && lastY[i] > 42) {
      // Clear if letter was just deactivated (but only if below header)
      tft.fillRect((int)fallingLetters[i].x - 2, lastY[i] - 2, 24, 28, COLOR_BACKGROUND);
      lastY[i] = 0;
    }
  }
}

/*
 * Draw turret laser when shooting
 */
void drawLaserShot(LGFX& tft, int targetX, int targetY) {
  // Skip legacy drawing when using LVGL
  if (shooterUseLVGL) return;

  // Draw laser from turret to target
  tft.drawLine(160, GROUND_Y - 26, targetX + 10, targetY + 10, ST77XX_CYAN);
  tft.drawLine(159, GROUND_Y - 26, targetX + 10, targetY + 10, ST77XX_WHITE);
  tft.drawLine(161, GROUND_Y - 26, targetX + 10, targetY + 10, ST77XX_WHITE);
}

/*
 * Draw explosion effect
 */
void drawExplosion(LGFX& tft, int x, int y) {
  // Skip legacy drawing when using LVGL
  if (shooterUseLVGL) return;

  // Simple star burst explosion
  tft.drawCircle(x + 10, y + 10, 8, ST77XX_YELLOW);
  tft.drawCircle(x + 10, y + 10, 6, ST77XX_RED);
  tft.drawCircle(x + 10, y + 10, 4, ST77XX_WHITE);
  // Rays
  for (int i = 0; i < 8; i++) {
    float angle = i * 3.14159 / 4;
    int x2 = x + 10 + (int)(12 * cos(angle));
    int y2 = y + 10 + (int)(12 * sin(angle));
    tft.drawLine(x + 10, y + 10, x2, y2, ST77XX_YELLOW);
  }
}

/*
 * Draw HUD (score, lives, morse input)
 */
void drawHUD(LGFX& tft) {
  // Skip legacy drawing when using LVGL
  if (shooterUseLVGL) return;

  // Score (top left corner)
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE, COLOR_BACKGROUND);
  tft.setCursor(10, 50);
  tft.print("Score:");
  tft.setCursor(50, 50);
  tft.print(gameScore);

  // Lives (top left, second line)
  tft.setCursor(10, 62);
  tft.setTextColor(gameLives <= 2 ? ST77XX_RED : ST77XX_GREEN, COLOR_BACKGROUND);
  tft.print("Lives:");
  tft.setCursor(50, 62);
  tft.print(gameLives);

  // Decoded text display (bottom, above ground)
  if (shooterDecodedText.length() > 0) {
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_CYAN, COLOR_BACKGROUND);
    tft.setCursor(10, GROUND_Y + 10);
    tft.print(shooterDecodedText);
    tft.print("   ");  // Clear extra space
  } else {
    // Clear decoded text area when empty
    tft.fillRect(10, GROUND_Y + 10, 100, 20, COLOR_BACKGROUND);
  }
}

/*
 * Update falling letters (physics)
 */
void updateFallingLetters() {
  const DifficultyParams& params = DIFF_PARAMS[shooterDifficulty];

  for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
    if (fallingLetters[i].active) {
      fallingLetters[i].y += params.fallSpeed;

      // Update LVGL display (y+40 for header offset)
      updateShooterLetter(i, fallingLetters[i].letter,
                         (int)fallingLetters[i].x,
                         (int)fallingLetters[i].y + 40, true);

      // Check if letter hit the ground (GAME_GROUND_Y = 200 for LVGL layout)
      if (fallingLetters[i].y >= GAME_GROUND_Y) {
        fallingLetters[i].active = false;
        updateShooterLetter(i, ' ', 0, 0, false);  // Hide letter
        gameLives--;
        updateShooterLives(gameLives);  // Update LVGL
        beep(TONE_ERROR, 200);  // Hit ground sound

        if (gameLives <= 0) {
          gameOver = true;
          showShooterGameOver();  // Show game over overlay
        }
      }
    }
  }
}

/*
 * Spawn new falling letter
 */
void spawnFallingLetter() {
  const DifficultyParams& params = DIFF_PARAMS[shooterDifficulty];

  if (millis() - lastSpawnTime < (unsigned long)params.spawnInterval) {
    return;  // Not time to spawn yet
  }

  // Find empty slot
  for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
    if (!fallingLetters[i].active) {
      initFallingLetter(i);
      lastSpawnTime = millis();
      return;
    }
  }
}

/*
 * Check decoded text and try to shoot matching letter
 */
bool checkMorseShoot(LGFX& tft) {
  if (shooterDecodedText.length() == 0) {
    return false;
  }

  // Get the last decoded character
  char decodedChar = shooterDecodedText[shooterDecodedText.length() - 1];

  // Convert to uppercase if needed
  if (decodedChar >= 'a' && decodedChar <= 'z') {
    decodedChar = decodedChar - 'a' + 'A';
  }

  // Find matching falling letter
  for (int j = 0; j < MAX_FALLING_LETTERS; j++) {
    if (fallingLetters[j].active && fallingLetters[j].letter == decodedChar) {
      // HIT!
      int targetX = (int)fallingLetters[j].x;
      int targetY = (int)fallingLetters[j].y;

      // Remove letter FIRST (before any redraw)
      fallingLetters[j].active = false;
      updateShooterLetter(j, ' ', 0, 0, false);  // Hide letter in LVGL

      // Play hit sounds (no delays for smoother LVGL updates)
      beep(1200, 50);  // Laser sound

      // Show hit effect in LVGL (y+40 for header offset)
      showShooterHitEffect(targetX, targetY + 40);

      // Legacy drawing (will no-op when using LVGL)
      drawLaserShot(tft, targetX, targetY);
      drawExplosion(tft, targetX, targetY);
      drawGroundScenery(tft);
      drawFallingLetters(tft);

      // Add score with difficulty multiplier
      const DifficultyParams& params = DIFF_PARAMS[shooterDifficulty];
      gameScore += 10 * params.scoreMultiplier;
      updateShooterScore(gameScore);  // Update LVGL

      // Update high score for current difficulty
      if (gameScore > shooterHighScores[shooterDifficulty]) {
        shooterHighScores[shooterDifficulty] = gameScore;
        saveShooterHighScore();
      }

      // Keep decoded text visible until next input starts
      // (cleared in updateMorseInputFast when paddle is pressed)
      return true;
    }
  }

  // Decoded character but no matching letter falling - clear text
  beep(600, 100);  // Miss sound
  shooterDecodedText = "";
  return false;
}

/*
 * Read paddle input and decode morse using adaptive decoder
 * Supports both straight key and iambic keying
 */
void updateMorseInputFast(LGFX& tft) {
  morseInput.ditPressed = (digitalRead(DIT_PIN) == LOW) || (touchRead(TOUCH_DIT_PIN) > TOUCH_THRESHOLD);
  morseInput.dahPressed = (digitalRead(DAH_PIN) == LOW) || (touchRead(TOUCH_DAH_PIN) > TOUCH_THRESHOLD);

  unsigned long now = millis();
  MorseTiming timing(cwSpeed);
  bool toneOn = isTonePlaying();

  // Clear previous hit text when starting new input
  // This keeps successful hit visible until user starts keying next character
  if ((morseInput.ditPressed || morseInput.dahPressed) && shooterDecodedText.length() > 0 && !morseInput.keyerActive) {
    shooterDecodedText = "";
    updateShooterDecoded("");
  }

  // Check for decoder timeout (flush if no activity for word gap duration)
  // Using word gap (7 dits) instead of character gap (3 dits) for more forgiving timing
  if (shooterLastElementTime > 0 && !morseInput.ditPressed && !morseInput.dahPressed && !morseInput.keyerActive) {
    unsigned long timeSinceLastElement = now - shooterLastElementTime;
    float wordGapDuration = MorseWPM::wordGap(shooterDecoder.getWPM());

    // Flush buffered data after word gap silence (more forgiving for straight key)
    if (timeSinceLastElement > wordGapDuration) {
      shooterDecoder.flush();
      shooterLastElementTime = 0;  // Reset timeout

      // Check for shot after flush
      if (shooterDecodedText.length() > 0) {
        checkMorseShoot(tft);
      }
    }
  }

  // STRAIGHT KEY MODE
  if (cwKeyType == KEY_STRAIGHT) {
    // Use DIT pin as straight key
    if (morseInput.ditPressed && !toneOn) {
      // Tone starting
      if (shooterLastToneState == false) {
        // Send silence duration to decoder (negative)
        if (shooterLastStateChangeTime > 0) {
          float silenceDuration = now - shooterLastStateChangeTime;
          if (silenceDuration > 0) {
            shooterDecoder.addTiming(-silenceDuration);
          }
        }
        shooterLastStateChangeTime = now;
        shooterLastToneState = true;
      }
      startTone(cwTone);
    }
    else if (morseInput.ditPressed && toneOn) {
      // Tone continuing
      continueTone(cwTone);
    }
    else if (!morseInput.ditPressed && toneOn) {
      // Tone stopping
      if (shooterLastToneState == true) {
        // Send tone duration to decoder (positive)
        float toneDuration = now - shooterLastStateChangeTime;
        if (toneDuration > 0) {
          shooterDecoder.addTiming(toneDuration);
          shooterLastElementTime = now;  // Update timeout tracker
        }
        shooterLastStateChangeTime = now;
        shooterLastToneState = false;
      }
      stopTone();
    }
    return;
  }

  // IAMBIC KEYER MODE (Mode A or B)

  // If not actively sending or spacing, check for new input
  if (!morseInput.keyerActive && !morseInput.inSpacing) {
    if (morseInput.ditPressed || morseInput.ditMemory) {
      // Start sending dit
      if (shooterLastToneState == false) {
        // Send silence duration to decoder
        if (shooterLastStateChangeTime > 0) {
          float silenceDuration = now - shooterLastStateChangeTime;
          if (silenceDuration > 0) {
            shooterDecoder.addTiming(-silenceDuration);
          }
        }
        shooterLastStateChangeTime = now;
        shooterLastToneState = true;
      }

      morseInput.keyerActive = true;
      morseInput.sendingDit = true;
      morseInput.sendingDah = false;
      morseInput.inSpacing = false;
      morseInput.elementStartTime = now;
      startTone(cwTone);
      morseInput.ditMemory = false;
    }
    else if (morseInput.dahPressed || morseInput.dahMemory) {
      // Start sending dah
      if (shooterLastToneState == false) {
        // Send silence duration to decoder
        if (shooterLastStateChangeTime > 0) {
          float silenceDuration = now - shooterLastStateChangeTime;
          if (silenceDuration > 0) {
            shooterDecoder.addTiming(-silenceDuration);
          }
        }
        shooterLastStateChangeTime = now;
        shooterLastToneState = true;
      }

      morseInput.keyerActive = true;
      morseInput.sendingDit = false;
      morseInput.sendingDah = true;
      morseInput.inSpacing = false;
      morseInput.elementStartTime = now;
      startTone(cwTone);
      morseInput.dahMemory = false;
    }
  }
  // Currently sending an element
  else if (morseInput.keyerActive && !morseInput.inSpacing) {
    unsigned long elementDuration = morseInput.sendingDit ? timing.ditDuration : timing.dahDuration;

    // Keep tone playing
    continueTone(cwTone);

    // Check for paddle input during element send (squeeze keying)
    if (morseInput.ditPressed && morseInput.dahPressed) {
      // Both pressed - remember opposite paddle
      if (morseInput.sendingDit) {
        morseInput.dahMemory = true;
      } else {
        morseInput.ditMemory = true;
      }
    }
    else if (morseInput.sendingDit && morseInput.dahPressed) {
      morseInput.dahMemory = true;
    }
    else if (morseInput.sendingDah && morseInput.ditPressed) {
      morseInput.ditMemory = true;
    }

    // Check if element is complete
    if (now - morseInput.elementStartTime >= elementDuration) {
      // Element complete, send tone duration to decoder
      if (shooterLastToneState == true) {
        float toneDuration = now - shooterLastStateChangeTime;
        if (toneDuration > 0) {
          shooterDecoder.addTiming(toneDuration);
          shooterLastElementTime = now;  // Update timeout tracker
        }
        shooterLastStateChangeTime = now;
        shooterLastToneState = false;
      }

      stopTone();
      morseInput.keyerActive = false;
      morseInput.sendingDit = false;
      morseInput.sendingDah = false;
      morseInput.inSpacing = true;
      morseInput.elementStartTime = now;
    }
  }
  // In inter-element spacing
  else if (morseInput.inSpacing) {
    // Check paddles during spacing
    if (morseInput.ditPressed && morseInput.dahPressed) {
      morseInput.ditMemory = true;
      morseInput.dahMemory = true;
    }
    else if (morseInput.ditPressed && !morseInput.ditMemory) {
      morseInput.ditMemory = true;
    }
    else if (morseInput.dahPressed && !morseInput.dahMemory) {
      morseInput.dahMemory = true;
    }

    // Wait for element gap duration
    if (now - morseInput.elementStartTime >= timing.ditDuration) {
      morseInput.inSpacing = false;
      // Ready to send next element if memory is set or paddle still pressed
    }
  }
}

/*
 * Draw shooter settings screen
 */
void drawShooterSettings(LGFX& tft) {
  if (shooterUseLVGL) return;  // LVGL handles display
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Title
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(80, 50);
  tft.print("SETTINGS");

  // Settings menu items
  int yPos = 75;
  int spacing = 32;

  // Option 0: Speed
  tft.setTextSize(2);
  if (shooterSettingsSelection == 0) {
    tft.setTextColor(ST77XX_BLACK, ST77XX_CYAN);  // Highlighted
  } else {
    tft.setTextColor(ST77XX_WHITE, COLOR_BACKGROUND);
  }
  tft.setCursor(20, yPos);
  tft.print("Speed: ");
  tft.print(cwSpeed);
  tft.print(" WPM   ");

  // Option 1: Tone
  yPos += spacing;
  if (shooterSettingsSelection == 1) {
    tft.setTextColor(ST77XX_BLACK, ST77XX_CYAN);
  } else {
    tft.setTextColor(ST77XX_WHITE, COLOR_BACKGROUND);
  }
  tft.setCursor(20, yPos);
  tft.print("Tone: ");
  tft.print(cwTone);
  tft.print(" Hz   ");

  // Option 2: Key Type
  yPos += spacing;
  if (shooterSettingsSelection == 2) {
    tft.setTextColor(ST77XX_BLACK, ST77XX_CYAN);
  } else {
    tft.setTextColor(ST77XX_WHITE, COLOR_BACKGROUND);
  }
  tft.setCursor(20, yPos);
  tft.print("Key: ");
  if (cwKeyType == KEY_STRAIGHT) {
    tft.print("Straight  ");
  } else if (cwKeyType == KEY_IAMBIC_A) {
    tft.print("Iambic A  ");
  } else {
    tft.print("Iambic B  ");
  }

  // Option 3: Save & Return
  yPos += spacing + 5;
  if (shooterSettingsSelection == 3) {
    tft.setTextColor(ST77XX_BLACK, ST77XX_GREEN);
  } else {
    tft.setTextColor(ST77XX_GREEN, COLOR_BACKGROUND);
  }
  tft.setCursor(50, yPos);
  tft.print("SAVE & PLAY");

  // Instructions (moved higher to avoid overlap)
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING, COLOR_BACKGROUND);
  tft.setCursor(20, 195);
  tft.print("\x18\x19:Select  \x1B\x1A:Change  ENTER:OK");
}

/*
 * Handle shooter settings input
 */
int handleShooterSettingsInput(char key, LGFX& tft) {
  if (key == KEY_UP) {
    shooterSettingsSelection--;
    if (shooterSettingsSelection < 0) shooterSettingsSelection = 3;
    drawShooterSettings(tft);
    beep(TONE_MENU_NAV, BEEP_SHORT);
    return 1;
  }
  else if (key == KEY_DOWN) {
    shooterSettingsSelection++;
    if (shooterSettingsSelection > 3) shooterSettingsSelection = 0;
    drawShooterSettings(tft);
    beep(TONE_MENU_NAV, BEEP_SHORT);
    return 1;
  }
  else if (key == KEY_LEFT) {
    if (shooterSettingsSelection == 0) {
      // Decrease speed
      if (cwSpeed > WPM_MIN) {
        cwSpeed--;
        drawShooterSettings(tft);
        beep(TONE_MENU_NAV, BEEP_SHORT);
      }
    }
    else if (shooterSettingsSelection == 1) {
      // Decrease tone
      if (cwTone > 400) {
        cwTone -= 50;
        drawShooterSettings(tft);
        beep(TONE_MENU_NAV, BEEP_SHORT);
      }
    }
    else if (shooterSettingsSelection == 2) {
      // Cycle key type backward
      if (cwKeyType == KEY_IAMBIC_B) {
        cwKeyType = KEY_IAMBIC_A;
      } else if (cwKeyType == KEY_IAMBIC_A) {
        cwKeyType = KEY_STRAIGHT;
      } else {
        cwKeyType = KEY_IAMBIC_B;
      }
      drawShooterSettings(tft);
      beep(TONE_MENU_NAV, BEEP_SHORT);
    }
    return 1;
  }
  else if (key == KEY_RIGHT) {
    if (shooterSettingsSelection == 0) {
      // Increase speed
      if (cwSpeed < WPM_MAX) {
        cwSpeed++;
        drawShooterSettings(tft);
        beep(TONE_MENU_NAV, BEEP_SHORT);
      }
    }
    else if (shooterSettingsSelection == 1) {
      // Increase tone
      if (cwTone < 1200) {
        cwTone += 50;
        drawShooterSettings(tft);
        beep(TONE_MENU_NAV, BEEP_SHORT);
      }
    }
    else if (shooterSettingsSelection == 2) {
      // Cycle key type forward
      if (cwKeyType == KEY_STRAIGHT) {
        cwKeyType = KEY_IAMBIC_A;
      } else if (cwKeyType == KEY_IAMBIC_A) {
        cwKeyType = KEY_IAMBIC_B;
      } else {
        cwKeyType = KEY_STRAIGHT;
      }
      drawShooterSettings(tft);
      beep(TONE_MENU_NAV, BEEP_SHORT);
    }
    return 1;
  }
  else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    if (shooterSettingsSelection == 3) {
      // Save & Return
      saveCWSettings();  // Save to preferences
      inShooterSettings = false;
      gamePaused = false;  // Unpause the game
      resetGame();  // Start new game
      drawMorseShooterUI(tft);
      beep(TONE_SELECT, BEEP_MEDIUM);
      return 2;  // Full redraw
    }
  }
  else if (key == KEY_ESC) {
    // Cancel without saving
    loadCWSettings();  // Reload original settings
    inShooterSettings = false;
    gamePaused = false;  // Unpause the game
    drawMorseShooterUI(tft);
    beep(TONE_MENU_NAV, BEEP_SHORT);
    return 2;
  }

  return 0;
}

/*
 * Draw game over screen
 */
void drawGameOver(LGFX& tft) {
  if (shooterUseLVGL) return;  // LVGL handles display
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Game Over text
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(50, 80);
  tft.print("GAME OVER");

  // Final score
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(80, 120);
  tft.print("Score: ");
  tft.print(gameScore);

  // High score
  tft.setCursor(70, 145);
  tft.setTextColor(ST77XX_YELLOW);
  tft.print("Best: ");
  tft.print(shooterHighScores[shooterDifficulty]);

  // Instructions
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(50, 180);
  tft.print("ENTER Play Again");
  tft.setCursor(80, 195);
  tft.print("ESC Exit");
}

/*
 * Initialize game (called when entering from Games menu)
 */
void startMorseShooter(LGFX& tft) {
  resetGame();

  // Setup decoder callback to capture decoded text
  shooterDecoder.messageCallback = [](String morse, String text) {
    // Append decoded characters
    for (int i = 0; i < text.length(); i++) {
      shooterDecodedText += text[i];
    }

    // Update LVGL display
    updateShooterDecoded(shooterDecodedText.c_str());

    Serial.print("Morse Shooter decoded: ");
    Serial.print(text);
    Serial.print(" (");
    Serial.print(morse);
    Serial.println(")");
  };

  // UI is now handled by LVGL - see lv_game_screens.h
}

/*
 * Draw main game UI
 */
void drawMorseShooterUI(LGFX& tft) {
  // Skip legacy drawing when using LVGL
  if (shooterUseLVGL) return;

  // Clear screen
  tft.fillScreen(COLOR_BACKGROUND);

  // Draw header
  drawHeader();

  // If in settings mode, show settings
  if (inShooterSettings) {
    drawShooterSettings(tft);
    return;
  }

  if (gameOver) {
    drawGameOver(tft);
    return;
  }

  // Draw game elements
  drawGroundScenery(tft);
  drawFallingLetters(tft);
  drawHUD(tft);
}

/*
 * Update morse input (called every loop for responsive keying)
 * This is separate from visual updates
 */
void updateMorseShooterInput(LGFX& tft) {
  if (gameOver || gamePaused) {
    return;
  }
  updateMorseInputFast(tft);
}

/*
 * Update game visuals (called once per second to avoid screen tearing)
 * This is separate from input polling
 * Screen is FROZEN while any paddle is held or pattern is being entered
 */
void updateMorseShooterVisuals(LGFX& tft) {
  if (gameOver || gamePaused) {
    return;
  }

  // FREEZE screen only during active keying (not when decoded text exists)
  // This allows physics/ground collision to continue while hit text is displayed
  bool isKeying = morseInput.keyerActive || morseInput.inSpacing ||
                  morseInput.ditPressed || morseInput.dahPressed;

  if (isKeying) {
    return;
  }

  unsigned long now = millis();

  // Only update game physics and visuals once per second
  if (now - lastGameUpdate >= GAME_UPDATE_INTERVAL) {
    lastGameUpdate = now;

    // Update game logic
    updateFallingLetters();
    spawnFallingLetter();

    // Redraw only changed elements (no full screen clear)
    drawFallingLetters(tft, true);  // Clear old positions, draw new
    drawHUD(tft);
  }
}

/*
 * Handle keyboard input for game
 * Returns: -1 to exit game, 0 for normal input, 2 for full redraw
 */
int handleMorseShooterInput(char key, LGFX& tft) {
  // If in settings mode, route to settings handler
  if (inShooterSettings) {
    return handleShooterSettingsInput(key, tft);
  }

  if (key == KEY_ESC) {
    return -1;  // Exit to games menu
  }

  if (gameOver) {
    if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
      // Restart game
      resetGame();
      return 2;  // Full redraw
    }
    return 0;
  }

  // Settings with 'S' key
  if (key == 's' || key == 'S') {
    inShooterSettings = true;
    gamePaused = true;  // Pause the game
    shooterSettingsSelection = 0;
    drawShooterSettings(tft);
    beep(TONE_SELECT, BEEP_MEDIUM);
    return 2;  // Full redraw
  }

  // Pause/unpause with SPACE
  if (key == ' ') {
    gamePaused = !gamePaused;
    if (gamePaused) {
      tft.setTextSize(2);
      tft.setTextColor(ST77XX_YELLOW, COLOR_BACKGROUND);
      tft.setCursor(110, 100);
      tft.print("PAUSED");
    }
    return 2;  // Redraw
  }

  return 0;
}

#endif // GAME_MORSE_SHOOTER_H
