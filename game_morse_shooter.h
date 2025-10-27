/*
 * Morse Shooter Game
 * Classic arcade-style game where players shoot falling letters using morse code
 */

#ifndef GAME_MORSE_SHOOTER_H
#define GAME_MORSE_SHOOTER_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "config.h"
#include "morse_code.h"
#include "i2s_audio.h"

// ============================================
// Game Constants
// ============================================

#define MAX_FALLING_LETTERS 5
#define LETTER_FALL_SPEED 1      // Pixels per update (1 = slow and steady)
#define LETTER_SPAWN_INTERVAL 3000  // ms between spawns
#define GROUND_Y 225              // Y position of ground (very bottom of screen)
#define MAX_LIVES 5               // Lives (letters that can hit ground)
#define GAME_UPDATE_INTERVAL 1000  // ms between game updates (1 second)

// Available characters for the game
const char SHOOTER_CHARSET[] = "ETIANMSURWDKGOHVFLPJBXCYZQ0123456789";
const int CHARSET_SIZE = 36;

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
  String pattern;          // Current morse pattern being entered
  unsigned long lastInputTime;  // Last dit/dah input time
  unsigned long lastReleaseTime; // When paddles were last released
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
int highScore = 0;

// Morse timing - use device CW settings
#define GAME_ELEMENT_TIMEOUT 500 // ms before pattern is considered complete
#define GAME_LETTER_TIMEOUT 1200  // ms before resetting pattern (longer for easier play)

/*
 * Initialize a falling letter (with collision avoidance)
 */
void initFallingLetter(int index) {
  fallingLetters[index].letter = SHOOTER_CHARSET[random(CHARSET_SIZE)];

  // Try to find a spawn position that doesn't overlap with existing letters
  int attempts = 0;
  bool positionOk = false;
  int newX;

  while (!positionOk && attempts < 20) {
    newX = random(20, SCREEN_WIDTH - 40);
    positionOk = true;

    // Check if this position is too close to any active letter
    for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
      if (i != index && fallingLetters[i].active) {
        // Letters are about 20 pixels wide, so check for 30 pixel spacing
        if (abs(newX - (int)fallingLetters[i].x) < 30 &&
            abs(75 - (int)fallingLetters[i].y) < 40) {
          positionOk = false;
          break;
        }
      }
    }
    attempts++;
  }

  fallingLetters[index].x = newX;
  fallingLetters[index].y = 75;  // Start well below header (header is 0-42)
  fallingLetters[index].active = true;
}

/*
 * Reset game state
 */
void resetGame() {
  // Clear all falling letters
  for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
    fallingLetters[i].active = false;
  }

  // Reset morse input
  morseInput.pattern = "";
  morseInput.lastInputTime = 0;
  morseInput.lastReleaseTime = 0;
  morseInput.ditPressed = false;
  morseInput.dahPressed = false;
  morseInput.keyerActive = false;
  morseInput.inSpacing = false;
  morseInput.sendingDit = false;
  morseInput.sendingDah = false;
  morseInput.ditMemory = false;
  morseInput.dahMemory = false;
  morseInput.elementStartTime = 0;

  // Reset game variables
  gameScore = 0;
  gameLives = MAX_LIVES;
  lastSpawnTime = millis();
  lastGameUpdate = millis();
  gameStartTime = millis();
  gameOver = false;
  gamePaused = false;
}

/*
 * Draw old-school ground scenery
 */
