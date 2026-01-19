/*
 * VAIL SUMMIT - FreeRTOS Task Manager
 * Dual-core task management for ESP32-S3
 *
 * Core 0: Audio Task (high priority) - I2S generation, morse tones, decoder timing
 * Core 1: UI Task (Arduino loop) - LVGL rendering, input handling, network
 */

#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "config.h"

// ============================================
// Task Configuration
// ============================================

#define AUDIO_TASK_STACK_SIZE   8192    // Stack size for audio task
#define AUDIO_TASK_PRIORITY     (configMAX_PRIORITIES - 1)  // Highest priority
#define AUDIO_TASK_CORE         0       // Run audio on Core 0

// ============================================
// Task Handles
// ============================================

static TaskHandle_t audioTaskHandle = NULL;

// ============================================
// Thread-Safe Audio Request Structure
// ============================================

// Tone request types
enum ToneRequestType {
    TONE_REQ_NONE = 0,
    TONE_REQ_PLAY,          // Play a tone for specific duration
    TONE_REQ_START,         // Start continuous tone
    TONE_REQ_CONTINUE,      // Continue current tone (fill buffer)
    TONE_REQ_STOP           // Stop current tone
};

// Thread-safe tone request (written by UI core, read by audio core)
struct ToneRequest {
    volatile ToneRequestType type;
    volatile int frequency;
    volatile int duration_ms;
};

static volatile ToneRequest toneRequest = {TONE_REQ_NONE, 0, 0};

// Audio state (managed by audio task, read by UI for status)
static volatile bool audioTaskRunning = false;
static volatile bool toneCurrentlyPlaying = false;
static volatile int currentToneFrequency = 0;

// Mutex for protecting shared state
static SemaphoreHandle_t audioMutex = NULL;

// ============================================
// Decoded Character Queue
// ============================================

// Queue for decoded characters (audio core produces, UI core consumes)
#define DECODED_CHAR_QUEUE_SIZE 32
static QueueHandle_t decodedCharQueue = NULL;

// ============================================
// Paddle Input State (sampled by audio task)
// ============================================

struct PaddleState {
    volatile bool ditPressed;
    volatile bool dahPressed;
    volatile unsigned long ditPressTime;
    volatile unsigned long dahPressTime;
};

static volatile PaddleState paddleState = {false, false, 0, 0};

// ============================================
// Async Morse String Playback
// ============================================

#define MORSE_PLAYBACK_MAX_LENGTH 128  // Max string length for async playback

// Morse playback state machine
enum MorsePlaybackState {
    MORSE_IDLE = 0,           // No playback active
    MORSE_PLAYING_ELEMENT,    // Playing a dit or dah
    MORSE_ELEMENT_GAP,        // Gap between dits/dahs in same letter
    MORSE_LETTER_GAP,         // Gap between letters
    MORSE_WORD_GAP,           // Gap between words
    MORSE_COMPLETE            // Playback finished
};

// Morse playback request structure
struct MorsePlaybackRequest {
    volatile bool active;           // Playback in progress
    volatile bool cancelled;        // Cancellation requested
    char text[MORSE_PLAYBACK_MAX_LENGTH];  // Text to play
    volatile int textLength;        // Length of text
    volatile int wpm;               // Words per minute
    volatile int toneHz;            // Tone frequency
    volatile int charIndex;         // Current character in text
    volatile int elementIndex;      // Current element in character pattern
    volatile MorsePlaybackState state;  // Current state
    volatile unsigned long stateEndTime;  // When current state ends
    volatile bool complete;         // Playback finished flag
};

static volatile MorsePlaybackRequest morsePlayback = {
    false, false, "", 0, 0, 0, 0, 0, MORSE_IDLE, 0, false
};

