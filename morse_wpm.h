/*
 * Morse WPM Timing Utilities
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

#ifndef MORSE_WPM_H
#define MORSE_WPM_H

namespace MorseWPM {
  // PARIS standard: 50 dit units per word
  constexpr float DITS_PER_WORD = 50.0f;
  constexpr float MS_IN_MINUTE = 60000.0f;

  /**
   * Calculate dit length in milliseconds for a given WPM
   * Based on PARIS standard: "PARIS " = 50 dit units
   * @param wpm Words per minute
   * @return Dit duration in milliseconds
   */
  inline float ditLength(float wpm) {
    return (MS_IN_MINUTE / DITS_PER_WORD) / wpm;
  }

  /**
   * Calculate dah length in milliseconds for a given WPM
   * Standard timing: dah = 3 × dit
   * @param wpm Words per minute
   * @return Dah duration in milliseconds
   */
  inline float dahLength(float wpm) {
    return 3.0f * ditLength(wpm);
  }

  /**
   * Calculate WPM from dit length
   * @param ditLen Dit duration in milliseconds
   * @return Words per minute
   */
  inline float wpm(float ditLen) {
    if (ditLen <= 0) return 0.0f;
    return (MS_IN_MINUTE / DITS_PER_WORD) / ditLen;
  }

  /**
   * Calculate element gap (inter-element space) in milliseconds
   * Standard timing: element gap = 1 × dit
   * @param wpm Words per minute
   * @return Element gap duration in milliseconds
   */
  inline float elementGap(float wpm) {
    return ditLength(wpm);
  }

  /**
   * Calculate character gap (space between letters) in milliseconds
   * Standard timing: character gap = 3 × dit
   * @param wpm Words per minute
   * @return Character gap duration in milliseconds
   */
  inline float characterGap(float wpm) {
    return 3.0f * ditLength(wpm);
  }

  /**
   * Calculate word gap (space between words) in milliseconds
   * Standard timing: word gap = 7 × dit
   * @param wpm Words per minute
   * @return Word gap duration in milliseconds
   */
  inline float wordGap(float wpm) {
    return 7.0f * ditLength(wpm);
  }

  /**
   * Calculate Farnsworth ratio for spacing extension
   * Farnsworth timing slows down gaps between characters/words while
   * keeping character speeds fast for learning
   * @param wpm Character speed (WPM)
   * @param fwpm Effective speed (Farnsworth WPM)
   * @return Ratio for spacing extension
   */
  inline float farnsworthRatio(float wpm, float fwpm) {
    if (fwpm <= 0 || wpm <= 0 || fwpm >= wpm) return 1.0f;
    return (50.0f * wpm - 31.0f * fwpm) / (19.0f * fwpm);
  }

  /**
   * Calculate Farnsworth dit length for spacing
   * @param wpm Character speed (WPM)
   * @param fwpm Effective speed (Farnsworth WPM)
   * @return Farnsworth dit duration in milliseconds
   */
  inline float farnsworthDitLength(float wpm, float fwpm) {
    return ditLength(wpm) * farnsworthRatio(wpm, fwpm);
  }

  /**
   * Calculate Farnsworth character gap
   * @param wpm Character speed (WPM)
   * @param fwpm Effective speed (Farnsworth WPM)
   * @return Character gap duration in milliseconds
   */
  inline float farnsworthCharacterGap(float wpm, float fwpm) {
    return 3.0f * farnsworthDitLength(wpm, fwpm);
  }

  /**
   * Calculate Farnsworth word gap
   * @param wpm Character speed (WPM)
   * @param fwpm Effective speed (Farnsworth WPM)
   * @return Word gap duration in milliseconds
   */
  inline float farnsworthWordGap(float wpm, float fwpm) {
    return 7.0f * farnsworthDitLength(wpm, fwpm);
  }
}

#endif // MORSE_WPM_H
