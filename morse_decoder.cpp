/*
 * Morse Code Decoder Implementation
 *
 * Ported from morse-pro library by Stephen C. Phillips
 * Adapted for Arduino/SAMD21 embedded use.
 */

#include "morse_decoder.h"

MorseDecoder::MorseDecoder() {
  onCharacter = nullptr;
  onBackspace = nullptr;
  onEnter = nullptr;
  onSpace = nullptr;
  onError = nullptr;
  onWordGap = nullptr;
  adaptive = true;
}

void MorseDecoder::begin(uint16_t initialDitLen) {
  setDitLength(initialDitLen);
  reset();
}

void MorseDecoder::reset() {
  currentPattern = 0;
  patternLength = 0;
  bufferIndex = 0;
  bufferCount = 0;
  lastEventTime = 0;
  pendingFlush = false;
  lastTiming = 0;
  keyIsDown = false;
  suppressNextSpace = false;
  pendingWordSpace = false;

  // Clear timing buffer
  for (uint8_t i = 0; i < TIMING_BUFFER_SIZE; i++) {
    ditBuffer[i] = 0;
  }
}

void MorseDecoder::setDitLength(uint16_t len) {
  if (len < MIN_DIT_LEN) len = MIN_DIT_LEN;
  if (len > MAX_DIT_LEN) len = MAX_DIT_LEN;
  ditLen = len;
  updateThresholds();
}

void MorseDecoder::updateThresholds() {
  // Threshold between dit (1x) and dah (3x) at midpoint (2x)
  ditDahThreshold = ditLen * 2;

  // Standard PARIS timing puts an element space at 1 dit and a character
  // space at 3 dits, so the boundary between them sits at 2 dits. Hand sent
  // code routinely lands character gaps a little short of 3 dits, and a
  // 3 dit threshold merged neighbouring letters into one invalid pattern.
  charSpaceThreshold = ditLen * 2;
  dahSpaceThreshold = ditLen * 7;
}

void MorseDecoder::addTiming(int16_t duration) {
  if (duration == 0) return;

  unsigned long now = millis();

  Serial.print(F("DECODER: addTiming(")); Serial.print(duration); Serial.println(F(")"));
  Serial.print(F("  thresholds: ditDah=")); Serial.print(ditDahThreshold);
  Serial.print(F(" charSpace=")); Serial.print(charSpaceThreshold);
  Serial.print(F(" wordSpace=")); Serial.println(dahSpaceThreshold);

  // Handle noise: ignore very short durations
  if (abs(duration) <= NOISE_THRESHOLD) {
    Serial.println(F("  -> ignored (noise)"));
    return;
  }

  // Process immediately based on type
  if (duration > 0) {
    // TONE: classify as dit or dah and add to pattern
    bool isDah = (duration >= ditDahThreshold);
    Serial.print(F("  -> element: ")); Serial.println(isDah ? "DAH" : "DIT");
    addElement(isDah);

    // Update adaptive timing
    if (adaptive) {
      updateAdaptive(duration, isDah);
    }
  } else {
    // SILENCE: check if it's a character or word boundary
    uint16_t silence = -duration;

    if (silence >= dahSpaceThreshold) {
      // Word space - decode current pattern, then maybe output space
      Serial.println(F("  -> WORD SPACE"));
      if (patternLength > 0) {
        decodePattern(true);  // Force output - word space is definitive end
      }
      // Report the gap itself. This is independent of whether a space
      // character gets typed, so listeners that only care about word
      // boundaries (KSKS detection) see every gap, suppressed or not. It
      // runs after the pending pattern is decoded so the letter before the
      // gap does not consume the boundary meant for the letter after it.
      if (onWordGap) onWordGap();
      // Output space unless suppressed (e.g., after backspace)
      if (suppressNextSpace) {
        Serial.println(F("  -> SPACE SUPPRESSED (after backspace)"));
        suppressNextSpace = false;
      } else {
        // Don't output space yet - mark it pending
        // We'll output it when we know the next pattern isn't a backspace
        pendingWordSpace = true;
        Serial.println(F("  -> SPACE PENDING (waiting to see if backspace follows)"));
      }
    } else if (silence >= charSpaceThreshold) {
      // Character space - decode current pattern
      Serial.println(F("  -> CHAR SPACE"));
      if (patternLength > 0) {
        decodePattern();
      }
    } else {
      // Element space - just wait for more elements
      Serial.println(F("  -> element space (continuing pattern)"));
    }
  }

  lastEventTime = now;
  pendingFlush = (patternLength > 0);
}

