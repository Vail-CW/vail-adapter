/*
 * Vail Master - CW Sending Trainer
 * Scored practice for morse code sending with real-time adaptive decoding
 *
 * Based on the Vail Master web tool, providing:
 * - Sprint, Sweepstakes, Mixed, Uniform, and Free Practice modes
 * - Iambic Master-compatible scoring
 * - Problem character analytics
 * - Score history tracking
 */

#ifndef TRAINING_VAIL_MASTER_H
#define TRAINING_VAIL_MASTER_H

#include <Arduino.h>
#include <Preferences.h>
#include "../core/config.h"
#include "../core/task_manager.h"
#include "../settings/settings_cw.h"
#include "../audio/morse_decoder_adaptive.h"
#include "training_vail_master_data.h"

// ============================================
// Enums and Constants
// ============================================

enum VailMasterMode {
    VM_MODE_SPRINT = 0,         // ARRL Sprint format
    VM_MODE_SWEEPSTAKES,        // ARRL Sweepstakes format
    VM_MODE_MIXED,              // Random character groups
    VM_MODE_UNIFORM,            // Single repeated character
    VM_MODE_FREE_PRACTICE       // Unscored decoder only
};

enum VailMasterState {
    VM_STATE_MENU,              // Mode selection menu
    VM_STATE_READY,             // Ready for next trial (showing target)
    VM_STATE_LISTENING,         // User keying in response
    VM_STATE_FEEDBACK,          // Showing result after match
    VM_STATE_RUN_COMPLETE,      // Run finished, showing summary
    VM_STATE_SETTINGS,          // Settings screen
    VM_STATE_HISTORY,           // Score history view
    VM_STATE_CHARSET_EDIT       // Character set editor
};

// Maximum values
#define VM_MAX_TRIALS 50
#define VM_MAX_SCORES 20
#define VM_MAX_TARGET_LEN 64
#define VM_MAX_ECHO_LEN 128
#define VM_MAX_CHARSET_LEN 50
#define VM_MAX_PROBLEM_CHARS 40

// Default settings
#define VM_DEFAULT_WPM 20
#define VM_DEFAULT_RUN_LENGTH 10
#define VM_DEFAULT_GROUP_COUNT 2
#define VM_DEFAULT_GROUP_LENGTH 5

// Timing
#define VM_FEEDBACK_DELAY_MS 1000    // Delay after correct match before next trial
#define VM_MATCH_CHECK_INTERVAL 100  // How often to check for match (ms)

// ============================================
// Data Structures
// ============================================

// Trial data for a single attempt
struct VailMasterTrial {
    char target[VM_MAX_TARGET_LEN];
    char echo[VM_MAX_ECHO_LEN];
    unsigned long startTime;        // When target was shown
    unsigned long firstKeyTime;     // When user started keying
    unsigned long endTime;          // When trial completed
    int score;                      // Score for this trial
    int maxScore;                   // Maximum possible score
    bool perfect;                   // No errors (echo == target)
    bool completed;                 // Trial was completed
};

// Session data for a complete run
struct VailMasterSession {
    VailMasterMode mode;
    int runLength;                  // 10, 25, or 50 trials
    int currentTrial;               // 0 to runLength-1
    int serialNumber;               // For Sprint mode serial numbers
    int totalScore;
    int maxPossibleScore;
    int perfectCount;
    int currentStreak;
    int bestStreak;
    unsigned long runStartTime;
    unsigned long runEndTime;
    VailMasterTrial trials[VM_MAX_TRIALS];
};

// Mode-specific settings for Mixed mode
struct VailMasterMixedSettings {
    int groupCount;                 // 1-5 groups per trial
    int groupLength;                // 3-10 characters per group
    char charset[VM_MAX_CHARSET_LEN];
    int charsetLength;
};

// Score record for storage
struct VailMasterScoreRecord {
    VailMasterMode mode;
    int runLength;
    int totalScore;
    int perfectCount;
    int perfectPercent;             // 0-100
    int bestStreak;
    float efficiency;               // 0-100%
    uint32_t timestamp;             // Unix timestamp (seconds since epoch)
};

// Problem character tracking
struct VailMasterProblemChar {
    char character;
    int attempts;
    int errors;
};

// ============================================
// Global State
// ============================================

