/*
 * Memory Chain Game - Fresh Implementation
 *
 * A simple memory game where players listen to and repeat
 * increasingly long sequences of Morse code characters.
 *
 * Design principles:
 * - Simple 3-state machine: READY, PLAYING, GAME_OVER
 * - Synchronous transitions (no complex callbacks)
 * - UI updates only when screen exists
 * - Polling-based decoder (no callback timing issues)
 */

#ifndef GAME_MEMORY_CHAIN_H
#define GAME_MEMORY_CHAIN_H

#include <Preferences.h>
#include "../core/config.h"
#include "../core/morse_code.h"
#include "../audio/i2s_audio.h"
#include "../audio/morse_decoder_adaptive.h"
#include "../lvgl/lv_screen_manager.h"
#include "../lvgl/lv_theme_summit.h"
#include "../lvgl/lv_widgets_summit.h"

// Forward declarations
extern void onLVGLBackNavigation();

// ============================================
// Constants
// ============================================

#define MC_MAX_SEQUENCE 100
#define MC_INPUT_TIMEOUT 3000   // ms to wait for input after last key
#define MC_FEEDBACK_DELAY 800   // ms to show CORRECT/WRONG
#define MC_SEQUENCE_GAP 500     // ms pause before/after sequence

// Character set (Koch order - easier characters first)
const char MC_CHARSET[] = "KMRSUAPTLOWI.NJEF0YVG5/Q9ZH38B?427C1D6X";

// ============================================
// Game State
// ============================================

enum MCState {
    MC_STATE_READY,      // Waiting to start / showing GET READY
    MC_STATE_PLAYING,    // Active gameplay
    MC_STATE_GAME_OVER   // Game ended
};

enum MCPlayPhase {
    MC_PHASE_PLAYING_SEQUENCE,  // Playing morse to user
    MC_PHASE_USER_INPUT         // Waiting for user input
};

struct MemoryChainGame {
    // Sequence data
    char sequence[MC_MAX_SEQUENCE + 1];
    int length;
    int playerPos;

    // Game state
    MCState state;
    MCPlayPhase phase;
    int lives;
    int score;
    int highScore;

    // Timing
    unsigned long stateStartTime;
    unsigned long lastInputTime;

    // Decoder
    char lastDecoded;
    bool hasNewChar;
};

// Global game instance
static MemoryChainGame mcGame;
static MorseDecoderAdaptive mcDecoder(15, 20, 30);
static Preferences mcPrefs;

// Keyer state for iambic/straight key
static bool mcKeyerActive = false;
static bool mcSendingDit = false;
static bool mcSendingDah = false;
static bool mcInSpacing = false;
static bool mcDitMemory = false;
static bool mcDahMemory = false;
static unsigned long mcElementStart = 0;
static bool mcLastToneState = false;
static unsigned long mcLastStateChange = 0;

// Straight key debounce state
#define MC_STRAIGHT_KEY_DEBOUNCE_MS 8   // Ignore state changes shorter than this
static bool mcDebouncedKeyState = false;
static unsigned long mcKeyLastChangeTime = 0;
static bool mcKeyLastRawState = false;

// ============================================
// LVGL Screen Elements
// ============================================

static lv_obj_t* mc_screen = NULL;
static lv_obj_t* mc_level_label = NULL;
static lv_obj_t* mc_score_label = NULL;
static lv_obj_t* mc_status_label = NULL;
static lv_obj_t* mc_lives_container = NULL;
static lv_obj_t* mc_message_label = NULL;

// ============================================
// UI Update Functions
// ============================================

// Delay that keeps LVGL running so UI updates are visible
void mcDelayWithUI(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        lv_timer_handler();
        delay(5);  // Small yield
    }
}

void mcUpdateLevel(int level) {
    if (mc_level_label != NULL) {
        lv_label_set_text_fmt(mc_level_label, "%d", level);
    }
}

void mcUpdateScore(int score) {
    if (mc_score_label != NULL) {
        lv_label_set_text_fmt(mc_score_label, "%d", score);
    }
}

void mcUpdateStatus(const char* status) {
    if (mc_status_label != NULL) {
        lv_label_set_text(mc_status_label, status);
    }
}

void mcUpdateMessage(const char* msg) {
    if (mc_message_label != NULL) {
        lv_label_set_text(mc_message_label, msg);
    }
}

