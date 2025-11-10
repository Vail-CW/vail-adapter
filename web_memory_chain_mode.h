/*
 * Web Memory Chain Mode - Device Side
 * Runs game logic on device, sends updates to browser via WebSocket
 */

#ifndef WEB_MEMORY_CHAIN_MODE_H
#define WEB_MEMORY_CHAIN_MODE_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "config.h"
#include "morse_code.h"
#include "i2s_audio.h"
#include "web_memory_chain_socket.h"

// Game state for web mode
struct WebMemoryChainGame {
  char sequence[100];          // Current sequence
  int sequenceLength;          // Length of sequence
  int playerPosition;          // Where player is in reproduction
  int currentScore;            // Current chain length
  int highScore;               // High score this session
  bool gameActive;             // Game is running
  bool playingSequence;        // Device is playing sequence
  bool waitingForPlayer;       // Player's turn to reproduce
  unsigned long stateStartTime; // When current state began

  // Settings from browser
  int difficulty;              // 0=Beginner, 1=Intermediate, 2=Advanced
  int mode;                    // 0=Standard, 1=Practice(3 lives), 2=Timed
  int wpm;                     // Speed
  bool soundEnabled;           // Play audio
  bool showHints;              // Show sequence on screen
  int lives;                   // Remaining lives (practice mode)
  unsigned long gameStartTime; // For timed mode
};

WebMemoryChainGame webMemoryGame;

// Character sets by difficulty
const char* WEB_MEMORY_CHARSET_BEGINNER = "ETIANMSURWDKGOHVFLPJBXCYZQ";
const char* WEB_MEMORY_CHARSET_INTERMEDIATE = "ETIANMSURWDKGOHVFLPJBXCYZQ0123456789";
const char* WEB_MEMORY_CHARSET_ADVANCED = "ETIANMSURWDKGOHVFLPJBXCYZQ0123456789";

/*
 * Get character set based on difficulty
 */
const char* getWebMemoryCharset() {
  switch (webMemoryGame.difficulty) {
    case 0: return WEB_MEMORY_CHARSET_BEGINNER;
    case 1: return WEB_MEMORY_CHARSET_INTERMEDIATE;
    case 2: return WEB_MEMORY_CHARSET_ADVANCED;
    default: return WEB_MEMORY_CHARSET_BEGINNER;
  }
}

/*
 * Get random character from current charset
 */
char getWebMemoryRandomChar() {
  const char* charset = getWebMemoryCharset();
  int len = strlen(charset);
  return charset[random(len)];
}

/*
 * Generate new sequence (adds one character)
 */
void generateWebMemorySequence() {
  if (webMemoryGame.sequenceLength < 99) {
    webMemoryGame.sequence[webMemoryGame.sequenceLength] = getWebMemoryRandomChar();
    webMemoryGame.sequenceLength++;
    webMemoryGame.sequence[webMemoryGame.sequenceLength] = '\0';

    Serial.printf("Generated sequence (length %d): %s\n",
                  webMemoryGame.sequenceLength, webMemoryGame.sequence);
  }
}

/*
 * Play sequence as morse code
 */
void playWebMemorySequence() {
  if (!webMemoryGame.soundEnabled) {
    // Silent mode - just delay
    delay(webMemoryGame.sequenceLength * 1000);
    return;
  }

  MorseTiming timing(webMemoryGame.wpm);

  for (int i = 0; i < webMemoryGame.sequenceLength; i++) {
    char c = webMemoryGame.sequence[i];
    const char* pattern = getMorseCode(c);

    if (pattern) {
      Serial.printf("Playing: %c (%s)\n", c, pattern);

      // Play each element
      for (int j = 0; pattern[j] != '\0'; j++) {
        if (pattern[j] == '.') {
          startTone(700);
          delay(timing.ditDuration());
          stopTone();
        } else if (pattern[j] == '-') {
          startTone(700);
          delay(timing.dahDuration());
          stopTone();
        }

        // Inter-element gap
        if (pattern[j + 1] != '\0') {
          delay(timing.ditDuration());
        }
      }

      // Letter gap
      delay(timing.letterGap());
    }
  }

  // Extra delay before player's turn
  delay(500);
}

/*
 * Start new round
 */
void startWebMemoryRound() {
  webMemoryGame.playerPosition = 0;
  webMemoryGame.playingSequence = true;
  webMemoryGame.waitingForPlayer = false;
  webMemoryGame.stateStartTime = millis();

  // Generate new sequence
  generateWebMemorySequence();

  // Send state to browser
  sendMemoryChainState("playing", "Listen carefully...");

  // Send sequence to browser (show based on hints setting)
  String seqStr = String(webMemoryGame.sequence);
  sendMemoryChainSequence(seqStr, webMemoryGame.showHints);

  // Play sequence
  Serial.println("Playing sequence...");
  playWebMemorySequence();

  // Now player's turn
  webMemoryGame.playingSequence = false;
  webMemoryGame.waitingForPlayer = true;
  webMemoryGame.stateStartTime = millis();

  sendMemoryChainState("listening", "Your turn! Reproduce the sequence");

  Serial.println("Waiting for player input...");
}

/*
 * Callback when decoder finishes a character
 */