// Morse code lookup table (matches morse_code.h)
static const char* morseTableInternal[] = {
    ".-",    // A
    "-...",  // B
    "-.-.",  // C
    "-..",   // D
    ".",     // E
    "..-.",  // F
    "--.",   // G
    "....",  // H
    "..",    // I
    ".---",  // J
    "-.-",   // K
    ".-..",  // L
    "--",    // M
    "-.",    // N
    "---",   // O
    ".--.",  // P
    "--.-",  // Q
    ".-.",   // R
    "...",   // S
    "-",     // T
    "..-",   // U
    "...-",  // V
    ".--",   // W
    "-..-",  // X
    "-.--",  // Y
    "--..",  // Z
    "-----", // 0
    ".----", // 1
    "..---", // 2
    "...--", // 3
    "....-", // 4
    ".....", // 5
    "-....", // 6
    "--...", // 7
    "---..", // 8
    "----.", // 9
    ".-.-.-", // Period
    "--..--", // Comma
    "..--..", // Question mark
    ".----.", // Apostrophe
    "-.-.--", // Exclamation
    "-..-.",  // Slash
    "-.--.",  // Parenthesis (
    "-.--.-", // Parenthesis )
    ".-...",  // Ampersand
    "---...", // Colon
    "-.-.-.", // Semicolon
    "-...-",  // Equals
    ".-.-.",  // Plus
    "-....-", // Hyphen/Minus
    "..--.-", // Underscore
    ".-..-.", // Quote
    "...-..-", // Dollar
    ".--.-."  // At sign
};

// Get morse pattern for a character (internal version for audio task)
static const char* getMorsePatternInternal(char c) {
    c = toupper(c);
    if (c >= 'A' && c <= 'Z') {
        return morseTableInternal[c - 'A'];
    } else if (c >= '0' && c <= '9') {
        return morseTableInternal[26 + (c - '0')];
    } else if (c == '.') {
        return morseTableInternal[36];
    } else if (c == ',') {
        return morseTableInternal[37];
    } else if (c == '?') {
        return morseTableInternal[38];
    } else if (c == '/') {
        return morseTableInternal[41];
    } else if (c == '-') {
        return morseTableInternal[47];
    }
    return nullptr;
}

// ============================================
// Thread-Safe API Functions (called from UI core)
// ============================================

/*
 * Request a tone to be played
 * Non-blocking - sets request flags for audio task
 */
void requestPlayTone(int frequency, int duration_ms) {
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        toneRequest.frequency = frequency;
        toneRequest.duration_ms = duration_ms;
        toneRequest.type = TONE_REQ_PLAY;
        xSemaphoreGive(audioMutex);
    }
}

/*
 * Request to start a continuous tone
 */
void requestStartTone(int frequency) {
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        toneRequest.frequency = frequency;
        toneRequest.duration_ms = 0;
        toneRequest.type = TONE_REQ_START;
        xSemaphoreGive(audioMutex);
    }
}

/*
 * Request to stop the current tone
 */
void requestStopTone() {
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        toneRequest.type = TONE_REQ_STOP;
        xSemaphoreGive(audioMutex);
    }
}

/*
 * Request a beep (blocking on UI side - waits for completion)
 */
void requestBeep(int frequency, int duration_ms) {
    requestPlayTone(frequency, duration_ms);
    // Wait for tone to complete (approximate)
    vTaskDelay(pdMS_TO_TICKS(duration_ms + 20));
}

/*
 * Check if a tone is currently playing
 */
bool isAudioTonePlaying() {
    return toneCurrentlyPlaying;
}

/*
 * Get a decoded character from the queue (non-blocking)
 * Returns 0 if queue is empty
 */
char getDecodedChar() {
    char c = 0;
    if (decodedCharQueue != NULL) {
        xQueueReceive(decodedCharQueue, &c, 0);
    }
    return c;
}

/*
 * Check if there are decoded characters available
 */
bool hasDecodedChars() {
    if (decodedCharQueue == NULL) return false;
    return uxQueueMessagesWaiting(decodedCharQueue) > 0;
}

// ============================================
// Async Morse String Playback API (called from UI core)
// ============================================

/*
 * Request to play a morse string asynchronously
 * Non-blocking - returns immediately, audio task handles playback
 */
void requestPlayMorseString(const char* str, int wpm, int toneHz = TONE_SIDETONE) {
    if (str == nullptr || strlen(str) == 0) return;

    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        // Stop any current playback
        morsePlayback.cancelled = true;

        // Wait a moment for audio task to see cancellation
        xSemaphoreGive(audioMutex);
        vTaskDelay(pdMS_TO_TICKS(10));

        // Now set up new playback
        if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            // Copy text (safely)
            int len = strlen(str);
            if (len > MORSE_PLAYBACK_MAX_LENGTH - 1) {
                len = MORSE_PLAYBACK_MAX_LENGTH - 1;
            }
            strncpy((char*)morsePlayback.text, str, len);
            ((char*)morsePlayback.text)[len] = '\0';

            morsePlayback.textLength = len;
            morsePlayback.wpm = wpm;
            morsePlayback.toneHz = toneHz;
            morsePlayback.charIndex = 0;
            morsePlayback.elementIndex = 0;
            morsePlayback.state = MORSE_IDLE;  // Will start on next audio task cycle
            morsePlayback.stateEndTime = 0;
            morsePlayback.complete = false;
            morsePlayback.cancelled = false;
            morsePlayback.active = true;

            Serial.printf("[MorsePlayback] Started: '%s' @ %d WPM, %d Hz\n", str, wpm, toneHz);
            xSemaphoreGive(audioMutex);
        }
    }
}