void mcUpdateLives(int lives) {
    if (mc_lives_container != NULL) {
        uint32_t count = lv_obj_get_child_cnt(mc_lives_container);
        for (uint32_t i = 0; i < count && i < 3; i++) {
            lv_obj_t* icon = lv_obj_get_child(mc_lives_container, i);
            if ((int)i < lives) {
                lv_obj_set_style_text_color(icon, LV_COLOR_ERROR, 0);
            } else {
                lv_obj_set_style_text_color(icon, LV_COLOR_TEXT_DISABLED, 0);
            }
        }
    }
}

// ============================================
// Preferences
// ============================================

void mcLoadPrefs() {
    mcPrefs.begin("memchain", true);
    mcGame.highScore = mcPrefs.getInt("highscore", 0);
    mcPrefs.end();
}

void mcSaveHighScore() {
    mcPrefs.begin("memchain", false);
    mcPrefs.putInt("highscore", mcGame.highScore);
    mcPrefs.end();
}

// ============================================
// Key Event Handler
// ============================================

static void mc_key_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    if (key == LV_KEY_ESC) {
        onLVGLBackNavigation();
        lv_event_stop_processing(e);
    }
    else if (key == ' ') {
        // Space = replay sequence (only during user input phase)
        if (mcGame.state == MC_STATE_PLAYING && mcGame.phase == MC_PHASE_USER_INPUT) {
            extern int cwTone, cwSpeed;
            mcGame.phase = MC_PHASE_PLAYING_SEQUENCE;
            mcUpdateStatus("LISTEN...");

            for (int i = 0; i < mcGame.length; i++) {
                playMorseChar(mcGame.sequence[i], cwSpeed, cwTone);
                MorseTiming timing(cwSpeed);
                if (i < mcGame.length - 1) {
                    delay(timing.letterGap);
                }
            }
            delay(MC_SEQUENCE_GAP);

            mcGame.phase = MC_PHASE_USER_INPUT;
            mcGame.playerPos = 0;
            mcGame.lastInputTime = millis();
            mcDecoder.reset();
            mcDecoder.flush();
            mcUpdateStatus("YOUR TURN");
        }
    }
    else if ((key == 0x0D || key == 0x0A) && mcGame.state == MC_STATE_GAME_OVER) {
        // Enter = restart after game over
        mcGame.state = MC_STATE_READY;
        mcGame.stateStartTime = millis();
        mcGame.length = 0;
        mcGame.score = 0;
        mcGame.lives = 3;
        mcGame.playerPos = 0;

        mcUpdateLevel(1);
        mcUpdateScore(0);
        mcUpdateLives(3);
        mcUpdateStatus("GET READY");
        mcUpdateMessage("Listen to the sequence, then repeat it");
    }
}

// ============================================
// LVGL Screen Creation
// ============================================