static VailMasterState vmState = VM_STATE_MENU;
static VailMasterSession vmSession;
static VailMasterMixedSettings vmMixedSettings;
static VailMasterScoreRecord vmScoreHistory[VM_MAX_SCORES];
static int vmScoreHistoryCount = 0;
static VailMasterProblemChar vmProblemChars[VM_MAX_PROBLEM_CHARS];
static int vmProblemCharCount = 0;

// Settings
static int vmWPM = VM_DEFAULT_WPM;
static int vmRunLength = VM_DEFAULT_RUN_LENGTH;

// Decoder state (separate from practice mode decoder)
static MorseDecoderAdaptive vmDecoder(VM_DEFAULT_WPM, VM_DEFAULT_WPM, 30);
static String vmEchoText = "";
static bool vmNeedsUIUpdate = false;
static unsigned long vmLastMatchCheck = 0;

// Keyer state (shared with main CW settings)
static bool vmDitPressed = false;
static bool vmDahPressed = false;
static bool vmKeyerActive = false;
static bool vmSendingDit = false;
static bool vmSendingDah = false;
static bool vmInSpacing = false;
static bool vmDitMemory = false;
static bool vmDahMemory = false;
static unsigned long vmElementStartTime = 0;
static int vmDitDuration = 0;

// Timing capture for decoder
static unsigned long vmLastStateChangeTime = 0;
static bool vmLastToneState = false;
static unsigned long vmLastElementTime = 0;

// Feedback timing
static unsigned long vmFeedbackStartTime = 0;

// Active state flag
static bool vmActive = false;

// Preferences
static Preferences vmPrefs;

// ============================================
// Forward Declarations
// ============================================

void vmLoadSettings();
void vmSaveSettings();
void vmLoadScoreHistory(VailMasterMode mode, int runLength);
void vmSaveScore(const VailMasterScoreRecord& score);
void vmStartSession(VailMasterMode mode);
void vmStartTrial();
void vmEndTrial(bool success);
void vmEndSession();
void vmCheckMatch();
void vmUpdateKeyer();
void vmStraightKeyHandler();
void vmIambicKeyerHandler();
String vmGenerateTarget();
int vmCalculateScore(const char* target, const char* echo);
void vmUpdateProblemChars(const char* target, const char* echo);
void vmClearEcho();
void vmHandleEsc();
void vmHandleSpace();
void vmHandleClear();
void vmHandleRestart();

// ============================================
// Settings Load/Save
// ============================================