/*
 * Check if morse playback is currently active
 */
bool isMorsePlaybackActive() {
    return morsePlayback.active && !morsePlayback.complete;
}

/*
 * Check if morse playback has completed
 */
bool isMorsePlaybackComplete() {
    return morsePlayback.complete;
}

/*
 * Cancel current morse playback
 */
void cancelMorsePlayback() {
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        morsePlayback.cancelled = true;
        xSemaphoreGive(audioMutex);
    }
}

/*
 * Get current playback progress (0.0 to 1.0)
 */
float getMorsePlaybackProgress() {
    if (!morsePlayback.active || morsePlayback.textLength == 0) return 0.0f;
    return (float)morsePlayback.charIndex / (float)morsePlayback.textLength;
}

/*
 * Reset morse playback state (call when entering modes)
 */
void resetMorsePlayback() {
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        morsePlayback.active = false;
        morsePlayback.cancelled = false;
        morsePlayback.complete = false;
        morsePlayback.charIndex = 0;
        morsePlayback.elementIndex = 0;
        morsePlayback.state = MORSE_IDLE;
        xSemaphoreGive(audioMutex);
    }
}

// ============================================
// Internal Audio Task Functions
// ============================================

// Forward declarations for i2s_audio.h functions that will be called internally
extern void playToneInternal(int frequency, int duration_ms);
extern void startToneInternal(int frequency);
extern void continueToneInternal(int frequency);
extern void stopToneInternal();
extern bool isTonePlayingInternal();

/*
 * Process audio requests from the queue
 * Called by audio task
 */
void processAudioRequests() {
    ToneRequestType reqType = TONE_REQ_NONE;
    int reqFreq = 0;
    int reqDuration = 0;

    // Get current request with mutex protection
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        reqType = toneRequest.type;
        reqFreq = toneRequest.frequency;
        reqDuration = toneRequest.duration_ms;
        toneRequest.type = TONE_REQ_NONE;  // Clear request
        xSemaphoreGive(audioMutex);
    }

    // Process the request
    switch (reqType) {
        case TONE_REQ_PLAY:
            toneCurrentlyPlaying = true;
            currentToneFrequency = reqFreq;
            playToneInternal(reqFreq, reqDuration);
            toneCurrentlyPlaying = false;
            currentToneFrequency = 0;
            break;

        case TONE_REQ_START:
            toneCurrentlyPlaying = true;
            currentToneFrequency = reqFreq;
            startToneInternal(reqFreq);
            break;

        case TONE_REQ_CONTINUE:
            if (toneCurrentlyPlaying) {
                continueToneInternal(currentToneFrequency);
            }
            break;

        case TONE_REQ_STOP:
            stopToneInternal();
            toneCurrentlyPlaying = false;
            currentToneFrequency = 0;
            break;

        default:
            // If tone is playing, keep buffer filled
            if (toneCurrentlyPlaying) {
                continueToneInternal(currentToneFrequency);
            }
            break;
    }
}

/*
 * Sample paddle input
 * Called by audio task for precise timing
 */
void samplePaddleInput() {
    // Read paddle pins
    bool dit = (digitalRead(DIT_PIN) == PADDLE_ACTIVE);
    bool dah = (digitalRead(DAH_PIN) == PADDLE_ACTIVE);

    // Read capacitive touch
    if (!dit) {
        dit = (touchRead(TOUCH_DIT_PIN) > TOUCH_THRESHOLD);
    }
    if (!dah) {
        dah = (touchRead(TOUCH_DAH_PIN) > TOUCH_THRESHOLD);
    }

    // Update state with timestamps
    unsigned long now = millis();
    if (dit && !paddleState.ditPressed) {
        paddleState.ditPressTime = now;
    }
    if (dah && !paddleState.dahPressed) {
        paddleState.dahPressTime = now;
    }

    paddleState.ditPressed = dit;
    paddleState.dahPressed = dah;
}