void onWebMemoryDecoded(String morse, String text) {
  if (!webMemoryGame.waitingForPlayer) return;

  Serial.printf("Decoded: %s = %s (expecting: %c)\n",
                morse.c_str(), text.c_str(),
                webMemoryGame.sequence[webMemoryGame.playerPosition]);

  // Check if correct
  char expected = webMemoryGame.sequence[webMemoryGame.playerPosition];
  char received = text.length() > 0 ? text.charAt(0) : '?';

  if (received == expected) {
    // Correct character
    webMemoryGame.playerPosition++;

    // Check if sequence complete
    if (webMemoryGame.playerPosition >= webMemoryGame.sequenceLength) {
      // Sequence complete!
      webMemoryGame.currentScore = webMemoryGame.sequenceLength;
      if (webMemoryGame.currentScore > webMemoryGame.highScore) {
        webMemoryGame.highScore = webMemoryGame.currentScore;
      }

      sendMemoryChainFeedback(true);
      sendMemoryChainScore(webMemoryGame.currentScore, webMemoryGame.highScore);

      Serial.printf("Round complete! Score: %d\n", webMemoryGame.currentScore);

      delay(1000);

      // Start next round
      startWebMemoryRound();
    }
  } else {
    // Wrong character
    Serial.println("Wrong character!");
    sendMemoryChainFeedback(false);

    if (webMemoryGame.mode == 1) {
      // Practice mode - lose a life
      webMemoryGame.lives--;
      if (webMemoryGame.lives > 0) {
        // Try again
        delay(1000);
        webMemoryGame.playerPosition = 0;
        sendMemoryChainState("listening", String("Wrong! " + String(webMemoryGame.lives) + " lives left. Try again"));
      } else {
        // Game over
        sendMemoryChainGameOver(webMemoryGame.currentScore, "Out of lives");
        webMemoryGame.gameActive = false;
      }
    } else {
      // Standard mode - game over
      sendMemoryChainGameOver(webMemoryGame.currentScore, "Wrong character");
      webMemoryGame.gameActive = false;
    }
  }
}

/*
 * Initialize web memory chain mode with settings from browser
 */
void startWebMemoryChainMode(Adafruit_ST7789& tft, int difficulty, int mode, int wpm, bool sound, bool hints) {
  Serial.println("Starting Web Memory Chain Mode...");

  // Initialize game state
  memset(&webMemoryGame, 0, sizeof(webMemoryGame));
  webMemoryGame.difficulty = difficulty;
  webMemoryGame.mode = mode;
  webMemoryGame.wpm = wpm;
  webMemoryGame.soundEnabled = sound;
  webMemoryGame.showHints = hints;
  webMemoryGame.lives = (mode == 1) ? 3 : 1;  // 3 lives for practice mode
  webMemoryGame.gameActive = true;
  webMemoryGame.gameStartTime = millis();
  webMemoryGame.currentScore = 0;
  webMemoryGame.highScore = 0;
  webMemoryGame.sequenceLength = 0;

  // Configure decoder
  webMemoryChainDecoder.setSpeed(wpm);
  webMemoryChainDecoder.setCallback(onWebMemoryDecoded);

  // Draw static screen on device
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);

  int y = 40;
  tft.setCursor(20, y);
  tft.println("WEB MODE ACTIVE");

  y += 30;
  tft.setTextSize(1);
  tft.setCursor(20, y);
  tft.println("Memory Chain game is");
  y += 15;
  tft.setCursor(20, y);
  tft.println("running in your browser");

  y += 30;
  tft.setCursor(20, y);
  tft.print("Difficulty: ");
  const char* diffNames[] = {"Beginner", "Intermediate", "Advanced"};
  tft.println(diffNames[difficulty]);

  y += 15;
  tft.setCursor(20, y);
  tft.print("Mode: ");
  const char* modeNames[] = {"Standard", "Practice", "Timed"};
  tft.println(modeNames[mode]);

  y += 15;
  tft.setCursor(20, y);
  tft.print("Speed: ");
  tft.print(wpm);
  tft.println(" WPM");

  y += 30;
  tft.setCursor(20, y);
  tft.setTextColor(COLOR_ACCENT);
  tft.println("Press ESC to exit");

  // Send initial state to browser
  sendMemoryChainState("ready", "Click Start Game to begin");
  sendMemoryChainScore(0, 0);

  // Start first round
  delay(1000);
  startWebMemoryRound();
}

/*
 * Handle keyboard input in web memory chain mode
 */
int handleWebMemoryChainInput(char key, Adafruit_ST7789& tft) {
  if (key == KEY_ESC) {
    Serial.println("Exiting web memory chain mode");
    webMemoryGame.gameActive = false;
    return -1;  // Exit mode
  }
  return 0;  // Stay in mode
}

/*
 * Update function (called from main loop)
 */
void updateWebMemoryChain() {
  if (!webMemoryGame.gameActive) return;

  // Check for timed mode timeout
  if (webMemoryGame.mode == 2) {  // Timed mode
    unsigned long elapsed = (millis() - webMemoryGame.gameStartTime) / 1000;
    if (elapsed >= 60) {  // 60 second time limit
      sendMemoryChainGameOver(webMemoryGame.currentScore, "Time's up!");
      webMemoryGame.gameActive = false;
    }
  }

  // Let decoder process any queued timings
  webMemoryChainDecoder.update();
}

#endif // WEB_MEMORY_CHAIN_MODE_H