void vmLoadSettings() {
    vmPrefs.begin("vmaster", true);

    vmWPM = vmPrefs.getInt("wpm", VM_DEFAULT_WPM);
    vmRunLength = vmPrefs.getInt("runlen", VM_DEFAULT_RUN_LENGTH);

    // Mixed mode settings
    vmMixedSettings.groupCount = vmPrefs.getInt("grpcnt", VM_DEFAULT_GROUP_COUNT);
    vmMixedSettings.groupLength = vmPrefs.getInt("grplen", VM_DEFAULT_GROUP_LENGTH);

    // Load custom charset or use default
    String charset = vmPrefs.getString("charset", "");
    if (charset.length() > 0 && charset.length() < VM_MAX_CHARSET_LEN) {
        strcpy(vmMixedSettings.charset, charset.c_str());
        vmMixedSettings.charsetLength = charset.length();
    } else {
        // Default to letters
        strcpy(vmMixedSettings.charset, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        vmMixedSettings.charsetLength = 26;
    }

    vmPrefs.end();

    // Validate
    if (vmWPM < WPM_MIN) vmWPM = WPM_MIN;
    if (vmWPM > WPM_MAX) vmWPM = WPM_MAX;
    if (vmRunLength != 10 && vmRunLength != 25 && vmRunLength != 50) {
        vmRunLength = VM_DEFAULT_RUN_LENGTH;
    }
    if (vmMixedSettings.groupCount < 1) vmMixedSettings.groupCount = 1;
    if (vmMixedSettings.groupCount > 5) vmMixedSettings.groupCount = 5;
    if (vmMixedSettings.groupLength < 3) vmMixedSettings.groupLength = 3;
    if (vmMixedSettings.groupLength > 10) vmMixedSettings.groupLength = 10;

    Serial.printf("[VailMaster] Settings loaded: WPM=%d, RunLen=%d, Groups=%dx%d\n",
                  vmWPM, vmRunLength, vmMixedSettings.groupCount, vmMixedSettings.groupLength);
}

void vmSaveSettings() {
    vmPrefs.begin("vmaster", false);
    vmPrefs.putInt("wpm", vmWPM);
    vmPrefs.putInt("runlen", vmRunLength);
    vmPrefs.putInt("grpcnt", vmMixedSettings.groupCount);
    vmPrefs.putInt("grplen", vmMixedSettings.groupLength);
    vmPrefs.putString("charset", String(vmMixedSettings.charset));
    vmPrefs.end();

    Serial.println("[VailMaster] Settings saved");
}

// ============================================
// Score History Load/Save
// ============================================

void vmLoadScoreHistory(VailMasterMode mode, int runLength) {
    vmPrefs.begin("vmaster", true);

    vmScoreHistoryCount = 0;

    for (int slot = 0; slot < VM_MAX_SCORES; slot++) {
        char key[16];
        snprintf(key, sizeof(key), "s%d%d%d", (int)mode, runLength, slot);

        uint32_t timestamp = vmPrefs.getUInt((String(key) + "t").c_str(), 0);
        if (timestamp > 0) {
            vmScoreHistory[vmScoreHistoryCount].mode = mode;
            vmScoreHistory[vmScoreHistoryCount].runLength = runLength;
            vmScoreHistory[vmScoreHistoryCount].totalScore = vmPrefs.getInt((String(key) + "s").c_str(), 0);
            vmScoreHistory[vmScoreHistoryCount].perfectCount = vmPrefs.getInt((String(key) + "p").c_str(), 0);
            vmScoreHistory[vmScoreHistoryCount].perfectPercent = vmPrefs.getInt((String(key) + "pp").c_str(), 0);
            vmScoreHistory[vmScoreHistoryCount].bestStreak = vmPrefs.getInt((String(key) + "st").c_str(), 0);
            vmScoreHistory[vmScoreHistoryCount].efficiency = vmPrefs.getFloat((String(key) + "e").c_str(), 0.0f);
            vmScoreHistory[vmScoreHistoryCount].timestamp = timestamp;
            vmScoreHistoryCount++;
        }
    }

    vmPrefs.end();

    // Sort by timestamp (newest first)
    for (int i = 0; i < vmScoreHistoryCount - 1; i++) {
        for (int j = i + 1; j < vmScoreHistoryCount; j++) {
            if (vmScoreHistory[j].timestamp > vmScoreHistory[i].timestamp) {
                VailMasterScoreRecord temp = vmScoreHistory[i];
                vmScoreHistory[i] = vmScoreHistory[j];
                vmScoreHistory[j] = temp;
            }
        }
    }

    Serial.printf("[VailMaster] Loaded %d scores for mode %d, length %d\n",
                  vmScoreHistoryCount, (int)mode, runLength);
}

void vmSaveScore(const VailMasterScoreRecord& score) {
    vmPrefs.begin("vmaster", false);

    // Find oldest slot or first empty slot
    int slot = 0;
    uint32_t oldestTime = UINT32_MAX;
    int oldestSlot = 0;

    for (int i = 0; i < VM_MAX_SCORES; i++) {
        char key[16];
        snprintf(key, sizeof(key), "s%d%d%d", (int)score.mode, score.runLength, i);

        uint32_t timestamp = vmPrefs.getUInt((String(key) + "t").c_str(), 0);
        if (timestamp == 0) {
            slot = i;
            break;
        }
        if (timestamp < oldestTime) {
            oldestTime = timestamp;
            oldestSlot = i;
        }
        slot = oldestSlot;  // Use oldest if all full
    }

    // Save score data
    char key[16];
    snprintf(key, sizeof(key), "s%d%d%d", (int)score.mode, score.runLength, slot);

    vmPrefs.putInt((String(key) + "s").c_str(), score.totalScore);
    vmPrefs.putInt((String(key) + "p").c_str(), score.perfectCount);
    vmPrefs.putInt((String(key) + "pp").c_str(), score.perfectPercent);
    vmPrefs.putInt((String(key) + "st").c_str(), score.bestStreak);
    vmPrefs.putFloat((String(key) + "e").c_str(), score.efficiency);
    vmPrefs.putUInt((String(key) + "t").c_str(), score.timestamp);

    vmPrefs.end();

    Serial.printf("[VailMaster] Score saved to slot %d\n", slot);
}

// ============================================
// Session Management
// ============================================

void vmStartSession(VailMasterMode mode) {
    Serial.printf("[VailMaster] Starting session: mode=%d, runLength=%d\n", (int)mode, vmRunLength);

    // Initialize session
    memset(&vmSession, 0, sizeof(vmSession));
    vmSession.mode = mode;
    vmSession.runLength = vmRunLength;
    vmSession.currentTrial = 0;
    vmSession.serialNumber = 1;
    vmSession.runStartTime = millis();

    // Clear problem characters
    vmProblemCharCount = 0;
    memset(vmProblemChars, 0, sizeof(vmProblemChars));

    // Initialize decoder
    vmDecoder.reset();
    vmDecoder.flush();
    vmDecoder.setWPM(vmWPM);
    vmEchoText = "";

    // Setup decoder callback
    vmDecoder.messageCallback = [](String morse, String text) {
        for (int i = 0; i < text.length(); i++) {
            char c = text[i];
            // Convert to uppercase
            if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
            vmEchoText += c;
        }
        vmNeedsUIUpdate = true;

        Serial.printf("[VailMaster] Decoded: %s -> %s\n", morse.c_str(), text.c_str());
    };

    // Calculate dit duration
    vmDitDuration = DIT_DURATION(vmWPM);

    // Reset keyer state
    vmDitPressed = false;
    vmDahPressed = false;
    vmKeyerActive = false;
    vmInSpacing = false;
    vmDitMemory = false;
    vmDahMemory = false;
    vmLastStateChangeTime = 0;
    vmLastToneState = false;
    vmLastElementTime = 0;

    // Mark as active
    vmActive = true;

    // Start first trial
    vmStartTrial();
}

void vmStartTrial() {
    VailMasterTrial& trial = vmSession.trials[vmSession.currentTrial];
    memset(&trial, 0, sizeof(trial));

    // Generate target text
    String target = vmGenerateTarget();
    strncpy(trial.target, target.c_str(), VM_MAX_TARGET_LEN - 1);
    trial.target[VM_MAX_TARGET_LEN - 1] = '\0';

    // Record start time
    trial.startTime = millis();

    // Clear echo
    vmEchoText = "";
    vmDecoder.reset();
    vmDecoder.flush();

    // Set state
    vmState = VM_STATE_READY;
    vmNeedsUIUpdate = true;

    Serial.printf("[VailMaster] Trial %d/%d: Target = '%s'\n",
                  vmSession.currentTrial + 1, vmSession.runLength, trial.target);
}

void vmEndTrial(bool success) {
    VailMasterTrial& trial = vmSession.trials[vmSession.currentTrial];

    // Copy echo text
    strncpy(trial.echo, vmEchoText.c_str(), VM_MAX_ECHO_LEN - 1);
    trial.echo[VM_MAX_ECHO_LEN - 1] = '\0';

    // Record end time
    trial.endTime = millis();
    trial.completed = true;

    // Calculate score
    trial.score = vmCalculateScore(trial.target, trial.echo);

    // Calculate max possible score (WPM * target length)
    int targetLen = 0;
    for (int i = 0; trial.target[i] != '\0'; i++) {
        if (trial.target[i] != ' ') targetLen++;
    }
    trial.maxScore = vmWPM * targetLen;

    // Check if perfect
    trial.perfect = (strcmp(trial.target, trial.echo) == 0);

    // Update session stats
    vmSession.totalScore += trial.score;
    vmSession.maxPossibleScore += trial.maxScore;

    if (trial.perfect) {
        vmSession.perfectCount++;
        vmSession.currentStreak++;
        if (vmSession.currentStreak > vmSession.bestStreak) {
            vmSession.bestStreak = vmSession.currentStreak;
        }
    } else {
        vmSession.currentStreak = 0;
    }

    // Update problem characters
    vmUpdateProblemChars(trial.target, trial.echo);

    Serial.printf("[VailMaster] Trial complete: score=%d/%d, perfect=%s, streak=%d\n",
                  trial.score, trial.maxScore, trial.perfect ? "yes" : "no", vmSession.currentStreak);

    // Move to next trial or end session
    vmSession.currentTrial++;

    if (vmSession.currentTrial >= vmSession.runLength) {
        vmEndSession();
    } else {
        // Show feedback briefly, then start next trial
        vmState = VM_STATE_FEEDBACK;
        vmFeedbackStartTime = millis();
        vmNeedsUIUpdate = true;
    }
}

void vmEndSession() {
    vmSession.runEndTime = millis();
    vmActive = false;

    // Calculate final stats
    float efficiency = 0;
    if (vmSession.maxPossibleScore > 0) {
        efficiency = (float)vmSession.totalScore / vmSession.maxPossibleScore * 100.0f;
    }

    int perfectPercent = 0;
    if (vmSession.runLength > 0) {
        perfectPercent = vmSession.perfectCount * 100 / vmSession.runLength;
    }

    Serial.printf("[VailMaster] Session complete: score=%d, efficiency=%.1f%%, perfect=%d/%d (%d%%)\n",
                  vmSession.totalScore, efficiency, vmSession.perfectCount, vmSession.runLength, perfectPercent);

    // Save score if not free practice
    if (vmSession.mode != VM_MODE_FREE_PRACTICE) {
        VailMasterScoreRecord record;
        record.mode = vmSession.mode;
        record.runLength = vmSession.runLength;
        record.totalScore = vmSession.totalScore;
        record.perfectCount = vmSession.perfectCount;
        record.perfectPercent = perfectPercent;
        record.bestStreak = vmSession.bestStreak;
        record.efficiency = efficiency;
        record.timestamp = millis() / 1000;  // Simple timestamp

        vmSaveScore(record);
    }

    vmState = VM_STATE_RUN_COMPLETE;
    vmNeedsUIUpdate = true;

    // Stop any playing tone
    requestStopTone();  // Non-blocking request to Core 0
}

// ============================================
// Target Generation
// ============================================

String vmGenerateTarget() {
    String target = "";

    switch (vmSession.mode) {
        case VM_MODE_SPRINT: {
            // Format: CALLSIGN SERIAL# CALLSIGN NAME STATE
            String call = vmGetRandomCallsign();
            String name = vmGetRandomName();
            String state = vmGetRandomState();

            char buffer[VM_MAX_TARGET_LEN];
            snprintf(buffer, sizeof(buffer), "%s %d %s %s %s",
                     call.c_str(), vmSession.serialNumber++,
                     call.c_str(), name.c_str(), state.c_str());
            target = String(buffer);
            break;
        }

        case VM_MODE_SWEEPSTAKES: {
            // Format: NR PRECEDENCE CALLSIGN CHECK SECTION
            String call = vmGetRandomCallsign();
            String section = vmGetRandomSection();
            char prec = vmGetRandomPrecedence();
            int check = random(50, 100);  // Two-digit year (50-99)

            char buffer[VM_MAX_TARGET_LEN];
            snprintf(buffer, sizeof(buffer), "%d %c %s %02d %s",
                     vmSession.serialNumber++, prec, call.c_str(), check, section.c_str());
            target = String(buffer);
            break;
        }

        case VM_MODE_MIXED: {
            // Random groups of mixed characters
            for (int g = 0; g < vmMixedSettings.groupCount; g++) {
                if (g > 0) target += " ";
                for (int c = 0; c < vmMixedSettings.groupLength; c++) {
                    int idx = random(vmMixedSettings.charsetLength);
                    target += vmMixedSettings.charset[idx];
                }
            }
            break;
        }

        case VM_MODE_UNIFORM: {
            // Random groups of single repeated character
            char ch = vmMixedSettings.charset[random(vmMixedSettings.charsetLength)];
            for (int g = 0; g < vmMixedSettings.groupCount; g++) {
                if (g > 0) target += " ";
                for (int c = 0; c < vmMixedSettings.groupLength; c++) {
                    target += ch;
                }
            }
            break;
        }

        case VM_MODE_FREE_PRACTICE:
        default:
            target = "";  // No target in free practice
            break;
    }

    return target;
}

// ============================================
// Scoring
// ============================================

int vmCalculateScore(const char* target, const char* echo) {
    // Count non-space characters
    int targetLen = 0;
    for (int i = 0; target[i] != '\0'; i++) {
        if (target[i] != ' ') targetLen++;
    }

    int echoLen = 0;
    for (int i = 0; echo[i] != '\0'; i++) {
        if (echo[i] != ' ') echoLen++;
    }

    if (targetLen == 0) return 0;

    // Perfect score: WPM * Target Length
    int maxScore = vmWPM * targetLen;

    // Check if perfect match (ignoring case)
    String normTarget = String(target);
    String normEcho = String(echo);
    normTarget.toUpperCase();
    normEcho.toUpperCase();

    if (normTarget == normEcho) {
        return maxScore;
    }

    // With errors: WPM * Target Length * 0.9 * (Target Length / Echo Length)
    float ratio = (echoLen > 0) ? (float)targetLen / echoLen : 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;  // Cap at 100% if echo is shorter

    return (int)(maxScore * 0.9f * ratio);
}

void vmUpdateProblemChars(const char* target, const char* echo) {
    int targetIdx = 0;
    int echoIdx = 0;

    while (target[targetIdx] != '\0') {
        char tc = target[targetIdx];
        if (tc >= 'a' && tc <= 'z') tc = tc - 'a' + 'A';

        // Skip spaces in target
        if (tc == ' ') {
            targetIdx++;
            continue;
        }

        // Skip spaces in echo
        while (echo[echoIdx] == ' ') echoIdx++;

        char ec = echo[echoIdx];
        if (ec >= 'a' && ec <= 'z') ec = ec - 'a' + 'A';

        // Find or create entry for this character
        int idx = -1;
        for (int i = 0; i < vmProblemCharCount; i++) {
            if (vmProblemChars[i].character == tc) {
                idx = i;
                break;
            }
        }

        if (idx < 0 && vmProblemCharCount < VM_MAX_PROBLEM_CHARS) {
            idx = vmProblemCharCount++;
            vmProblemChars[idx].character = tc;
            vmProblemChars[idx].attempts = 0;
            vmProblemChars[idx].errors = 0;
        }

        if (idx >= 0) {
            vmProblemChars[idx].attempts++;
            if (tc != ec || echo[echoIdx] == '\0') {
                vmProblemChars[idx].errors++;
            }
        }

        targetIdx++;
        if (echo[echoIdx] != '\0') echoIdx++;
    }
}

// ============================================
// Match Detection
// ============================================

void vmCheckMatch() {
    if (vmState != VM_STATE_LISTENING && vmState != VM_STATE_READY) return;
    if (vmSession.mode == VM_MODE_FREE_PRACTICE) return;

    unsigned long now = millis();
    if (now - vmLastMatchCheck < VM_MATCH_CHECK_INTERVAL) return;
    vmLastMatchCheck = now;

    const char* target = vmSession.trials[vmSession.currentTrial].target;

    // Normalize and compare
    String normTarget = String(target);
    String normEcho = vmEchoText;
    normTarget.toUpperCase();
    normTarget.trim();
    normEcho.toUpperCase();
    normEcho.trim();

    // Match when echo ends with target (allows restart without clearing)
    if (normEcho.length() >= normTarget.length() && normEcho.endsWith(normTarget)) {
        Serial.println("[VailMaster] Match detected!");
        beep(TONE_SUCCESS, 100);  // Success beep
        vmEndTrial(true);
    }
}

// ============================================
// Keyer Update (Main Loop)
// ============================================

void vmUpdateKeyer() {
    if (!vmActive) return;
    if (vmState == VM_STATE_MENU || vmState == VM_STATE_RUN_COMPLETE ||
        vmState == VM_STATE_SETTINGS || vmState == VM_STATE_HISTORY ||
        vmState == VM_STATE_CHARSET_EDIT) return;

    // Handle feedback delay
    if (vmState == VM_STATE_FEEDBACK) {
        if (millis() - vmFeedbackStartTime >= VM_FEEDBACK_DELAY_MS) {
            vmStartTrial();
        }
        return;
    }

    // Read paddle inputs from audio task (sampled at ~1ms on Core 0)
    getPaddleState(&vmDitPressed, &vmDahPressed);

    // Transition from READY to LISTENING on first key
    if (vmState == VM_STATE_READY && (vmDitPressed || vmDahPressed)) {
        vmState = VM_STATE_LISTENING;
        vmSession.trials[vmSession.currentTrial].firstKeyTime = millis();
        Serial.println("[VailMaster] First key detected, listening...");
    }

    // Handle keying
    if (cwKeyType == KEY_STRAIGHT) {
        vmStraightKeyHandler();
    } else {
        vmIambicKeyerHandler();
    }

    // Check for decoder timeout (flush after word gap)
    if (vmLastElementTime > 0 && !vmDitPressed && !vmDahPressed) {
        unsigned long timeSinceElement = millis() - vmLastElementTime;
        float wordGap = MorseWPM::wordGap(vmDecoder.getWPM());

        if (timeSinceElement > wordGap) {
            vmDecoder.flush();
            vmLastElementTime = 0;
        }
    }

    // Check for match
    vmCheckMatch();
}

void vmStraightKeyHandler() {
    unsigned long currentTime = millis();

    // Use vmLastToneState to track what we've requested (not what's actually playing)
    // This prevents flooding the request queue with redundant requests
    if (vmDitPressed && !vmLastToneState) {
        // Tone starting - only request once
        if (vmLastStateChangeTime > 0) {
            float silenceDuration = currentTime - vmLastStateChangeTime;
            if (silenceDuration > 0) {
                vmDecoder.addTiming(-silenceDuration);
            }
        }
        vmLastStateChangeTime = currentTime;
        vmLastToneState = true;
        requestStartTone(cwTone);  // Non-blocking request to Core 0
    }
    else if (vmDitPressed && vmLastToneState) {
        // Key held - audio task keeps buffer filled, nothing to do
    }
    else if (!vmDitPressed && vmLastToneState) {
        // Tone stopping - only request once
        float toneDuration = currentTime - vmLastStateChangeTime;
        if (toneDuration > 0) {
            vmDecoder.addTiming(toneDuration);
            vmLastElementTime = currentTime;
        }
        vmLastStateChangeTime = currentTime;
        vmLastToneState = false;
        requestStopTone();  // Non-blocking request to Core 0
    }
}

void vmIambicKeyerHandler() {
    unsigned long currentTime = millis();

    // If not actively sending or spacing, check for new input
    if (!vmKeyerActive && !vmInSpacing) {
        if (vmDitPressed || vmDitMemory) {
            // Start sending dit - only request tone once
            if (vmLastToneState == false) {
                if (vmLastStateChangeTime > 0) {
                    float silenceDuration = currentTime - vmLastStateChangeTime;
                    if (silenceDuration > 0) {
                        vmDecoder.addTiming(-silenceDuration);
                    }
                }
                vmLastStateChangeTime = currentTime;
                vmLastToneState = true;
                requestStartTone(cwTone);  // Non-blocking request to Core 0 - only once!
            }

            vmKeyerActive = true;
            vmSendingDit = true;
            vmSendingDah = false;
            vmInSpacing = false;
            vmElementStartTime = currentTime;
            vmDitMemory = false;
        }
        else if (vmDahPressed || vmDahMemory) {
            // Start sending dah - only request tone once
            if (vmLastToneState == false) {
                if (vmLastStateChangeTime > 0) {
                    float silenceDuration = currentTime - vmLastStateChangeTime;
                    if (silenceDuration > 0) {
                        vmDecoder.addTiming(-silenceDuration);
                    }
                }
                vmLastStateChangeTime = currentTime;
                vmLastToneState = true;
                requestStartTone(cwTone);  // Non-blocking request to Core 0 - only once!
            }

            vmKeyerActive = true;
            vmSendingDit = false;
            vmSendingDah = true;
            vmInSpacing = false;
            vmElementStartTime = currentTime;
            vmDahMemory = false;
        }
    }
    // Currently sending an element
    else if (vmKeyerActive && !vmInSpacing) {
        unsigned long elementDuration = vmSendingDit ? vmDitDuration : (vmDitDuration * 3);

        // No action needed - audio task keeps buffer filled

        // Check for paddle input during element (iambic squeeze)
        if (vmDitPressed && vmDahPressed) {
            if (vmSendingDit) vmDahMemory = true;
            else vmDitMemory = true;
        }
        else if (vmSendingDit && vmDahPressed) {
            vmDahMemory = true;
        }
        else if (vmSendingDah && vmDitPressed) {
            vmDitMemory = true;
        }

        // Check if element is complete
        if (currentTime - vmElementStartTime >= elementDuration) {
            if (vmLastToneState == true) {
                float toneDuration = currentTime - vmLastStateChangeTime;
                if (toneDuration > 0) {
                    vmDecoder.addTiming(toneDuration);
                    vmLastElementTime = currentTime;
                }
                vmLastStateChangeTime = currentTime;
                vmLastToneState = false;
                requestStopTone();  // Non-blocking request to Core 0 - only once!
            }

            vmKeyerActive = false;
            vmSendingDit = false;
            vmSendingDah = false;
            vmInSpacing = true;
            vmElementStartTime = currentTime;
        }
    }
    // In inter-element spacing
    else if (vmInSpacing) {
        // Check for paddle input during spacing
        if (vmDitPressed && vmDahPressed) {
            vmDitMemory = true;
            vmDahMemory = true;
        }
        else if (vmDitPressed && !vmDitMemory) {
            vmDitMemory = true;
        }
        else if (vmDahPressed && !vmDahMemory) {
            vmDahMemory = true;
        }

        // Wait for 1 dit duration (inter-element gap)
        if (currentTime - vmElementStartTime >= vmDitDuration) {
            vmInSpacing = false;
        }
    }
}

// ============================================
// Action Handlers
// ============================================

void vmClearEcho() {
    vmEchoText = "";
    vmDecoder.reset();
    vmDecoder.flush();
    vmNeedsUIUpdate = true;
    beep(TONE_MENU_NAV, BEEP_SHORT);
    Serial.println("[VailMaster] Echo cleared");
}

void vmHandleEsc() {
    requestStopTone();  // Non-blocking request to Core 0
    vmDecoder.flush();
    vmActive = false;
    vmState = VM_STATE_MENU;
    Serial.println("[VailMaster] Exiting via ESC");
}

void vmHandleSpace() {
    // Skip current trial (mark as failed)
    if (vmState == VM_STATE_READY || vmState == VM_STATE_LISTENING) {
        Serial.println("[VailMaster] Skipping trial");
        beep(TONE_MENU_NAV, BEEP_SHORT);
        vmEndTrial(false);
    }
}

void vmHandleClear() {
    vmClearEcho();
}

void vmHandleRestart() {
    // Restart entire run
    if (vmState != VM_STATE_MENU) {
        Serial.println("[VailMaster] Restarting run");
        beep(TONE_MENU_NAV, BEEP_SHORT);
        vmStartSession(vmSession.mode);
    }
}

// ============================================
// Start Function (called from mode integration)
// ============================================

void startVailMaster(LGFX& tft) {
    Serial.println("[VailMaster] Starting Vail Master mode");

    // Load settings
    vmLoadSettings();

    // Initialize state
    vmState = VM_STATE_MENU;
    vmActive = false;
    vmNeedsUIUpdate = true;

    // Reinitialize I2S for clean audio
    i2s_zero_dma_buffer(I2S_NUM_0);
    delay(50);
}

// ============================================
// Mode Name Helpers
// ============================================

const char* vmGetModeName(VailMasterMode mode) {
    switch (mode) {
        case VM_MODE_SPRINT: return "Sprint";
        case VM_MODE_SWEEPSTAKES: return "Sweepstakes";
        case VM_MODE_MIXED: return "Mixed";
        case VM_MODE_UNIFORM: return "Uniform";
        case VM_MODE_FREE_PRACTICE: return "Free Practice";
        default: return "Unknown";
    }
}

const char* vmGetModeShortName(VailMasterMode mode) {
    switch (mode) {
        case VM_MODE_SPRINT: return "SPR";
        case VM_MODE_SWEEPSTAKES: return "SS";
        case VM_MODE_MIXED: return "MIX";
        case VM_MODE_UNIFORM: return "UNI";
        case VM_MODE_FREE_PRACTICE: return "FREE";
        default: return "???";
    }
}

#endif // TRAINING_VAIL_MASTER_H
