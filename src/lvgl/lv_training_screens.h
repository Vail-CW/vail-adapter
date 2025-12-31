/*
 * VAIL SUMMIT - LVGL Training Screens
 * Provides LVGL UI for training modes
 * Audio-critical logic remains in original modules
 */

#ifndef LV_TRAINING_SCREENS_H
#define LV_TRAINING_SCREENS_H

#include <lvgl.h>
#include <WiFi.h>
#include "lv_theme_summit.h"
#include "lv_widgets_summit.h"
#include "lv_screen_manager.h"
#include "../core/config.h"
#include "../storage/sd_card.h"
#include "../training/training_license_core.h"
#include "../training/training_license_data.h"
#include "../training/training_license_stats.h"
#include "../training/training_license_downloader.h"

// External state from training modules
extern int cwSpeed;
extern int cwTone;
extern String decodedText;

// cwKeyType access function (defined in main sketch)
int getCwKeyTypeAsInt();

// LVGL-callable practice mode functions (from training_practice.h)
extern void practiceHandleEsc();
extern void practiceHandleClear();
extern void practiceAdjustSpeed(int delta);
extern void practiceCycleKeyType(int direction);
extern void practiceToggleDecoding();

// Key acceleration helper (from lv_init.h)
extern int getKeyAccelerationStep();

// ============================================
// Practice Mode Screen
// ============================================

static lv_obj_t* practice_screen = NULL;
static lv_obj_t* practice_decoder_box = NULL;
static lv_obj_t* practice_decoder_text = NULL;
static lv_obj_t* practice_wpm_label = NULL;
static lv_obj_t* practice_key_label = NULL;

// Key event callback for practice mode keyboard input
// Note: LV_KEY_PREV/NEXT are consumed by LVGL for group navigation
// We use LV_EVENT_VALUE_CHANGED or handle raw keys instead
static void practice_key_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    Serial.printf("[Practice LVGL] Key event: %lu (0x%02lX)\n", key, key);

    switch(key) {
        case LV_KEY_ESC:
            // ESC is handled by global_esc_handler in screen_manager
            // but we also call practiceHandleEsc to clean up practice state
            practiceHandleEsc();
            break;
        case 'c':
        case 'C':
            practiceHandleClear();
            // Update display after clear
            if (practice_decoder_text != NULL) {
                lv_label_set_text(practice_decoder_text, "_");
            }
            break;
        case 'd':
        case 'D':
            practiceToggleDecoding();
            break;
        case LV_KEY_UP:
            {
                int step = getKeyAccelerationStep();
                Serial.printf("[Practice] Speed UP by %d\n", step);
                practiceAdjustSpeed(step);
                // Update display
                if (practice_wpm_label != NULL) {
                    lv_label_set_text_fmt(practice_wpm_label, "%d WPM", cwSpeed);
                }
            }
            break;
        case LV_KEY_DOWN:
            {
                int step = getKeyAccelerationStep();
                Serial.printf("[Practice] Speed DOWN by %d\n", step);
                practiceAdjustSpeed(-step);
                // Update display
                if (practice_wpm_label != NULL) {
                    lv_label_set_text_fmt(practice_wpm_label, "%d WPM", cwSpeed);
                }
            }
            break;
        case LV_KEY_LEFT:
            practiceCycleKeyType(-1);
            // Update display
            if (practice_key_label != NULL) {
                int keyType = getCwKeyTypeAsInt();
                const char* key_type_str = (keyType == 0) ? "Straight" : ((keyType == 1) ? "Iambic A" : "Iambic B");
                lv_label_set_text(practice_key_label, key_type_str);
            }
            break;
        case LV_KEY_RIGHT:
            practiceCycleKeyType(1);
            // Update display
            if (practice_key_label != NULL) {
                int keyType = getCwKeyTypeAsInt();
                const char* key_type_str = (keyType == 0) ? "Straight" : ((keyType == 1) ? "Iambic A" : "Iambic B");
                lv_label_set_text(practice_key_label, key_type_str);
            }
            break;
    }
}