/*
 * Get current paddle state (called from UI or decoder)
 */
void getPaddleState(bool* dit, bool* dah) {
    *dit = paddleState.ditPressed;
    *dah = paddleState.dahPressed;
}

/*
 * Process morse string playback state machine
 * Called by audio task - runs non-blocking state machine for async playback
 */
void processMorsePlayback() {
    // Skip if no active playback
    if (!morsePlayback.active) return;

    // Check for cancellation
    if (morsePlayback.cancelled) {
        stopToneInternal();
        morsePlayback.active = false;
        morsePlayback.complete = true;
        morsePlayback.cancelled = false;
        Serial.println("[MorsePlayback] Cancelled");
        return;
    }

    unsigned long now = millis();
    int ditDuration = 1200 / morsePlayback.wpm;
    int dahDuration = ditDuration * 3;
    int elementGap = ditDuration;
    int letterGap = ditDuration * 3;
    int wordGap = ditDuration * 7;

    // State machine
    switch (morsePlayback.state) {
        case MORSE_IDLE: {
            // Start playing - find first valid character
            while (morsePlayback.charIndex < morsePlayback.textLength) {
                char c = morsePlayback.text[morsePlayback.charIndex];

                if (c == ' ') {
                    // Word gap - transition to MORSE_WORD_GAP
                    morsePlayback.state = MORSE_WORD_GAP;
                    morsePlayback.stateEndTime = now + (wordGap - letterGap);  // Subtract letter gap already added
                    morsePlayback.charIndex++;
                    return;
                }

                const char* pattern = getMorsePatternInternal(c);
                if (pattern != nullptr) {
                    // Valid character - start playing first element
                    morsePlayback.elementIndex = 0;
                    char element = pattern[0];

                    if (element == '.') {
                        startToneInternal(morsePlayback.toneHz);
                        morsePlayback.state = MORSE_PLAYING_ELEMENT;
                        morsePlayback.stateEndTime = now + ditDuration;
                    } else if (element == '-') {
                        startToneInternal(morsePlayback.toneHz);
                        morsePlayback.state = MORSE_PLAYING_ELEMENT;
                        morsePlayback.stateEndTime = now + dahDuration;
                    }
                    return;
                }

                // Unknown character - skip it
                morsePlayback.charIndex++;
            }

            // No more characters - playback complete
            morsePlayback.state = MORSE_COMPLETE;
            morsePlayback.complete = true;
            morsePlayback.active = false;
            Serial.println("[MorsePlayback] Complete");
            break;
        }

        case MORSE_PLAYING_ELEMENT: {
            // Keep I2S buffer filled during tone
            continueToneInternal(morsePlayback.toneHz);

            // Check if element duration has elapsed
            if (now >= morsePlayback.stateEndTime) {
                stopToneInternal();

                // Get current character pattern
                char c = morsePlayback.text[morsePlayback.charIndex];
                const char* pattern = getMorsePatternInternal(c);

                if (pattern == nullptr) {
                    // Should not happen, but handle gracefully
                    morsePlayback.charIndex++;
                    morsePlayback.state = MORSE_IDLE;
                    return;
                }

                // Move to next element
                morsePlayback.elementIndex++;

                if (pattern[morsePlayback.elementIndex] != '\0') {
                    // More elements in this character - add element gap
                    morsePlayback.state = MORSE_ELEMENT_GAP;
                    morsePlayback.stateEndTime = now + elementGap;
                } else {
                    // Character complete - move to next character
                    morsePlayback.charIndex++;

                    if (morsePlayback.charIndex >= morsePlayback.textLength) {
                        // All done
                        morsePlayback.state = MORSE_COMPLETE;
                        morsePlayback.complete = true;
                        morsePlayback.active = false;
                        Serial.println("[MorsePlayback] Complete");
                    } else if (morsePlayback.text[morsePlayback.charIndex] == ' ') {
                        // Next is space - word gap (already have letter gap implicitly)
                        morsePlayback.charIndex++;
                        morsePlayback.state = MORSE_WORD_GAP;
                        morsePlayback.stateEndTime = now + wordGap;
                    } else {
                        // Next is a character - letter gap
                        morsePlayback.state = MORSE_LETTER_GAP;
                        morsePlayback.stateEndTime = now + letterGap;
                    }
                }
            }
            break;
        }

        case MORSE_ELEMENT_GAP: {
            // Wait for element gap to complete
            if (now >= morsePlayback.stateEndTime) {
                // Start next element
                char c = morsePlayback.text[morsePlayback.charIndex];
                const char* pattern = getMorsePatternInternal(c);

                if (pattern != nullptr && pattern[morsePlayback.elementIndex] != '\0') {
                    char element = pattern[morsePlayback.elementIndex];

                    if (element == '.') {
                        startToneInternal(morsePlayback.toneHz);
                        morsePlayback.state = MORSE_PLAYING_ELEMENT;
                        morsePlayback.stateEndTime = now + ditDuration;
                    } else if (element == '-') {
                        startToneInternal(morsePlayback.toneHz);
                        morsePlayback.state = MORSE_PLAYING_ELEMENT;
                        morsePlayback.stateEndTime = now + dahDuration;
                    }
                } else {
                    // Pattern ended unexpectedly - move to next char
                    morsePlayback.charIndex++;
                    morsePlayback.state = MORSE_IDLE;
                }
            }
            break;
        }

        case MORSE_LETTER_GAP: {
            // Wait for letter gap to complete
            if (now >= morsePlayback.stateEndTime) {
                // Start next character
                morsePlayback.state = MORSE_IDLE;
            }
            break;
        }

        case MORSE_WORD_GAP: {
            // Wait for word gap to complete
            if (now >= morsePlayback.stateEndTime) {
                // Continue to next character
                morsePlayback.state = MORSE_IDLE;
            }
            break;
        }

        case MORSE_COMPLETE: {
            // Nothing to do - playback finished
            break;
        }
    }
}

