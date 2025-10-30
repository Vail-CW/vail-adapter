/*
 * Adaptive Morse Code Decoder
 * Based on morse-pro by Stephen C Phillips (https://github.com/scp93ch/morse-pro)
 * Ported to C++ for ESP32/Arduino by VAIL SUMMIT project
 *
 * Original license: EUPL v1.2
 * Copyright (c) 2024 Stephen C Phillips
 * Modifications Copyright (c) 2025 VAIL SUMMIT Contributors
 *
 * Licensed under the European Union Public Licence (EUPL) v1.2
 * https://opensource.org/licenses/EUPL-1.2
 */

#ifndef MORSE_DECODER_ADAPTIVE_H
#define MORSE_DECODER_ADAPTIVE_H

#include "morse_decoder.h"

/**
 * MorseDecoderAdaptive - Adaptive morse code decoder
 * Extends base decoder with automatic speed tracking
 * Uses weighted averaging of recent timings to adapt to speed changes
 */
class MorseDecoderAdaptive : public MorseDecoder {
private:
  std::vector<float> ditLengths;   // Circular buffer of dit length estimates
  std::vector<float> fditLengths;  // Circular buffer of farnsworth dit estimates
  int bufferSize;                  // Maximum buffer size
  bool lockSpeed;                  // If true, disable adaptation

  /**
   * Calculate weighted average of buffer
   * Newer values weighted more heavily (linear: 1, 2, 3, ..., bufferSize)
   * @param buffer Vector of values
   * @return Weighted average
   */
  float weightedAverage(const std::vector<float>& buffer) {
    if (buffer.empty()) return 0.0f;

    float sum = 0.0f;
    float denominator = 0.0f;

    for (size_t i = 0; i < buffer.size(); i++) {
      float weight = (float)(i + 1);  // Linear weighting: 1, 2, 3, ...
      sum += buffer[i] * weight;
      denominator += weight;
    }

    return (denominator > 0) ? (sum / denominator) : 0.0f;
  }

protected:
  /**
   * Called after each element is decoded
   * Infers dit/fdit length from the element and updates speed estimate
   * @param duration Timing duration
   * @param character Decoded character (. - ' ' /)
   */
  void addDecode(float duration, char character) override {
    if (lockSpeed) return;  // Speed adaptation disabled

    float absDuration = abs(duration);
    float inferredDit = -1.0f;
    float inferredFdit = -1.0f;

    // Infer dit/fdit length based on element type
    switch (character) {
      case '.':
        // Dit = 1 dit duration
        inferredDit = absDuration;
        break;

      case '-':
        // Dah = 3 dits
        inferredDit = absDuration / 3.0f;
        break;

      case '\0':
        // Element gap (within character) = 1 dit
        inferredDit = absDuration;
        break;

      case ' ':
        // Character gap = 3 fdits
        inferredFdit = absDuration / 3.0f;
        break;

      case '/':
        // Word gap = 7 fdits
        inferredFdit = absDuration / 7.0f;
        break;
    }

    // Add to circular buffers
    if (inferredDit > 0) {
      ditLengths.push_back(inferredDit);
      if (ditLengths.size() > (size_t)bufferSize) {
        ditLengths.erase(ditLengths.begin());  // Remove oldest
      }
    }

    if (inferredFdit > 0) {
      fditLengths.push_back(inferredFdit);
      if (fditLengths.size() > (size_t)bufferSize) {
        fditLengths.erase(fditLengths.begin());  // Remove oldest
      }
    }

    // Update estimates using weighted average
    if (!ditLengths.empty()) {
      ditLen = weightedAverage(ditLengths);
    }

    if (!fditLengths.empty()) {
      fditLen = weightedAverage(fditLengths);
    }

    // Update thresholds
    updateThresholds();

    // Trigger speed callback
    if (speedCallback != nullptr) {
      speedCallback(getWPM(), getFarnsworthWPM());
    }
  }

public:
  /**
   * Constructor
   * @param wpm Initial words per minute estimate
   * @param fwpm Initial Farnsworth WPM estimate
   * @param bufSize Buffer size for averaging (default 30)
   */
  MorseDecoderAdaptive(float wpm = 20.0f, float fwpm = 20.0f, int bufSize = 30)
    : MorseDecoder(wpm, fwpm), bufferSize(bufSize), lockSpeed(false) {
    ditLengths.reserve(bufSize);
    fditLengths.reserve(bufSize);
  }

  /**
   * Set speed lock (disable/enable adaptation)
   * @param lock True to lock speed, false to enable adaptation
   */
  void setSpeedLock(bool lock) {
    lockSpeed = lock;
  }

  /**
   * Get speed lock status
   * @return True if speed is locked
   */
  bool isSpeedLocked() const {
    return lockSpeed;
  }

  /**
   * Set buffer size for adaptive averaging
   * @param size Number of samples to average
   */
  void setBufferSize(int size) {
    bufferSize = size;

    // Trim buffers if needed
    while (ditLengths.size() > (size_t)bufferSize) {
      ditLengths.erase(ditLengths.begin());
    }
    while (fditLengths.size() > (size_t)bufferSize) {
      fditLengths.erase(fditLengths.begin());
    }
  }

  /**
   * Get current buffer size
   * @return Buffer size
   */
  int getBufferSize() const {
    return bufferSize;
  }

  /**
   * Get number of samples in dit buffer
   * @return Sample count
   */
  int getDitSampleCount() const {
    return ditLengths.size();
  }

  /**
   * Get number of samples in fdit buffer
   * @return Sample count
   */
  int getFditSampleCount() const {
    return fditLengths.size();
  }

  /**
   * Reset decoder state (clears buffers)
   */
  void reset() {
    MorseDecoder::reset();
    ditLengths.clear();
    fditLengths.clear();
  }
};

#endif // MORSE_DECODER_ADAPTIVE_H
