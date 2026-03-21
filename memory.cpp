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

void recordKeyEvent(RecordingState& state, bool keyDown, uint8_t paddle) {
  if (!state.isRecording) return;

  unsigned long now = millis();
  unsigned long duration = now - state.lastEventTime;

  // Handle the state transition
  if (keyDown != state.keyCurrentlyDown) {
    // On very first key down, don't record the delay before it - just start timing
    if (state.transitionCount == 0 && !state.keyCurrentlyDown && keyDown) {
      Serial.print("REC: First key DOWN - starting timing (paddle=");
      Serial.print(paddle == PADDLE_DIT_FLAG ? "DIT" : "DAH");
      Serial.println(")");
      state.currentPaddle = paddle;
    } else if (state.transitionCount < MAX_TRANSITIONS_PER_MEMORY) {
      // Record the previous transition with its paddle info
      uint16_t encodedTransition = ENCODE_TRANSITION(state.currentPaddle, duration);
      state.transitions[state.transitionCount++] = encodedTransition;

      Serial.print("REC[");
      Serial.print(state.transitionCount - 1);
      Serial.print("]: ");
      Serial.print(state.keyCurrentlyDown ? "DN" : "UP");  // Print what we just recorded (previous state duration)
      Serial.print(" paddle=");
      Serial.print(state.currentPaddle == PADDLE_DIT_FLAG ? "DIT" : "DAH");
      Serial.print(" dur=");
      Serial.print(duration);
      Serial.println("ms");

      // If this was a key-up event ending, update the last key-release time
      if (!keyDown) {
        state.lastKeyReleaseTime = now;
      }

      // Update paddle for next transition if key is going down
      if (keyDown) {
        state.currentPaddle = paddle;
      }
    }

    state.keyCurrentlyDown = keyDown;
    state.lastEventTime = now;
  } else if (keyDown && paddle != state.currentPaddle) {
    // Paddle changed while key is still down (shouldn't normally happen, but handle it)
    state.currentPaddle = paddle;
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

  // If the last recorded transition was a key-down (odd count), we need to add a final key-up
  // to properly end the tone. Use a short fixed duration (50ms) for this final spacing.
  if (state.transitionCount > 0 && (state.transitionCount % 2) == 1) {
    const uint16_t FINAL_KEY_UP_DURATION = 50; // Short spacing to end the final tone
    if (state.transitionCount < MAX_TRANSITIONS_PER_MEMORY) {
      uint16_t encodedTransition = ENCODE_TRANSITION(state.currentPaddle, FINAL_KEY_UP_DURATION);
      state.transitions[state.transitionCount++] = encodedTransition;
      Serial.print("Added final key-UP transition: ");
      Serial.print(FINAL_KEY_UP_DURATION);
      Serial.println("ms");
    }
  }

  // Copy the recording to the memory structure
  memory.transitionCount = state.transitionCount;
  for (uint16_t i = 0; i < state.transitionCount; i++) {
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

  // Check if we're past the last transition (cleanup phase)
  if (state.currentTransitionIndex >= state.memory->transitionCount) {
    // If key is still down, turn it off
    if (state.keyCurrentlyDown) {
      state.keyCurrentlyDown = false;
      Serial.println("Final key-UP before playback complete");
      return; // Let main loop process the key-up, then we'll stop on next update
    }
    // Key is up, safe to stop
    state.stopPlayback();
    Serial.println("Playback complete");
    return;
  }

  unsigned long now = millis();
  unsigned long elapsed = now - state.transitionStartTime;

  // Decode current transition
  uint16_t encodedTransition = state.memory->transitions[state.currentTransitionIndex];
  uint16_t duration = DECODE_DURATION(encodedTransition);
  uint8_t paddle = DECODE_PADDLE(encodedTransition);

  // Check if current transition is complete
  if (elapsed >= duration) {
    // Log the transition that just completed (showing the state that WAS active)
    Serial.print("PLAY[");
    Serial.print(state.currentTransitionIndex);
    Serial.print("]: ");
    Serial.print(state.keyCurrentlyDown ? "DN" : "UP");  // Current state (before toggle)
    Serial.print(" paddle=");
    Serial.print(paddle == PADDLE_DIT_FLAG ? "DIT" : "DAH");
    Serial.print(" dur=");
    Serial.print(duration);
    Serial.println("ms");

    // Toggle key state for next transition
    state.keyCurrentlyDown = !state.keyCurrentlyDown;

    // Move to next transition
    state.currentTransitionIndex++;

    // Start timing for the next transition (if there is one)
    if (state.currentTransitionIndex < state.memory->transitionCount) {
      state.transitionStartTime = now;
      // Decode the paddle for the NEXT transition (for correct routing on next key-down)
      uint16_t nextEncoded = state.memory->transitions[state.currentTransitionIndex];
      state.currentPaddle = DECODE_PADDLE(nextEncoded);
    }
  }
}