// ============================================
// Audio Task
// ============================================

/*
 * Audio task - runs on Core 0
 * High priority, dedicated to audio processing
 */
void audioTask(void* parameter) {
    Serial.println("[AudioTask] Started on Core 0");
    audioTaskRunning = true;

    while (true) {
        // Process any pending audio requests (single tone API)
        processAudioRequests();

        // Process morse string playback (async playback API)
        processMorsePlayback();

        // Sample paddle input with precise timing
        samplePaddleInput();

        // Yield to allow other tasks, but keep loop tight (~1ms)
        vTaskDelay(1);
    }
}

// ============================================
// Task Setup
// ============================================

/*
 * Initialize task manager and start audio task on Core 0
 * Call this from setup() after hardware initialization
 */
void setupTaskManager() {
    Serial.println("[TaskManager] Initializing...");

    // Create mutex for audio state protection
    audioMutex = xSemaphoreCreateMutex();
    if (audioMutex == NULL) {
        Serial.println("[TaskManager] ERROR: Failed to create audio mutex!");
        return;
    }

    // Create queue for decoded characters
    decodedCharQueue = xQueueCreate(DECODED_CHAR_QUEUE_SIZE, sizeof(char));
    if (decodedCharQueue == NULL) {
        Serial.println("[TaskManager] ERROR: Failed to create decoded char queue!");
        return;
    }

    // Create audio task pinned to Core 0
    BaseType_t result = xTaskCreatePinnedToCore(
        audioTask,              // Task function
        "AudioTask",            // Task name
        AUDIO_TASK_STACK_SIZE,  // Stack size
        NULL,                   // Parameters
        AUDIO_TASK_PRIORITY,    // Priority
        &audioTaskHandle,       // Task handle
        AUDIO_TASK_CORE         // Core ID
    );

    if (result != pdPASS) {
        Serial.println("[TaskManager] ERROR: Failed to create audio task!");
        return;
    }

    Serial.printf("[TaskManager] Audio task created on Core %d with priority %d\n",
                  AUDIO_TASK_CORE, AUDIO_TASK_PRIORITY);
    Serial.printf("[TaskManager] UI runs on Core %d (Arduino loop)\n", 1);
}

/*
 * Check if audio task is running
 */
bool isAudioTaskRunning() {
    return audioTaskRunning;
}

/*
 * Send a decoded character to the UI queue
 * Called from decoder running on audio task
 */
void sendDecodedChar(char c) {
    if (decodedCharQueue != NULL) {
        xQueueSend(decodedCharQueue, &c, 0);  // Non-blocking
    }
}

#endif // TASK_MANAGER_H
