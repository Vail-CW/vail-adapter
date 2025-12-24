/*
 * Ham Radio License Study - Data Structures and SD Card Loading
 * Handles question pool loading from SD card JSON files
 */

#ifndef TRAINING_LICENSE_DATA_H
#define TRAINING_LICENSE_DATA_H

#include <SD.h>
#include <ArduinoJson.h>
#include "../storage/sd_card.h"
#include "../core/config.h"

// Forward declaration from training_license_core.h
struct QuestionProgress;

// ============================================
// Question Data Structures
// ============================================

struct LicenseQuestion {
  char id[8];              // Question ID (e.g., "T1A01")
  char question[256];      // Question text
  char answers[4][80];     // 4 answer choices (A, B, C, D)
  uint8_t correctAnswer;   // Correct answer index (0=A, 1=B, 2=C, 3=D)
  char refs[64];           // Reference material (optional, e.g., "[97.1]")
};

struct QuestionPool {
  const char* license;           // License name: "Technician", "General", "Extra"
  const char* filename;          // SD card path: "/sd/license/technician.json"
  int totalQuestions;            // Total questions in pool
  LicenseQuestion* questions;    // Dynamically allocated array of questions
  QuestionProgress* progress;    // Progress tracking array (loaded from Preferences)
  bool loaded;                   // Load status flag
};

// ============================================
// Question Pool Instances
// ============================================

// Three question pools (only one loaded at a time to save RAM)
QuestionPool techPool = {
  "Technician",
  "/license/technician.json",
  0,      // Will be set during load
  nullptr,
  nullptr,
  false
};

QuestionPool genPool = {
  "General",
  "/license/general.json",
  0,
  nullptr,
  nullptr,
  false
};

QuestionPool extraPool = {
  "Extra",
  "/license/extra.json",
  0,
  nullptr,
  nullptr,
  false
};

// Pointer to currently active pool
QuestionPool* activePool = nullptr;

// ============================================
// Question Pool Loading Functions
// ============================================

/**
 * Load question pool from SD card JSON file
 * Returns true on success, false on error
 */
bool loadQuestionPool(QuestionPool* pool) {
  if (pool->loaded) {
    Serial.println("Question pool already loaded");
    return true;
  }

  if (!sdCardAvailable) {
    Serial.println("ERROR: SD card not available");
    return false;
  }

  Serial.print("Loading question pool: ");
  Serial.println(pool->filename);

  // Open JSON file
  File file = SD.open(pool->filename);
  if (!file) {
    Serial.print("ERROR: Failed to open file: ");
    Serial.println(pool->filename);
    return false;
  }

  // Read file into buffer
  size_t fileSize = file.size();
  Serial.print("File size: ");
  Serial.print(fileSize);
  Serial.println(" bytes");

  char* jsonBuffer = (char*)malloc(fileSize + 1);
  if (!jsonBuffer) {
    Serial.println("ERROR: Failed to allocate JSON buffer");
    file.close();
    return false;
  }

  file.readBytes(jsonBuffer, fileSize);
  jsonBuffer[fileSize] = '\0';
  file.close();

  // Parse JSON
  DynamicJsonDocument doc(fileSize + 2048);  // Add buffer for parsing overhead
  DeserializationError error = deserializeJson(doc, jsonBuffer);
  free(jsonBuffer);

  if (error) {
    Serial.print("ERROR: JSON parse failed: ");
    Serial.println(error.c_str());
    return false;
  }

  // Get questions array
  JsonArray questions = doc.as<JsonArray>();
  pool->totalQuestions = questions.size();

  if (pool->totalQuestions == 0) {
    Serial.println("ERROR: No questions found in file");
    return false;
  }

  Serial.print("Found ");
  Serial.print(pool->totalQuestions);
  Serial.println(" questions");

  // Allocate question array
  pool->questions = (LicenseQuestion*)malloc(sizeof(LicenseQuestion) * pool->totalQuestions);
  if (!pool->questions) {
    Serial.println("ERROR: Failed to allocate question array");
    return false;
  }

  // Parse each question
  int idx = 0;
  for (JsonObject q : questions) {
    LicenseQuestion* lq = &pool->questions[idx];

    // Copy question ID
    const char* id = q["id"] | "";
    strncpy(lq->id, id, sizeof(lq->id) - 1);
    lq->id[sizeof(lq->id) - 1] = '\0';

    // Copy question text
    const char* question = q["question"] | "";
    strncpy(lq->question, question, sizeof(lq->question) - 1);
    lq->question[sizeof(lq->question) - 1] = '\0';

    // Copy answers
    JsonArray answers = q["answers"];
    for (int i = 0; i < 4 && i < answers.size(); i++) {
      const char* answer = answers[i] | "";
      strncpy(lq->answers[i], answer, sizeof(lq->answers[i]) - 1);
      lq->answers[i][sizeof(lq->answers[i]) - 1] = '\0';
    }

    // Fill remaining answers with empty strings
    for (int i = answers.size(); i < 4; i++) {
      lq->answers[i][0] = '\0';
    }

    // Parse correct answer
    // Format can be either integer index (0-3) or letter ("A"-"D")
    if (q["correct"].is<int>()) {
      lq->correctAnswer = q["correct"];
    } else {
      const char* correctStr = q["correct"] | "A";
      lq->correctAnswer = correctStr[0] - 'A';
    }

    // Validate correct answer index
    if (lq->correctAnswer > 3) {
      lq->correctAnswer = 0;
    }

    // Copy references (optional field)
    lq->refs[0] = '\0';  // Default to empty
    if (q.containsKey("refs")) {
      if (q["refs"].is<JsonArray>()) {
        // Array of references
        JsonArray refs = q["refs"];
        String refStr = "";
        for (JsonVariant ref : refs) {
          if (refStr.length() > 0) refStr += ", ";
          refStr += ref.as<String>();
        }
        strncpy(lq->refs, refStr.c_str(), sizeof(lq->refs) - 1);
        lq->refs[sizeof(lq->refs) - 1] = '\0';
      } else {
        // Single reference string
        const char* refs = q["refs"] | "";
        strncpy(lq->refs, refs, sizeof(lq->refs) - 1);
        lq->refs[sizeof(lq->refs) - 1] = '\0';
      }
    }

    idx++;
  }

  pool->loaded = true;
  Serial.print("Successfully loaded ");
  Serial.print(pool->totalQuestions);
  Serial.print(" questions for ");
  Serial.println(pool->license);

  return true;
}

/**
 * Unload question pool and free memory
 */
void unloadQuestionPool(QuestionPool* pool) {
  if (pool->questions) {
    free(pool->questions);
    pool->questions = nullptr;
  }
  // Note: progress array is managed separately in training_license_core.h
  pool->loaded = false;

  Serial.print("Unloaded question pool: ");
  Serial.println(pool->license);
}

/**
 * Get question pool by license type
 */
QuestionPool* getQuestionPool(int licenseType) {
  switch (licenseType) {
    case 0: return &techPool;
    case 1: return &genPool;
    case 2: return &extraPool;
    default: return nullptr;
  }
}

/**
 * Get license name by type
 */
const char* getLicenseName(int licenseType) {
  switch (licenseType) {
    case 0: return "Technician";
    case 1: return "General";
    case 2: return "Extra";
    default: return "Unknown";
  }
}

/**
 * Get short license name for UI
 */
const char* getLicenseShortName(int licenseType) {
  switch (licenseType) {
    case 0: return "Tech";
    case 1: return "Gen";
    case 2: return "Extra";
    default: return "?";
  }
}

#endif // TRAINING_LICENSE_DATA_H
