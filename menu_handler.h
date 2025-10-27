#ifndef MENU_HANDLER_H
#define MENU_HANDLER_H

#include <Arduino.h>
#include "buttons.h"
#include "adapter.h"
#include "memory.h"

// Operating modes
typedef enum {
  MODE_NORMAL = 0,
  MODE_SPEED_SETTING,
  MODE_TONE_SETTING,
  MODE_KEY_SETTING,
  MODE_MEMORY_MANAGEMENT,
  MODE_RECORDING_MEMORY_1,
  MODE_RECORDING_MEMORY_2,
  MODE_RECORDING_MEMORY_3,
  MODE_PLAYING_MEMORY
} OperatingMode;

// Menu handler state structure
struct MenuHandlerState {
  OperatingMode currentMode;
  int tempSpeedWPM;
  uint8_t tempToneNote;
  uint8_t tempKeyerType;
  unsigned long lastActivityTime;
};

// Initialize menu handler
void initMenuHandler(VailAdapter* adapterRef,
                     CWMemory* memoryRef,
                     RecordingState* recordingRef,
                     PlaybackState* playbackRef);

// Get current menu state
MenuHandlerState& getMenuState();

// Update menu handler (call from main loop)
void updateMenuHandler(unsigned long currentTime, ButtonDebouncer& buttonDebouncer);

// Helper functions exposed for main loop
const char* buttonStateToString(ButtonState state);

// Conversion utilities
int ditDurationToWPM(uint16_t ditDuration);
uint16_t wpmToDitDuration(int wpm);

// Apply temporary settings (for testing before committing)
void applyTemporarySpeed(int wpm);
void applyTemporaryTone(uint8_t noteNumber);
void applyTemporaryKeyerType(uint8_t keyerType);

#endif // MENU_HANDLER_H