lv_obj_t* createPracticeScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "PRACTICE");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Settings display row - disable scrolling and move content up
    lv_obj_t* settings_row = lv_obj_create(screen);
    lv_obj_set_size(settings_row, SCREEN_WIDTH - 40, 50);
    lv_obj_set_pos(settings_row, 20, HEADER_HEIGHT + 10);
    lv_obj_set_layout(settings_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(settings_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(settings_row, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(settings_row, LV_OBJ_FLAG_SCROLLABLE);  // Remove scroll indicators
    applyCardStyle(settings_row);

    // Speed indicator - move content up slightly
    lv_obj_t* speed_box = lv_obj_create(settings_row);
    lv_obj_set_size(speed_box, 100, 44);
    lv_obj_clear_flag(speed_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(speed_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(speed_box, 0, 0);
    lv_obj_set_style_pad_all(speed_box, 0, 0);

    lv_obj_t* speed_lbl = lv_label_create(speed_box);
    lv_label_set_text(speed_lbl, "Speed");
    lv_obj_set_style_text_color(speed_lbl, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(speed_lbl, getThemeFonts()->font_small, 0);
    lv_obj_align(speed_lbl, LV_ALIGN_TOP_MID, 0, 0);

    practice_wpm_label = lv_label_create(speed_box);
    lv_label_set_text_fmt(practice_wpm_label, "%d WPM", cwSpeed);
    lv_obj_set_style_text_color(practice_wpm_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(practice_wpm_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_align(practice_wpm_label, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Tone indicator
    lv_obj_t* tone_box = lv_obj_create(settings_row);
    lv_obj_set_size(tone_box, 100, 44);
    lv_obj_clear_flag(tone_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(tone_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tone_box, 0, 0);
    lv_obj_set_style_pad_all(tone_box, 0, 0);

    lv_obj_t* tone_lbl = lv_label_create(tone_box);
    lv_label_set_text(tone_lbl, "Tone");
    lv_obj_set_style_text_color(tone_lbl, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(tone_lbl, getThemeFonts()->font_small, 0);
    lv_obj_align(tone_lbl, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* tone_val = lv_label_create(tone_box);
    lv_label_set_text_fmt(tone_val, "%d Hz", cwTone);
    lv_obj_set_style_text_color(tone_val, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(tone_val, getThemeFonts()->font_subtitle, 0);
    lv_obj_align(tone_val, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Key type indicator
    lv_obj_t* key_box = lv_obj_create(settings_row);
    lv_obj_set_size(key_box, 120, 44);
    lv_obj_clear_flag(key_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(key_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(key_box, 0, 0);
    lv_obj_set_style_pad_all(key_box, 0, 0);

    lv_obj_t* key_lbl = lv_label_create(key_box);
    lv_label_set_text(key_lbl, "Key");
    lv_obj_set_style_text_color(key_lbl, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(key_lbl, getThemeFonts()->font_small, 0);
    lv_obj_align(key_lbl, LV_ALIGN_TOP_MID, 0, 0);

    practice_key_label = lv_label_create(key_box);
    int keyType = getCwKeyTypeAsInt();
    const char* key_type_str = (keyType == 0) ? "Straight" : ((keyType == 1) ? "Iambic A" : "Iambic B");
    lv_label_set_text(practice_key_label, key_type_str);
    lv_obj_set_style_text_color(practice_key_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(practice_key_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_align(practice_key_label, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Decoder box - expanded now that stats row is removed
    practice_decoder_box = lv_obj_create(screen);
    lv_obj_set_size(practice_decoder_box, SCREEN_WIDTH - 20, 130);  // Wider box (20px margin total)
    lv_obj_set_pos(practice_decoder_box, 10, HEADER_HEIGHT + 70);
    lv_obj_set_style_bg_color(practice_decoder_box, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_border_color(practice_decoder_box, LV_COLOR_BORDER_SUBTLE, 0);
    lv_obj_set_style_border_width(practice_decoder_box, 1, 0);
    lv_obj_set_style_radius(practice_decoder_box, 8, 0);
    lv_obj_set_style_pad_all(practice_decoder_box, 10, 0);  // Reduced padding
    lv_obj_clear_flag(practice_decoder_box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* decoder_title = lv_label_create(practice_decoder_box);
    lv_label_set_text(decoder_title, "Decoded:");
    lv_obj_set_style_text_color(decoder_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(decoder_title, getThemeFonts()->font_small, 0);
    lv_obj_align(decoder_title, LV_ALIGN_TOP_LEFT, 0, 0);

    practice_decoder_text = lv_label_create(practice_decoder_box);
    lv_label_set_text(practice_decoder_text, "_");
    lv_obj_set_style_text_color(practice_decoder_text, LV_COLOR_ACCENT_GREEN, 0);
    lv_obj_set_style_text_font(practice_decoder_text, getThemeFonts()->font_title, 0);
    lv_label_set_long_mode(practice_decoder_text, LV_LABEL_LONG_WRAP);  // Wrap with newlines
    lv_obj_set_width(practice_decoder_text, SCREEN_WIDTH - 40);  // Wider text area (was -80)
    lv_obj_align(practice_decoder_text, LV_ALIGN_TOP_LEFT, 0, 18);  // Below "Decoded:" label

    // Footer with keyboard shortcuts
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, LV_SYMBOL_UP LV_SYMBOL_DOWN " Speed   " LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " Key   C Clear   ESC Exit");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    // Invisible focus container for keyboard input
    // This widget receives all keyboard input and routes it through practice_key_event_cb
    // NOTE: Cannot use LV_OBJ_FLAG_HIDDEN as hidden objects don't receive keyboard events
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);  // Minimal size (invisible but can receive focus)
    lv_obj_set_pos(focus_container, -10, -10);  // Position off-screen
    lv_obj_set_style_bg_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, 0);  // No focus outline
    lv_obj_set_style_outline_width(focus_container, 0, LV_STATE_FOCUSED);  // No focus outline when focused
    lv_obj_clear_flag(focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(focus_container, LV_OBJ_FLAG_CLICKABLE);  // Must be clickable to receive focus

    // Add keyboard event handler - use LV_EVENT_ALL to catch all events including navigation
    lv_obj_add_event_cb(focus_container, practice_key_event_cb, LV_EVENT_KEY, NULL);

    // Add to navigation group (enables keyboard input + ESC handling via global handler)
    addNavigableWidget(focus_container);

    // Put group in edit mode - this makes PREV/NEXT keys go to the widget instead of navigation
    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }

    // Ensure focus is on our container
    lv_group_focus_obj(focus_container);

    practice_screen = screen;
    return screen;
}

// Update decoder display with new text
void updatePracticeDecoderDisplay(const char* text) {
    if (practice_decoder_text != NULL) {
        if (text == NULL || strlen(text) == 0) {
            lv_label_set_text(practice_decoder_text, "_");
        } else {
            lv_label_set_text(practice_decoder_text, text);
        }
    }
}

// ============================================
// Hear It Type It Screen
// ============================================

// External state and functions from training_hear_it_type_it.h
// Note: HearItState enum is already defined in training_hear_it_type_it.h which is included first
extern int handleHearItTypeItInput(char key, LGFX& tft);
extern String userInput;
extern bool inSettingsMode;
extern HearItState currentHearItState;
extern HearItSettings hearItSettings;
extern HearItSettings tempSettings;
extern HearItStats sessionStats;
extern String currentCallsign;
extern bool hearItUseLVGL;
extern void saveHearItSettings();
extern void startNewCallsign();
extern void playCurrentCallsign();

// Forward declaration for back navigation
extern void onLVGLBackNavigation();

// Forward declarations for functions defined later in this file
void updateHearItDisplay(const char* prompt, bool show_prompt);

static lv_obj_t* hear_it_screen = NULL;
static lv_obj_t* hear_it_prompt = NULL;
static lv_obj_t* hear_it_input = NULL;
static lv_obj_t* hear_it_result = NULL;
static lv_obj_t* hear_it_score_label = NULL;
static lv_obj_t* hear_it_footer_help = NULL;

// Settings form widgets
static lv_obj_t* hear_it_settings_container = NULL;
static lv_obj_t* hear_it_training_container = NULL;
static lv_obj_t* hear_it_mode_row = NULL;     // Mode row container for focus styling
static lv_obj_t* hear_it_mode_value = NULL;   // Mode display label (replaces dropdown)
static lv_obj_t* hear_it_length_row = NULL;   // Length row container for focus styling
static lv_obj_t* hear_it_length_slider = NULL;
static lv_obj_t* hear_it_length_value = NULL;
static lv_obj_t* hear_it_start_btn = NULL;

// Mode names for display
static const char* hear_it_mode_names[] = {"Callsigns", "Letters", "Numbers", "Mixed", "Custom"};
static const int hear_it_mode_count = 5;

// Timers that need to be cancelled when leaving the screen
static lv_timer_t* hear_it_pending_timer = NULL;
static lv_timer_t* hear_it_start_timer = NULL;

// Forward declarations for timer callbacks
static void hear_it_skip_timer_cb(lv_timer_t* timer);

// Track which settings widget is currently focused (0=mode, 1=speed, 2=length, 3=button)
static int hear_it_settings_focus = 0;

// Focus container for settings navigation (invisible widget that receives key events)
static lv_obj_t* hear_it_focus_container = NULL;

// Speed row widgets
static lv_obj_t* hear_it_speed_row = NULL;
static lv_obj_t* hear_it_speed_slider = NULL;
static lv_obj_t* hear_it_speed_value = NULL;

// Forward declaration for focus update function
static void hear_it_update_focus();

// External function to get current settings display string
// Defined in training_hear_it_type_it.h
extern String getHearItSettingsString();

// Update footer based on current state
void updateHearItFooter() {
    if (hear_it_footer_help == NULL) return;

    if (currentHearItState == HEAR_IT_STATE_SETTINGS) {
        lv_label_set_text(hear_it_footer_help, "UP/DN Navigate   L/R Adjust   ENTER Start   ESC Back");
    } else {
        lv_label_set_text(hear_it_footer_help, FOOTER_TRAINING_ACTIVE);
    }
}

// Update display based on state (settings vs training)
void updateHearItSettingsDisplay() {
    if (currentHearItState == HEAR_IT_STATE_SETTINGS) {
        // SETTINGS MODE: Show settings form, hide training UI
        if (hear_it_settings_container != NULL) {
            lv_obj_clear_flag(hear_it_settings_container, LV_OBJ_FLAG_HIDDEN);
        }
        if (hear_it_training_container != NULL) {
            lv_obj_add_flag(hear_it_training_container, LV_OBJ_FLAG_HIDDEN);
        }
        if (hear_it_score_label != NULL) {
            lv_obj_add_flag(hear_it_score_label, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        // TRAINING MODE: Hide settings form, show training UI
        if (hear_it_settings_container != NULL) {
            lv_obj_add_flag(hear_it_settings_container, LV_OBJ_FLAG_HIDDEN);
        }
        if (hear_it_training_container != NULL) {
            lv_obj_clear_flag(hear_it_training_container, LV_OBJ_FLAG_HIDDEN);
            // Focus the input textarea
            if (hear_it_input != NULL) {
                lv_group_focus_obj(hear_it_input);
            }
        }
        if (hear_it_score_label != NULL) {
            lv_obj_clear_flag(hear_it_score_label, LV_OBJ_FLAG_HIDDEN);
        }
    }
    // Update footer text
    updateHearItFooter();
}

// Key event callback for Hear It Type It training mode (input textarea)
static void hear_it_key_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    // Only handle keys in TRAINING state (settings form handles itself)
    if (currentHearItState != HEAR_IT_STATE_TRAINING) return;

    uint32_t key = lv_event_get_key(e);

    Serial.printf("[HearIt LVGL] Training key: %lu (0x%02lX)\n", key, key);

    // Map LVGL keys to legacy key codes
    char legacy_key = 0;

    if (key == LV_KEY_ENTER) {
        legacy_key = KEY_ENTER;
    } else if (key == LV_KEY_ESC) {
        // ESC goes back to settings - stop morse playback
        cancelHearItTimers();
        currentHearItState = HEAR_IT_STATE_SETTINGS;
        inSettingsMode = true;
        hear_it_settings_focus = 0;  // Reset focus to first item
        updateHearItSettingsDisplay();
        hear_it_update_focus();

        // Re-add focus container to nav group (was removed when entering training mode)
        lv_group_t* group = getLVGLInputGroup();
        if (group != NULL && hear_it_focus_container != NULL) {
            lv_group_add_obj(group, hear_it_focus_container);
        }

        // Re-focus the settings focus container so arrow keys work
        if (hear_it_focus_container != NULL) {
            lv_group_focus_obj(hear_it_focus_container);
        }

        // Ensure group is in edit mode for arrow key navigation
        if (group != NULL) {
            lv_group_set_editing(group, true);
        }

        beep(TONE_MENU_NAV, BEEP_SHORT);
        lv_event_stop_processing(e);  // Prevent global ESC handler from also firing
        return;
    } else if (key == LV_KEY_LEFT) {
        legacy_key = KEY_LEFT;  // Replay
    } else if (key == LV_KEY_RIGHT) {  // RIGHT arrow - Skip to next callsign
        // Handle skip directly in LVGL (non-blocking)
        beep(TONE_MENU_NAV, BEEP_SHORT);

        // Cancel any pending timer
        if (hear_it_pending_timer != NULL) {
            lv_timer_del(hear_it_pending_timer);
            hear_it_pending_timer = NULL;
        }

        // Generate new callsign
        startNewCallsign();

        // Clear input field
        if (hear_it_input != NULL) {
            lv_textarea_set_text(hear_it_input, "");
        }

        // Update prompt to show "Get Ready..."
        if (hear_it_prompt != NULL) {
            lv_label_set_text(hear_it_prompt, "Skipped - Get Ready...");
            lv_obj_set_style_text_color(hear_it_prompt, LV_COLOR_WARNING, 0);
        }

        // Clear any result text
        if (hear_it_result != NULL) {
            lv_label_set_text(hear_it_result, "");
        }

        // Use timer to start playback (non-blocking, allows UI to update)
        hear_it_pending_timer = lv_timer_create(hear_it_skip_timer_cb, 500, NULL);
        return;  // Don't route to legacy handler
    } else if (key == LV_KEY_BACKSPACE) {
        return;  // Let LVGL handle backspace
    } else if (key >= 32 && key <= 126) {
        // Printable character - let LVGL handle typing
        return;
    } else {
        return;  // Unknown key, ignore
    }

    // Route special keys to legacy handler
    int result = handleHearItTypeItInput(legacy_key, tft);
    Serial.printf("[HearIt LVGL] Handler result: %d\n", result);

    // Handle result
    if (result == -1) {
        onLVGLBackNavigation();
    } else {
        // Sync textarea with userInput
        if (hear_it_input != NULL) {
            lv_textarea_set_text(hear_it_input, userInput.c_str());
        }
        // Update score display
        if (hear_it_score_label != NULL) {
            lv_label_set_text_fmt(hear_it_score_label, "Score: %d/%d",
                                  sessionStats.totalCorrect, sessionStats.totalAttempts);
        }
    }
}

// Value changed callback - sync textarea to userInput
static void hear_it_value_changed_cb(lv_event_t* e) {
    lv_obj_t* ta = lv_event_get_target(e);
    if (currentHearItState == HEAR_IT_STATE_TRAINING) {
        const char* text = lv_textarea_get_text(ta);
        userInput = String(text);
        Serial.printf("[HearIt LVGL] Value changed: '%s'\n", text);
    }
}

// Show feedback via LVGL (replaces legacy tft.print)
void showHearItFeedback(bool correct, const String& answer) {
    if (hear_it_result == NULL) return;

    if (correct) {
        lv_label_set_text_fmt(hear_it_result, LV_SYMBOL_OK " Correct! (%s)", answer.c_str());
        lv_obj_set_style_text_color(hear_it_result, LV_COLOR_SUCCESS, 0);
    } else {
        lv_label_set_text(hear_it_result, LV_SYMBOL_CLOSE " Try again");
        lv_obj_set_style_text_color(hear_it_result, LV_COLOR_ERROR, 0);
    }
}

// Update score display
void updateHearItScore() {
    if (hear_it_score_label != NULL) {
        lv_label_set_text_fmt(hear_it_score_label, "Score: %d/%d",
                              sessionStats.totalCorrect, sessionStats.totalAttempts);
    }
}

// Clear the input textarea
void clearHearItInput() {
    if (hear_it_input != NULL) {
        lv_textarea_set_text(hear_it_input, "");
    }
}

// Timer callback for delayed start
static void hear_it_start_timer_cb(lv_timer_t* timer) {
    Serial.println("[HearIt] Timer fired - starting training");
    hear_it_start_timer = NULL;  // Clear reference before delete
    startNewCallsign();
    playCurrentCallsign();
    if (hear_it_prompt != NULL) {
        lv_label_set_text(hear_it_prompt, "Type what you hear:");
    }
    lv_timer_del(timer);
}

// Cancel all pending Hear It timers
void cancelHearItTimers() {
    if (hear_it_pending_timer != NULL) {
        lv_timer_del(hear_it_pending_timer);
        hear_it_pending_timer = NULL;
    }
    if (hear_it_start_timer != NULL) {
        lv_timer_del(hear_it_start_timer);
        hear_it_start_timer = NULL;
    }
    stopTone();  // Also stop any playing tone
}

// Cleanup Hear It Type It screen - reset all static pointers
// Called when leaving the screen to prevent use-after-free on re-entry
void cleanupHearItTypeItScreen() {
    Serial.println("[HearIt LVGL] Cleaning up Hear It Type It screen");
    cancelHearItTimers();  // Cancel timers and set to NULL

    // Reset all widget pointers to prevent dangling references
    hear_it_screen = NULL;
    hear_it_prompt = NULL;
    hear_it_input = NULL;
    hear_it_result = NULL;
    hear_it_score_label = NULL;
    hear_it_footer_help = NULL;
    hear_it_settings_container = NULL;
    hear_it_training_container = NULL;
    hear_it_mode_row = NULL;
    hear_it_mode_value = NULL;
    hear_it_length_row = NULL;
    hear_it_length_slider = NULL;
    hear_it_length_value = NULL;
    hear_it_speed_row = NULL;
    hear_it_speed_slider = NULL;
    hear_it_speed_value = NULL;
    hear_it_start_btn = NULL;
    hear_it_focus_container = NULL;
}

// Timer callback for correct answer - play next callsign after feedback delay
static void hear_it_correct_timer_cb(lv_timer_t* timer) {
    Serial.println("[HearIt] Correct timer fired - next callsign");
    hear_it_pending_timer = NULL;  // Clear reference before delete
    startNewCallsign();
    delay(500);  // Brief pause before playback
    playCurrentCallsign();
    lv_timer_del(timer);
}

// Timer callback for incorrect answer - replay same callsign after feedback delay
static void hear_it_incorrect_timer_cb(lv_timer_t* timer) {
    Serial.println("[HearIt] Incorrect timer fired - replaying");
    hear_it_pending_timer = NULL;  // Clear reference before delete
    delay(500);  // Brief pause before replay
    playCurrentCallsign();
    lv_timer_del(timer);
}

// Timer callback for skip action - play new callsign after brief delay
static void hear_it_skip_timer_cb(lv_timer_t* timer) {
    Serial.println("[HearIt] Skip timer fired - playing new callsign");
    hear_it_pending_timer = NULL;  // Clear reference before delete
    lv_timer_del(timer);

    playCurrentCallsign();

    // Update prompt after playback starts
    if (hear_it_prompt != NULL) {
        lv_label_set_text(hear_it_prompt, "Type what you hear:");
        lv_obj_set_style_text_color(hear_it_prompt, LV_COLOR_TEXT_SECONDARY, 0);
    }
}

// Schedule next callsign after feedback delay (called from legacy handler)
void scheduleHearItNextCallsign(bool wasCorrect) {
    // Cancel any existing pending timer
    if (hear_it_pending_timer != NULL) {
        lv_timer_del(hear_it_pending_timer);
    }
    if (wasCorrect) {
        hear_it_pending_timer = lv_timer_create(hear_it_correct_timer_cb, 1500, NULL);
    } else {
        hear_it_pending_timer = lv_timer_create(hear_it_incorrect_timer_cb, 1500, NULL);
    }
}

// Slider value changed callback
static void hear_it_length_slider_cb(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);
    tempSettings.groupLength = value;
    if (hear_it_length_value != NULL) {
        lv_label_set_text_fmt(hear_it_length_value, "%d", value);
    }
}

// Update mode display
static void hear_it_update_mode_display() {
    if (hear_it_mode_value != NULL) {
        lv_label_set_text_fmt(hear_it_mode_value, "< %s >", hear_it_mode_names[tempSettings.mode]);
    }
}

// Update visual focus indicator
static void hear_it_update_focus() {
    // Mode row styling (focus == 0)
    if (hear_it_mode_row) {
        if (hear_it_settings_focus == 0) {
            lv_obj_set_style_bg_color(hear_it_mode_row, LV_COLOR_CARD_TEAL, 0);
            lv_obj_set_style_bg_opa(hear_it_mode_row, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(hear_it_mode_row, LV_COLOR_ACCENT_CYAN, 0);
            lv_obj_set_style_border_width(hear_it_mode_row, 2, 0);
        } else {
            lv_obj_set_style_bg_opa(hear_it_mode_row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(hear_it_mode_row, 0, 0);
        }
    }
    if (hear_it_mode_value) {
        lv_obj_set_style_text_color(hear_it_mode_value,
            hear_it_settings_focus == 0 ? LV_COLOR_ACCENT_CYAN : LV_COLOR_TEXT_SECONDARY, 0);
    }

    // Speed row styling (focus == 1)
    if (hear_it_speed_row) {
        if (hear_it_settings_focus == 1) {
            lv_obj_set_style_bg_color(hear_it_speed_row, LV_COLOR_CARD_TEAL, 0);
            lv_obj_set_style_bg_opa(hear_it_speed_row, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(hear_it_speed_row, LV_COLOR_ACCENT_CYAN, 0);
            lv_obj_set_style_border_width(hear_it_speed_row, 2, 0);
        } else {
            lv_obj_set_style_bg_opa(hear_it_speed_row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(hear_it_speed_row, 0, 0);
        }
    }
    if (hear_it_speed_slider) {
        if (hear_it_settings_focus == 1) {
            lv_obj_add_state(hear_it_speed_slider, LV_STATE_FOCUSED);
        } else {
            lv_obj_clear_state(hear_it_speed_slider, LV_STATE_FOCUSED);
        }
    }

    // Length row styling (focus == 2)
    if (hear_it_length_row) {
        if (hear_it_settings_focus == 2) {
            lv_obj_set_style_bg_color(hear_it_length_row, LV_COLOR_CARD_TEAL, 0);
            lv_obj_set_style_bg_opa(hear_it_length_row, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(hear_it_length_row, LV_COLOR_ACCENT_CYAN, 0);
            lv_obj_set_style_border_width(hear_it_length_row, 2, 0);
        } else {
            lv_obj_set_style_bg_opa(hear_it_length_row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(hear_it_length_row, 0, 0);
        }
    }
    if (hear_it_length_slider) {
        if (hear_it_settings_focus == 2) {
            lv_obj_add_state(hear_it_length_slider, LV_STATE_FOCUSED);
        } else {
            lv_obj_clear_state(hear_it_length_slider, LV_STATE_FOCUSED);
        }
    }

    // Button styling (focus == 3)
    if (hear_it_start_btn) {
        if (hear_it_settings_focus == 3) {
            lv_obj_add_state(hear_it_start_btn, LV_STATE_FOCUSED);
        } else {
            lv_obj_clear_state(hear_it_start_btn, LV_STATE_FOCUSED);
        }
    }
}

// Unified settings key handler - handles all navigation for settings widgets
// Attached to a focus container, not the widgets themselves
static void hear_it_settings_key_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // Handle ESC for back navigation (check first)
    if (key == LV_KEY_ESC) {
        // Stop all playing morse code and pending timers
        cancelHearItTimers();
        onLVGLBackNavigation();
        lv_event_stop_processing(e);  // Prevent global ESC handler from also firing
        return;
    }

    // Handle UP/DOWN for navigation between settings
    if (key == LV_KEY_UP) {
        if (hear_it_settings_focus > 0) {
            hear_it_settings_focus--;
            hear_it_update_focus();
        }
        return;
    }
    if (key == LV_KEY_DOWN) {
        if (hear_it_settings_focus < 3) {
            hear_it_settings_focus++;
            hear_it_update_focus();
        }
        return;
    }

    // Handle LEFT/RIGHT for value adjustment based on current focus
    if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT) {
        if (hear_it_settings_focus == 0) {
            // Mode - cycle through options
            int mode = (int)tempSettings.mode;
            if (key == LV_KEY_RIGHT) {
                mode = (mode + 1) % hear_it_mode_count;
            } else {
                mode = (mode - 1 + hear_it_mode_count) % hear_it_mode_count;
            }
            tempSettings.mode = (HearItMode)mode;
            hear_it_update_mode_display();
        }
        else if (hear_it_settings_focus == 1 && hear_it_speed_slider) {
            // Speed slider - adjust WPM value
            int step = getKeyAccelerationStep();
            int delta = (key == LV_KEY_RIGHT) ? step : -step;
            int current = lv_slider_get_value(hear_it_speed_slider);
            int new_val = current + delta;

            if (new_val < 10) new_val = 10;
            if (new_val > 40) new_val = 40;

            lv_slider_set_value(hear_it_speed_slider, new_val, LV_ANIM_OFF);
            tempSettings.wpm = new_val;
            if (hear_it_speed_value != NULL) {
                lv_label_set_text_fmt(hear_it_speed_value, "%d", new_val);
            }
        }
        else if (hear_it_settings_focus == 2 && hear_it_length_slider) {
            // Group length slider - adjust value
            int step = getKeyAccelerationStep();
            int delta = (key == LV_KEY_RIGHT) ? step : -step;
            int current = lv_slider_get_value(hear_it_length_slider);
            int new_val = current + delta;

            if (new_val < 1) new_val = 1;
            if (new_val > 10) new_val = 10;

            lv_slider_set_value(hear_it_length_slider, new_val, LV_ANIM_OFF);
            lv_event_send(hear_it_length_slider, LV_EVENT_VALUE_CHANGED, NULL);
        }
        // Button (focus == 3) doesn't respond to LEFT/RIGHT
        return;
    }

    // Handle ENTER
    if (key == LV_KEY_ENTER) {
        if (hear_it_settings_focus == 3 && hear_it_start_btn) {
            // Trigger start button
            lv_event_send(hear_it_start_btn, LV_EVENT_CLICKED, NULL);
        }
        // Mode and slider don't need ENTER - they use LEFT/RIGHT
        return;
    }
}

// Start button clicked callback
static void hear_it_start_btn_cb(lv_event_t* e) {
    Serial.println("[HearIt] Start button clicked");

    // Save settings from form widgets (tempSettings already updated by key handler)
    if (hear_it_speed_slider != NULL) {
        tempSettings.wpm = lv_slider_get_value(hear_it_speed_slider);
    }
    if (hear_it_length_slider != NULL) {
        tempSettings.groupLength = lv_slider_get_value(hear_it_length_slider);
    }

    // Apply and save settings
    hearItSettings = tempSettings;
    saveHearItSettings();

    // Reset session stats
    sessionStats.totalAttempts = 0;
    sessionStats.totalCorrect = 0;
    sessionStats.sessionStartTime = millis();

    // Transition to training state
    currentHearItState = HEAR_IT_STATE_TRAINING;
    inSettingsMode = false;
    userInput = "";

    // Update UI
    updateHearItSettingsDisplay();

    // Clear any previous feedback
    if (hear_it_result != NULL) {
        lv_label_set_text(hear_it_result, "");
    }

    // Clear the input field
    if (hear_it_input != NULL) {
        lv_textarea_set_text(hear_it_input, "");
    }

    // Put the input group in editing mode so TAB key reaches our handler
    lv_group_t* group = getLVGLInputGroup();
    if (group) {
        lv_group_set_editing(group, true);
        // Remove focus container from nav group so TAB doesn't navigate away
        // This ensures TAB reaches the textarea's key handler for skip functionality
        if (hear_it_focus_container != NULL) {
            lv_group_remove_obj(hear_it_focus_container);
        }
    }

    // Show "Get Ready..." message
    if (hear_it_prompt != NULL) {
        lv_label_set_text(hear_it_prompt, "Get Ready...");
        lv_obj_set_style_text_color(hear_it_prompt, LV_COLOR_WARNING, 0);
    }

    // Play sound to indicate start
    beep(TONE_SELECT, BEEP_LONG);

    // Create timer to start after 3 seconds
    hear_it_start_timer = lv_timer_create(hear_it_start_timer_cb, 3000, NULL);
}


lv_obj_t* createHearItTypeItScreen() {
    // Copy current settings to temp for editing
    tempSettings = hearItSettings;

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "HEAR IT TYPE IT");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Score display (hidden initially - shown in training mode)
    hear_it_score_label = lv_label_create(screen);
    lv_label_set_text(hear_it_score_label, "Score: 0/0");
    lv_obj_set_style_text_color(hear_it_score_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(hear_it_score_label, getThemeFonts()->font_input, 0);
    lv_obj_align(hear_it_score_label, LV_ALIGN_TOP_RIGHT, -20, HEADER_HEIGHT + 10);
    lv_obj_add_flag(hear_it_score_label, LV_OBJ_FLAG_HIDDEN);

    // ========================================
    // SETTINGS CONTAINER (shown in settings mode)
    // ========================================
    hear_it_settings_container = lv_obj_create(screen);
    lv_obj_set_size(hear_it_settings_container, SCREEN_WIDTH - 40, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 10);
    lv_obj_set_pos(hear_it_settings_container, 20, HEADER_HEIGHT + 5);
    lv_obj_set_layout(hear_it_settings_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hear_it_settings_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(hear_it_settings_container, 6, 0);
    lv_obj_set_style_pad_all(hear_it_settings_container, 8, 0);
    applyCardStyle(hear_it_settings_container);

    // Create an invisible focus container to receive all key events
    // This bypasses LVGL's widget-level key handling
    // Use static variable so we can re-focus it when returning from training mode
    hear_it_focus_container = lv_obj_create(hear_it_settings_container);
    lv_obj_set_size(hear_it_focus_container, 0, 0);
    lv_obj_set_style_bg_opa(hear_it_focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hear_it_focus_container, 0, 0);
    lv_obj_clear_flag(hear_it_focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(hear_it_focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(hear_it_focus_container, hear_it_settings_key_handler, LV_EVENT_KEY, NULL);
    addNavigableWidget(hear_it_focus_container);

    // Put group in edit mode - this makes UP/DOWN keys go to the widget instead of being consumed by LVGL
    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }

    // Ensure focus is on our container
    lv_group_focus_obj(hear_it_focus_container);

    // Reset focus state and set initial focus
    hear_it_settings_focus = 0;

    // Mode row (label + value selector)
    hear_it_mode_row = lv_obj_create(hear_it_settings_container);
    lv_obj_set_size(hear_it_mode_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(hear_it_mode_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hear_it_mode_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hear_it_mode_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(hear_it_mode_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hear_it_mode_row, 0, 0);
    lv_obj_set_style_pad_all(hear_it_mode_row, 4, 0);
    lv_obj_set_style_radius(hear_it_mode_row, 6, 0);
    lv_obj_clear_flag(hear_it_mode_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* mode_label = lv_label_create(hear_it_mode_row);
    lv_label_set_text(mode_label, "Mode");
    lv_obj_add_style(mode_label, getStyleLabelSubtitle(), 0);

    // Mode value - shows "< Callsigns >" style selector
    hear_it_mode_value = lv_label_create(hear_it_mode_row);
    lv_label_set_text_fmt(hear_it_mode_value, "< %s >", hear_it_mode_names[tempSettings.mode]);
    lv_obj_set_style_text_color(hear_it_mode_value, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(hear_it_mode_value, getThemeFonts()->font_subtitle, 0);

    // Speed row (WPM setting) - container wraps header and slider for focus styling
    hear_it_speed_row = lv_obj_create(hear_it_settings_container);
    lv_obj_set_size(hear_it_speed_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(hear_it_speed_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hear_it_speed_row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(hear_it_speed_row, 4, 0);
    lv_obj_set_style_bg_opa(hear_it_speed_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hear_it_speed_row, 0, 0);
    lv_obj_set_style_pad_all(hear_it_speed_row, 4, 0);
    lv_obj_set_style_radius(hear_it_speed_row, 6, 0);
    lv_obj_clear_flag(hear_it_speed_row, LV_OBJ_FLAG_SCROLLABLE);

    // Speed header row (label + value on same line)
    lv_obj_t* speed_header = lv_obj_create(hear_it_speed_row);
    lv_obj_set_size(speed_header, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(speed_header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(speed_header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(speed_header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(speed_header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(speed_header, 0, 0);
    lv_obj_set_style_pad_all(speed_header, 0, 0);

    lv_obj_t* speed_label = lv_label_create(speed_header);
    lv_label_set_text(speed_label, "Speed (WPM)");
    lv_obj_add_style(speed_label, getStyleLabelSubtitle(), 0);

    hear_it_speed_value = lv_label_create(speed_header);
    lv_label_set_text_fmt(hear_it_speed_value, "%d", tempSettings.wpm);
    lv_obj_set_style_text_color(hear_it_speed_value, LV_COLOR_ACCENT_CYAN, 0);

    // Speed slider - inside speed container (compact size)
    hear_it_speed_slider = lv_slider_create(hear_it_speed_row);
    lv_obj_set_width(hear_it_speed_slider, lv_pct(100));
    lv_obj_set_height(hear_it_speed_slider, 8);
    lv_slider_set_range(hear_it_speed_slider, 10, 40);
    lv_slider_set_value(hear_it_speed_slider, tempSettings.wpm, LV_ANIM_OFF);
    applySliderStyle(hear_it_speed_slider);
    // Make knob smaller for compact look
    lv_obj_set_style_pad_all(hear_it_speed_slider, 4, LV_PART_KNOB);
    // Don't add to nav group - focus container handles all keys

    // Group Length container (wraps header and slider for focus styling)
    hear_it_length_row = lv_obj_create(hear_it_settings_container);
    lv_obj_set_size(hear_it_length_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(hear_it_length_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hear_it_length_row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(hear_it_length_row, 4, 0);
    lv_obj_set_style_bg_opa(hear_it_length_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hear_it_length_row, 0, 0);
    lv_obj_set_style_pad_all(hear_it_length_row, 4, 0);
    lv_obj_set_style_radius(hear_it_length_row, 6, 0);
    lv_obj_clear_flag(hear_it_length_row, LV_OBJ_FLAG_SCROLLABLE);

    // Length header row (label + value on same line)
    lv_obj_t* length_header = lv_obj_create(hear_it_length_row);
    lv_obj_set_size(length_header, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(length_header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(length_header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(length_header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(length_header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(length_header, 0, 0);
    lv_obj_set_style_pad_all(length_header, 0, 0);

    lv_obj_t* length_label = lv_label_create(length_header);
    lv_label_set_text(length_label, "Group Length");
    lv_obj_add_style(length_label, getStyleLabelSubtitle(), 0);

    hear_it_length_value = lv_label_create(length_header);
    lv_label_set_text_fmt(hear_it_length_value, "%d", tempSettings.groupLength);
    lv_obj_set_style_text_color(hear_it_length_value, LV_COLOR_ACCENT_CYAN, 0);

    // Group Length slider - inside length container (compact size)
    hear_it_length_slider = lv_slider_create(hear_it_length_row);
    lv_obj_set_width(hear_it_length_slider, lv_pct(100));
    lv_obj_set_height(hear_it_length_slider, 8);
    lv_slider_set_range(hear_it_length_slider, 1, 10);
    lv_slider_set_value(hear_it_length_slider, tempSettings.groupLength, LV_ANIM_OFF);
    applySliderStyle(hear_it_length_slider);
    // Make knob smaller for compact look
    lv_obj_set_style_pad_all(hear_it_length_slider, 4, LV_PART_KNOB);
    lv_obj_add_event_cb(hear_it_length_slider, hear_it_length_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
    // Don't add to nav group - focus container handles all keys

    // Start Training button
    hear_it_start_btn = lv_btn_create(hear_it_settings_container);
    lv_obj_set_size(hear_it_start_btn, lv_pct(100), 40);
    lv_obj_set_style_bg_color(hear_it_start_btn, LV_COLOR_CARD_TEAL, 0);
    lv_obj_set_style_bg_color(hear_it_start_btn, LV_COLOR_CARD_TEAL, LV_STATE_FOCUSED);
    lv_obj_set_style_radius(hear_it_start_btn, 8, 0);
    lv_obj_set_style_border_width(hear_it_start_btn, 1, 0);
    lv_obj_set_style_border_color(hear_it_start_btn, LV_COLOR_BORDER_SUBTLE, 0);
    lv_obj_set_style_border_width(hear_it_start_btn, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(hear_it_start_btn, LV_COLOR_ACCENT_CYAN, LV_STATE_FOCUSED);

    lv_obj_t* btn_label = lv_label_create(hear_it_start_btn);
    lv_label_set_text(btn_label, "Start Training");
    lv_obj_center(btn_label);
    lv_obj_set_style_text_color(btn_label, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(btn_label, getThemeFonts()->font_subtitle, 0);

    lv_obj_add_event_cb(hear_it_start_btn, hear_it_start_btn_cb, LV_EVENT_CLICKED, NULL);
    // Don't add to nav group - focus container handles all keys

    // Set initial focus visual
    hear_it_update_focus();

    // ========================================
    // TRAINING CONTAINER (hidden initially, shown during training)
    // ========================================
    hear_it_training_container = lv_obj_create(screen);
    lv_obj_set_size(hear_it_training_container, SCREEN_WIDTH - 40, 180);
    // Position below score label (centered horizontally, offset down from center)
    lv_obj_align(hear_it_training_container, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_layout(hear_it_training_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hear_it_training_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(hear_it_training_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(hear_it_training_container, 15, 0);
    applyCardStyle(hear_it_training_container);
    lv_obj_add_flag(hear_it_training_container, LV_OBJ_FLAG_HIDDEN);  // Hidden initially

    // Prompt label
    hear_it_prompt = lv_label_create(hear_it_training_container);
    lv_label_set_text(hear_it_prompt, "Type what you hear:");
    lv_obj_set_style_text_font(hear_it_prompt, getThemeFonts()->font_input, 0);
    lv_obj_set_style_text_color(hear_it_prompt, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_align(hear_it_prompt, LV_TEXT_ALIGN_CENTER, 0);

    // Input textarea
    hear_it_input = lv_textarea_create(hear_it_training_container);
    lv_obj_set_size(hear_it_input, 300, 50);
    lv_textarea_set_one_line(hear_it_input, true);
    lv_textarea_set_placeholder_text(hear_it_input, "Type your answer");
    lv_obj_add_style(hear_it_input, getStyleTextarea(), 0);
    lv_obj_set_style_text_font(hear_it_input, getThemeFonts()->font_subtitle, 0);
    lv_obj_add_event_cb(hear_it_input, hear_it_key_event_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(hear_it_input, hear_it_value_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
    addNavigableWidget(hear_it_input);

    // Result indicator
    hear_it_result = lv_label_create(hear_it_training_container);
    lv_label_set_text(hear_it_result, "");
    lv_obj_set_style_text_font(hear_it_result, getThemeFonts()->font_subtitle, 0);

    // ========================================
    // FOOTER
    // ========================================
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    hear_it_footer_help = lv_label_create(footer);
    lv_label_set_text(hear_it_footer_help, FOOTER_NAV_ENTER_ESC);  // Use standardized footer
    lv_obj_set_style_text_color(hear_it_footer_help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(hear_it_footer_help, getThemeFonts()->font_small, 0);
    lv_obj_center(hear_it_footer_help);

    hear_it_screen = screen;

    // Initialize display state
    updateHearItSettingsDisplay();

    return screen;
}

// Update Hear It Type It display based on state
// Note: This is called by legacy handler and mostly handles training mode updates
void updateHearItDisplay(const char* prompt, bool show_prompt) {
    // Settings display and visibility is handled by updateHearItSettingsDisplay()
    // This function focuses on training mode prompt updates

    if (hear_it_prompt != NULL && currentHearItState == HEAR_IT_STATE_TRAINING) {
        if (show_prompt && prompt != NULL) {
            // Training mode with prompt visible
            lv_label_set_text(hear_it_prompt, prompt);
            lv_obj_set_style_text_color(hear_it_prompt, LV_COLOR_ACCENT_CYAN, 0);
        } else {
            // Training mode - listening
            lv_label_set_text(hear_it_prompt, "Type what you hear:");
            lv_obj_set_style_text_color(hear_it_prompt, LV_COLOR_TEXT_SECONDARY, 0);
        }
    }
    // Update footer when display updates
    updateHearItFooter();
}

void updateHearItResult(bool correct) {
    if (hear_it_result != NULL) {
        if (correct) {
            lv_label_set_text(hear_it_result, LV_SYMBOL_OK " Correct!");
            lv_obj_set_style_text_color(hear_it_result, LV_COLOR_SUCCESS, 0);
        } else {
            lv_label_set_text(hear_it_result, LV_SYMBOL_CLOSE " Try again");
            lv_obj_set_style_text_color(hear_it_result, LV_COLOR_ERROR, 0);
        }
    }
}

// ============================================
// Koch Method Screen
// ============================================

static lv_obj_t* koch_screen = NULL;
static lv_obj_t* koch_level_label = NULL;
static lv_obj_t* koch_chars_label = NULL;
static lv_obj_t* koch_progress_bar = NULL;

// Key event callback for Koch Method keyboard input
static void koch_key_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    Serial.printf("[Koch LVGL] Key event: %lu (0x%02lX)\n", key, key);

    switch(key) {
        case LV_KEY_ESC:
            onLVGLBackNavigation();
            break;
        case LV_KEY_ENTER:
            // TODO: Start Koch practice (future feature)
            beep(TONE_SELECT, BEEP_MEDIUM);
            break;
        case 's':
        case 'S':
            // TODO: Open Koch settings (future feature)
            beep(TONE_MENU_NAV, BEEP_SHORT);
            break;
    }
}

lv_obj_t* createKochMethodScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "KOCH METHOD");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Level indicator
    lv_obj_t* level_card = lv_obj_create(screen);
    lv_obj_set_size(level_card, SCREEN_WIDTH - 40, 80);
    lv_obj_set_pos(level_card, 20, HEADER_HEIGHT + 15);
    lv_obj_set_layout(level_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(level_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(level_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(level_card, 5, 0);
    applyCardStyle(level_card);

    lv_obj_t* level_title = lv_label_create(level_card);
    lv_label_set_text(level_title, "Current Level");
    lv_obj_add_style(level_title, getStyleLabelBody(), 0);

    koch_level_label = lv_label_create(level_card);
    lv_label_set_text(koch_level_label, "Level 1");
    lv_obj_set_style_text_font(koch_level_label, getThemeFonts()->font_title, 0);
    lv_obj_set_style_text_color(koch_level_label, LV_COLOR_ACCENT_CYAN, 0);

    // Characters learned
    lv_obj_t* chars_card = lv_obj_create(screen);
    lv_obj_set_size(chars_card, SCREEN_WIDTH - 40, 60);
    lv_obj_set_pos(chars_card, 20, HEADER_HEIGHT + 105);
    lv_obj_set_layout(chars_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(chars_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(chars_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    applyCardStyle(chars_card);

    lv_obj_t* chars_title = lv_label_create(chars_card);
    lv_label_set_text(chars_title, "Characters Learned");
    lv_obj_add_style(chars_title, getStyleLabelBody(), 0);

    koch_chars_label = lv_label_create(chars_card);
    lv_label_set_text(koch_chars_label, "K M");
    lv_obj_set_style_text_font(koch_chars_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(koch_chars_label, LV_COLOR_ACCENT_GREEN, 0);

    // Progress bar
    lv_obj_t* progress_container = lv_obj_create(screen);
    lv_obj_set_size(progress_container, SCREEN_WIDTH - 40, 50);
    lv_obj_set_pos(progress_container, 20, HEADER_HEIGHT + 175);
    lv_obj_set_layout(progress_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(progress_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(progress_container, 5, 0);
    lv_obj_set_style_bg_opa(progress_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(progress_container, 0, 0);

    lv_obj_t* progress_title = lv_label_create(progress_container);
    lv_label_set_text(progress_title, "Level Progress");
    lv_obj_add_style(progress_title, getStyleLabelBody(), 0);

    koch_progress_bar = lv_bar_create(progress_container);
    lv_obj_set_size(koch_progress_bar, SCREEN_WIDTH - 60, 20);
    lv_bar_set_range(koch_progress_bar, 0, 100);
    lv_bar_set_value(koch_progress_bar, 0, LV_ANIM_OFF);
    applyBarStyle(koch_progress_bar);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "ENTER Start Practice   S Settings   ESC Back");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    // Invisible focus container for keyboard input
    // This widget receives all keyboard input and routes it through koch_key_event_cb
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);  // Minimal size (invisible but can receive focus)
    lv_obj_set_pos(focus_container, -10, -10);  // Position off-screen
    lv_obj_set_style_bg_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, 0);  // No focus outline
    lv_obj_set_style_outline_width(focus_container, 0, LV_STATE_FOCUSED);  // No focus outline when focused
    lv_obj_clear_flag(focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(focus_container, LV_OBJ_FLAG_CLICKABLE);  // Must be clickable to receive focus

    // Add keyboard event handler
    lv_obj_add_event_cb(focus_container, koch_key_event_cb, LV_EVENT_KEY, NULL);

    // Add to navigation group (enables keyboard input + ESC handling)
    addNavigableWidget(focus_container);

    // Put group in edit mode - this makes keys go to the widget instead of navigation
    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }

    // Ensure focus is on our container
    lv_group_focus_obj(focus_container);

    koch_screen = screen;
    return screen;
}

// Update Koch method display
void updateKochDisplay(int level, const char* chars, int progress_percent) {
    if (koch_level_label != NULL) {
        lv_label_set_text_fmt(koch_level_label, "Level %d", level);
    }
    if (koch_chars_label != NULL && chars != NULL) {
        lv_label_set_text(koch_chars_label, chars);
    }
    if (koch_progress_bar != NULL) {
        lv_bar_set_value(koch_progress_bar, progress_percent, LV_ANIM_ON);
    }
}

// ============================================
// CW Academy Screens
// ============================================

static lv_obj_t* cwa_screen = NULL;

lv_obj_t* createCWAcademyTrackSelectScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "CW ACADEMY");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Content - Track selection list
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, SCREEN_WIDTH - 40, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 20);
    lv_obj_set_pos(content, 20, HEADER_HEIGHT + 10);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 10, 0);
    lv_obj_set_style_pad_all(content, 10, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);

    // Track buttons
    const char* tracks[] = {"Beginner", "Intermediate", "Advanced", "Extra"};
    const char* descs[] = {
        "Learn letters A-Z at 5-10 WPM",
        "Numbers and punctuation at 10-15 WPM",
        "Build speed to 20+ WPM",
        "Expert level challenges"
    };

    for (int i = 0; i < 4; i++) {
        lv_obj_t* track_btn = lv_obj_create(content);
        lv_obj_set_size(track_btn, lv_pct(100), 50);
        lv_obj_set_layout(track_btn, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(track_btn, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(track_btn, 10, 0);
        applyCardStyle(track_btn);
        lv_obj_add_flag(track_btn, LV_OBJ_FLAG_CLICKABLE);
        addNavigableWidget(track_btn);

        lv_obj_t* track_title = lv_label_create(track_btn);
        lv_label_set_text(track_title, tracks[i]);
        lv_obj_add_style(track_title, getStyleLabelSubtitle(), 0);

        lv_obj_t* track_desc = lv_label_create(track_btn);
        lv_label_set_text(track_desc, descs[i]);
        lv_obj_add_style(track_desc, getStyleLabelBody(), 0);
    }

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "UP/DN Select   ENTER Choose Track   ESC Back");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    cwa_screen = screen;
    return screen;
}

// ============================================
// License Study Screens
// ============================================

// Forward declarations from training_license modules
extern struct LicenseStudySession licenseSession;
extern struct QuestionPool* activePool;
extern struct QuestionPool techPool;
extern struct QuestionPool genPool;
extern struct QuestionPool extraPool;
extern bool loadQuestionPool(struct QuestionPool* pool);
extern void loadLicenseProgress(int licenseType);
extern int selectNextQuestion(struct QuestionPool* pool);
extern void updateQuestionProgress(struct QuestionProgress* qp, bool correct);
extern void saveLicenseProgress(int licenseType);

/*
 * Calculate overall mastery percentage for a question pool
 * Returns percentage of questions that have been mastered (5+ correct)
 */
static int calculatePoolMastery(struct QuestionPool* pool) {
    if (!pool || !pool->progress || pool->totalQuestions == 0) return 0;

    int mastered = 0;
    for (int i = 0; i < pool->totalQuestions; i++) {
        if (pool->progress[i].correct >= 5) {
            mastered++;
        }
    }
    return (mastered * 100) / pool->totalQuestions;
}

// Static widget pointers for license screens
static lv_obj_t* license_select_screen = NULL;
static lv_obj_t* license_select_cards[4] = {NULL, NULL, NULL, NULL};  // Stats card + 3 license cards
static lv_obj_t* license_quiz_screen = NULL;
static lv_obj_t* license_question_label = NULL;
static lv_obj_t* license_answer_btns[4] = {NULL, NULL, NULL, NULL};
static lv_obj_t* license_header_label = NULL;
static lv_obj_t* license_feedback_label = NULL;
static lv_obj_t* license_stats_screen = NULL;
static unsigned long license_last_advance_time = 0;  // Debounce for answer clicks

// Stats overlay for quiz screen
static lv_obj_t* license_stats_overlay = NULL;
static lv_obj_t* license_stats_overlay_label = NULL;
static bool license_stats_overlay_visible = false;

// Combined stats screen (all licenses)
static lv_obj_t* license_all_stats_screen = NULL;
static lv_obj_t* license_stats_tab_btns[3] = {NULL, NULL, NULL};
static lv_obj_t* license_stats_content = NULL;
static int license_stats_selected_tab = 0;

// License names for display
static const char* licenseNames[] = {"Technician", "General", "Amateur Extra"};
static const char* licenseShortNames[] = {"TECH", "GEN", "EXTRA"};
static const char* licenseDescriptions[] = {
    "Entry-level license with VHF/UHF and some HF",
    "Intermediate license with more HF privileges",
    "Full privileges on all amateur bands"
};

// Forward declaration for mode selection callback
extern void onLVGLMenuSelect(int target_mode);

/*
 * Event handler for license type selection
 * Checks for files and navigates to appropriate screen
 */
static void license_type_select_handler(lv_event_t* e) {
    lv_obj_t* target = lv_event_get_target(e);
    int license_type = (int)(intptr_t)lv_obj_get_user_data(target);

    Serial.printf("[LicenseScreen] Selected license type: %d\n", license_type);

    // Store selected license
    licenseSession.selectedLicense = license_type;

    // Initialize SD card if not already done
    if (!sdCardAvailable) {
        Serial.println("[LicenseScreen] Initializing SD card...");
        initSDCard();
    }

    // Check if files exist before navigating
    if (!allQuestionFilesExist()) {
        Serial.println("[LicenseScreen] Question files missing, checking requirements...");

        // Check SD card first
        if (!sdCardAvailable) {
            Serial.println("[LicenseScreen] SD card not available");
            onLVGLMenuSelect(58);  // LVGL_MODE_LICENSE_SD_ERROR
            return;
        }

        // Check WiFi
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[LicenseScreen] WiFi not connected");
            onLVGLMenuSelect(57);  // LVGL_MODE_LICENSE_WIFI_ERROR
            return;
        }

        // Need to download - go to download screen
        Serial.println("[LicenseScreen] Navigating to download screen");
        onLVGLMenuSelect(56);  // LVGL_MODE_LICENSE_DOWNLOAD
        return;
    }

    // Files exist, navigate directly to quiz mode
    onLVGLMenuSelect(51);  // LVGL_MODE_LICENSE_QUIZ
}

/*
 * Event handler for stats button
 */
static void license_stats_btn_handler(lv_event_t* e) {
    // Navigate to stats mode (52 = LVGL_MODE_LICENSE_STATS)
    onLVGLMenuSelect(52);
}

/*
 * Event handler for View Statistics card
 */
static void license_view_stats_handler(lv_event_t* e) {
    // Navigate to combined stats screen (60 = LVGL_MODE_LICENSE_ALL_STATS)
    onLVGLMenuSelect(60);
}

/*
 * Event handler for navigating between license select cards with arrow keys
 * Now handles 4 cards: Stats + 3 license types
 */
static void license_select_nav_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    if (key != LV_KEY_UP && key != LV_KEY_DOWN &&
        key != LV_KEY_PREV && key != LV_KEY_NEXT) return;

    // Find which card is focused (4 cards now)
    lv_obj_t* focused = lv_event_get_target(e);
    int focused_idx = -1;
    for (int i = 0; i < 4; i++) {
        if (license_select_cards[i] == focused) {
            focused_idx = i;
            break;
        }
    }
    if (focused_idx < 0) return;

    // Calculate target index (4 cards: 0=stats, 1-3=licenses)
    int target_idx = -1;
    if (key == LV_KEY_DOWN || key == LV_KEY_NEXT) {
        if (focused_idx < 3) {
            target_idx = focused_idx + 1;
        }
    } else if (key == LV_KEY_UP || key == LV_KEY_PREV) {
        if (focused_idx > 0) {
            target_idx = focused_idx - 1;
        }
    }

    // Focus target card
    if (target_idx >= 0 && target_idx < 4 && license_select_cards[target_idx]) {
        lv_group_focus_obj(license_select_cards[target_idx]);
        lv_event_stop_processing(e);
    }
}

/*
 * Create License Select Screen (Mode 70)
 * Allows user to choose Technician, General, or Extra class
 * Includes View Statistics option at top
 */
lv_obj_t* createLicenseSelectScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "LICENSE STUDY");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Content area - scrollable container for license options
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, SCREEN_WIDTH - 20, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 10);
    lv_obj_set_pos(content, 10, HEADER_HEIGHT + 5);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(content, 10, 0);
    lv_obj_set_style_pad_all(content, 10, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    // Enable scrolling for when content doesn't fit
    lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);

    // Card index counter (0 = stats, 1-3 = license types)
    int cardIdx = 0;

    // View Statistics card (first card)
    {
        lv_obj_t* stats_card = lv_btn_create(content);
        lv_obj_set_size(stats_card, lv_pct(100), 55);  // Stats card height
        lv_obj_set_style_bg_color(stats_card, getThemeColors()->card_secondary, 0);  // Theme card
        lv_obj_set_style_bg_opa(stats_card, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(stats_card, LV_COLOR_ACCENT_CYAN, 0);
        lv_obj_set_style_border_width(stats_card, 2, 0);
        lv_obj_set_style_radius(stats_card, 10, 0);
        lv_obj_set_style_pad_all(stats_card, 10, 0);
        // Focused state
        lv_obj_set_style_bg_color(stats_card, LV_COLOR_ACCENT_CYAN, LV_STATE_FOCUSED);
        lv_obj_set_style_border_color(stats_card, LV_COLOR_BORDER_ACCENT, LV_STATE_FOCUSED);
        lv_obj_set_style_shadow_width(stats_card, 20, LV_STATE_FOCUSED);
        lv_obj_set_style_shadow_color(stats_card, LV_COLOR_ACCENT_CYAN, LV_STATE_FOCUSED);
        lv_obj_set_style_shadow_opa(stats_card, LV_OPA_50, LV_STATE_FOCUSED);

        lv_obj_t* stats_title = lv_label_create(stats_card);
        lv_label_set_text(stats_title, "View Statistics");
        lv_obj_set_style_text_font(stats_title, getThemeFonts()->font_input, 0);
        lv_obj_set_style_text_color(stats_title, LV_COLOR_ACCENT_CYAN, 0);
        lv_obj_set_style_text_color(stats_title, getThemeColors()->text_on_accent, LV_STATE_FOCUSED);
        lv_obj_align(stats_title, LV_ALIGN_TOP_LEFT, 0, 0);

        lv_obj_t* stats_desc = lv_label_create(stats_card);
        lv_label_set_text(stats_desc, "See progress for all license types");
        lv_obj_set_style_text_font(stats_desc, getThemeFonts()->font_small, 0);
        lv_obj_set_style_text_color(stats_desc, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_style_text_color(stats_desc, getThemeColors()->text_on_accent, LV_STATE_FOCUSED);
        lv_obj_align(stats_desc, LV_ALIGN_BOTTOM_LEFT, 0, 0);

        lv_obj_add_event_cb(stats_card, license_view_stats_handler, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(stats_card, license_select_nav_handler, LV_EVENT_KEY, NULL);

        addNavigableWidget(stats_card);
        license_select_cards[cardIdx++] = stats_card;
    }

    // License type cards (3 cards)
    for (int i = 0; i < 3; i++) {
        lv_obj_t* card = lv_btn_create(content);
        lv_obj_set_size(card, lv_pct(100), 60);  // License type card height
        lv_obj_set_style_bg_color(card, LV_COLOR_CARD_TEAL, 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(card, LV_COLOR_BORDER_SUBTLE, 0);
        lv_obj_set_style_border_width(card, 2, 0);
        lv_obj_set_style_radius(card, 10, 0);
        lv_obj_set_style_pad_all(card, 10, 0);
        // Focused state
        lv_obj_set_style_bg_color(card, LV_COLOR_ACCENT_CYAN, LV_STATE_FOCUSED);
        lv_obj_set_style_border_color(card, LV_COLOR_BORDER_ACCENT, LV_STATE_FOCUSED);
        lv_obj_set_style_shadow_width(card, 20, LV_STATE_FOCUSED);
        lv_obj_set_style_shadow_color(card, LV_COLOR_ACCENT_CYAN, LV_STATE_FOCUSED);
        lv_obj_set_style_shadow_opa(card, LV_OPA_50, LV_STATE_FOCUSED);

        lv_obj_t* card_title = lv_label_create(card);
        lv_label_set_text(card_title, licenseNames[i]);
        lv_obj_set_style_text_font(card_title, getThemeFonts()->font_input, 0);
        lv_obj_set_style_text_color(card_title, LV_COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_style_text_color(card_title, getThemeColors()->text_on_accent, LV_STATE_FOCUSED);
        lv_obj_align(card_title, LV_ALIGN_TOP_LEFT, 0, 0);

        lv_obj_t* card_desc = lv_label_create(card);
        lv_label_set_text(card_desc, licenseDescriptions[i]);
        lv_obj_set_style_text_font(card_desc, getThemeFonts()->font_small, 0);
        lv_obj_set_style_text_color(card_desc, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_style_text_color(card_desc, getThemeColors()->text_on_accent, LV_STATE_FOCUSED);
        lv_obj_align(card_desc, LV_ALIGN_BOTTOM_LEFT, 0, 0);

        // Store license type and add handlers
        lv_obj_set_user_data(card, (void*)(intptr_t)i);
        lv_obj_add_event_cb(card, license_type_select_handler, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(card, license_select_nav_handler, LV_EVENT_KEY, NULL);

        addNavigableWidget(card);
        license_select_cards[cardIdx++] = card;
    }

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "ENTER: Select   ESC: Back");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    license_select_screen = screen;
    return screen;
}

/*
 * Update license quiz display with current question
 * NOTE: This function must be defined before the event handlers that call it
 */
void updateLicenseQuizDisplay() {
    if (!activePool || !license_question_label) return;

    struct LicenseQuestion* q = &activePool->questions[licenseSession.currentQuestionIndex];

    // Update header
    if (license_header_label) {
        int mastery = calculatePoolMastery(activePool);
        char header[64];
        snprintf(header, sizeof(header), "%d%% | %s | Q %d/%d",
            mastery, licenseShortNames[licenseSession.selectedLicense],
            licenseSession.sessionTotal + 1, activePool->totalQuestions);
        lv_label_set_text(license_header_label, header);
    }

    // Update question
    lv_label_set_text(license_question_label, q->question);

    // Update answer buttons
    for (int i = 0; i < 4; i++) {
        if (license_answer_btns[i]) {
            lv_obj_t* label = lv_obj_get_child(license_answer_btns[i], 0);
            if (label) {
                char buf[120];  // Larger buffer for potential padding
                snprintf(buf, sizeof(buf), "%c. %s", 'A' + i, q->answers[i]);

                // Smart padding: only add separator if text will scroll
                lv_coord_t label_width = SCREEN_WIDTH - 50;  // Match width from createLicenseQuizScreen
                const lv_font_t* font = lv_obj_get_style_text_font(label, 0);
                lv_coord_t text_width = lv_txt_get_width(buf, strlen(buf), font, 0, LV_TEXT_FLAG_NONE);

                if (text_width > label_width) {
                    // Text will scroll - add visible separator for clarity
                    size_t len = strlen(buf);
                    if (len < sizeof(buf) - 12) {
                        strcat(buf, "   \xE2\x80\xA2   ");  // " • " (bullet) as separator
                    }
                }
                lv_label_set_text(label, buf);
            }

            // Reset button styles
            lv_obj_remove_style(license_answer_btns[i], NULL, LV_STATE_USER_1);
            lv_obj_remove_style(license_answer_btns[i], NULL, LV_STATE_USER_2);

            if (licenseSession.showingFeedback) {
                // Show correct answer in green (always highlight the correct answer)
                if (i == q->correctAnswer) {
                    lv_obj_set_style_bg_color(license_answer_btns[i], LV_COLOR_SUCCESS, 0);
                    lv_obj_set_style_bg_opa(license_answer_btns[i], LV_OPA_70, 0);  // More visible
                    lv_obj_set_style_border_color(license_answer_btns[i], LV_COLOR_SUCCESS, 0);
                    lv_obj_set_style_border_width(license_answer_btns[i], 3, 0);
                }
                // Show wrong selection in red
                else if (i == licenseSession.selectedAnswerIndex && !licenseSession.correctAnswer) {
                    lv_obj_set_style_bg_color(license_answer_btns[i], LV_COLOR_ERROR, 0);
                    lv_obj_set_style_bg_opa(license_answer_btns[i], LV_OPA_70, 0);  // More visible
                    lv_obj_set_style_border_color(license_answer_btns[i], LV_COLOR_ERROR, 0);
                    lv_obj_set_style_border_width(license_answer_btns[i], 3, 0);
                }
            } else {
                // Normal state - use card style
                lv_obj_set_style_bg_color(license_answer_btns[i], LV_COLOR_CARD_TEAL, 0);
                lv_obj_set_style_bg_opa(license_answer_btns[i], LV_OPA_COVER, 0);
            }
        }
    }

    // Update feedback label
    if (license_feedback_label) {
        if (licenseSession.showingFeedback) {
            if (licenseSession.correctAnswer) {
                lv_label_set_text(license_feedback_label, "Correct! Press any key for next question...");
                lv_obj_set_style_text_color(license_feedback_label, LV_COLOR_SUCCESS, 0);
            } else {
                lv_label_set_text(license_feedback_label, "Incorrect. The correct answer is highlighted. Press any key...");
                lv_obj_set_style_text_color(license_feedback_label, LV_COLOR_ERROR, 0);
            }
            lv_obj_clear_flag(license_feedback_label, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(license_feedback_label, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/*
 * Update the stats overlay content
 */
void updateLicenseStatsOverlay() {
    if (!license_stats_overlay_label || !activePool) return;

    int mastery = calculatePoolMastery(activePool);
    int sessionAccuracy = licenseSession.sessionTotal > 0 ?
        (licenseSession.sessionCorrect * 100) / licenseSession.sessionTotal : 0;

    // Count questions by category
    int mastered = 0, weak = 0, never_seen = 0;
    for (int i = 0; i < activePool->totalQuestions; i++) {
        if (activePool->progress[i].correct == 0 && activePool->progress[i].incorrect == 0) {
            never_seen++;
        } else if (activePool->progress[i].correct >= 5) {
            mastered++;
        } else if (activePool->progress[i].aptitude < 40) {
            weak++;
        }
    }

    char stats_text[128];
    snprintf(stats_text, sizeof(stats_text),
        "Session: %d/%d (%d%%)\n"
        "Mastery: %d%%\n"
        "Mastered: %d | Weak: %d | New: %d",
        licenseSession.sessionCorrect, licenseSession.sessionTotal, sessionAccuracy,
        mastery, mastered, weak, never_seen);

    lv_label_set_text(license_stats_overlay_label, stats_text);
}

/*
 * Toggle the stats overlay visibility
 */
void toggleLicenseStatsOverlay() {
    if (!license_stats_overlay) return;

    license_stats_overlay_visible = !license_stats_overlay_visible;

    if (license_stats_overlay_visible) {
        updateLicenseStatsOverlay();
        lv_obj_clear_flag(license_stats_overlay, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(license_stats_overlay, LV_OBJ_FLAG_HIDDEN);
    }
}

/*
 * Event handler for answer button click
 */
static void license_answer_handler(lv_event_t* e) {
    lv_obj_t* target = lv_event_get_target(e);
    int answer_idx = (int)(intptr_t)lv_obj_get_user_data(target);

    if (!activePool || licenseSession.showingFeedback) return;

    // Debounce: ignore clicks within 200ms of advancing to prevent double-selection
    if (millis() - license_last_advance_time < 200) return;

    // Get current question
    struct LicenseQuestion* q = &activePool->questions[licenseSession.currentQuestionIndex];
    bool correct = (answer_idx == q->correctAnswer);

    // Update session state
    licenseSession.showingFeedback = true;
    licenseSession.correctAnswer = correct;
    licenseSession.selectedAnswerIndex = answer_idx;
    licenseSession.sessionTotal++;
    if (correct) {
        licenseSession.sessionCorrect++;
        beep(TONE_SUCCESS, BEEP_MEDIUM);
    } else {
        beep(TONE_ERROR, BEEP_LONG);
        // Set up boost for incorrect answer
        licenseSession.lastIncorrectIndex = licenseSession.currentQuestionIndex;
        licenseSession.boostDecayQuestions = 12;
    }

    // Update progress
    updateQuestionProgress(&activePool->progress[licenseSession.currentQuestionIndex], correct);
    saveLicenseProgress(licenseSession.selectedLicense);

    // Update UI to show feedback
    updateLicenseQuizDisplay();
}

/*
 * Event handler for Tab key to toggle stats overlay
 */
static void license_tab_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    // Tab key is 0x09 ('\t')
    if (key == '\t') {
        toggleLicenseStatsOverlay();
        lv_event_stop_processing(e);
    }
}

/*
 * Event handler for navigating between answer buttons with arrow keys
 * Since LV_KEY_UP/DOWN don't auto-navigate groups, we handle them manually
 */
static void license_nav_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    // Don't navigate if showing feedback (any key should advance)
    if (licenseSession.showingFeedback) return;

    uint32_t key = lv_event_get_key(e);
    if (key != LV_KEY_UP && key != LV_KEY_DOWN &&
        key != LV_KEY_PREV && key != LV_KEY_NEXT) return;

    // Find which button is focused
    lv_obj_t* focused = lv_event_get_target(e);
    int focused_idx = -1;
    for (int i = 0; i < 4; i++) {
        if (license_answer_btns[i] == focused) {
            focused_idx = i;
            break;
        }
    }
    if (focused_idx < 0) return;

    // Calculate target index
    int target_idx = -1;
    if (key == LV_KEY_DOWN || key == LV_KEY_NEXT) {
        if (focused_idx < 3) {
            target_idx = focused_idx + 1;
        }
    } else if (key == LV_KEY_UP || key == LV_KEY_PREV) {
        if (focused_idx > 0) {
            target_idx = focused_idx - 1;
        }
    }

    // Focus target button
    if (target_idx >= 0 && target_idx < 4 && license_answer_btns[target_idx]) {
        lv_group_focus_obj(license_answer_btns[target_idx]);
        lv_event_stop_processing(e);
    }
}

/*
 * Event handler for next question (any key after feedback)
 */
static void license_next_handler(lv_event_t* e) {
    if (!licenseSession.showingFeedback) return;

    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ENTER || key == LV_KEY_NEXT || key == LV_KEY_PREV ||
        key == LV_KEY_UP || key == LV_KEY_DOWN) {
        // Move to next question
        licenseSession.showingFeedback = false;
        licenseSession.currentQuestionIndex = selectNextQuestion(activePool);

        // Decay boost counter
        if (licenseSession.boostDecayQuestions > 0) {
            licenseSession.boostDecayQuestions--;
        }

        // Set debounce timestamp to prevent immediate answer selection
        license_last_advance_time = millis();

        updateLicenseQuizDisplay();

        // Focus first answer button
        if (license_answer_btns[0]) {
            lv_group_focus_obj(license_answer_btns[0]);
        }

        lv_event_stop_processing(e);
    }
}

/*
 * Create License Quiz Screen (Mode 71)
 * Displays question and 4 answer choices
 */
lv_obj_t* createLicenseQuizScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Header with progress info
    lv_obj_t* header = lv_obj_create(screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 35);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_add_style(header, getStyleStatusBar(), 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    license_header_label = lv_label_create(header);
    lv_label_set_text(license_header_label, "0% | TECH | Q 1/423");
    lv_obj_set_style_text_font(license_header_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(license_header_label, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_center(license_header_label);

    // Question area
    lv_obj_t* question_container = lv_obj_create(screen);
    lv_obj_set_size(question_container, SCREEN_WIDTH - 20, 90);
    lv_obj_set_pos(question_container, 10, 40);
    lv_obj_set_style_bg_opa(question_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(question_container, 0, 0);
    lv_obj_set_style_pad_all(question_container, 5, 0);
    lv_obj_add_flag(question_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(question_container, LV_DIR_VER);

    license_question_label = lv_label_create(question_container);
    lv_label_set_long_mode(license_question_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(license_question_label, SCREEN_WIDTH - 40);
    lv_label_set_text(license_question_label, "Loading question...");
    lv_obj_set_style_text_font(license_question_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(license_question_label, LV_COLOR_TEXT_PRIMARY, 0);

    // Answer buttons (4 options)
    // Layout: buttons end at 130 + (4*32) + (3*2) = 264, feedback at 268, footer at 295
    int btn_y = 130;
    int btn_height = 32;
    int btn_spacing = 2;

    for (int i = 0; i < 4; i++) {
        license_answer_btns[i] = lv_btn_create(screen);
        lv_obj_set_size(license_answer_btns[i], SCREEN_WIDTH - 20, btn_height);
        lv_obj_set_pos(license_answer_btns[i], 10, btn_y + i * (btn_height + btn_spacing));
        lv_obj_add_style(license_answer_btns[i], getStyleMenuCard(), 0);
        lv_obj_add_style(license_answer_btns[i], getStyleMenuCardFocused(), LV_STATE_FOCUSED);
        lv_obj_set_style_pad_left(license_answer_btns[i], 10, 0);
        lv_obj_set_style_pad_right(license_answer_btns[i], 10, 0);

        lv_obj_t* btn_label = lv_label_create(license_answer_btns[i]);
        char prefix[16];
        snprintf(prefix, sizeof(prefix), "%c. Loading...", 'A' + i);
        lv_label_set_text(btn_label, prefix);
        lv_obj_set_style_text_font(btn_label, getThemeFonts()->font_small, 0);
        lv_label_set_long_mode(btn_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(btn_label, SCREEN_WIDTH - 50);
        lv_obj_align(btn_label, LV_ALIGN_LEFT_MID, 0, 0);

        // Store answer index and add handlers
        lv_obj_set_user_data(license_answer_btns[i], (void*)(intptr_t)i);
        lv_obj_add_event_cb(license_answer_btns[i], license_answer_handler, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(license_answer_btns[i], license_nav_handler, LV_EVENT_KEY, NULL);  // Arrow key navigation
        lv_obj_add_event_cb(license_answer_btns[i], license_next_handler, LV_EVENT_KEY, NULL);  // Advance after feedback
        lv_obj_add_event_cb(license_answer_btns[i], license_tab_handler, LV_EVENT_KEY, NULL);  // Tab for stats overlay

        addNavigableWidget(license_answer_btns[i]);
    }

    // Feedback label (hidden by default) - positioned below buttons (264) and above footer (295)
    license_feedback_label = lv_label_create(screen);
    lv_label_set_text(license_feedback_label, "");
    lv_obj_set_style_text_font(license_feedback_label, getThemeFonts()->font_small, 0);
    lv_obj_set_pos(license_feedback_label, 10, 268);  // Below buttons at 264
    lv_obj_set_width(license_feedback_label, SCREEN_WIDTH - 20);
    lv_label_set_long_mode(license_feedback_label, LV_LABEL_LONG_WRAP);
    lv_obj_add_flag(license_feedback_label, LV_OBJ_FLAG_HIDDEN);

    // Stats overlay (hidden by default) - shows on Tab key press
    license_stats_overlay = lv_obj_create(screen);
    lv_obj_set_size(license_stats_overlay, 180, 80);
    lv_obj_set_pos(license_stats_overlay, 10, 180);  // Bottom-left area
    lv_obj_set_style_bg_color(license_stats_overlay, getThemeColors()->bg_deep, 0);
    lv_obj_set_style_bg_opa(license_stats_overlay, LV_OPA_90, 0);
    lv_obj_set_style_border_color(license_stats_overlay, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_border_width(license_stats_overlay, 2, 0);
    lv_obj_set_style_radius(license_stats_overlay, 8, 0);
    lv_obj_set_style_pad_all(license_stats_overlay, 8, 0);
    lv_obj_clear_flag(license_stats_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(license_stats_overlay, LV_OBJ_FLAG_HIDDEN);

    license_stats_overlay_label = lv_label_create(license_stats_overlay);
    lv_label_set_text(license_stats_overlay_label, "Stats loading...");
    lv_obj_set_style_text_font(license_stats_overlay_label, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(license_stats_overlay_label, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(license_stats_overlay_label, LV_ALIGN_TOP_LEFT, 0, 0);

    // Reset overlay visibility state
    license_stats_overlay_visible = false;

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, 25);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - 25);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "Tab: Stats   ENTER: Submit   ESC: Exit");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    license_quiz_screen = screen;
    return screen;
}

/*
 * Create License Stats Screen (Mode 72)
 * Shows mastery progress and pool coverage
 */
lv_obj_t* createLicenseStatsScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "LICENSE STATISTICS");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Stats content area
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, SCREEN_WIDTH - 40, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 20);
    lv_obj_set_pos(content, 20, HEADER_HEIGHT + 10);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(content, 10, 0);
    lv_obj_set_style_pad_all(content, 15, 0);
    applyCardStyle(content);

    // License type label
    lv_obj_t* license_lbl = lv_label_create(content);
    lv_label_set_text_fmt(license_lbl, "License: %s", licenseNames[licenseSession.selectedLicense]);
    lv_obj_set_style_text_font(license_lbl, getThemeFonts()->font_input, 0);
    lv_obj_set_style_text_color(license_lbl, LV_COLOR_ACCENT_CYAN, 0);

    // Session stats
    lv_obj_t* session_lbl = lv_label_create(content);
    int accuracy = licenseSession.sessionTotal > 0 ?
        (licenseSession.sessionCorrect * 100) / licenseSession.sessionTotal : 0;
    lv_label_set_text_fmt(session_lbl, "Session: %d/%d correct (%d%%)",
        licenseSession.sessionCorrect, licenseSession.sessionTotal, accuracy);
    lv_obj_set_style_text_font(session_lbl, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(session_lbl, LV_COLOR_TEXT_PRIMARY, 0);

    // Overall mastery
    if (activePool) {
        int mastery = calculatePoolMastery(activePool);
        lv_obj_t* mastery_lbl = lv_label_create(content);
        lv_label_set_text_fmt(mastery_lbl, "Overall Mastery: %d%%", mastery);
        lv_obj_set_style_text_font(mastery_lbl, getThemeFonts()->font_body, 0);
        lv_obj_set_style_text_color(mastery_lbl, mastery >= 80 ? LV_COLOR_SUCCESS : LV_COLOR_WARNING, 0);

        // Question pool stats
        lv_obj_t* pool_lbl = lv_label_create(content);
        lv_label_set_text_fmt(pool_lbl, "Question Pool: %d questions", activePool->totalQuestions);
        lv_obj_set_style_text_font(pool_lbl, getThemeFonts()->font_body, 0);
        lv_obj_set_style_text_color(pool_lbl, LV_COLOR_TEXT_SECONDARY, 0);

        // Count questions by category
        int mastered = 0, improving = 0, weak = 0, never_seen = 0;
        for (int i = 0; i < activePool->totalQuestions; i++) {
            int apt = activePool->progress[i].aptitude;
            if (activePool->progress[i].correct == 0 && activePool->progress[i].incorrect == 0) {
                never_seen++;
            } else if (apt >= 100) {
                mastered++;
            } else if (apt >= 40) {
                improving++;
            } else {
                weak++;
            }
        }

        lv_obj_t* breakdown_lbl = lv_label_create(content);
        lv_label_set_text_fmt(breakdown_lbl, "Mastered: %d  Improving: %d  Weak: %d  New: %d",
            mastered, improving, weak, never_seen);
        lv_obj_set_style_text_font(breakdown_lbl, getThemeFonts()->font_small, 0);
        lv_obj_set_style_text_color(breakdown_lbl, LV_COLOR_TEXT_SECONDARY, 0);
    } else {
        lv_obj_t* no_data_lbl = lv_label_create(content);
        lv_label_set_text(no_data_lbl, "No question pool loaded");
        lv_obj_set_style_text_font(no_data_lbl, getThemeFonts()->font_body, 0);
        lv_obj_set_style_text_color(no_data_lbl, LV_COLOR_ERROR, 0);
    }

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "ESC: Back to License Select");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    license_stats_screen = screen;
    return screen;
}

// ============================================
// License Download Screen (Mode 56)
// Shows download progress for question pool files
// ============================================

// Static labels for download status updates
static lv_obj_t* license_download_screen = NULL;
static lv_obj_t* license_download_file_labels[3] = {NULL, NULL, NULL};
static lv_obj_t* license_download_status_labels[3] = {NULL, NULL, NULL};
static lv_obj_t* license_download_message_label = NULL;

/*
 * Update the status for a file download
 * fileIndex: 0=Technician, 1=General, 2=Extra
 * success: true=OK, false=FAILED
 */
void updateLicenseDownloadFileStatus(int fileIndex, bool success) {
    if (fileIndex < 0 || fileIndex >= 3) return;
    if (license_download_status_labels[fileIndex] == NULL) return;

    if (success) {
        lv_label_set_text(license_download_status_labels[fileIndex], "OK");
        lv_obj_set_style_text_color(license_download_status_labels[fileIndex], LV_COLOR_SUCCESS, 0);
    } else {
        lv_label_set_text(license_download_status_labels[fileIndex], "FAILED");
        lv_obj_set_style_text_color(license_download_status_labels[fileIndex], LV_COLOR_ERROR, 0);
    }
}

/*
 * Show download completion message
 */
void showLicenseDownloadComplete(bool allSuccess) {
    if (license_download_message_label == NULL) return;

    lv_obj_clear_flag(license_download_message_label, LV_OBJ_FLAG_HIDDEN);

    if (allSuccess) {
        lv_label_set_text(license_download_message_label, "Download Complete! Starting quiz...");
        lv_obj_set_style_text_color(license_download_message_label, LV_COLOR_SUCCESS, 0);
    } else {
        lv_label_set_text(license_download_message_label, "Some downloads failed. Press ESC to go back.");
        lv_obj_set_style_text_color(license_download_message_label, LV_COLOR_ERROR, 0);
    }
}

/*
 * Create License Download Screen
 */
lv_obj_t* createLicenseDownloadScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "LICENSE STUDY");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Content area
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, SCREEN_WIDTH - 40, SCREEN_HEIGHT - HEADER_HEIGHT - 40);
    lv_obj_set_pos(content, 20, HEADER_HEIGHT + 10);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Main status message
    lv_obj_t* status_label = lv_label_create(content);
    lv_label_set_text(status_label, "Downloading Question Files...");
    lv_obj_set_style_text_font(status_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(status_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t* subtitle = lv_label_create(content);
    lv_label_set_text(subtitle, "This will take a minute...");
    lv_obj_set_style_text_font(subtitle, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(subtitle, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 50);

    // File download status rows
    const char* fileNames[] = {"Technician", "General", "Extra"};
    int y_start = 100;
    int row_height = 30;

    for (int i = 0; i < 3; i++) {
        // File name label
        license_download_file_labels[i] = lv_label_create(content);
        lv_label_set_text_fmt(license_download_file_labels[i], "%s...", fileNames[i]);
        lv_obj_set_style_text_font(license_download_file_labels[i], getThemeFonts()->font_body, 0);
        lv_obj_set_style_text_color(license_download_file_labels[i], LV_COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_pos(license_download_file_labels[i], 60, y_start + i * row_height);

        // Status label (initially shows waiting indicator)
        license_download_status_labels[i] = lv_label_create(content);
        lv_label_set_text(license_download_status_labels[i], "...");
        lv_obj_set_style_text_font(license_download_status_labels[i], getThemeFonts()->font_body, 0);
        lv_obj_set_style_text_color(license_download_status_labels[i], LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_pos(license_download_status_labels[i], 250, y_start + i * row_height);
    }

    // Completion message (hidden initially)
    license_download_message_label = lv_label_create(content);
    lv_label_set_text(license_download_message_label, "");
    lv_obj_set_style_text_font(license_download_message_label, getThemeFonts()->font_body, 0);
    lv_obj_align(license_download_message_label, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_flag(license_download_message_label, LV_OBJ_FLAG_HIDDEN);

    // Invisible focus container for ESC handling (used after download completes/fails)
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);
    lv_obj_set_pos(focus_container, -10, -10);
    lv_obj_set_style_bg_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, 0);
    lv_obj_clear_flag(focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(focus_container, LV_OBJ_FLAG_CLICKABLE);
    addNavigableWidget(focus_container);

    license_download_screen = screen;
    return screen;
}

/*
 * Perform license question file downloads with LVGL UI updates
 * Returns true if all downloads succeeded
 */
bool performLicenseDownloadsLVGL() {
    // Create /license directory if it doesn't exist
    if (!SD.exists("/license")) {
        Serial.println("[LicenseDownload] Creating /license directory...");
        if (!SD.mkdir("/license")) {
            Serial.println("[LicenseDownload] ERROR: Failed to create directory");
            return false;
        }
    }

    bool allSuccess = true;

    // Download Technician
    if (!questionFileExists("/license/technician.json")) {
        lv_timer_handler();  // Keep UI responsive
        Serial.println("[LicenseDownload] Downloading Technician...");
        bool tech_ok = downloadFile(TECHNICIAN_URL, "/license/technician.json") == DOWNLOAD_SUCCESS;
        updateLicenseDownloadFileStatus(0, tech_ok);
        if (!tech_ok) allSuccess = false;
        lv_timer_handler();
    } else {
        updateLicenseDownloadFileStatus(0, true);
        lv_timer_handler();
    }

    // Download General
    if (!questionFileExists("/license/general.json")) {
        lv_timer_handler();
        Serial.println("[LicenseDownload] Downloading General...");
        bool gen_ok = downloadFile(GENERAL_URL, "/license/general.json") == DOWNLOAD_SUCCESS;
        updateLicenseDownloadFileStatus(1, gen_ok);
        if (!gen_ok) allSuccess = false;
        lv_timer_handler();
    } else {
        updateLicenseDownloadFileStatus(1, true);
        lv_timer_handler();
    }

    // Download Extra
    if (!questionFileExists("/license/extra.json")) {
        lv_timer_handler();
        Serial.println("[LicenseDownload] Downloading Extra...");
        bool extra_ok = downloadFile(EXTRA_URL, "/license/extra.json") == DOWNLOAD_SUCCESS;
        updateLicenseDownloadFileStatus(2, extra_ok);
        if (!extra_ok) allSuccess = false;
        lv_timer_handler();
    } else {
        updateLicenseDownloadFileStatus(2, true);
        lv_timer_handler();
    }

    // Show completion message
    showLicenseDownloadComplete(allSuccess);

    // Brief pause to show completion
    unsigned long start = millis();
    while (millis() - start < 2000) {
        lv_timer_handler();
        delay(50);
    }

    return allSuccess;
}

/*
 * Start license quiz with LVGL (no legacy UI calls)
 * Call this after ensuring files exist and LVGL screen is loaded
 */
void startLicenseQuizLVGL(int licenseType) {
    Serial.printf("[LicenseQuiz] Starting quiz for license type %d\n", licenseType);

    // Unload previous pool if different
    if (activePool && activePool->loaded && licenseSession.selectedLicense != licenseType) {
        unloadLicenseProgress(activePool);
        unloadQuestionPool(activePool);
        activePool = nullptr;
    }

    // Get question pool for selected license
    QuestionPool* pool = getQuestionPool(licenseType);
    if (!pool) {
        Serial.println("[LicenseQuiz] ERROR: Invalid license type");
        return;
    }

    // Load question pool from SD card
    if (!pool->loaded) {
        Serial.println("[LicenseQuiz] Loading question pool from SD...");
        if (!loadQuestionPool(pool)) {
            Serial.println("[LicenseQuiz] ERROR: Failed to load question pool");
            return;
        }
    }

    // Load progress from Preferences
    if (!pool->progress) {
        Serial.println("[LicenseQuiz] Loading progress from Preferences...");
        loadLicenseProgress(licenseType);
    }

    // Set active pool
    activePool = pool;

    // Start session
    startLicenseSession(licenseType);

    Serial.printf("[LicenseQuiz] Started quiz for %s (%d questions)\n",
        getLicenseName(licenseType), pool->totalQuestions);
}

// ============================================
// License WiFi Required Screen (Mode 57)
// Shown when WiFi is needed but not connected
// ============================================

/*
 * Create License WiFi Required Error Screen
 */
lv_obj_t* createLicenseWiFiRequiredScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "LICENSE STUDY");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Content area
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, SCREEN_WIDTH - 40, SCREEN_HEIGHT - HEADER_HEIGHT - 60);
    lv_obj_set_pos(content, 20, HEADER_HEIGHT + 20);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Error message
    lv_obj_t* error_label = lv_label_create(content);
    lv_label_set_text(error_label, "WiFi Required");
    lv_obj_set_style_text_font(error_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(error_label, LV_COLOR_WARNING, 0);
    lv_obj_align(error_label, LV_ALIGN_TOP_MID, 0, 30);

    // Instructions
    lv_obj_t* line1 = lv_label_create(content);
    lv_label_set_text(line1, "Question files need to be downloaded.");
    lv_obj_set_style_text_font(line1, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(line1, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(line1, LV_ALIGN_TOP_MID, 0, 80);

    lv_obj_t* line2 = lv_label_create(content);
    lv_label_set_text(line2, "Please connect to WiFi first:");
    lv_obj_set_style_text_font(line2, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(line2, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(line2, LV_ALIGN_TOP_MID, 0, 110);

    lv_obj_t* line3 = lv_label_create(content);
    lv_label_set_text(line3, "Settings > WiFi Setup");
    lv_obj_set_style_text_font(line3, getThemeFonts()->font_input, 0);
    lv_obj_set_style_text_color(line3, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(line3, LV_ALIGN_TOP_MID, 0, 145);

    // Create invisible focus container for ESC handling
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);
    lv_obj_set_pos(focus_container, -10, -10);
    lv_obj_set_style_bg_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, 0);
    lv_obj_clear_flag(focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(focus_container, LV_OBJ_FLAG_CLICKABLE);
    addNavigableWidget(focus_container);

    return screen;
}

// ============================================
// License SD Card Error Screen (Mode 58)
// Shown when SD card is not available
// ============================================

/*
 * Create License SD Card Error Screen
 */
lv_obj_t* createLicenseSDCardErrorScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "LICENSE STUDY");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Content area
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, SCREEN_WIDTH - 40, SCREEN_HEIGHT - HEADER_HEIGHT - 60);
    lv_obj_set_pos(content, 20, HEADER_HEIGHT + 20);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Error message
    lv_obj_t* error_label = lv_label_create(content);
    lv_label_set_text(error_label, "SD Card Error");
    lv_obj_set_style_text_font(error_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(error_label, LV_COLOR_ERROR, 0);
    lv_obj_align(error_label, LV_ALIGN_TOP_MID, 0, 30);

    // Instructions
    lv_obj_t* line1 = lv_label_create(content);
    lv_label_set_text(line1, "Cannot access SD card for question files.");
    lv_obj_set_style_text_font(line1, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(line1, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(line1, LV_ALIGN_TOP_MID, 0, 80);

    lv_obj_t* line2 = lv_label_create(content);
    lv_label_set_text(line2, "Please check that an SD card is inserted");
    lv_obj_set_style_text_font(line2, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(line2, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(line2, LV_ALIGN_TOP_MID, 0, 110);

    lv_obj_t* line3 = lv_label_create(content);
    lv_label_set_text(line3, "and formatted as FAT32.");
    lv_obj_set_style_text_font(line3, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(line3, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(line3, LV_ALIGN_TOP_MID, 0, 135);

    // Create invisible focus container for ESC handling
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);
    lv_obj_set_pos(focus_container, -10, -10);
    lv_obj_set_style_bg_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, 0);
    lv_obj_clear_flag(focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(focus_container, LV_OBJ_FLAG_CLICKABLE);
    addNavigableWidget(focus_container);

    return screen;
}

// ============================================
// License All Stats Screen (Mode 60)
// Shows stats for all three license types with tabs
// ============================================

// Cached stats for all 3 license types
static LicenseStatsWithSession license_cached_stats[3];

/*
 * Update the stats content area for the selected tab
 */
void updateLicenseAllStatsContent() {
    if (!license_stats_content) return;

    // Clear existing content
    lv_obj_clean(license_stats_content);

    int tab = license_stats_selected_tab;
    LicenseStatsWithSession* stats = &license_cached_stats[tab];

    if (!stats->hasData) {
        // No data for this license type
        lv_obj_t* no_data = lv_label_create(license_stats_content);
        lv_label_set_text(no_data, "No study data yet");
        lv_obj_set_style_text_font(no_data, getThemeFonts()->font_input, 0);
        lv_obj_set_style_text_color(no_data, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_align(no_data, LV_ALIGN_CENTER, 0, -20);

        lv_obj_t* hint = lv_label_create(license_stats_content);
        lv_label_set_text(hint, "Start a quiz to track progress");
        lv_obj_set_style_text_font(hint, getThemeFonts()->font_small, 0);
        lv_obj_set_style_text_color(hint, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_align(hint, LV_ALIGN_CENTER, 0, 10);
        return;
    }

    // License name header
    lv_obj_t* license_title = lv_label_create(license_stats_content);
    lv_label_set_text(license_title, licenseNames[tab]);
    lv_obj_set_style_text_font(license_title, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(license_title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(license_title, LV_ALIGN_TOP_LEFT, 10, 5);

    // Pool info
    lv_obj_t* pool_info = lv_label_create(license_stats_content);
    lv_label_set_text_fmt(pool_info, "Question Pool: %d questions", stats->stats.totalQuestions);
    lv_obj_set_style_text_font(pool_info, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(pool_info, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(pool_info, LV_ALIGN_TOP_LEFT, 10, 30);

    // Progress bar for mastery
    int masteryPct = (stats->stats.totalQuestions > 0) ?
        (stats->stats.questionsMastered * 100) / stats->stats.totalQuestions : 0;

    lv_obj_t* bar = lv_bar_create(license_stats_content);
    lv_obj_set_size(bar, 280, 20);
    lv_obj_align(bar, LV_ALIGN_TOP_LEFT, 10, 55);
    lv_bar_set_value(bar, masteryPct, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, getThemeColors()->bg_layer2, 0);
    lv_obj_set_style_bg_color(bar, masteryPct >= 70 ? LV_COLOR_SUCCESS : LV_COLOR_ACCENT_CYAN, LV_PART_INDICATOR);

    lv_obj_t* mastery_lbl = lv_label_create(license_stats_content);
    lv_label_set_text_fmt(mastery_lbl, "Pool Mastery: %d%%", masteryPct);
    lv_obj_set_style_text_font(mastery_lbl, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(mastery_lbl, masteryPct >= 70 ? LV_COLOR_SUCCESS : LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(mastery_lbl, LV_ALIGN_TOP_LEFT, 300, 55);

    // Coverage
    lv_obj_t* coverage_lbl = lv_label_create(license_stats_content);
    lv_label_set_text_fmt(coverage_lbl, "Questions Attempted: %d (%.0f%%)",
        stats->stats.questionsAttempted, stats->stats.poolCoverage);
    lv_obj_set_style_text_font(coverage_lbl, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(coverage_lbl, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(coverage_lbl, LV_ALIGN_TOP_LEFT, 10, 85);

    // Breakdown
    lv_obj_t* breakdown_lbl = lv_label_create(license_stats_content);
    lv_label_set_text_fmt(breakdown_lbl, "Mastered: %d   Improving: %d   Weak: %d   New: %d",
        stats->stats.questionsMastered, stats->stats.questionsImproving,
        stats->stats.questionsWeak, stats->stats.questionsNeverSeen);
    lv_obj_set_style_text_font(breakdown_lbl, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(breakdown_lbl, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(breakdown_lbl, LV_ALIGN_TOP_LEFT, 10, 115);

    // Session stats
    if (stats->sessionTotal > 0) {
        int sessionAcc = (stats->sessionCorrect * 100) / stats->sessionTotal;
        lv_obj_t* session_lbl = lv_label_create(license_stats_content);
        lv_label_set_text_fmt(session_lbl, "Session: %d/%d correct (%d%%)",
            stats->sessionCorrect, stats->sessionTotal, sessionAcc);
        lv_obj_set_style_text_font(session_lbl, getThemeFonts()->font_body, 0);
        lv_obj_set_style_text_color(session_lbl, LV_COLOR_TEXT_PRIMARY, 0);
        lv_obj_align(session_lbl, LV_ALIGN_TOP_LEFT, 10, 145);
    }
}

/*
 * Update tab button styling based on selection
 */
void updateLicenseTabStyles() {
    for (int i = 0; i < 3; i++) {
        if (!license_stats_tab_btns[i]) continue;

        if (i == license_stats_selected_tab) {
            // Selected tab
            lv_obj_set_style_bg_color(license_stats_tab_btns[i], LV_COLOR_ACCENT_CYAN, 0);
            lv_obj_set_style_text_color(lv_obj_get_child(license_stats_tab_btns[i], 0),
                getThemeColors()->text_on_accent, 0);
        } else {
            // Unselected tab
            lv_obj_set_style_bg_color(license_stats_tab_btns[i], getThemeColors()->bg_layer2, 0);
            lv_obj_set_style_text_color(lv_obj_get_child(license_stats_tab_btns[i], 0),
                LV_COLOR_TEXT_SECONDARY, 0);
        }
    }
}

/*
 * Tab button click handler
 */
static void license_all_stats_tab_handler(lv_event_t* e) {
    lv_obj_t* target = lv_event_get_target(e);
    int tab = (int)(intptr_t)lv_obj_get_user_data(target);

    if (tab >= 0 && tab < 3) {
        license_stats_selected_tab = tab;
        updateLicenseTabStyles();
        updateLicenseAllStatsContent();
    }
}

/*
 * Key handler for tab navigation (Left/Right arrows, Tab key)
 */
static void license_all_stats_key_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // Tab or Right arrow = next tab
    if (key == '\t' || key == LV_KEY_RIGHT) {
        license_stats_selected_tab = (license_stats_selected_tab + 1) % 3;
        updateLicenseTabStyles();
        updateLicenseAllStatsContent();
        lv_event_stop_processing(e);
    }
    // Left arrow = previous tab
    else if (key == LV_KEY_LEFT) {
        license_stats_selected_tab = (license_stats_selected_tab + 2) % 3;  // +2 is same as -1 mod 3
        updateLicenseTabStyles();
        updateLicenseAllStatsContent();
        lv_event_stop_processing(e);
    }
}

/*
 * Create License All Stats Screen (Mode 60)
 * Shows stats for all three license types with tabbed interface
 */
lv_obj_t* createLicenseAllStatsScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Load stats for all 3 license types
    license_cached_stats[0] = loadStatsOnly(0);  // Technician
    license_cached_stats[1] = loadStatsOnly(1);  // General
    license_cached_stats[2] = loadStatsOnly(2);  // Extra
    license_stats_selected_tab = 0;

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "LICENSE STATISTICS");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    createCompactStatusBar(screen);

    // Tab bar
    lv_obj_t* tab_bar = lv_obj_create(screen);
    lv_obj_set_size(tab_bar, SCREEN_WIDTH - 20, 35);
    lv_obj_set_pos(tab_bar, 10, HEADER_HEIGHT + 5);
    lv_obj_set_style_bg_opa(tab_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tab_bar, 0, 0);
    lv_obj_set_layout(tab_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tab_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tab_bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(tab_bar, 0, 0);
    lv_obj_clear_flag(tab_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Create tab buttons
    const char* tabLabels[] = {"TECH", "GENERAL", "EXTRA"};
    for (int i = 0; i < 3; i++) {
        lv_obj_t* tab_btn = lv_btn_create(tab_bar);
        lv_obj_set_size(tab_btn, 120, 30);
        lv_obj_set_style_radius(tab_btn, 5, 0);
        lv_obj_set_style_border_width(tab_btn, 1, 0);
        lv_obj_set_style_border_color(tab_btn, LV_COLOR_ACCENT_CYAN, 0);

        lv_obj_t* tab_label = lv_label_create(tab_btn);
        lv_label_set_text(tab_label, tabLabels[i]);
        lv_obj_set_style_text_font(tab_label, getThemeFonts()->font_body, 0);
        lv_obj_center(tab_label);

        lv_obj_set_user_data(tab_btn, (void*)(intptr_t)i);
        lv_obj_add_event_cb(tab_btn, license_all_stats_tab_handler, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(tab_btn, license_all_stats_key_handler, LV_EVENT_KEY, NULL);

        addNavigableWidget(tab_btn);
        license_stats_tab_btns[i] = tab_btn;
    }

    // Apply initial tab styling
    updateLicenseTabStyles();

    // Content area
    license_stats_content = lv_obj_create(screen);
    lv_obj_set_size(license_stats_content, SCREEN_WIDTH - 20, SCREEN_HEIGHT - HEADER_HEIGHT - 80);
    lv_obj_set_pos(license_stats_content, 10, HEADER_HEIGHT + 45);
    applyCardStyle(license_stats_content);
    lv_obj_clear_flag(license_stats_content, LV_OBJ_FLAG_SCROLLABLE);

    // Populate initial content
    updateLicenseAllStatsContent();

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "Tab/Arrows: Switch   ESC: Back");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    license_all_stats_screen = screen;
    return screen;
}

// ============================================
// Screen Selector
// Mode values MUST match MenuMode enum in menu_ui.h
// ============================================

lv_obj_t* createTrainingScreenForMode(int mode) {
    switch (mode) {
        case 6:  // MODE_PRACTICE
            return createPracticeScreen();
        case 2:  // MODE_HEAR_IT_MENU
        case 3:  // MODE_HEAR_IT_TYPE_IT
            return createHearItTypeItScreen();
        case 7:  // MODE_KOCH_METHOD
            return createKochMethodScreen();
        case 8:  // MODE_CW_ACADEMY_TRACK_SELECT
            return createCWAcademyTrackSelectScreen();
        case 50: // MODE_LICENSE_SELECT
            return createLicenseSelectScreen();
        case 51: // MODE_LICENSE_QUIZ
            return createLicenseQuizScreen();
        case 52: // MODE_LICENSE_STATS
            return createLicenseStatsScreen();
        case 56: // MODE_LICENSE_DOWNLOAD
            return createLicenseDownloadScreen();
        case 57: // MODE_LICENSE_WIFI_ERROR
            return createLicenseWiFiRequiredScreen();
        case 58: // MODE_LICENSE_SD_ERROR
            return createLicenseSDCardErrorScreen();
        case 60: // MODE_LICENSE_ALL_STATS
            return createLicenseAllStatsScreen();
        default:
            Serial.printf("[TrainingScreens] Unknown training mode: %d\n", mode);
            return NULL;
    }
}

#endif // LV_TRAINING_SCREENS_H