lv_obj_t* createMemoryChainScreen() {
    // Reset screen pointers
    mc_screen = NULL;
    mc_level_label = NULL;
    mc_score_label = NULL;
    mc_status_label = NULL;
    mc_lives_container = NULL;
    mc_message_label = NULL;

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Status bar
    createCompactStatusBar(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_set_layout(title_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(title_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(title_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(title_bar, 15, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "MEMORY CHAIN");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);

    // Score in title bar
    mc_score_label = lv_label_create(title_bar);
    lv_label_set_text(mc_score_label, "0");
    lv_obj_set_style_text_color(mc_score_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(mc_score_label, getThemeFonts()->font_subtitle, 0);

    // Level card
    lv_obj_t* level_card = lv_obj_create(screen);
    lv_obj_set_size(level_card, 150, 80);
    lv_obj_set_pos(level_card, 20, HEADER_HEIGHT + 20);
    lv_obj_set_layout(level_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(level_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(level_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    applyCardStyle(level_card);

    lv_obj_t* level_title = lv_label_create(level_card);
    lv_label_set_text(level_title, "Level");
    lv_obj_add_style(level_title, getStyleLabelBody(), 0);

    mc_level_label = lv_label_create(level_card);
    lv_label_set_text(mc_level_label, "1");
    lv_obj_set_style_text_font(mc_level_label, getThemeFonts()->font_large, 0);
    lv_obj_set_style_text_color(mc_level_label, LV_COLOR_ACCENT_CYAN, 0);

    // Lives card
    lv_obj_t* lives_card = lv_obj_create(screen);
    lv_obj_set_size(lives_card, 150, 80);
    lv_obj_set_pos(lives_card, SCREEN_WIDTH - 170, HEADER_HEIGHT + 20);
    lv_obj_set_layout(lives_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(lives_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(lives_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    applyCardStyle(lives_card);

    lv_obj_t* lives_title = lv_label_create(lives_card);
    lv_label_set_text(lives_title, "Lives");
    lv_obj_add_style(lives_title, getStyleLabelBody(), 0);

    mc_lives_container = lv_obj_create(lives_card);
    lv_obj_set_size(mc_lives_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(mc_lives_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(mc_lives_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(mc_lives_container, 5, 0);
    lv_obj_set_style_bg_opa(mc_lives_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mc_lives_container, 0, 0);
    lv_obj_set_style_pad_all(mc_lives_container, 0, 0);

    // Create 3 life icons (all visible initially)
    for (int i = 0; i < 3; i++) {
        lv_obj_t* icon = lv_label_create(mc_lives_container);
        lv_label_set_text(icon, LV_SYMBOL_OK);
        lv_obj_set_style_text_color(icon, LV_COLOR_ERROR, 0);
        lv_obj_set_style_text_font(icon, getThemeFonts()->font_subtitle, 0);
    }

    // Status display (main area)
    lv_obj_t* status_card = lv_obj_create(screen);
    lv_obj_set_size(status_card, SCREEN_WIDTH - 40, 100);
    lv_obj_set_pos(status_card, 20, HEADER_HEIGHT + 115);
    lv_obj_set_layout(status_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(status_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    applyCardStyle(status_card);

    mc_status_label = lv_label_create(status_card);
    lv_label_set_text(mc_status_label, "GET READY");
    lv_obj_set_style_text_font(mc_status_label, getThemeFonts()->font_title, 0);
    lv_obj_set_style_text_color(mc_status_label, LV_COLOR_ACCENT_GREEN, 0);

    // Message label
    mc_message_label = lv_label_create(screen);
    lv_label_set_text(mc_message_label, "Listen to the sequence, then repeat it");
    lv_obj_add_style(mc_message_label, getStyleLabelBody(), 0);
    lv_obj_align(mc_message_label, LV_ALIGN_CENTER, 0, 80);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "Paddle: Repeat   SPACE: Replay   ESC: Exit");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    // Focus container for keyboard input
    lv_obj_t* focus = lv_obj_create(screen);
    lv_obj_set_size(focus, 1, 1);
    lv_obj_set_pos(focus, -10, -10);
    lv_obj_set_style_bg_opa(focus, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus, 0, 0);
    lv_obj_set_style_outline_width(focus, 0, 0);
    lv_obj_set_style_outline_width(focus, 0, LV_STATE_FOCUSED);
    lv_obj_clear_flag(focus, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(focus, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(focus, mc_key_event_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus);

    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }
    lv_group_focus_obj(focus);

    mc_screen = screen;
    return screen;
}

// ============================================
// Game Logic
// ============================================

void mcAddChar() {
    if (mcGame.length >= MC_MAX_SEQUENCE) return;

    // Pick random character from charset
    int idx = random(0, strlen(MC_CHARSET));
    mcGame.sequence[mcGame.length] = MC_CHARSET[idx];
    mcGame.length++;
    mcGame.sequence[mcGame.length] = '\0';
}

void mcPlaySequence() {
    extern int cwTone, cwSpeed;

    mcGame.phase = MC_PHASE_PLAYING_SEQUENCE;
    mcUpdateStatus("LISTEN...");

    for (int i = 0; i < mcGame.length; i++) {
        playMorseChar(mcGame.sequence[i], cwSpeed, cwTone);
        MorseTiming timing(cwSpeed);
        if (i < mcGame.length - 1) {
            delay(timing.letterGap);
        }
    }
}

void mcStartUserInput() {
    mcGame.phase = MC_PHASE_USER_INPUT;
    mcGame.playerPos = 0;
    mcGame.lastInputTime = millis();
    mcGame.lastDecoded = 0;
    mcGame.hasNewChar = false;

    // Reset keyer state
    mcKeyerActive = false;
    mcSendingDit = false;
    mcSendingDah = false;
    mcInSpacing = false;
    mcDitMemory = false;
    mcDahMemory = false;
    mcLastToneState = false;
    mcLastStateChange = 0;

    // Reset straight key debounce state
    mcDebouncedKeyState = false;
    mcKeyLastChangeTime = 0;
    mcKeyLastRawState = false;

    // Reset decoder
    mcDecoder.reset();
    mcDecoder.flush();

    mcUpdateStatus("YOUR TURN");
    lv_timer_handler();  // Update UI
}

void mcHandleCorrect() {
    Serial.println("[MC] Correct!");

    stopTone();
    mcGame.score = mcGame.length;

    if (mcGame.score > mcGame.highScore) {
        mcGame.highScore = mcGame.score;
        mcSaveHighScore();
    }

    mcUpdateStatus("CORRECT!");
    mcUpdateScore(mcGame.score);
    lv_timer_handler();  // Update UI immediately
    beep(1000, 200);
    mcDelayWithUI(MC_FEEDBACK_DELAY);

    // Next round
    mcAddChar();
    mcUpdateLevel(mcGame.length);
    mcUpdateMessage("");
    lv_timer_handler();  // Update UI

    mcDelayWithUI(MC_SEQUENCE_GAP);
    mcPlaySequence();
    mcDelayWithUI(MC_SEQUENCE_GAP);

    mcStartUserInput();
}

void mcHandleWrong() {
    Serial.println("[MC] Wrong!");

    stopTone();
    mcGame.lives--;

    mcUpdateStatus("WRONG!");
    mcUpdateLives(mcGame.lives);
    lv_timer_handler();  // Update UI immediately
    beep(200, 300);
    mcDelayWithUI(MC_FEEDBACK_DELAY);

    if (mcGame.lives <= 0) {
        // Game over
        mcGame.state = MC_STATE_GAME_OVER;
        mcUpdateStatus("GAME OVER");
        mcUpdateMessage("Press ENTER to restart");
        lv_timer_handler();  // Update UI
        Serial.printf("[MC] Game over! Score: %d\n", mcGame.score);
        return;
    }

    // Retry same sequence
    mcUpdateStatus("TRY AGAIN");
    lv_timer_handler();  // Update UI
    mcDelayWithUI(MC_SEQUENCE_GAP);
    mcPlaySequence();
    mcDelayWithUI(MC_SEQUENCE_GAP);

    mcStartUserInput();
}

void mcProcessDecodedChar(char c) {
    if (mcGame.state != MC_STATE_PLAYING || mcGame.phase != MC_PHASE_USER_INPUT) {
        return;
    }

    Serial.printf("[MC] Decoded: '%c', expecting: '%c' at pos %d\n",
                  c, mcGame.sequence[mcGame.playerPos], mcGame.playerPos);

    mcGame.lastInputTime = millis();

    char expected = mcGame.sequence[mcGame.playerPos];

    if (c != expected) {
        mcHandleWrong();
        return;
    }

    mcGame.playerPos++;

    if (mcGame.playerPos >= mcGame.length) {
        // Completed sequence!
        mcHandleCorrect();
    } else {
        Serial.printf("[MC] Correct so far: %d/%d\n", mcGame.playerPos, mcGame.length);
    }
}

// ============================================
// Decoder Callback Setup
// ============================================

void mcSetupDecoder() {
    static bool setup = false;
    if (setup) return;

    mcDecoder.messageCallback = [](String morse, String text) {
        Serial.printf("[MC] Decoder callback: morse='%s' text='%s' state=%d phase=%d\n",
                      morse.c_str(), text.c_str(), mcGame.state, mcGame.phase);
        if (text.length() > 0 && mcGame.state == MC_STATE_PLAYING && mcGame.phase == MC_PHASE_USER_INPUT) {
            mcGame.lastDecoded = text[0];
            mcGame.hasNewChar = true;
            Serial.printf("[MC] Stored decoded char: '%c'\n", mcGame.lastDecoded);
        }
    };

    setup = true;
}

// ============================================
// Straight Key Handler
// ============================================

void mcStraightKeyHandler(bool rawKeyDown) {
    extern int cwTone, cwSpeed;
    unsigned long now = millis();
    bool toneOn = isTonePlaying();

    // Software debouncing for scratchy/intermittent contacts
    // If raw state changed from last reading, reset debounce timer
    if (rawKeyDown != mcKeyLastRawState) {
        mcKeyLastChangeTime = now;
        mcKeyLastRawState = rawKeyDown;
    }

    // Only update debounced state if raw state has been stable for debounce period
    if ((now - mcKeyLastChangeTime) >= MC_STRAIGHT_KEY_DEBOUNCE_MS) {
        mcDebouncedKeyState = rawKeyDown;
    }

    // Use debounced state for keying
    bool keyDown = mcDebouncedKeyState;

    if (keyDown && !toneOn) {
        // Tone starting
        if (mcLastToneState == false) {
            Serial.printf("[MC] Straight key DOWN - starting tone %d Hz\n", cwTone);
            if (mcLastStateChange > 0) {
                float silence = now - mcLastStateChange;
                if (silence > 0) {
                    mcDecoder.addTiming(-silence);
                }
            }
            mcLastStateChange = now;
            mcLastToneState = true;
        }
        startTone(cwTone);
    }
    else if (keyDown && toneOn) {
        // Tone continuing
        continueTone(cwTone);
    }
    else if (!keyDown && toneOn) {
        // Tone stopping
        if (mcLastToneState == true) {
            float tone = now - mcLastStateChange;
            Serial.printf("[MC] Straight key UP - duration: %.0f ms\n", tone);
            if (tone > 0) {
                mcDecoder.addTiming(tone);
            }
            mcLastStateChange = now;
            mcLastToneState = false;
        }
        stopTone();
    }
}

// ============================================
// Iambic Keyer Handler
// ============================================

void mcIambicHandler(bool ditPressed, bool dahPressed) {
    extern int cwTone, cwSpeed;
    MorseTiming timing(cwSpeed);
    unsigned long ditDuration = timing.ditDuration;
    unsigned long now = millis();

    // Start new element
    if (!mcKeyerActive && !mcInSpacing) {
        bool startDit = mcDitMemory || ditPressed;
        bool startDah = mcDahMemory || dahPressed;

        if (startDit || startDah) {
            mcDitMemory = false;
            mcDahMemory = false;

            if (startDit && startDah) {
                // Squeeze - alternate
                mcSendingDit = !mcSendingDah;
                mcSendingDah = !mcSendingDit;
            } else {
                mcSendingDit = startDit;
                mcSendingDah = startDah;
            }

            mcKeyerActive = true;
            mcElementStart = now;

            Serial.printf("[MC] Starting %s\n", mcSendingDit ? "DIT" : "DAH");

            // Record silence before tone
            if (!mcLastToneState && mcLastStateChange > 0) {
                float silence = now - mcLastStateChange;
                if (silence > 0) {
                    mcDecoder.addTiming(-silence);
                }
            }
            mcLastStateChange = now;
            mcLastToneState = true;

            startTone(cwTone);  // Use startTone to begin!
        }
    }
    // Sending element
    else if (mcKeyerActive && !mcInSpacing) {
        unsigned long duration = mcSendingDit ? ditDuration : (ditDuration * 3);

        continueTone(cwTone);  // Continue existing tone

        // Check paddle memory
        if (ditPressed && dahPressed) {
            if (mcSendingDit) mcDahMemory = true;
            else mcDitMemory = true;
        } else if (mcSendingDit && dahPressed) {
            mcDahMemory = true;
        } else if (mcSendingDah && ditPressed) {
            mcDitMemory = true;
        }

        // Element complete?
        if (now - mcElementStart >= duration) {
            stopTone();

            float tone = now - mcLastStateChange;
            if (tone > 0) {
                mcDecoder.addTiming(tone);
            }
            mcLastStateChange = now;
            mcLastToneState = false;

            mcKeyerActive = false;
            mcInSpacing = true;
            mcElementStart = now;
        }
    }
    // Inter-element spacing
    else if (mcInSpacing) {
        if (now - mcElementStart >= ditDuration) {
            mcInSpacing = false;

            // Check for next element
            if (mcDitMemory) {
                mcDitMemory = false;
            } else if (mcDahMemory) {
                mcDahMemory = false;
            } else if (ditPressed) {
                mcDitMemory = true;
            } else if (dahPressed) {
                mcDahMemory = true;
            }
        }
    }
}

// ============================================
// Main Update Function
// ============================================

void memoryChainUpdate() {
    unsigned long now = millis();

    switch (mcGame.state) {
        case MC_STATE_READY:
            // Wait 1 second then start
            if (now - mcGame.stateStartTime > 1000) {
                mcGame.state = MC_STATE_PLAYING;

                // Add first character and play
                mcAddChar();
                mcUpdateLevel(mcGame.length);
                lv_timer_handler();  // Update UI

                mcDelayWithUI(MC_SEQUENCE_GAP);
                mcPlaySequence();
                mcDelayWithUI(MC_SEQUENCE_GAP);

                mcStartUserInput();
            }
            break;

        case MC_STATE_PLAYING:
            if (mcGame.phase == MC_PHASE_USER_INPUT) {
                // Check for decoded character (polling)
                if (mcGame.hasNewChar) {
                    mcGame.hasNewChar = false;
                    mcProcessDecodedChar(mcGame.lastDecoded);
                }

                // Flush decoder after silence
                if (mcLastStateChange > 0 && !isTonePlaying()) {
                    extern int cwSpeed;
                    MorseTiming timing(cwSpeed);
                    float gap = timing.ditDuration * 5;

                    if (now - mcLastStateChange > gap) {
                        Serial.printf("[MC] Flushing decoder after %lu ms silence\n", now - mcLastStateChange);
                        mcDecoder.flush();
                        mcLastStateChange = 0;
                    }
                }

                // Input timeout (only if player started)
                if (mcGame.playerPos > 0) {
                    if (now - mcGame.lastInputTime > MC_INPUT_TIMEOUT) {
                        Serial.println("[MC] Input timeout");
                        mcHandleWrong();
                    }
                }
            }
            break;

        case MC_STATE_GAME_OVER:
            // Wait for ENTER key (handled by key event)
            break;
    }
}

// ============================================
// Paddle Input Handler
// ============================================

void memoryChainHandlePaddle(bool ditPressed, bool dahPressed) {
    // Only accept input during user input phase
    if (mcGame.state != MC_STATE_PLAYING || mcGame.phase != MC_PHASE_USER_INPUT) {
        return;
    }

    // Debug: log paddle presses
    static bool lastDit = false, lastDah = false;
    if (ditPressed != lastDit || dahPressed != lastDah) {
        Serial.printf("[MC] Paddle: dit=%d dah=%d\n", ditPressed, dahPressed);
        lastDit = ditPressed;
        lastDah = dahPressed;
    }

    // Setup decoder callback once
    mcSetupDecoder();

    // Route to appropriate handler
    extern KeyType cwKeyType;
    if (cwKeyType == KEY_STRAIGHT) {
        mcStraightKeyHandler(ditPressed);
    } else {
        mcIambicHandler(ditPressed, dahPressed);
    }
}

// ============================================
// Game Start Function
// ============================================

void memoryChainStart() {
    Serial.println("[MC] ========================================");
    Serial.println("[MC] STARTING MEMORY CHAIN");
    Serial.println("[MC] ========================================");

    // Load preferences
    mcLoadPrefs();

    // Initialize game state
    memset(mcGame.sequence, 0, sizeof(mcGame.sequence));
    mcGame.length = 0;
    mcGame.playerPos = 0;
    mcGame.state = MC_STATE_READY;
    mcGame.phase = MC_PHASE_PLAYING_SEQUENCE;
    mcGame.lives = 3;
    mcGame.score = 0;
    mcGame.stateStartTime = millis();
    mcGame.lastInputTime = millis();
    mcGame.lastDecoded = 0;
    mcGame.hasNewChar = false;

    // Reset keyer state
    mcKeyerActive = false;
    mcSendingDit = false;
    mcSendingDah = false;
    mcInSpacing = false;
    mcDitMemory = false;
    mcDahMemory = false;
    mcLastToneState = false;
    mcLastStateChange = 0;

    // Reset decoder
    extern int cwSpeed;
    mcDecoder.reset();
    mcDecoder.flush();
    mcDecoder.setWPM(cwSpeed);

    // Setup decoder callback
    mcSetupDecoder();

    // UI will be updated when screen loads
    // (screen already exists at this point since mode integration
    //  calls this AFTER screen creation)

    Serial.printf("[MC] Game initialized - Lives: %d, High Score: %d\n",
                  mcGame.lives, mcGame.highScore);

    // Update UI with initial values
    mcUpdateLevel(1);
    mcUpdateScore(0);
    mcUpdateLives(mcGame.lives);
    mcUpdateStatus("GET READY");
    mcUpdateMessage("Listen to the sequence, then repeat it");
}

#endif // GAME_MEMORY_CHAIN_H
