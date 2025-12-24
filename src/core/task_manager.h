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
        // Process any pending audio requests
        processAudioRequests();

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