void MorseDecoder::tick(unsigned long currentTime) {
  // Only process if we have a pending pattern and valid lastEventTime
  if (patternLength == 0 || lastEventTime == 0) {
    return;
  }

  // IMPORTANT: Only trigger timeout during SILENCE (key up)
  // If key is still down, we're in the middle of an element - don't decode yet
  if (keyIsDown) {
    return;
  }

  unsigned long elapsed = currentTime - lastEventTime;

  // Sanity check - don't process if elapsed time is unreasonably large (overflow protection)
  if (elapsed > 30000) {
    return;
  }

  // Once the silence reaches the character space threshold, treat it as the
  // end of the character and decode what we have.
  if (elapsed >= charSpaceThreshold) {
    Serial.print(F("TICK: timeout after ")); Serial.print(elapsed);
    Serial.print(F("ms (threshold=")); Serial.print(charSpaceThreshold); Serial.println(F("ms), decoding"));
    decodePattern(true);  // Force output - timeout is definitive end
    lastEventTime = currentTime;  // Reset to prevent repeated triggering
  }
}

void MorseDecoder::flush() {
  // Decode any pending pattern
  if (patternLength > 0) {
    decodePattern(true);  // Force output - explicit flush
  }
  pendingFlush = false;
}

void MorseDecoder::addElement(bool isDah) {
  // Allow patterns longer than 8 to support 8+ dits for backspace
  // But if we get a dah after 8+ elements, decode first (no valid pattern that long with dahs)
  if (patternLength >= 8 && isDah) {
    // Pattern too long and has dahs - decode what we have and start fresh
    decodePattern();
  }

  // Add element to pattern (LSB first)
  // Note: currentPattern is uint8_t so bits beyond 8 are lost, but that's OK
  // since 8+ element patterns are only valid if all dits (backspace)
  if (isDah) {
    currentPattern |= (1 << patternLength);
  }
  patternLength++;

  Serial.print(F("  pattern now: 0b"));
  for (int i = patternLength - 1; i >= 0; i--) {
    Serial.print((currentPattern >> i) & 1);
  }
  Serial.print(F(" len=")); Serial.println(patternLength);
}

