#ifndef MEMORY_H
#define MEMORY_H

#include <Arduino.h>
// Note: FlashStorage_SAMD.h is included in main .ino file only to avoid linking issues

// ============================================================================
// CW MEMORY STORAGE SYSTEM
// ============================================================================
// This module implements 3 independent CW memory slots with run-length
// encoding for efficient storage of key timing sequences.
//
// Storage Format:
// - Each memory slot stores alternating key-down/key-up durations
// - Durations are in milliseconds (uint16_t, max 65.5 seconds per event)
// - Recording is automatically trimmed to end at last key-release
// - Maximum recording duration: 25 seconds
// - Maximum transitions: 200 per slot (conservative estimate)
// ============================================================================

// EEPROM Memory Map
// -----------------
// Addresses 0-5: Existing settings (see config.h)
// Address 6: Start of CW memory storage

#define EEPROM_MEMORY_START_ADDR 6

// Memory slot configuration
#define MAX_MEMORY_SLOTS 3
#define MAX_RECORDING_DURATION_MS 25000  // 25 seconds
#define MAX_TRANSITIONS_PER_MEMORY 200   // Conservative: ~8 transitions/sec * 25 sec

// Each memory slot structure in EEPROM:
// - 2 bytes: length (number of transitions stored)
// - 400 bytes: transition data (200 transitions × 2 bytes each)
// Total per slot: 402 bytes

#define MEMORY_SLOT_SIZE_BYTES 402
#define MEMORY_LENGTH_SIZE 2
#define MEMORY_DATA_SIZE (MAX_TRANSITIONS_PER_MEMORY * 2)

// EEPROM addresses for each memory slot
#define EEPROM_MEMORY_1_ADDR (EEPROM_MEMORY_START_ADDR)
#define EEPROM_MEMORY_2_ADDR (EEPROM_MEMORY_1_ADDR + MEMORY_SLOT_SIZE_BYTES)
#define EEPROM_MEMORY_3_ADDR (EEPROM_MEMORY_2_ADDR + MEMORY_SLOT_SIZE_BYTES)

// Total EEPROM usage: 6 (settings) + 3×402 (memories) = 1212 bytes
// SAMD21 has 16KB, so we're using < 8% of available space

// ============================================================================
// Data Structures
// ============================================================================

// In-memory representation of a CW memory slot
struct CWMemory {
  uint16_t transitionCount;           // Number of transitions stored (0 = empty)
  uint16_t transitions[MAX_TRANSITIONS_PER_MEMORY];  // Alternating key-down/key-up durations (ms)

  CWMemory() : transitionCount(0) {
    // Initialize all transitions to 0
    for (int i = 0; i < MAX_TRANSITIONS_PER_MEMORY; i++) {
      transitions[i] = 0;
    }
  }

  bool isEmpty() const {
    return transitionCount == 0;
  }

  void clear() {
    transitionCount = 0;
    // No need to clear the array, just reset the count
  }

  uint32_t getDurationMs() const {
    // Calculate total duration of the memory in milliseconds
    uint32_t total = 0;
    for (uint16_t i = 0; i < transitionCount; i++) {
      total += transitions[i];
    }
    return total;
  }
};

// Recording state for capturing live CW input
struct RecordingState {
  uint8_t slotNumber;                 // Which slot we're recording to (0-2)
  bool isRecording;                   // Currently recording flag
  unsigned long recordingStartTime;   // When recording started (millis())
  unsigned long lastEventTime;        // Time of last key event
  unsigned long lastKeyReleaseTime;   // Time of last key-release (for trimming)
  bool keyCurrentlyDown;              // Current state of the key
  uint16_t transitionCount;           // Number of transitions captured so far
  uint16_t transitions[MAX_TRANSITIONS_PER_MEMORY];  // Captured transitions

  RecordingState() : slotNumber(0), isRecording(false), recordingStartTime(0),
                      lastEventTime(0), lastKeyReleaseTime(0), keyCurrentlyDown(false),
                      transitionCount(0) {
    for (int i = 0; i < MAX_TRANSITIONS_PER_MEMORY; i++) {
      transitions[i] = 0;
    }
  }

  void startRecording(uint8_t slot) {
    slotNumber = slot;
    isRecording = true;
    recordingStartTime = millis();
    lastEventTime = recordingStartTime;
    lastKeyReleaseTime = recordingStartTime;
    keyCurrentlyDown = false;
    transitionCount = 0;
  }

  void stopRecording() {
    isRecording = false;
  }

  bool hasReachedMaxDuration() const {
    return (millis() - recordingStartTime) >= MAX_RECORDING_DURATION_MS;
  }

  bool hasReachedMaxTransitions() const {
    return transitionCount >= MAX_TRANSITIONS_PER_MEMORY;
  }
};

// Playback state for playing back a memory
struct PlaybackState {
  bool isPlaying;                     // Currently playing flag
  uint8_t slotNumber;                 // Which slot we're playing (0-2)
  uint16_t currentTransitionIndex;    // Which transition we're on
  unsigned long transitionStartTime;  // When current transition started
  bool keyCurrentlyDown;              // Current key state during playback
  CWMemory* memory;                   // Pointer to the memory being played

  PlaybackState() : isPlaying(false), slotNumber(0), currentTransitionIndex(0),
                     transitionStartTime(0), keyCurrentlyDown(false), memory(nullptr) {}

  void startPlayback(uint8_t slot, CWMemory* mem) {
    slotNumber = slot;
    memory = mem;
    isPlaying = true;
    currentTransitionIndex = 0;
    transitionStartTime = millis();
    keyCurrentlyDown = true;  // First transition is always key-down
  }

  void stopPlayback() {
    isPlaying = false;
    keyCurrentlyDown = false;
    memory = nullptr;
  }
};

// ============================================================================
// Function Declarations
// ============================================================================
// Note: EEPROM functions are declared in main .ino file to avoid linking issues

// Recording operations
void startRecording(RecordingState& state, uint8_t slotNumber);
void stopRecording(RecordingState& state, CWMemory& memory);
void recordKeyEvent(RecordingState& state, bool keyDown);

// Playback operations
bool startPlayback(PlaybackState& state, uint8_t slotNumber, CWMemory& memory);
void updatePlayback(PlaybackState& state);  // Call this in loop()

#endif // MEMORY_H
