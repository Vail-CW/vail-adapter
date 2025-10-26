#include "memory.h"

// Note: EEPROM operations are in main .ino file to avoid linking issues

// ============================================================================
// Recording Operations
// ============================================================================

void startRecording(RecordingState& state, uint8_t slotNumber) {
  if (slotNumber >= MAX_MEMORY_SLOTS) return;

  state.startRecording(slotNumber);

  Serial.print("Started recording to memory slot ");
  Serial.println(slotNumber + 1);
}

void recordKeyEvent(RecordingState& state, bool keyDown) {
  if (!state.isRecording) return;

  unsigned long now = millis();
  unsigned long duration = now - state.lastEventTime;

  // Handle the state transition
  if (keyDown != state.keyCurrentlyDown) {
    // On very first key down, don't record the delay before it - just start timing
    if (state.transitionCount == 0 && !state.keyCurrentlyDown && keyDown) {
      Serial.println("REC: First key DOWN - starting timing");
    } else if (state.transitionCount < MAX_TRANSITIONS_PER_MEMORY) {
      // Record all other transitions
      state.transitions[state.transitionCount++] = (uint16_t)duration;

      Serial.print("REC[");
      Serial.print(state.transitionCount - 1);
      Serial.print("]: ");
      Serial.print(state.keyCurrentlyDown ? "DN" : "UP");  // Print what we just recorded (previous state duration)
      Serial.print(" dur=");
      Serial.print(duration);
      Serial.println("ms");

      // If this was a key-up event ending, update the last key-release time
      if (!keyDown) {
        state.lastKeyReleaseTime = now;
      }
    }

    state.keyCurrentlyDown = keyDown;
    state.lastEventTime = now;
  }
}

void stopRecording(RecordingState& state, CWMemory& memory) {
  if (!state.isRecording) return;

  // Calculate the time elapsed since the last key-release
  unsigned long now = millis();
  unsigned long timeSinceLastRelease = now - state.lastKeyReleaseTime;

  Serial.print("Recording stopped. Time since last key release: ");
  Serial.print(timeSinceLastRelease);
  Serial.println("ms");

  // Trim the recording to end at the last key-release
  // We need to find which transition corresponds to the last key-release
  // and discard everything after it

  // Calculate the trimmed count
  uint16_t trimmedCount = state.transitionCount;

  // If the last event was a key-down (key still held), we keep it
  // If the last event was a key-up, we're already trimmed correctly

  // Copy the trimmed recording to the memory structure
  memory.transitionCount = trimmedCount;
  for (uint16_t i = 0; i < trimmedCount; i++) {
    memory.transitions[i] = state.transitions[i];
  }

  state.stopRecording();

  Serial.print("Recorded ");
  Serial.print(memory.transitionCount);
  Serial.print(" transitions (");
  Serial.print(memory.getDurationMs());
  Serial.println("ms)");
}

// ============================================================================
// Playback Operations
// ============================================================================

bool startPlayback(PlaybackState& state, uint8_t slotNumber, CWMemory& memory) {
  if (slotNumber >= MAX_MEMORY_SLOTS) return false;
  if (memory.isEmpty()) return false;

  state.startPlayback(slotNumber, &memory);

  Serial.print("Started playback of memory slot ");
  Serial.print(slotNumber + 1);
  Serial.print(" (");
  Serial.print(memory.transitionCount);
  Serial.print(" transitions, ");
  Serial.print(memory.getDurationMs());
  Serial.println("ms)");

  return true;
}

void updatePlayback(PlaybackState& state) {
  if (!state.isPlaying || state.memory == nullptr) return;

  unsigned long now = millis();
  unsigned long elapsed = now - state.transitionStartTime;

  // Check if current transition is complete
  if (elapsed >= state.memory->transitions[state.currentTransitionIndex]) {
    // Move to next transition
    state.currentTransitionIndex++;

    // Check if we've reached the end
    if (state.currentTransitionIndex >= state.memory->transitionCount) {
      state.stopPlayback();
      Serial.println("Playback complete");
      return;
    }

    // Start the next transition
    state.transitionStartTime = now;
    state.keyCurrentlyDown = !state.keyCurrentlyDown;

    Serial.print("PLAY[");
    Serial.print(state.currentTransitionIndex);
    Serial.print("]: ");
    Serial.print(state.keyCurrentlyDown ? "DN" : "UP");
    Serial.print(" dur=");
    Serial.print(state.memory->transitions[state.currentTransitionIndex]);
    Serial.println("ms");
  }
}