void MorseDecoder::decodePattern(bool forceOutput) {
  if (patternLength == 0) return;

  Serial.print(F("DECODE: pattern=0b"));
  for (int i = patternLength - 1; i >= 0; i--) {
    Serial.print((currentPattern >> i) & 1);
  }
  Serial.print(F(" len=")); Serial.println(patternLength);

  // Special case: 8 or more dits = backspace
  if (patternLength >= 8 && currentPattern == 0) {
    Serial.print(F("  -> BACKSPACE (")); Serial.print(patternLength); Serial.println(F(" dits)"));
    // Cancel any pending word space - backspace means user is correcting
    if (pendingWordSpace) {
      Serial.println(F("  -> PENDING SPACE CANCELLED (backspace)"));
      pendingWordSpace = false;
    }
    if (onBackspace) {
      onBackspace();
    }
    // Suppress next space to prevent unwanted space after backspace
    suppressNextSpace = true;
    currentPattern = 0;
    patternLength = 0;
    return;
  }

  // Special case: BK prosign (-...-.-) = enter
  // Pattern: dah dit dit dit dah dit dah = 0b1010001, length 7
  // Check BK BEFORE BT since BK starts with BT pattern
  if (patternLength == 7 && currentPattern == 0b1010001) {
    // Cancel pending space - don't want trailing space before newline
    if (pendingWordSpace) {
      Serial.println(F("  -> PENDING SPACE CANCELLED (before enter)"));
      pendingWordSpace = false;
    }
    Serial.println(F("  -> ENTER (BK)"));
    if (onEnter) {
      onEnter();
    }
    // Suppress next automatic word space - BK already starts a new line
    suppressNextSpace = true;
    currentPattern = 0;
    patternLength = 0;
    return;
  }

  // Special case: BT prosign (-...-) = space
  // Pattern: dah dit dit dit dah = 0b10001, length 5
  // Only trigger on timeout (forceOutput=true), not on char space
  // This allows BK (-...-.-) to complete without BT triggering first
  if (patternLength == 5 && currentPattern == 0b10001 && forceOutput) {
    // Output pending space first if any
    if (pendingWordSpace) {
      Serial.println(F("  -> OUTPUT PENDING SPACE (before BT space)"));
      if (onSpace) onSpace();
      pendingWordSpace = false;
    }
    Serial.println(F("  -> SPACE (BT)"));
    if (onSpace) {
      onSpace();
    }
    // Suppress next automatic word space - BT already inserted a space
    suppressNextSpace = true;
    currentPattern = 0;
    patternLength = 0;
    return;
  }

  // Look up in table
  char c = lookupPattern(currentPattern, patternLength);

  if (c != 0) {
    // Output any pending word space before the character
    if (pendingWordSpace) {
      Serial.println(F("  -> OUTPUT PENDING SPACE (before char)"));
      if (onSpace) onSpace();
      pendingWordSpace = false;
    }
    // A real character clears any space suppression left over from a
    // backspace, BK or BT. Without this a gap several characters later was
    // still being swallowed.
    suppressNextSpace = false;
    Serial.print(F("  -> CHAR: ")); Serial.println(c);
    if (onCharacter) {
      onCharacter(c);
    }
  } else {
    // INVALID pattern - cancel pending space and play error
    // (user sent garbage - don't output the pending space)
    if (pendingWordSpace) {
      Serial.println(F("  -> PENDING SPACE CANCELLED (invalid pattern)"));
      pendingWordSpace = false;
    }
    suppressNextSpace = true;  // Also suppress next auto-space

    // Play error feedback
    if (onError) onError();

    Serial.println(F("  -> NOT FOUND (ignored)"));
  }

  currentPattern = 0;
  patternLength = 0;
}

char MorseDecoder::lookupPattern(uint8_t pattern, uint8_t length) {
  for (uint8_t i = 0; i < MORSE_TABLE_SIZE; i++) {
    MorseEntry entry;
    memcpy_P(&entry, &morseTable[i], sizeof(MorseEntry));

    if (entry.length == length && entry.pattern == pattern) {
      // Skip special characters (handled elsewhere)
      if (entry.character == MORSE_BACKSPACE || entry.character == MORSE_ENTER) {
        continue;
      }
      return entry.character;
    }
  }
  return 0; // Not found
}

void MorseDecoder::updateAdaptive(uint16_t duration, bool isDah) {
  // Convert to dit-equivalent duration
  uint16_t ditEquivalent;
  if (isDah) {
    ditEquivalent = duration / 3;
  } else {
    ditEquivalent = duration;
  }

  // Sanity check - don't accept extremely short or long values
  if (ditEquivalent < MIN_DIT_LEN || ditEquivalent > MAX_DIT_LEN) {
    return;
  }

  // Add to rolling buffer
  ditBuffer[bufferIndex] = ditEquivalent;
  bufferIndex = (bufferIndex + 1) % TIMING_BUFFER_SIZE;
  if (bufferCount < TIMING_BUFFER_SIZE) {
    bufferCount++;
  }

  // Calculate weighted average (linear weighting - newer is heavier)
  uint32_t sum = 0;
  uint16_t totalWeight = 0;

  for (uint8_t i = 0; i < bufferCount; i++) {
    // Weight increases linearly from 1 to bufferCount
    // Newest entry (at bufferIndex-1) gets highest weight
    uint8_t age = (bufferIndex - 1 - i + TIMING_BUFFER_SIZE) % TIMING_BUFFER_SIZE;
    uint8_t weight = bufferCount - age;
    if (weight < 1) weight = 1;

    sum += (uint32_t)ditBuffer[i] * weight;
    totalWeight += weight;
  }

  if (totalWeight > 0) {
    setDitLength(sum / totalWeight);
  }
}