void drawGroundScenery(Adafruit_ST7789& tft) {
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
void drawFallingLetters(Adafruit_ST7789& tft, bool clearOld = false) {
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
void drawLaserShot(Adafruit_ST7789& tft, int targetX, int targetY) {
  // Draw laser from turret to target
  tft.drawLine(160, GROUND_Y - 26, targetX + 10, targetY + 10, ST77XX_CYAN);
  tft.drawLine(159, GROUND_Y - 26, targetX + 10, targetY + 10, ST77XX_WHITE);
  tft.drawLine(161, GROUND_Y - 26, targetX + 10, targetY + 10, ST77XX_WHITE);
}

/*
 * Draw explosion effect
 */
void drawExplosion(Adafruit_ST7789& tft, int x, int y) {
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
void drawHUD(Adafruit_ST7789& tft) {
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

  // Morse input display (bottom, above ground)
  if (morseInput.pattern.length() > 0) {
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_CYAN, COLOR_BACKGROUND);
    tft.setCursor(10, GROUND_Y + 10);
    tft.print(morseInput.pattern);
    tft.print("   ");  // Clear extra space
  } else {
    // Clear morse input area when empty
    tft.fillRect(10, GROUND_Y + 10, 100, 20, COLOR_BACKGROUND);
  }
}

/*
 * Update falling letters (physics)
 */
void updateFallingLetters() {
  for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
    if (fallingLetters[i].active) {
      fallingLetters[i].y += LETTER_FALL_SPEED;

      // Check if letter hit the ground
      if (fallingLetters[i].y >= GROUND_Y - 20) {
        fallingLetters[i].active = false;
        gameLives--;
        beep(TONE_ERROR, 200);  // Hit ground sound

        if (gameLives <= 0) {
          gameOver = true;
        }
      }
    }
  }
}

/*
 * Spawn new falling letter
 */
void spawnFallingLetter() {
  if (millis() - lastSpawnTime < LETTER_SPAWN_INTERVAL) {
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
 * Check morse input and try to shoot matching letter
 */
bool checkMorseShoot(Adafruit_ST7789& tft) {
  if (morseInput.pattern.length() == 0) {
    return false;
  }

  // Find which letter matches the current pattern
  for (int i = 0; i < CHARSET_SIZE; i++) {
    const char* expectedPattern = getMorseCode(SHOOTER_CHARSET[i]);
    if (expectedPattern != nullptr && morseInput.pattern.equals(expectedPattern)) {
      // Found matching pattern! Now find matching falling letter
      for (int j = 0; j < MAX_FALLING_LETTERS; j++) {
        if (fallingLetters[j].active && fallingLetters[j].letter == SHOOTER_CHARSET[i]) {
          // HIT!
          int targetX = (int)fallingLetters[j].x;
          int targetY = (int)fallingLetters[j].y;

          // Remove letter FIRST (before any redraw)
          fallingLetters[j].active = false;

          // Draw laser and explosion
          drawLaserShot(tft, targetX, targetY);
          beep(1200, 50);  // Laser sound
          delay(100);
          drawExplosion(tft, targetX, targetY);
          beep(1000, 100);  // Explosion sound
          delay(150);

          // Clean up - clear everything between header and ground, then redraw
          tft.fillRect(0, 42, SCREEN_WIDTH, GROUND_Y - 42, COLOR_BACKGROUND);  // Clear play area

          // Redraw all game elements (shot letter won't be drawn since it's inactive)
          drawGroundScenery(tft);
          drawFallingLetters(tft);  // Redraw remaining letters only

          // Add score
          gameScore += 10;

          // Update high score
          if (gameScore > highScore) {
            highScore = gameScore;
          }

          // Clear morse input
          morseInput.pattern = "";
          return true;
        }
      }

      // Correct morse code but no matching letter falling
      beep(600, 100);  // Miss sound
      morseInput.pattern = "";
      return false;
    }
  }

  // Invalid morse pattern - check if it could be a prefix of a valid pattern
  bool couldBeValid = false;
  for (int i = 0; i < CHARSET_SIZE; i++) {
    const char* pattern = getMorseCode(SHOOTER_CHARSET[i]);
    if (pattern != nullptr && String(pattern).startsWith(morseInput.pattern)) {
      couldBeValid = true;
      break;
    }
  }

  if (!couldBeValid) {
    // Invalid pattern - reset
    beep(400, 50);  // Error beep
    morseInput.pattern = "";
  }

  return false;
}

/*
 * Read paddle input and build morse pattern
 * Proper iambic keyer implementation (same as practice mode)
 */
void updateMorseInputFast(Adafruit_ST7789& tft) {
  morseInput.ditPressed = (digitalRead(DIT_PIN) == LOW) || (touchRead(TOUCH_DIT_PIN) > TOUCH_THRESHOLD);
  morseInput.dahPressed = (digitalRead(DAH_PIN) == LOW) || (touchRead(TOUCH_DAH_PIN) > TOUCH_THRESHOLD);

  unsigned long now = millis();
  MorseTiming timing(cwSpeed);

  bool isKeying = morseInput.ditPressed || morseInput.dahPressed || morseInput.keyerActive || morseInput.inSpacing;

  // Track when keying stops (transition from active to idle)
  static bool wasKeyingLastTime = false;
  if (isKeying) {
    wasKeyingLastTime = true;
  } else if (wasKeyingLastTime) {
    // Just stopped keying - mark the time
    morseInput.lastReleaseTime = now;
    wasKeyingLastTime = false;
  }

  // Check for pattern completion timeout when completely idle
  if (morseInput.pattern.length() > 0 && !isKeying) {
    if ((now - morseInput.lastReleaseTime) > GAME_LETTER_TIMEOUT) {
      // Pattern complete - check for match
      checkMorseShoot(tft);
      morseInput.pattern = "";
    }
  }

  // IAMBIC KEYER STATE MACHINE (same as practice mode)

  // If not actively sending or spacing, check for new input
  if (!morseInput.keyerActive && !morseInput.inSpacing) {
    if (morseInput.ditPressed || morseInput.ditMemory) {
      // Start sending dit
      morseInput.keyerActive = true;
      morseInput.sendingDit = true;
      morseInput.sendingDah = false;
      morseInput.inSpacing = false;
      morseInput.elementStartTime = now;
      morseInput.pattern += ".";
      startTone(cwTone);
      morseInput.ditMemory = false;
    }
    else if (morseInput.dahPressed || morseInput.dahMemory) {
      // Start sending dah
      morseInput.keyerActive = true;
      morseInput.sendingDit = false;
      morseInput.sendingDah = true;
      morseInput.inSpacing = false;
      morseInput.elementStartTime = now;
      morseInput.pattern += "-";
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
      // Element complete, stop tone and start spacing
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
    if (now - morseInput.elementStartTime >= timing.elementGap) {
      morseInput.inSpacing = false;
      // Ready to send next element if memory is set or paddle still pressed
    }
  }
}

/*
 * Draw game over screen
 */
void drawGameOver(Adafruit_ST7789& tft) {
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
  tft.print(highScore);

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
void startMorseShooter(Adafruit_ST7789& tft) {
  resetGame();
  drawMorseShooterUI(tft);
}

/*
 * Draw main game UI
 */
void drawMorseShooterUI(Adafruit_ST7789& tft) {
  // Clear screen
  tft.fillScreen(COLOR_BACKGROUND);

  // Draw header
  drawHeader();

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
void updateMorseShooterInput(Adafruit_ST7789& tft) {
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
void updateMorseShooterVisuals(Adafruit_ST7789& tft) {
  if (gameOver || gamePaused) {
    return;
  }

  // FREEZE screen completely during any keying activity or if pattern exists
  bool isKeying = morseInput.keyerActive || morseInput.inSpacing ||
                  morseInput.ditPressed || morseInput.dahPressed ||
                  morseInput.pattern.length() > 0;

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
int handleMorseShooterInput(char key, Adafruit_ST7789& tft) {
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
