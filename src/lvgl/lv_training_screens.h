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
#include "lv_vail_master_screens.h"
#include "lv_licw_screens.h"

// External state from training modules
extern int cwSpeed;
extern int cwTone;
extern String decodedText;

// CW Academy external state (from training_cwa_core.h)
// Enums are defined in training_cwa_core.h - just use extern for variables
extern CWATrack cwaSelectedTrack;
extern int cwaSelectedSession;
extern void saveCWAProgress();
extern void loadCWAProgress();
extern const char* cwaTrackNames[];
extern const char* cwaTrackDescriptions[];
extern const struct CWASession cwaSessionData[];

// CW Academy practice types (from training_cwa_core.h)
extern CWAPracticeType cwaSelectedPracticeType;
extern const char* cwaPracticeTypeNames[];
extern const char* cwaPracticeTypeDescriptions[];

// CW Academy message types (from training_cwa_core.h)
extern CWAMessageType cwaSelectedMessageType;
extern const char* cwaMessageTypeNames[];
extern const char* cwaMessageTypeDescriptions[];

// User callsign (from network/vail_repeater.h)
extern String vailCallsign;

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
extern String getCWASessionChars(int session);
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
static lv_obj_t* hear_it_chars_row = NULL;    // Characters row for Custom mode
static lv_obj_t* hear_it_chars_value = NULL;  // Characters count/preset display
static lv_obj_t* hear_it_start_btn = NULL;

// Mode names for display
static const char* hear_it_mode_names[] = {"Callsigns", "Letters", "Numbers", "Mixed", "Custom"};
static const int hear_it_mode_count = 5;

// Timers that need to be cancelled when leaving the screen
static lv_timer_t* hear_it_pending_timer = NULL;
static lv_timer_t* hear_it_start_timer = NULL;

// Forward declarations for timer callbacks
static void hear_it_skip_timer_cb(lv_timer_t* timer);

// Track which settings widget is currently focused (0=mode, 1=speed, 2=length, 3=chars, 4=button)
// Note: chars row (focus=3) is only visible in Custom mode
static int hear_it_settings_focus = 0;

// Preset picker modal widgets and state
static lv_obj_t* hear_it_preset_modal = NULL;
static lv_obj_t* hear_it_preset_list = NULL;
static int hear_it_preset_selection = 0;      // 0=Koch, 1=CWA, 2=All, 3=Cancel
static bool hear_it_in_submenu = false;       // True when in Koch/CWA lesson picker
static int hear_it_submenu_selection = 0;     // Selection within submenu
static const int PRESET_MENU_ITEMS = 5;       // Main menu items count (Koch, CWA, All, Select, Cancel)

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

// Update footer based on current state and focus position
void updateHearItFooter() {
    if (hear_it_footer_help == NULL) return;

    if (currentHearItState == HEAR_IT_STATE_SETTINGS) {
        // Context-aware footer based on focus position
        if (hear_it_settings_focus == 3 && tempSettings.mode == MODE_CUSTOM_CHARS) {
            // Characters row focused
            lv_label_set_text(hear_it_footer_help, "UP/DN Navigate   L/R Edit Characters   ESC Back");
        } else if (hear_it_settings_focus == 4) {
            // Start button focused
            lv_label_set_text(hear_it_footer_help, "UP/DN Navigate   ENTER Start Training   ESC Back");
        } else {
            // Mode, Speed, or Length row focused
            lv_label_set_text(hear_it_footer_help, "UP/DN Navigate   L/R Adjust   ESC Back");
        }
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
    hear_it_chars_row = NULL;
    hear_it_chars_value = NULL;
    hear_it_speed_row = NULL;
    hear_it_speed_slider = NULL;
    hear_it_speed_value = NULL;
    hear_it_start_btn = NULL;
    hear_it_focus_container = NULL;
    hear_it_preset_modal = NULL;
    hear_it_preset_list = NULL;
    hear_it_in_submenu = false;
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

// Update characters row display - shows char count or preset name
static void hear_it_update_chars_display() {
    if (hear_it_chars_value == NULL) return;

    int count = tempSettings.customChars.length();
    if (tempSettings.presetType == PRESET_KOCH) {
        lv_label_set_text_fmt(hear_it_chars_value, "< Koch L%d (%d) >",
                              tempSettings.presetLesson, count);
    } else if (tempSettings.presetType == PRESET_CWA) {
        lv_label_set_text_fmt(hear_it_chars_value, "< CWA S%d (%d) >",
                              tempSettings.presetLesson, count);
    } else {
        lv_label_set_text_fmt(hear_it_chars_value, "< %d chars >", count);
    }
}

// Show/hide characters row based on mode
static void hear_it_update_chars_visibility() {
    if (hear_it_chars_row == NULL) return;

    if (tempSettings.mode == MODE_CUSTOM_CHARS) {
        lv_obj_clear_flag(hear_it_chars_row, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(hear_it_chars_row, LV_OBJ_FLAG_HIDDEN);
    }
}

// ============================================
// PRESET PICKER MODAL
// ============================================

// Forward declarations for preset picker
static void closeHearItPresetPicker();
static void updatePresetPickerDisplay();

// Menu item labels
static const char* preset_main_menu[] = {"Koch Method (1-44)", "CW Academy (1-10)", "All Characters (36)", "Select Characters...", "Cancel"};
static const int KOCH_TOTAL = 44;
static const int CWA_TOTAL = 10;

// Character grid state
static bool hear_it_in_char_grid = false;
static bool hear_it_char_selected[36];  // A-Z (26) + 0-9 (10)
static int hear_it_grid_cursor = 0;

// External reference to Koch sequence
extern const char KOCH_SEQUENCE[];

// Get Koch characters for a lesson (returns count)
static String getKochCharsForLesson(int lesson) {
    String chars = "";
    for (int i = 0; i < lesson && i < KOCH_TOTAL; i++) {
        char ch = KOCH_SEQUENCE[i];
        // Only include alphanumeric (skip space, comma, ?, /)
        if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) {
            chars += ch;
        }
    }
    return chars;
}

// Apply Koch preset and update tempSettings
static void applyKochPresetToTemp(int lesson) {
    tempSettings.customChars = getKochCharsForLesson(lesson);
    tempSettings.presetType = PRESET_KOCH;
    tempSettings.presetLesson = lesson;
}

// Apply CWA preset and update tempSettings
static void applyCWAPresetToTemp(int session) {
    tempSettings.customChars = getCWASessionChars(session);
    tempSettings.presetType = PRESET_CWA;
    tempSettings.presetLesson = session;
}

// Apply all characters preset
static void applyAllCharsPreset() {
    tempSettings.customChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    tempSettings.presetType = PRESET_NONE;
    tempSettings.presetLesson = 0;
}

// Key handler for preset picker modal
static void hear_it_preset_key_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // Handle ESC - close modal or go back from submenu/grid
    if (key == LV_KEY_ESC) {
        if (hear_it_in_char_grid) {
            // Apply character selections and close grid
            tempSettings.customChars = "";
            for (int i = 0; i < 36; i++) {
                if (hear_it_char_selected[i]) {
                    char ch = (i < 26) ? ('A' + i) : ('0' + i - 26);
                    tempSettings.customChars += ch;
                }
            }
            // Ensure at least one character is selected
            if (tempSettings.customChars.length() == 0) {
                tempSettings.customChars = "A";
                hear_it_char_selected[0] = true;
            }
            tempSettings.presetType = PRESET_NONE;  // Manual selection = no preset
            hear_it_in_char_grid = false;
            closeHearItPresetPicker();
            hear_it_update_chars_display();
        } else if (hear_it_in_submenu) {
            // Go back to main menu
            hear_it_in_submenu = false;
            hear_it_preset_selection = 0;
            updatePresetPickerDisplay();
        } else {
            // Close modal without applying
            closeHearItPresetPicker();
        }
        lv_event_stop_processing(e);
        return;
    }

    // Handle character grid navigation (6x6 grid)
    if (hear_it_in_char_grid) {
        if (key == LV_KEY_UP) {
            if (hear_it_grid_cursor >= 6) {
                hear_it_grid_cursor -= 6;
                updatePresetPickerDisplay();
            }
        } else if (key == LV_KEY_DOWN) {
            if (hear_it_grid_cursor < 30) {
                hear_it_grid_cursor += 6;
                updatePresetPickerDisplay();
            }
        } else if (key == LV_KEY_LEFT) {
            if (hear_it_grid_cursor > 0) {
                hear_it_grid_cursor--;
                updatePresetPickerDisplay();
            }
        } else if (key == LV_KEY_RIGHT) {
            if (hear_it_grid_cursor < 35) {
                hear_it_grid_cursor++;
                updatePresetPickerDisplay();
            }
        }
        // Note: ENTER handled below
        if (key != LV_KEY_ENTER) return;
    }

    // Get max items based on current menu (not used for char grid)
    int maxItems;
    if (hear_it_in_submenu) {
        maxItems = (hear_it_preset_selection == 0) ? KOCH_TOTAL + 1 : CWA_TOTAL + 1;  // +1 for Back
    } else {
        maxItems = PRESET_MENU_ITEMS;
    }

    // Handle UP/DOWN navigation (main menu and submenus)
    if (key == LV_KEY_UP) {
        if (hear_it_in_submenu) {
            if (hear_it_submenu_selection > 0) {
                hear_it_submenu_selection--;
                updatePresetPickerDisplay();
            }
        } else {
            if (hear_it_preset_selection > 0) {
                hear_it_preset_selection--;
                updatePresetPickerDisplay();
            }
        }
        return;
    }
    if (key == LV_KEY_DOWN) {
        if (hear_it_in_submenu) {
            if (hear_it_submenu_selection < maxItems - 1) {
                hear_it_submenu_selection++;
                updatePresetPickerDisplay();
            }
        } else {
            if (hear_it_preset_selection < maxItems - 1) {
                hear_it_preset_selection++;
                updatePresetPickerDisplay();
            }
        }
        return;
    }

    // Handle ENTER - select item
    if (key == LV_KEY_ENTER) {
        if (hear_it_in_submenu) {
            int submenuMax = (hear_it_preset_selection == 0) ? KOCH_TOTAL : CWA_TOTAL;
            if (hear_it_submenu_selection == submenuMax) {
                // "Back" selected - return to main menu
                hear_it_in_submenu = false;
                updatePresetPickerDisplay();
            } else {
                // Apply preset
                if (hear_it_preset_selection == 0) {
                    // Koch lesson (1-based)
                    applyKochPresetToTemp(hear_it_submenu_selection + 1);
                } else {
                    // CWA session (1-based)
                    applyCWAPresetToTemp(hear_it_submenu_selection + 1);
                }
                closeHearItPresetPicker();
                hear_it_update_chars_display();
            }
        } else if (hear_it_in_char_grid) {
            // Toggle character selection
            hear_it_char_selected[hear_it_grid_cursor] = !hear_it_char_selected[hear_it_grid_cursor];
            updatePresetPickerDisplay();
        } else {
            switch (hear_it_preset_selection) {
                case 0:  // Koch Method
                case 1:  // CW Academy
                    hear_it_in_submenu = true;
                    hear_it_submenu_selection = 0;
                    updatePresetPickerDisplay();
                    break;
                case 2:  // All Characters
                    applyAllCharsPreset();
                    closeHearItPresetPicker();
                    hear_it_update_chars_display();
                    break;
                case 3:  // Select Characters...
                    // Initialize grid from current tempSettings
                    for (int i = 0; i < 36; i++) {
                        char ch = (i < 26) ? ('A' + i) : ('0' + i - 26);
                        hear_it_char_selected[i] = (tempSettings.customChars.indexOf(ch) >= 0);
                    }
                    hear_it_grid_cursor = 0;
                    hear_it_in_char_grid = true;
                    updatePresetPickerDisplay();
                    break;
                case 4:  // Cancel
                    closeHearItPresetPicker();
                    break;
            }
        }
        return;
    }
}

// Update the preset picker display
static void updatePresetPickerDisplay() {
    if (hear_it_preset_list == NULL) return;

    // Clear existing children
    lv_obj_clean(hear_it_preset_list);

    if (hear_it_in_char_grid) {
        // Show character grid (6x6 layout)
        // Count selected characters
        int selectedCount = 0;
        for (int i = 0; i < 36; i++) {
            if (hear_it_char_selected[i]) selectedCount++;
        }

        // Title showing count
        lv_obj_t* title = lv_label_create(hear_it_preset_list);
        lv_label_set_text_fmt(title, "Selected: %d   (ENTER toggle, ESC done)", selectedCount);
        lv_obj_set_style_text_color(title, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_width(title, lv_pct(100));
        lv_obj_set_style_pad_bottom(title, 8, 0);

        // Create 6 rows of 6 characters each
        for (int row = 0; row < 6; row++) {
            lv_obj_t* row_obj = lv_obj_create(hear_it_preset_list);
            lv_obj_set_size(row_obj, lv_pct(100), LV_SIZE_CONTENT);
            lv_obj_set_layout(row_obj, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(row_obj, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(row_obj, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_bg_opa(row_obj, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(row_obj, 0, 0);
            lv_obj_set_style_pad_all(row_obj, 2, 0);

            for (int col = 0; col < 6; col++) {
                int idx = row * 6 + col;
                char ch = (idx < 26) ? ('A' + idx) : ('0' + idx - 26);

                lv_obj_t* cell = lv_label_create(row_obj);
                char cell_text[4];
                snprintf(cell_text, sizeof(cell_text), "%c", ch);
                lv_label_set_text(cell, cell_text);

                bool isSelected = hear_it_char_selected[idx];
                bool isCursor = (idx == hear_it_grid_cursor);

                // Style: cursor highlight and selection checkbox visual
                if (isCursor) {
                    lv_obj_set_style_bg_color(cell, LV_COLOR_ACCENT_CYAN, 0);
                    lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
                    lv_obj_set_style_text_color(cell, LV_COLOR_BG_DEEP, 0);
                } else if (isSelected) {
                    lv_obj_set_style_bg_color(cell, LV_COLOR_CARD_TEAL, 0);
                    lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
                    lv_obj_set_style_text_color(cell, LV_COLOR_ACCENT_CYAN, 0);
                } else {
                    lv_obj_set_style_bg_opa(cell, LV_OPA_TRANSP, 0);
                    lv_obj_set_style_text_color(cell, LV_COLOR_TEXT_SECONDARY, 0);
                }

                lv_obj_set_style_pad_all(cell, 6, 0);
                lv_obj_set_style_radius(cell, 4, 0);
                lv_obj_set_style_text_font(cell, getThemeFonts()->font_subtitle, 0);
            }
        }

        // Scroll to make cursor row visible
        // Get the row container for the cursor and scroll to it
        int cursorRow = hear_it_grid_cursor / 6;
        // Child 0 is the title, then rows 1-6 are the grid rows
        uint32_t childIndex = cursorRow + 1;  // +1 for title
        if (childIndex < lv_obj_get_child_cnt(hear_it_preset_list)) {
            lv_obj_t* rowObj = lv_obj_get_child(hear_it_preset_list, childIndex);
            if (rowObj) {
                lv_obj_scroll_to_view(rowObj, LV_ANIM_OFF);
            }
        }
        return;
    }

    if (hear_it_in_submenu) {
        // Show submenu (Koch lessons or CWA sessions)
        bool isKoch = (hear_it_preset_selection == 0);
        int count = isKoch ? KOCH_TOTAL : CWA_TOTAL;

        for (int i = 0; i <= count; i++) {  // +1 for Back
            lv_obj_t* item = lv_label_create(hear_it_preset_list);

            if (i == count) {
                // Back option
                lv_label_set_text(item, LV_SYMBOL_LEFT " Back");
            } else {
                // Lesson/Session option
                if (isKoch) {
                    String chars = getKochCharsForLesson(i + 1);
                    String label = String(i + 1) + ": " + chars.substring(0, 8);
                    if (chars.length() > 8) label += "...";
                    label += " (" + String(chars.length()) + ")";
                    lv_label_set_text(item, label.c_str());
                } else {
                    String chars = getCWASessionChars(i + 1);
                    String label = "S" + String(i + 1) + ": " + chars.substring(0, 10);
                    if (chars.length() > 10) label += "...";
                    label += " (" + String(chars.length()) + ")";
                    lv_label_set_text(item, label.c_str());
                }
            }

            // Style based on selection
            bool selected = (i == hear_it_submenu_selection);
            lv_obj_set_style_text_color(item, selected ? LV_COLOR_ACCENT_CYAN : LV_COLOR_TEXT_PRIMARY, 0);
            lv_obj_set_style_bg_color(item, LV_COLOR_CARD_TEAL, 0);
            lv_obj_set_style_bg_opa(item, selected ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
            lv_obj_set_style_pad_all(item, 6, 0);
            lv_obj_set_width(item, lv_pct(100));
        }
    } else {
        // Show main menu
        for (int i = 0; i < PRESET_MENU_ITEMS; i++) {
            lv_obj_t* item = lv_label_create(hear_it_preset_list);
            lv_label_set_text(item, preset_main_menu[i]);

            // Style based on selection
            bool selected = (i == hear_it_preset_selection);
            lv_obj_set_style_text_color(item, selected ? LV_COLOR_ACCENT_CYAN : LV_COLOR_TEXT_PRIMARY, 0);
            lv_obj_set_style_bg_color(item, LV_COLOR_CARD_TEAL, 0);
            lv_obj_set_style_bg_opa(item, selected ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
            lv_obj_set_style_pad_all(item, 8, 0);
            lv_obj_set_width(item, lv_pct(100));
        }
    }

    // Scroll to make selection visible
    lv_obj_scroll_to_y(hear_it_preset_list, hear_it_submenu_selection * 30, LV_ANIM_OFF);
}

// Show the preset picker modal
void showHearItPresetPicker() {
    // Reset selection state
    hear_it_preset_selection = 0;
    hear_it_in_submenu = false;
    hear_it_submenu_selection = 0;

    // Create modal overlay (covers entire screen)
    hear_it_preset_modal = lv_obj_create(lv_scr_act());
    lv_obj_set_size(hear_it_preset_modal, SCREEN_WIDTH - 60, SCREEN_HEIGHT - 80);
    lv_obj_center(hear_it_preset_modal);
    lv_obj_set_style_bg_color(hear_it_preset_modal, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_bg_opa(hear_it_preset_modal, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(hear_it_preset_modal, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_border_width(hear_it_preset_modal, 2, 0);
    lv_obj_set_style_radius(hear_it_preset_modal, 12, 0);
    lv_obj_set_style_pad_all(hear_it_preset_modal, 12, 0);
    lv_obj_clear_flag(hear_it_preset_modal, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* title = lv_label_create(hear_it_preset_modal);
    lv_label_set_text(title, "SELECT CHARACTER SET");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_set_style_text_color(title, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // List container (scrollable)
    hear_it_preset_list = lv_obj_create(hear_it_preset_modal);
    lv_obj_set_size(hear_it_preset_list, lv_pct(100), SCREEN_HEIGHT - 140);
    lv_obj_align(hear_it_preset_list, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_layout(hear_it_preset_list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hear_it_preset_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(hear_it_preset_list, 2, 0);
    lv_obj_set_style_bg_opa(hear_it_preset_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hear_it_preset_list, 0, 0);
    lv_obj_set_style_pad_all(hear_it_preset_list, 4, 0);

    // Populate initial menu
    updatePresetPickerDisplay();

    // Add key event handler
    lv_obj_add_event_cb(hear_it_preset_modal, hear_it_preset_key_handler, LV_EVENT_KEY, NULL);

    // Add modal to navigation group and focus it
    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_add_obj(group, hear_it_preset_modal);
        lv_group_focus_obj(hear_it_preset_modal);
        lv_group_set_editing(group, true);
    }

    // Make modal clickable to receive focus
    lv_obj_add_flag(hear_it_preset_modal, LV_OBJ_FLAG_CLICKABLE);
}

// Close the preset picker modal
static void closeHearItPresetPicker() {
    if (hear_it_preset_modal != NULL) {
        // Remove from nav group first
        lv_group_t* group = getLVGLInputGroup();
        if (group != NULL) {
            lv_group_remove_obj(hear_it_preset_modal);
        }

        // Clear list pointer before deletion (it's a child of modal)
        hear_it_preset_list = NULL;

        // Reset modal state before deletion
        hear_it_in_submenu = false;
        hear_it_in_char_grid = false;

        // Store modal pointer and clear static before deletion
        lv_obj_t* modalToDelete = hear_it_preset_modal;
        hear_it_preset_modal = NULL;

        // Re-focus the settings focus container BEFORE deleting modal
        // This ensures focus is moved away from the object being deleted
        if (hear_it_focus_container != NULL && group != NULL) {
            lv_group_focus_obj(hear_it_focus_container);
            lv_group_set_editing(group, true);
        }

        // Use async delete to avoid crash when called from within event handler
        lv_obj_del_async(modalToDelete);
    }
}

// ============================================
// END PRESET PICKER MODAL
// ============================================

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

    // Characters row styling (focus == 3) - only visible in Custom mode
    if (hear_it_chars_row) {
        if (hear_it_settings_focus == 3) {
            lv_obj_set_style_bg_color(hear_it_chars_row, LV_COLOR_CARD_TEAL, 0);
            lv_obj_set_style_bg_opa(hear_it_chars_row, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(hear_it_chars_row, LV_COLOR_ACCENT_CYAN, 0);
            lv_obj_set_style_border_width(hear_it_chars_row, 2, 0);
        } else {
            lv_obj_set_style_bg_opa(hear_it_chars_row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(hear_it_chars_row, 0, 0);
        }
    }
    if (hear_it_chars_value) {
        lv_obj_set_style_text_color(hear_it_chars_value,
            hear_it_settings_focus == 3 ? LV_COLOR_ACCENT_CYAN : LV_COLOR_TEXT_SECONDARY, 0);
    }

    // Button styling (focus == 4)
    if (hear_it_start_btn) {
        if (hear_it_settings_focus == 4) {
            lv_obj_add_state(hear_it_start_btn, LV_STATE_FOCUSED);
        } else {
            lv_obj_clear_state(hear_it_start_btn, LV_STATE_FOCUSED);
        }
    }

    // Update footer text based on current focus position
    updateHearItFooter();
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
        // Save current settings before exiting
        hearItSettings = tempSettings;
        saveHearItSettings();
        onLVGLBackNavigation();
        lv_event_stop_processing(e);  // Prevent global ESC handler from also firing
        return;
    }

    // Handle UP/DOWN for navigation between settings
    // Focus positions: 0=mode, 1=speed, 2=length, 3=chars (Custom only), 4=button
    if (key == LV_KEY_UP) {
        if (hear_it_settings_focus > 0) {
            hear_it_settings_focus--;
            // Skip chars row if not in Custom mode
            if (hear_it_settings_focus == 3 && tempSettings.mode != MODE_CUSTOM_CHARS) {
                hear_it_settings_focus--;
            }
            hear_it_update_focus();
        }
        return;
    }
    if (key == LV_KEY_DOWN) {
        if (hear_it_settings_focus < 4) {
            hear_it_settings_focus++;
            // Skip chars row if not in Custom mode
            if (hear_it_settings_focus == 3 && tempSettings.mode != MODE_CUSTOM_CHARS) {
                hear_it_settings_focus++;
            }
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
            hear_it_update_chars_visibility();  // Show/hide chars row based on mode
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
        else if (hear_it_settings_focus == 3 && tempSettings.mode == MODE_CUSTOM_CHARS) {
            // Chars row - LEFT/RIGHT opens preset picker (matches arrow visual)
            showHearItPresetPicker();
        }
        // Button (focus == 4) doesn't respond to LEFT/RIGHT
        return;
    }

    // Handle ENTER
    if (key == LV_KEY_ENTER) {
        if (hear_it_settings_focus == 4 && hear_it_start_btn) {
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
    // Load saved settings BEFORE copying to temp (fixes mode display bug)
    loadHearItSettings();

    // Copy current settings to temp for editing
    tempSettings = hearItSettings;
    Serial.printf("[HearIt] Screen create: hearItSettings.mode=%d, tempSettings.mode=%d\n",
                  (int)hearItSettings.mode, (int)tempSettings.mode);

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

    // Characters row (only visible in Custom mode) - simple row like Mode
    hear_it_chars_row = lv_obj_create(hear_it_settings_container);
    lv_obj_set_size(hear_it_chars_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(hear_it_chars_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hear_it_chars_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hear_it_chars_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(hear_it_chars_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hear_it_chars_row, 0, 0);
    lv_obj_set_style_pad_all(hear_it_chars_row, 4, 0);
    lv_obj_set_style_radius(hear_it_chars_row, 6, 0);
    lv_obj_clear_flag(hear_it_chars_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* chars_label = lv_label_create(hear_it_chars_row);
    lv_label_set_text(chars_label, "Characters");
    lv_obj_add_style(chars_label, getStyleLabelSubtitle(), 0);

    hear_it_chars_value = lv_label_create(hear_it_chars_row);
    lv_obj_set_style_text_color(hear_it_chars_value, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(hear_it_chars_value, getThemeFonts()->font_subtitle, 0);
    hear_it_update_chars_display();  // Set initial display text

    // Hide chars row if not in Custom mode initially
    hear_it_update_chars_visibility();

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
// Koch Method Screens
// ============================================

// Forward declarations for Koch core functions
extern struct KochProgress kochProgress;
extern int getKochSessionAccuracy();
extern int getKochCurrentStreak();
extern int getKochBestStreak();
extern int getKochMilestones();
extern int getKochLevelProgress();
extern String getKochCharacterSet();
extern String getKochCharactersSpaced();
extern String getCurrentKochGroup();
extern String getKochUserInput();
extern void appendKochUserInput(char c);
extern void deleteKochUserInput();
extern void clearKochUserInput();
extern void startNewKochGroupLVGL();
extern void playKochGroupLVGL();
extern bool submitKochAnswerLVGL();
extern void continueAfterFeedback();
extern bool isKochWaitingForInput();
extern bool isKochShowingFeedback();
extern bool wasKochAnswerCorrect();
extern String getKochFeedbackMessage();
extern bool canAdvanceLesson();
extern void advanceLesson();
extern void regressLesson();
extern void saveKochProgress();

// Mode constants (defined in lv_mode_integration.h)
#define KOCH_MODE_PRACTICE_INT 67
#define KOCH_MODE_SETTINGS_INT 68
#define KOCH_MODE_STATISTICS_INT 69
#define KOCH_MODE_HELP_INT 75
#define KOCH_MODE_CHAR_REF_INT 76
#define KOCH_MODE_NEW_CHAR_INT 77

// Koch Main Screen widgets
static lv_obj_t* koch_screen = NULL;
static lv_obj_t* koch_level_label = NULL;
static lv_obj_t* koch_chars_label = NULL;
static lv_obj_t* koch_progress_bar = NULL;
static lv_obj_t* koch_accuracy_label = NULL;
static lv_obj_t* koch_attempts_label = NULL;
static lv_obj_t* koch_streak_label = NULL;

// Koch Practice Screen widgets
static lv_obj_t* koch_practice_screen = NULL;
static lv_obj_t* koch_practice_correct_label = NULL;
static lv_obj_t* koch_practice_attempts_label = NULL;
static lv_obj_t* koch_practice_streak_label = NULL;
static lv_obj_t* koch_practice_prompt_label = NULL;
static lv_obj_t* koch_practice_input_label = NULL;
static lv_obj_t* koch_practice_feedback_label = NULL;
static lv_obj_t* koch_practice_progress_bar = NULL;

// Koch Settings Screen widgets
static lv_obj_t* koch_settings_screen = NULL;
static lv_obj_t* koch_settings_wpm_card = NULL;
static lv_obj_t* koch_settings_wpm_label = NULL;
static lv_obj_t* koch_settings_wpm_bar = NULL;
static lv_obj_t* koch_settings_length_card = NULL;
static lv_obj_t* koch_settings_length_label = NULL;
static lv_obj_t* koch_settings_length_bar = NULL;
static lv_obj_t* koch_settings_level_card = NULL;
static lv_obj_t* koch_settings_level_label = NULL;
static lv_obj_t* koch_settings_level_chars = NULL;
static int koch_settings_selection = 0;  // 0=WPM, 1=Length, 2=Level

// Update visual focus indicator for settings cards
static void updateKochSettingsFocus() {
    // Reset all cards to unfocused state
    if (koch_settings_wpm_card != NULL) {
        lv_obj_set_style_border_color(koch_settings_wpm_card, getThemeColors()->border_subtle, 0);
        lv_obj_set_style_border_width(koch_settings_wpm_card, 1, 0);
        lv_obj_set_style_border_opa(koch_settings_wpm_card, LV_OPA_50, 0);
    }
    if (koch_settings_length_card != NULL) {
        lv_obj_set_style_border_color(koch_settings_length_card, getThemeColors()->border_subtle, 0);
        lv_obj_set_style_border_width(koch_settings_length_card, 1, 0);
        lv_obj_set_style_border_opa(koch_settings_length_card, LV_OPA_50, 0);
    }
    if (koch_settings_level_card != NULL) {
        lv_obj_set_style_border_color(koch_settings_level_card, getThemeColors()->border_subtle, 0);
        lv_obj_set_style_border_width(koch_settings_level_card, 1, 0);
        lv_obj_set_style_border_opa(koch_settings_level_card, LV_OPA_50, 0);
    }

    // Highlight selected card with cyan border
    lv_obj_t* selected_card = NULL;
    switch (koch_settings_selection) {
        case 0: selected_card = koch_settings_wpm_card; break;
        case 1: selected_card = koch_settings_length_card; break;
        case 2: selected_card = koch_settings_level_card; break;
    }
    if (selected_card != NULL) {
        lv_obj_set_style_border_color(selected_card, LV_COLOR_ACCENT_CYAN, 0);
        lv_obj_set_style_border_width(selected_card, 2, 0);
        lv_obj_set_style_border_opa(selected_card, LV_OPA_COVER, 0);
    }
}

// Koch Statistics Screen widgets
static lv_obj_t* koch_stats_screen = NULL;

// Key event callback for Koch Main Screen
static void koch_main_key_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    Serial.printf("[Koch Main] Key: %lu (0x%02lX)\n", key, key);

    bool handled = true;
    switch(key) {
        case LV_KEY_ESC:
            onLVGLBackNavigation();
            break;
        case LV_KEY_ENTER:
            // Navigate to practice screen
            beep(TONE_SELECT, BEEP_MEDIUM);
            onLVGLMenuSelect(KOCH_MODE_PRACTICE_INT);
            break;
        case 's':
        case 'S':
            // Navigate to settings screen
            beep(TONE_MENU_NAV, BEEP_SHORT);
            onLVGLMenuSelect(KOCH_MODE_SETTINGS_INT);
            break;
        case 'p':
        case 'P':
            // Navigate to statistics screen
            beep(TONE_MENU_NAV, BEEP_SHORT);
            onLVGLMenuSelect(KOCH_MODE_STATISTICS_INT);
            break;
        case '.':
            // Navigate to help screen
            beep(TONE_MENU_NAV, BEEP_SHORT);
            onLVGLMenuSelect(KOCH_MODE_HELP_INT);
            break;
        case 'r':
        case 'R':
            // Navigate to character reference screen
            beep(TONE_MENU_NAV, BEEP_SHORT);
            onLVGLMenuSelect(KOCH_MODE_CHAR_REF_INT);
            break;
        default:
            handled = false;
            break;
    }

    // Stop event propagation to prevent double-handling
    if (handled) {
        lv_event_stop_bubbling(e);
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

    // Status bar
    createCompactStatusBar(screen);

    // Level card (top, full width) - needs room for level row + characters row
    // Card has 15px padding, so 80px total gives 50px content area
    lv_obj_t* level_card = lv_obj_create(screen);
    lv_obj_set_size(level_card, SCREEN_WIDTH - 40, 75);
    lv_obj_set_pos(level_card, 20, HEADER_HEIGHT + 8);
    applyCardStyle(level_card);
    lv_obj_set_style_pad_all(level_card, 10, 0);  // Reduce padding to fit content
    lv_obj_clear_flag(level_card, LV_OBJ_FLAG_SCROLLABLE);

    // Level info row - level label on left, mode indicator on right
    koch_level_label = lv_label_create(level_card);
    lv_label_set_text_fmt(koch_level_label, "Level %d of 40", kochProgress.currentLesson);
    lv_obj_set_style_text_font(koch_level_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(koch_level_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(koch_level_label, LV_ALIGN_TOP_LEFT, 0, 0);


    // Characters label - shows current level characters
    lv_obj_t* chars_title = lv_label_create(level_card);
    lv_label_set_text(chars_title, "Characters:");
    lv_obj_set_style_text_font(chars_title, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(chars_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(chars_title, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    String chars = getKochCharactersSpaced();
    koch_chars_label = lv_label_create(level_card);
    lv_label_set_text(koch_chars_label, chars.c_str());
    lv_obj_set_style_text_font(koch_chars_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(koch_chars_label, LV_COLOR_ACCENT_GREEN, 0);
    lv_obj_align_to(koch_chars_label, chars_title, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_label_set_long_mode(koch_chars_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(koch_chars_label, SCREEN_WIDTH - 150);

    // Stats row (3 small cards) - reduced padding for better fit
    const int card_width = (SCREEN_WIDTH - 60) / 3;
    const int card_height = 60;
    const int stats_y = HEADER_HEIGHT + 93;  // After level card (75px) + gaps

    // Accuracy card
    lv_obj_t* acc_card = lv_obj_create(screen);
    lv_obj_set_size(acc_card, card_width, card_height);
    lv_obj_set_pos(acc_card, 20, stats_y);
    applyCardStyle(acc_card);
    lv_obj_set_style_pad_all(acc_card, 5, 0);  // Reduce padding
    lv_obj_clear_flag(acc_card, LV_OBJ_FLAG_SCROLLABLE);

    int accuracy = getKochSessionAccuracy();
    koch_accuracy_label = lv_label_create(acc_card);
    lv_label_set_text_fmt(koch_accuracy_label, "%d%%", accuracy);
    lv_obj_set_style_text_font(koch_accuracy_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(koch_accuracy_label,
        accuracy >= 90 ? LV_COLOR_SUCCESS : (accuracy >= 70 ? LV_COLOR_WARNING : LV_COLOR_TEXT_PRIMARY), 0);
    lv_obj_align(koch_accuracy_label, LV_ALIGN_TOP_MID, 0, 2);

    lv_obj_t* acc_title = lv_label_create(acc_card);
    lv_label_set_text(acc_title, "Accuracy");
    lv_obj_set_style_text_font(acc_title, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(acc_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(acc_title, LV_ALIGN_BOTTOM_MID, 0, -2);

    // Attempts card
    lv_obj_t* att_card = lv_obj_create(screen);
    lv_obj_set_size(att_card, card_width, card_height);
    lv_obj_set_pos(att_card, 20 + card_width + 10, stats_y);
    applyCardStyle(att_card);
    lv_obj_set_style_pad_all(att_card, 5, 0);  // Reduce padding
    lv_obj_clear_flag(att_card, LV_OBJ_FLAG_SCROLLABLE);

    koch_attempts_label = lv_label_create(att_card);
    lv_label_set_text_fmt(koch_attempts_label, "%d/%d",
        kochProgress.sessionCorrect, kochProgress.sessionTotal);
    lv_obj_set_style_text_font(koch_attempts_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(koch_attempts_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(koch_attempts_label, LV_ALIGN_TOP_MID, 0, 2);

    lv_obj_t* att_title = lv_label_create(att_card);
    lv_label_set_text(att_title, "Attempts");
    lv_obj_set_style_text_font(att_title, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(att_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(att_title, LV_ALIGN_BOTTOM_MID, 0, -2);

    // Streak card
    lv_obj_t* streak_card = lv_obj_create(screen);
    lv_obj_set_size(streak_card, card_width, card_height);
    lv_obj_set_pos(streak_card, 20 + (card_width + 10) * 2, stats_y);
    applyCardStyle(streak_card);
    lv_obj_set_style_pad_all(streak_card, 5, 0);  // Reduce padding
    lv_obj_clear_flag(streak_card, LV_OBJ_FLAG_SCROLLABLE);

    koch_streak_label = lv_label_create(streak_card);
    lv_label_set_text_fmt(koch_streak_label, "%d", getKochCurrentStreak());
    lv_obj_set_style_text_font(koch_streak_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(koch_streak_label, LV_COLOR_WARNING, 0);
    lv_obj_align(koch_streak_label, LV_ALIGN_TOP_MID, 0, 2);

    lv_obj_t* streak_title = lv_label_create(streak_card);
    lv_label_set_text(streak_title, "Streak");
    lv_obj_set_style_text_font(streak_title, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(streak_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(streak_title, LV_ALIGN_BOTTOM_MID, 0, -2);

    // Progress section
    const int progress_y = stats_y + card_height + 15;
    lv_obj_t* progress_title = lv_label_create(screen);
    lv_label_set_text(progress_title, "Progress to Next Level (need 90% with 10+ attempts)");
    lv_obj_set_style_text_font(progress_title, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(progress_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_pos(progress_title, 20, progress_y);

    koch_progress_bar = lv_bar_create(screen);
    lv_obj_set_size(koch_progress_bar, SCREEN_WIDTH - 40, 18);
    lv_obj_set_pos(koch_progress_bar, 20, progress_y + 18);
    lv_bar_set_range(koch_progress_bar, 0, 100);
    int progress = getKochLevelProgress();
    lv_bar_set_value(koch_progress_bar, progress, LV_ANIM_OFF);
    applyBarStyle(koch_progress_bar);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "ENTER Practice  S Settings  R Chars  . Help");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    // Invisible focus container for keyboard input
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);
    lv_obj_set_pos(focus_container, -10, -10);
    lv_obj_set_style_bg_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, LV_STATE_FOCUSED);
    lv_obj_clear_flag(focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(focus_container, koch_main_key_event_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus_container);

    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }
    lv_group_focus_obj(focus_container);

    koch_screen = screen;
    return screen;
}

// Update Koch main screen display
void updateKochDisplay(int level, const char* chars, int progress_percent) {
    if (koch_level_label != NULL) {
        lv_label_set_text_fmt(koch_level_label, "Level %d of 40", level);
    }
    if (koch_chars_label != NULL && chars != NULL) {
        lv_label_set_text(koch_chars_label, chars);
    }
    if (koch_progress_bar != NULL) {
        lv_bar_set_value(koch_progress_bar, progress_percent, LV_ANIM_ON);
    }
    if (koch_accuracy_label != NULL) {
        int accuracy = getKochSessionAccuracy();
        lv_label_set_text_fmt(koch_accuracy_label, "%d%%", accuracy);
        lv_obj_set_style_text_color(koch_accuracy_label,
            accuracy >= 90 ? LV_COLOR_SUCCESS : (accuracy >= 70 ? LV_COLOR_WARNING : LV_COLOR_TEXT_PRIMARY), 0);
    }
    if (koch_attempts_label != NULL) {
        lv_label_set_text_fmt(koch_attempts_label, "%d/%d",
            kochProgress.sessionCorrect, kochProgress.sessionTotal);
    }
    if (koch_streak_label != NULL) {
        lv_label_set_text_fmt(koch_streak_label, "%d", getKochCurrentStreak());
    }
}

// ============================================
// Koch Practice Screen
// ============================================

static void koch_practice_key_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    Serial.printf("[Koch Practice] Key: %lu (0x%02lX)\n", key, key);

    bool handled = true;

    // If showing feedback, SPACE continues
    if (isKochShowingFeedback()) {
        if (key == ' ') {
            continueAfterFeedback();
            startNewKochGroupLVGL();
            // Update prompt
            if (koch_practice_prompt_label != NULL) {
                lv_label_set_text(koch_practice_prompt_label, "Press SPACE to hear morse");
                lv_obj_set_style_text_color(koch_practice_prompt_label, LV_COLOR_TEXT_SECONDARY, 0);
            }
            if (koch_practice_input_label != NULL) {
                lv_label_set_text(koch_practice_input_label, "_");
            }
            if (koch_practice_feedback_label != NULL) {
                lv_label_set_text(koch_practice_feedback_label, "");
            }
            beep(TONE_MENU_NAV, BEEP_SHORT);
        } else if (key == LV_KEY_ESC) {
            saveKochProgress();
            onLVGLBackNavigation();
        } else {
            handled = false;
        }
        if (handled) lv_event_stop_bubbling(e);
        return;
    }

    switch(key) {
        case LV_KEY_ESC:
            saveKochProgress();
            onLVGLBackNavigation();
            break;

        case ' ':
            // Play morse if not waiting for input, or replay if waiting
            if (!isKochWaitingForInput()) {
                // Generate new group if needed
                if (getCurrentKochGroup().length() == 0) {
                    startNewKochGroupLVGL();
                }
                playKochGroupLVGL();
                if (koch_practice_prompt_label != NULL) {
                    lv_label_set_text(koch_practice_prompt_label, "Type what you heard:");
                    lv_obj_set_style_text_color(koch_practice_prompt_label, LV_COLOR_ACCENT_CYAN, 0);
                }
            } else {
                // Replay
                playKochGroupLVGL();
            }
            break;

        case LV_KEY_ENTER:
            // Submit answer
            if (isKochWaitingForInput() && getKochUserInput().length() > 0) {
                bool correct = submitKochAnswerLVGL();

                // Update feedback display
                if (koch_practice_feedback_label != NULL) {
                    if (correct) {
                        lv_label_set_text(koch_practice_feedback_label, "CORRECT!");
                        lv_obj_set_style_text_color(koch_practice_feedback_label, LV_COLOR_SUCCESS, 0);
                    } else {
                        String fb = "Wrong: " + getCurrentKochGroup();
                        lv_label_set_text(koch_practice_feedback_label, fb.c_str());
                        lv_obj_set_style_text_color(koch_practice_feedback_label, LV_COLOR_ERROR, 0);
                    }
                }

                // Update stats
                if (koch_practice_correct_label != NULL) {
                    lv_label_set_text_fmt(koch_practice_correct_label, "%d", kochProgress.sessionCorrect);
                }
                if (koch_practice_attempts_label != NULL) {
                    lv_label_set_text_fmt(koch_practice_attempts_label, "%d", kochProgress.sessionTotal);
                }
                if (koch_practice_streak_label != NULL) {
                    lv_label_set_text_fmt(koch_practice_streak_label, "%d", getKochCurrentStreak());
                }
                if (koch_practice_progress_bar != NULL) {
                    lv_bar_set_value(koch_practice_progress_bar, getKochLevelProgress(), LV_ANIM_ON);
                }

                // Check if ready to advance to next level
                if (canAdvanceLesson()) {
                    // Advance and show new character introduction
                    advanceLesson();
                    saveKochProgress();
                    onLVGLMenuSelect(KOCH_MODE_NEW_CHAR_INT);
                } else {
                    // Update prompt for next attempt
                    if (koch_practice_prompt_label != NULL) {
                        lv_label_set_text(koch_practice_prompt_label, "Press SPACE to continue");
                        lv_obj_set_style_text_color(koch_practice_prompt_label, LV_COLOR_TEXT_SECONDARY, 0);
                    }
                }
            }
            break;

        case LV_KEY_BACKSPACE:
        case 127:  // DEL
            if (isKochWaitingForInput()) {
                deleteKochUserInput();
                String display = getKochUserInput();
                if (display.length() == 0) display = "_";
                else display += "_";
                if (koch_practice_input_label != NULL) {
                    lv_label_set_text(koch_practice_input_label, display.c_str());
                }
                beep(TONE_MENU_NAV, BEEP_SHORT);
            }
            break;

        case '.':
            // Navigate to help screen
            saveKochProgress();
            beep(TONE_MENU_NAV, BEEP_SHORT);
            onLVGLMenuSelect(KOCH_MODE_HELP_INT);
            break;

        default:
            // Handle letter/number input
            if (isKochWaitingForInput()) {
                char c = (char)key;
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == ',' || c == '?' || c == '/') {
                    appendKochUserInput(c);
                    String display = getKochUserInput() + "_";
                    if (koch_practice_input_label != NULL) {
                        lv_label_set_text(koch_practice_input_label, display.c_str());
                    }
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                }
            } else {
                handled = false;
            }
            break;
    }

    // Stop event propagation to prevent double-handling
    if (handled) {
        lv_event_stop_bubbling(e);
    }
}

lv_obj_t* createKochPracticeScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text_fmt(title, "KOCH PRACTICE - Level %d", kochProgress.currentLesson);
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    createCompactStatusBar(screen);

    // Stats bar - horizontal layout for session stats
    lv_obj_t* stats_bar = lv_obj_create(screen);
    lv_obj_set_size(stats_bar, SCREEN_WIDTH - 40, 32);
    lv_obj_set_pos(stats_bar, 20, HEADER_HEIGHT + 5);
    applyCardStyle(stats_bar);
    lv_obj_set_style_pad_all(stats_bar, 5, 0);  // Smaller padding
    lv_obj_set_style_pad_left(stats_bar, 10, 0);
    lv_obj_clear_flag(stats_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Stats inline: "Correct: X  |  Attempts: Y  |  Streak: Z"
    lv_obj_t* correct_title = lv_label_create(stats_bar);
    lv_label_set_text(correct_title, "Correct:");
    lv_obj_set_style_text_font(correct_title, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(correct_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(correct_title, LV_ALIGN_LEFT_MID, 0, 0);

    koch_practice_correct_label = lv_label_create(stats_bar);
    lv_label_set_text_fmt(koch_practice_correct_label, "%d", kochProgress.sessionCorrect);
    lv_obj_set_style_text_font(koch_practice_correct_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(koch_practice_correct_label, LV_COLOR_SUCCESS, 0);
    lv_obj_align_to(koch_practice_correct_label, correct_title, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    lv_obj_t* attempts_title = lv_label_create(stats_bar);
    lv_label_set_text(attempts_title, "Attempts:");
    lv_obj_set_style_text_font(attempts_title, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(attempts_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(attempts_title, LV_ALIGN_CENTER, -60, 0);

    koch_practice_attempts_label = lv_label_create(stats_bar);
    lv_label_set_text_fmt(koch_practice_attempts_label, "%d", kochProgress.sessionTotal);
    lv_obj_set_style_text_font(koch_practice_attempts_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(koch_practice_attempts_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align_to(koch_practice_attempts_label, attempts_title, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    lv_obj_t* streak_title = lv_label_create(stats_bar);
    lv_label_set_text(streak_title, "Streak:");
    lv_obj_set_style_text_font(streak_title, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(streak_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(streak_title, LV_ALIGN_RIGHT_MID, -60, 0);

    koch_practice_streak_label = lv_label_create(stats_bar);
    lv_label_set_text_fmt(koch_practice_streak_label, "%d", getKochCurrentStreak());
    lv_obj_set_style_text_font(koch_practice_streak_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(koch_practice_streak_label, LV_COLOR_WARNING, 0);
    lv_obj_align_to(koch_practice_streak_label, streak_title, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    // Prompt area - instructions for user
    koch_practice_prompt_label = lv_label_create(screen);
    lv_label_set_text(koch_practice_prompt_label, "Press SPACE to hear morse");
    lv_obj_set_style_text_font(koch_practice_prompt_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(koch_practice_prompt_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_pos(koch_practice_prompt_label, 20, HEADER_HEIGHT + 48);

    // Input card - shows user's typed input
    lv_obj_t* input_card = lv_obj_create(screen);
    lv_obj_set_size(input_card, SCREEN_WIDTH - 40, 55);
    lv_obj_set_pos(input_card, 20, HEADER_HEIGHT + 75);
    applyCardStyle(input_card);
    lv_obj_clear_flag(input_card, LV_OBJ_FLAG_SCROLLABLE);

    koch_practice_input_label = lv_label_create(input_card);
    lv_label_set_text(koch_practice_input_label, "_");
    lv_obj_set_style_text_font(koch_practice_input_label, getThemeFonts()->font_title, 0);
    lv_obj_set_style_text_color(koch_practice_input_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_center(koch_practice_input_label);

    // Feedback label - shows CORRECT!/Wrong
    koch_practice_feedback_label = lv_label_create(screen);
    lv_label_set_text(koch_practice_feedback_label, "");
    lv_obj_set_style_text_font(koch_practice_feedback_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(koch_practice_feedback_label, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_pos(koch_practice_feedback_label, 20, HEADER_HEIGHT + 140);
    lv_obj_set_width(koch_practice_feedback_label, SCREEN_WIDTH - 40);
    lv_obj_set_style_text_align(koch_practice_feedback_label, LV_TEXT_ALIGN_CENTER, 0);

    // Progress bar - shows level advancement progress
    lv_obj_t* progress_title = lv_label_create(screen);
    lv_label_set_text(progress_title, "Level Progress");
    lv_obj_set_style_text_font(progress_title, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(progress_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_pos(progress_title, 20, HEADER_HEIGHT + 175);

    koch_practice_progress_bar = lv_bar_create(screen);
    lv_obj_set_size(koch_practice_progress_bar, SCREEN_WIDTH - 40, 15);
    lv_obj_set_pos(koch_practice_progress_bar, 20, HEADER_HEIGHT + 193);
    lv_bar_set_range(koch_practice_progress_bar, 0, 100);
    lv_bar_set_value(koch_practice_progress_bar, getKochLevelProgress(), LV_ANIM_OFF);
    applyBarStyle(koch_practice_progress_bar);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "SPACE Play  ENTER Submit  . Help  ESC Back");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    // Focus container
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);
    lv_obj_set_pos(focus_container, -10, -10);
    lv_obj_set_style_bg_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, LV_STATE_FOCUSED);
    lv_obj_clear_flag(focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(focus_container, koch_practice_key_event_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus_container);

    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }
    lv_group_focus_obj(focus_container);

    koch_practice_screen = screen;
    return screen;
}

// ============================================
// Koch Settings Screen
// ============================================

static void updateKochSettingsDisplay() {
    if (koch_settings_wpm_label != NULL) {
        lv_label_set_text_fmt(koch_settings_wpm_label, "%d WPM", kochProgress.wpm);
    }
    if (koch_settings_wpm_bar != NULL) {
        // WPM range 15-30, map to 0-100
        int wpm_pct = ((kochProgress.wpm - 15) * 100) / 15;
        lv_bar_set_value(koch_settings_wpm_bar, wpm_pct, LV_ANIM_ON);
    }
    if (koch_settings_length_label != NULL) {
        lv_label_set_text_fmt(koch_settings_length_label, "%d chars", kochProgress.groupLength);
    }
    if (koch_settings_length_bar != NULL) {
        // Length range 3-10, map to 0-100
        int len_pct = ((kochProgress.groupLength - 3) * 100) / 7;
        lv_bar_set_value(koch_settings_length_bar, len_pct, LV_ANIM_ON);
    }
    if (koch_settings_level_label != NULL) {
        lv_label_set_text_fmt(koch_settings_level_label, "Level %d", kochProgress.currentLesson);
    }
    if (koch_settings_level_chars != NULL) {
        String chars = getKochCharactersSpaced();
        lv_label_set_text(koch_settings_level_chars, chars.c_str());
    }
}

static void koch_settings_key_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    Serial.printf("[Koch Settings] Key: %lu, Selection: %d\n", key, koch_settings_selection);

    bool handled = true;
    switch(key) {
        case LV_KEY_ESC:
            saveKochProgress();
            onLVGLBackNavigation();
            break;

        case LV_KEY_UP:
        case LV_KEY_PREV:
            if (koch_settings_selection > 0) {
                koch_settings_selection--;
                updateKochSettingsFocus();
                beep(TONE_MENU_NAV, BEEP_SHORT);
            }
            break;

        case LV_KEY_DOWN:
        case LV_KEY_NEXT:
            if (koch_settings_selection < 2) {
                koch_settings_selection++;
                updateKochSettingsFocus();
                beep(TONE_MENU_NAV, BEEP_SHORT);
            }
            break;

        case LV_KEY_LEFT:
            switch(koch_settings_selection) {
                case 0:  // WPM
                    if (kochProgress.wpm > 15) {
                        kochProgress.wpm--;
                        beep(TONE_MENU_NAV, BEEP_SHORT);
                    }
                    break;
                case 1:  // Length
                    if (kochProgress.groupLength > 3) {
                        kochProgress.groupLength--;
                        beep(TONE_MENU_NAV, BEEP_SHORT);
                    }
                    break;
                case 2:  // Level
                    regressLesson();
                    break;
            }
            updateKochSettingsDisplay();
            break;

        case LV_KEY_RIGHT:
            switch(koch_settings_selection) {
                case 0:  // WPM
                    if (kochProgress.wpm < 30) {
                        kochProgress.wpm++;
                        beep(TONE_MENU_NAV, BEEP_SHORT);
                    }
                    break;
                case 1:  // Length
                    if (kochProgress.groupLength < 10) {
                        kochProgress.groupLength++;
                        beep(TONE_MENU_NAV, BEEP_SHORT);
                    }
                    break;
                case 2:  // Level
                    advanceLesson();
                    break;
            }
            updateKochSettingsDisplay();
            break;

        default:
            handled = false;
            break;
    }

    // Stop event propagation to prevent double-handling
    if (handled) {
        lv_event_stop_bubbling(e);
    }
}

lv_obj_t* createKochSettingsScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    koch_settings_selection = 0;

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "KOCH SETTINGS");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    createCompactStatusBar(screen);

    const int card_y_start = HEADER_HEIGHT + 10;
    const int card_height = 58;
    const int card_gap = 8;

    // WPM Card
    koch_settings_wpm_card = lv_obj_create(screen);
    lv_obj_set_size(koch_settings_wpm_card, SCREEN_WIDTH - 40, card_height);
    lv_obj_set_pos(koch_settings_wpm_card, 20, card_y_start);
    applyCardStyle(koch_settings_wpm_card);
    lv_obj_set_style_pad_all(koch_settings_wpm_card, 8, 0);
    lv_obj_clear_flag(koch_settings_wpm_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* wpm_title = lv_label_create(koch_settings_wpm_card);
    lv_label_set_text(wpm_title, "Speed (WPM)");
    lv_obj_set_style_text_font(wpm_title, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(wpm_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(wpm_title, LV_ALIGN_TOP_LEFT, 0, 0);

    koch_settings_wpm_label = lv_label_create(koch_settings_wpm_card);
    lv_label_set_text_fmt(koch_settings_wpm_label, "%d WPM", kochProgress.wpm);
    lv_obj_set_style_text_font(koch_settings_wpm_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(koch_settings_wpm_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(koch_settings_wpm_label, LV_ALIGN_TOP_RIGHT, 0, 0);

    koch_settings_wpm_bar = lv_bar_create(koch_settings_wpm_card);
    lv_obj_set_size(koch_settings_wpm_bar, SCREEN_WIDTH - 76, 12);
    lv_obj_align(koch_settings_wpm_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_bar_set_range(koch_settings_wpm_bar, 0, 100);
    applyBarStyle(koch_settings_wpm_bar);

    // Group Length Card
    koch_settings_length_card = lv_obj_create(screen);
    lv_obj_set_size(koch_settings_length_card, SCREEN_WIDTH - 40, card_height);
    lv_obj_set_pos(koch_settings_length_card, 20, card_y_start + card_height + card_gap);
    applyCardStyle(koch_settings_length_card);
    lv_obj_set_style_pad_all(koch_settings_length_card, 8, 0);
    lv_obj_clear_flag(koch_settings_length_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* length_title = lv_label_create(koch_settings_length_card);
    lv_label_set_text(length_title, "Group Length");
    lv_obj_set_style_text_font(length_title, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(length_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(length_title, LV_ALIGN_TOP_LEFT, 0, 0);

    koch_settings_length_label = lv_label_create(koch_settings_length_card);
    lv_label_set_text_fmt(koch_settings_length_label, "%d chars", kochProgress.groupLength);
    lv_obj_set_style_text_font(koch_settings_length_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(koch_settings_length_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(koch_settings_length_label, LV_ALIGN_TOP_RIGHT, 0, 0);

    koch_settings_length_bar = lv_bar_create(koch_settings_length_card);
    lv_obj_set_size(koch_settings_length_bar, SCREEN_WIDTH - 76, 12);
    lv_obj_align(koch_settings_length_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_bar_set_range(koch_settings_length_bar, 0, 100);
    applyBarStyle(koch_settings_length_bar);

    // Level Card
    koch_settings_level_card = lv_obj_create(screen);
    lv_obj_set_size(koch_settings_level_card, SCREEN_WIDTH - 40, card_height);
    lv_obj_set_pos(koch_settings_level_card, 20, card_y_start + (card_height + card_gap) * 2);
    applyCardStyle(koch_settings_level_card);
    lv_obj_set_style_pad_all(koch_settings_level_card, 8, 0);
    lv_obj_clear_flag(koch_settings_level_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* level_title = lv_label_create(koch_settings_level_card);
    lv_label_set_text(level_title, "Current Level");
    lv_obj_set_style_text_font(level_title, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(level_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(level_title, LV_ALIGN_TOP_LEFT, 0, 0);

    koch_settings_level_label = lv_label_create(koch_settings_level_card);
    lv_label_set_text_fmt(koch_settings_level_label, "Level %d", kochProgress.currentLesson);
    lv_obj_set_style_text_font(koch_settings_level_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(koch_settings_level_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(koch_settings_level_label, LV_ALIGN_TOP_RIGHT, 0, 0);

    koch_settings_level_chars = lv_label_create(koch_settings_level_card);
    String chars = getKochCharactersSpaced();
    lv_label_set_text(koch_settings_level_chars, chars.c_str());
    lv_obj_set_style_text_font(koch_settings_level_chars, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(koch_settings_level_chars, LV_COLOR_ACCENT_GREEN, 0);
    lv_obj_align(koch_settings_level_chars, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_label_set_long_mode(koch_settings_level_chars, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(koch_settings_level_chars, SCREEN_WIDTH - 76);

    // Update display and focus indicator
    updateKochSettingsDisplay();
    updateKochSettingsFocus();

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "UP/DN Select   L/R Adjust   ESC Save & Back");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    // Focus container
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);
    lv_obj_set_pos(focus_container, -10, -10);
    lv_obj_set_style_bg_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, LV_STATE_FOCUSED);
    lv_obj_clear_flag(focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(focus_container, koch_settings_key_event_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus_container);

    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }
    lv_group_focus_obj(focus_container);

    koch_settings_screen = screen;
    return screen;
}

// ============================================
// Koch Statistics Screen
// ============================================

static void koch_stats_key_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    bool handled = true;
    switch(key) {
        case LV_KEY_ESC:
            onLVGLBackNavigation();
            break;
        default:
            handled = false;
            break;
    }

    // Stop event propagation to prevent double-handling
    if (handled) {
        lv_event_stop_bubbling(e);
    }
}

lv_obj_t* createKochStatisticsScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "KOCH STATISTICS");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    createCompactStatusBar(screen);

    // Progress card - overall level progress
    lv_obj_t* progress_card = lv_obj_create(screen);
    lv_obj_set_size(progress_card, SCREEN_WIDTH - 40, 50);
    lv_obj_set_pos(progress_card, 20, HEADER_HEIGHT + 8);
    applyCardStyle(progress_card);
    lv_obj_set_style_pad_all(progress_card, 8, 0);
    lv_obj_clear_flag(progress_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* progress_title = lv_label_create(progress_card);
    lv_label_set_text(progress_title, "Overall Progress");
    lv_obj_set_style_text_font(progress_title, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(progress_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(progress_title, LV_ALIGN_TOP_LEFT, 0, 0);

    int progress_pct = (kochProgress.currentLesson * 100) / 40;
    lv_obj_t* progress_value = lv_label_create(progress_card);
    lv_label_set_text_fmt(progress_value, "Level %d of 40 (%d%%)",
        kochProgress.currentLesson, progress_pct);
    lv_obj_set_style_text_font(progress_value, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(progress_value, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(progress_value, LV_ALIGN_TOP_RIGHT, 0, 0);

    lv_obj_t* progress_bar = lv_bar_create(progress_card);
    lv_obj_set_size(progress_bar, SCREEN_WIDTH - 70, 12);
    lv_obj_align(progress_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_bar_set_range(progress_bar, 0, 100);
    lv_bar_set_value(progress_bar, progress_pct, LV_ANIM_OFF);
    applyBarStyle(progress_bar);

    // Session Stats card
    const int card_width = (SCREEN_WIDTH - 50) / 2;
    const int stats_card_y = HEADER_HEIGHT + 65;
    lv_obj_t* session_card = lv_obj_create(screen);
    lv_obj_set_size(session_card, card_width, 70);
    lv_obj_set_pos(session_card, 20, stats_card_y);
    applyCardStyle(session_card);
    lv_obj_set_style_pad_all(session_card, 8, 0);
    lv_obj_clear_flag(session_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* session_title = lv_label_create(session_card);
    lv_label_set_text(session_title, "This Session");
    lv_obj_set_style_text_font(session_title, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(session_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(session_title, LV_ALIGN_TOP_MID, 0, 0);

    int accuracy = getKochSessionAccuracy();
    lv_obj_t* session_stats = lv_label_create(session_card);
    lv_label_set_text_fmt(session_stats, "%d/%d correct\n%d%% accuracy",
        kochProgress.sessionCorrect, kochProgress.sessionTotal, accuracy);
    lv_obj_set_style_text_font(session_stats, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(session_stats,
        accuracy >= 90 ? LV_COLOR_SUCCESS : (accuracy >= 70 ? LV_COLOR_WARNING : LV_COLOR_TEXT_PRIMARY), 0);
    lv_obj_align(session_stats, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_align(session_stats, LV_TEXT_ALIGN_CENTER, 0);

    // Streak Stats card
    lv_obj_t* streak_card = lv_obj_create(screen);
    lv_obj_set_size(streak_card, card_width, 70);
    lv_obj_set_pos(streak_card, 20 + card_width + 10, stats_card_y);
    applyCardStyle(streak_card);
    lv_obj_set_style_pad_all(streak_card, 8, 0);
    lv_obj_clear_flag(streak_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* streak_title = lv_label_create(streak_card);
    lv_label_set_text(streak_title, "Streaks");
    lv_obj_set_style_text_font(streak_title, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(streak_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(streak_title, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* streak_stats = lv_label_create(streak_card);
    lv_label_set_text_fmt(streak_stats, "Current: %d\nBest: %d",
        getKochCurrentStreak(), getKochBestStreak());
    lv_obj_set_style_text_font(streak_stats, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(streak_stats, LV_COLOR_WARNING, 0);
    lv_obj_align(streak_stats, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_align(streak_stats, LV_TEXT_ALIGN_CENTER, 0);

    // Achievements card
    const int achieve_y = stats_card_y + 78;
    lv_obj_t* achieve_card = lv_obj_create(screen);
    lv_obj_set_size(achieve_card, SCREEN_WIDTH - 40, 70);
    lv_obj_set_pos(achieve_card, 20, achieve_y);
    applyCardStyle(achieve_card);
    lv_obj_set_style_pad_all(achieve_card, 8, 0);
    lv_obj_clear_flag(achieve_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* achieve_title = lv_label_create(achieve_card);
    lv_label_set_text(achieve_title, "Achievements");
    lv_obj_set_style_text_font(achieve_title, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(achieve_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(achieve_title, LV_ALIGN_TOP_LEFT, 0, 0);

    int milestones = getKochMilestones();

    // Build achievements string - compact layout
    String ach = "";
    ach += (milestones & 0x01) ? "[X] First  " : "[ ] First  ";
    ach += (milestones & 0x02) ? "[X] 5 Streak  " : "[ ] 5 Streak  ";
    ach += (milestones & 0x04) ? "[X] 10 Streak\n" : "[ ] 10 Streak\n";
    ach += (milestones & 0x08) ? "[X] 10 Tries  " : "[ ] 10 Tries  ";
    ach += (milestones & 0x10) ? "[X] 90%  " : "[ ] 90%  ";
    ach += (milestones & 0x80) ? "[X] Complete" : "[ ] Complete";

    lv_obj_t* achieve_list = lv_label_create(achieve_card);
    lv_label_set_text(achieve_list, ach.c_str());
    lv_obj_set_style_text_font(achieve_list, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(achieve_list, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(achieve_list, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "ESC Back");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    // Focus container
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);
    lv_obj_set_pos(focus_container, -10, -10);
    lv_obj_set_style_bg_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, LV_STATE_FOCUSED);
    lv_obj_clear_flag(focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(focus_container, koch_stats_key_event_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus_container);

    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }
    lv_group_focus_obj(focus_container);

    koch_stats_screen = screen;
    return screen;
}

// ============================================
// Koch Help Screen
// ============================================

static lv_obj_t* koch_help_screen = NULL;
static lv_obj_t* koch_help_content = NULL;

static void koch_help_key_event_cb(lv_event_t* e) {
    uint32_t key = lv_event_get_key(e);

    // Handle ESC first and return immediately to prevent double-handling
    if (key == LV_KEY_ESC) {
        lv_event_stop_bubbling(e);
        beep(TONE_MENU_NAV, BEEP_SHORT);
        onLVGLBackNavigation();
        return;
    }

    switch (key) {
        case LV_KEY_UP:
        case LV_KEY_PREV:
            // Scroll up
            if (koch_help_content != NULL) {
                lv_obj_scroll_by(koch_help_content, 0, 30, LV_ANIM_ON);
            }
            break;
        case LV_KEY_DOWN:
        case LV_KEY_NEXT:
            // Scroll down
            if (koch_help_content != NULL) {
                lv_obj_scroll_by(koch_help_content, 0, -30, LV_ANIM_ON);
            }
            break;
        default:
            // Check for direct character press - play if unlocked
            {
                char c = (char)key;
                c = toupper(c);
                String chars = getKochCharacterSet();
                if (chars.indexOf(c) >= 0) {
                    // Character is unlocked, play it
                    playMorseChar(c, kochProgress.wpm);
                }
            }
            break;
    }

    // Stop bubbling for all keys
    lv_event_stop_bubbling(e);
}

lv_obj_t* createKochHelpScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "KOCH METHOD HELP");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    createCompactStatusBar(screen);

    // Scrollable content area
    koch_help_content = lv_obj_create(screen);
    lv_obj_set_size(koch_help_content, SCREEN_WIDTH - 20, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 10);
    lv_obj_set_pos(koch_help_content, 10, HEADER_HEIGHT + 5);
    lv_obj_set_style_bg_opa(koch_help_content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(koch_help_content, 0, 0);
    lv_obj_set_style_pad_all(koch_help_content, 5, 0);
    lv_obj_add_flag(koch_help_content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(koch_help_content, LV_DIR_VER);

    // Alias for readability in the rest of the function
    lv_obj_t* content = koch_help_content;

    // Help content - Section 1: What is Koch Method
    lv_obj_t* section1_title = lv_label_create(content);
    lv_label_set_text(section1_title, "WHAT IS THE KOCH METHOD?");
    lv_obj_set_style_text_font(section1_title, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(section1_title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(section1_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* section1_text = lv_label_create(content);
    lv_label_set_text(section1_text,
        "The Koch Method is a proven technique for\n"
        "learning Morse code by ear. You start with\n"
        "just 2 characters (K and M), practice until\n"
        "you copy them at 90% accuracy, then add one\n"
        "more character. Repeat until you know all 40!");
    lv_obj_set_style_text_font(section1_text, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(section1_text, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align_to(section1_text, section1_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    // Section 2: How to Practice
    lv_obj_t* section2_title = lv_label_create(content);
    lv_label_set_text(section2_title, "HOW TO PRACTICE:");
    lv_obj_set_style_text_font(section2_title, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(section2_title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align_to(section2_title, section1_text, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 12);

    lv_obj_t* section2_text = lv_label_create(content);
    lv_label_set_text(section2_text,
        "1. Press SPACE to hear a morse code group\n"
        "2. Type what you heard on your keyboard\n"
        "3. Press ENTER to check your answer\n"
        "4. Get 90%% accuracy with 10+ attempts\n"
        "   to advance to the next level");
    lv_obj_set_style_text_font(section2_text, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(section2_text, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align_to(section2_text, section2_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    // Section 3: Current Level Info
    lv_obj_t* section3_title = lv_label_create(content);
    lv_label_set_text(section3_title, "YOUR CURRENT LEVEL:");
    lv_obj_set_style_text_font(section3_title, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(section3_title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align_to(section3_title, section2_text, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 12);

    // Build current level characters string
    String levelChars = "";
    for (int i = 0; i < kochProgress.currentLesson && i < KOCH_TOTAL_LESSONS; i++) {
        levelChars += KOCH_SEQUENCE[i];
        if (i < kochProgress.currentLesson - 1) levelChars += " ";
    }

    char levelInfo[128];
    snprintf(levelInfo, sizeof(levelInfo),
        "Level %d includes: %s\n"
        "You need 90%% accuracy to unlock the\n"
        "next character.",
        kochProgress.currentLesson, levelChars.c_str());

    lv_obj_t* section3_text = lv_label_create(content);
    lv_label_set_text(section3_text, levelInfo);
    lv_obj_set_style_text_font(section3_text, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(section3_text, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align_to(section3_text, section3_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    // Section 4: Navigation
    lv_obj_t* section4_title = lv_label_create(content);
    lv_label_set_text(section4_title, "NAVIGATION:");
    lv_obj_set_style_text_font(section4_title, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(section4_title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align_to(section4_title, section3_text, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 12);

    lv_obj_t* section4_text = lv_label_create(content);
    lv_label_set_text(section4_text,
        "ENTER = Start practicing\n"
        "S = Adjust settings (speed, group length)\n"
        "R = Character reference (hear any char)\n"
        ". (period) = Show this help screen\n"
        "ESC = Go back\n\n"
        "TIP: Press any letter/number key to hear\n"
        "its morse code!");
    lv_obj_set_style_text_font(section4_text, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(section4_text, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align_to(section4_text, section4_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* footer_label = lv_label_create(footer);
    lv_label_set_text(footer_label, LV_SYMBOL_UP LV_SYMBOL_DOWN " Scroll  [A-Z] Play Char  ESC Back");
    lv_obj_set_style_text_color(footer_label, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(footer_label, getThemeFonts()->font_small, 0);
    lv_obj_center(footer_label);

    // Focus container for key handling
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);
    lv_obj_set_pos(focus_container, -10, -10);
    lv_obj_set_style_bg_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, LV_STATE_FOCUSED);
    lv_obj_clear_flag(focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(focus_container, koch_help_key_event_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus_container);

    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }
    lv_group_focus_obj(focus_container);

    koch_help_screen = screen;
    return screen;
}

// ============================================
// Koch Character Reference Screen
// ============================================

static lv_obj_t* koch_char_ref_screen = NULL;
static lv_obj_t* koch_char_ref_cells[40];  // One per possible character
static int koch_char_ref_selection = 0;     // Currently selected index
static int koch_char_ref_count = 0;         // Number of unlocked chars

static void updateKochCharRefSelection() {
    // Reset all cells to default style
    for (int i = 0; i < koch_char_ref_count; i++) {
        if (koch_char_ref_cells[i] != NULL) {
            lv_obj_set_style_border_color(koch_char_ref_cells[i], getThemeColors()->border_subtle, 0);
            lv_obj_set_style_border_width(koch_char_ref_cells[i], 1, 0);
            lv_obj_set_style_border_opa(koch_char_ref_cells[i], LV_OPA_50, 0);
        }
    }
    // Highlight selected cell
    if (koch_char_ref_selection >= 0 && koch_char_ref_selection < koch_char_ref_count) {
        lv_obj_t* selected = koch_char_ref_cells[koch_char_ref_selection];
        if (selected != NULL) {
            lv_obj_set_style_border_color(selected, LV_COLOR_ACCENT_CYAN, 0);
            lv_obj_set_style_border_width(selected, 2, 0);
            lv_obj_set_style_border_opa(selected, LV_OPA_COVER, 0);
        }
    }
}

static void koch_char_ref_key_event_cb(lv_event_t* e) {
    uint32_t key = lv_event_get_key(e);
    bool handled = true;

    const int COLS = 8;  // 8 characters per row
    int row = koch_char_ref_selection / COLS;
    int col = koch_char_ref_selection % COLS;

    switch (key) {
        case LV_KEY_ESC:
            beep(TONE_MENU_NAV, BEEP_SHORT);
            onLVGLBackNavigation();
            break;
        case LV_KEY_UP:
        case LV_KEY_PREV:
            if (row > 0) {
                koch_char_ref_selection -= COLS;
                if (koch_char_ref_selection < 0) koch_char_ref_selection = 0;
                updateKochCharRefSelection();
                beep(TONE_MENU_NAV, BEEP_SHORT);
            }
            break;
        case LV_KEY_DOWN:
        case LV_KEY_NEXT:
            if (koch_char_ref_selection + COLS < koch_char_ref_count) {
                koch_char_ref_selection += COLS;
                updateKochCharRefSelection();
                beep(TONE_MENU_NAV, BEEP_SHORT);
            }
            break;
        case LV_KEY_LEFT:
            if (koch_char_ref_selection > 0) {
                koch_char_ref_selection--;
                updateKochCharRefSelection();
                beep(TONE_MENU_NAV, BEEP_SHORT);
            }
            break;
        case LV_KEY_RIGHT:
            if (koch_char_ref_selection < koch_char_ref_count - 1) {
                koch_char_ref_selection++;
                updateKochCharRefSelection();
                beep(TONE_MENU_NAV, BEEP_SHORT);
            }
            break;
        case LV_KEY_ENTER:
            // Play the selected character
            if (koch_char_ref_selection >= 0 && koch_char_ref_selection < koch_char_ref_count) {
                String chars = getKochCharacterSet();
                if (koch_char_ref_selection < (int)chars.length()) {
                    char c = chars.charAt(koch_char_ref_selection);
                    playMorseChar(c, kochProgress.wpm);
                }
            }
            break;
        default:
            // Check for direct character press
            {
                char c = (char)key;
                c = toupper(c);
                String chars = getKochCharacterSet();
                int idx = chars.indexOf(c);
                if (idx >= 0) {
                    // Character is unlocked, play it and select it
                    koch_char_ref_selection = idx;
                    updateKochCharRefSelection();
                    playMorseChar(c, kochProgress.wpm);
                } else {
                    handled = false;
                }
            }
            break;
    }

    lv_event_stop_bubbling(e);
}

lv_obj_t* createKochCharRefScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Reset state
    koch_char_ref_selection = 0;
    for (int i = 0; i < 40; i++) {
        koch_char_ref_cells[i] = NULL;
    }

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "CHARACTER REFERENCE");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    createCompactStatusBar(screen);

    // Get unlocked characters
    String chars = getKochCharacterSet();
    koch_char_ref_count = chars.length();

    // Grid layout - 8 columns
    const int COLS = 8;
    const int cell_width = 55;
    const int cell_height = 45;
    const int start_x = (SCREEN_WIDTH - (COLS * cell_width)) / 2;
    const int start_y = HEADER_HEIGHT + 10;

    for (int i = 0; i < koch_char_ref_count && i < 40; i++) {
        int row = i / COLS;
        int col = i % COLS;
        int x = start_x + col * cell_width;
        int y = start_y + row * cell_height;

        // Create cell container
        lv_obj_t* cell = lv_obj_create(screen);
        lv_obj_set_size(cell, cell_width - 4, cell_height - 4);
        lv_obj_set_pos(cell, x + 2, y + 2);
        applyCardStyle(cell);
        lv_obj_set_style_pad_all(cell, 2, 0);
        lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);

        // Character label (large)
        char charStr[2] = {chars.charAt(i), '\0'};
        lv_obj_t* charLabel = lv_label_create(cell);
        lv_label_set_text(charLabel, charStr);
        lv_obj_set_style_text_font(charLabel, getThemeFonts()->font_subtitle, 0);
        lv_obj_set_style_text_color(charLabel, LV_COLOR_ACCENT_CYAN, 0);
        lv_obj_align(charLabel, LV_ALIGN_TOP_MID, 0, 0);

        // Morse pattern label (small)
        const char* pattern = getMorseCode(chars.charAt(i));
        lv_obj_t* patternLabel = lv_label_create(cell);
        lv_label_set_text(patternLabel, pattern ? pattern : "");
        lv_obj_set_style_text_font(patternLabel, getThemeFonts()->font_small, 0);
        lv_obj_set_style_text_color(patternLabel, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_align(patternLabel, LV_ALIGN_BOTTOM_MID, 0, 0);

        koch_char_ref_cells[i] = cell;
    }

    // Set initial selection highlight
    updateKochCharRefSelection();

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* footerLabel = lv_label_create(footer);
    lv_label_set_text(footerLabel, LV_SYMBOL_UP LV_SYMBOL_DOWN LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " Select  ENTER Play  ESC Back");
    lv_obj_set_style_text_color(footerLabel, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(footerLabel, getThemeFonts()->font_small, 0);
    lv_obj_center(footerLabel);

    // Focus container for key handling
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);
    lv_obj_set_pos(focus_container, -10, -10);
    lv_obj_set_style_bg_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, LV_STATE_FOCUSED);
    lv_obj_clear_flag(focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(focus_container, koch_char_ref_key_event_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus_container);

    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }
    lv_group_focus_obj(focus_container);

    koch_char_ref_screen = screen;
    return screen;
}

// ============================================
// Koch New Character Introduction Screen
// ============================================

static lv_obj_t* koch_new_char_screen = NULL;
static char koch_new_char_to_show = 'K';
static int koch_new_char_play_count = 0;
static unsigned long koch_new_char_last_play = 0;

static void koch_new_char_key_event_cb(lv_event_t* e) {
    uint32_t key = lv_event_get_key(e);

    switch (key) {
        case LV_KEY_ESC:
            beep(TONE_MENU_NAV, BEEP_SHORT);
            onLVGLBackNavigation();
            break;
        case ' ':
            // Replay the character
            playMorseChar(koch_new_char_to_show, kochProgress.wpm);
            break;
        case LV_KEY_ENTER:
            // Continue to practice
            beep(TONE_SELECT, BEEP_MEDIUM);
            onLVGLMenuSelect(KOCH_MODE_PRACTICE_INT);
            break;
        default:
            break;
    }

    lv_event_stop_bubbling(e);
}

lv_obj_t* createKochNewCharScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Get the new character from Koch progress
    koch_new_char_to_show = KOCH_SEQUENCE[kochProgress.currentLesson - 1];
    koch_new_char_play_count = 0;
    koch_new_char_last_play = 0;

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "NEW CHARACTER UNLOCKED!");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_set_style_text_color(title, LV_COLOR_SUCCESS, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    createCompactStatusBar(screen);

    // Center content area
    int center_y = SCREEN_HEIGHT / 2 - 20;

    // Large character display
    char charStr[2] = {koch_new_char_to_show, '\0'};
    lv_obj_t* charLabel = lv_label_create(screen);
    lv_label_set_text(charLabel, charStr);
    lv_obj_set_style_text_font(charLabel, getThemeFonts()->font_title, 0);
    lv_obj_set_style_text_color(charLabel, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(charLabel, LV_ALIGN_CENTER, 0, -40);

    // Morse pattern display
    const char* pattern = getMorseCode(koch_new_char_to_show);
    lv_obj_t* patternLabel = lv_label_create(screen);
    lv_label_set_text(patternLabel, pattern ? pattern : "");
    lv_obj_set_style_text_font(patternLabel, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(patternLabel, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(patternLabel, LV_ALIGN_CENTER, 0, 10);

    // Instruction text
    lv_obj_t* instructLabel = lv_label_create(screen);
    lv_label_set_text(instructLabel, "Listen carefully...");
    lv_obj_set_style_text_font(instructLabel, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(instructLabel, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(instructLabel, LV_ALIGN_CENTER, 0, 60);

    // Level info
    char levelInfo[64];
    snprintf(levelInfo, sizeof(levelInfo), "Level %d - You now have %d characters!",
             kochProgress.currentLesson, kochProgress.currentLesson);
    lv_obj_t* levelLabel = lv_label_create(screen);
    lv_label_set_text(levelLabel, levelInfo);
    lv_obj_set_style_text_font(levelLabel, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(levelLabel, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(levelLabel, LV_ALIGN_CENTER, 0, 90);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* footerLabel = lv_label_create(footer);
    lv_label_set_text(footerLabel, "SPACE Replay   ENTER Practice   ESC Back");
    lv_obj_set_style_text_color(footerLabel, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(footerLabel, getThemeFonts()->font_small, 0);
    lv_obj_center(footerLabel);

    // Focus container for key handling
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);
    lv_obj_set_pos(focus_container, -10, -10);
    lv_obj_set_style_bg_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, LV_STATE_FOCUSED);
    lv_obj_clear_flag(focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(focus_container, koch_new_char_key_event_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus_container);

    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }
    lv_group_focus_obj(focus_container);

    // Auto-play the character 3 times with delays
    // We'll do this in a simple way - play once now, and use a timer for the rest
    delay(500);  // Brief delay before first play
    playMorseChar(koch_new_char_to_show, kochProgress.wpm);

    koch_new_char_screen = screen;
    return screen;
}

// ============================================
// CW Academy Screens
// ============================================

static lv_obj_t* cwa_screen = NULL;
static lv_obj_t* cwa_track_btns[4] = {NULL, NULL, NULL, NULL};
static lv_obj_t* cwa_session_btns[16] = {NULL};
static lv_obj_t* cwa_practice_type_btns[3] = {NULL, NULL, NULL};
static lv_obj_t* cwa_message_type_btns[6] = {NULL};

// Forward declaration for menu select
extern void onLVGLMenuSelect(int target_mode);

/*
 * Linear navigation key handler for CWA selection screens
 * Handles UP/DOWN arrow keys for single-column lists
 * Similar to menu_grid_nav_handler but simplified for 1D navigation
 */
static void cwa_linear_nav_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // Only handle UP/DOWN keys (both KEY_UP/DOWN and PREV/NEXT mappings)
    if (key != LV_KEY_PREV && key != LV_KEY_NEXT &&
        key != LV_KEY_UP && key != LV_KEY_DOWN) return;

    // Stop propagation to prevent LVGL's default handling
    lv_event_stop_processing(e);

    lv_group_t* group = getLVGLInputGroup();
    if (!group) return;

    // Navigate to previous or next object in the group
    if (key == LV_KEY_PREV || key == LV_KEY_UP) {
        lv_group_focus_prev(group);
    } else if (key == LV_KEY_NEXT || key == LV_KEY_DOWN) {
        lv_group_focus_next(group);
    }

    // Scroll focused object into view
    lv_obj_t* focused = lv_group_get_focused(group);
    if (focused) {
        lv_obj_scroll_to_view(focused, LV_ANIM_ON);
    }
}

/*
 * Event handler for CW Academy track selection
 * Navigates to session select screen
 */
static void cwa_track_select_handler(lv_event_t* e) {
    lv_obj_t* target = lv_event_get_target(e);
    int track = (int)(intptr_t)lv_obj_get_user_data(target);

    Serial.printf("[CWAScreen] Selected track: %d\n", track);

    // Only Beginner track (0) is available, others coming soon
    if (track > 0) {
        beep(600, 150);  // Error beep
        Serial.println("[CWAScreen] Track not yet available");
        return;
    }

    cwaSelectedTrack = (CWATrack)track;
    saveCWAProgress();
    // Note: onLVGLMenuSelect plays the selection beep

    // Navigate to session select (mode 9)
    onLVGLMenuSelect(9);
}

/*
 * Event handler for CW Academy session selection
 * Navigates to practice type select screen
 */
static void cwa_session_select_handler(lv_event_t* e) {
    lv_obj_t* target = lv_event_get_target(e);
    int session = (int)(intptr_t)lv_obj_get_user_data(target);

    Serial.printf("[CWAScreen] Selected session: %d\n", session);

    cwaSelectedSession = session;
    saveCWAProgress();
    // Note: onLVGLMenuSelect plays the selection beep

    // Navigate to practice type select (mode 10)
    onLVGLMenuSelect(10);
}

/*
 * Event handler for CW Academy practice type selection
 */
static void cwa_practice_type_select_handler(lv_event_t* e) {
    lv_obj_t* target = lv_event_get_target(e);
    int practiceType = (int)(intptr_t)lv_obj_get_user_data(target);

    Serial.printf("[CWAScreen] Selected practice type: %d, session: %d\n", practiceType, cwaSelectedSession);

    // Check if locked (sending/daily drill locked for sessions 1-10)
    bool advancedLocked = (cwaSelectedSession <= 10);
    bool currentTypeLocked = advancedLocked && (practiceType != PRACTICE_COPY);

    if (currentTypeLocked) {
        beep(600, 150);  // Error beep
        Serial.println("[CWAScreen] Practice type locked");
        return;
    }

    cwaSelectedPracticeType = (CWAPracticeType)practiceType;
    saveCWAProgress();
    // Note: onLVGLMenuSelect plays the selection beep

    // Sessions 11-13 go directly to QSO practice (mode 14)
    if (cwaSelectedSession >= 11 && cwaSelectedSession <= 13) {
        onLVGLMenuSelect(14);
    } else {
        // Navigate to message type select (mode 11)
        onLVGLMenuSelect(11);
    }
}

/*
 * Event handler for CW Academy message type selection
 * Starts the appropriate practice mode
 */
static void cwa_message_type_select_handler(lv_event_t* e) {
    lv_obj_t* target = lv_event_get_target(e);
    int messageType = (int)(intptr_t)lv_obj_get_user_data(target);

    Serial.printf("[CWAScreen] Selected message type: %d\n", messageType);

    cwaSelectedMessageType = (CWAMessageType)messageType;
    saveCWAProgress();
    // Note: onLVGLMenuSelect plays the selection beep

    // Route based on practice type
    if (cwaSelectedPracticeType == PRACTICE_COPY) {
        onLVGLMenuSelect(12);  // Copy practice
    } else if (cwaSelectedPracticeType == PRACTICE_SENDING) {
        onLVGLMenuSelect(13);  // Sending practice
    } else {
        // Daily drill - default to copy for now
        onLVGLMenuSelect(12);
    }
}

lv_obj_t* createCWAcademyTrackSelectScreen() {
    // NOTE: Don't call clearNavigationGroup() here - onLVGLMenuSelect() already does it

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
    lv_obj_set_style_pad_row(content, 12, 0);
    lv_obj_set_style_pad_all(content, 10, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);

    // Track buttons (4 tracks) - only Beginner is available, others coming soon
    // Using correct CWA track names: Beginner, Fundamental, Intermediate, Advanced
    const char* track_texts[] = {
        "Beginner - Learn CW from zero",
        "Fundamental - Coming Soon",
        "Intermediate - Coming Soon",
        "Advanced - Coming Soon"
    };

    for (int i = 0; i < 4; i++) {
        bool isComingSoon = (i > 0);  // Only Beginner (index 0) is available

        lv_obj_t* track_btn = lv_btn_create(content);
        lv_obj_set_size(track_btn, lv_pct(100), 50);
        lv_obj_add_style(track_btn, getStyleMenuCard(), 0);
        lv_obj_add_style(track_btn, getStyleMenuCardFocused(), LV_STATE_FOCUSED);

        // Grey out coming soon tracks
        if (isComingSoon) {
            lv_obj_set_style_bg_opa(track_btn, LV_OPA_50, 0);
        }

        // Store track index in user_data and add event handlers
        lv_obj_set_user_data(track_btn, (void*)(intptr_t)i);
        lv_obj_add_event_cb(track_btn, cwa_track_select_handler, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(track_btn, cwa_linear_nav_handler, LV_EVENT_KEY, NULL);

        addNavigableWidget(track_btn);
        cwa_track_btns[i] = track_btn;

        // Track name and description - single line format
        lv_obj_t* track_label = lv_label_create(track_btn);
        lv_label_set_text(track_label, track_texts[i]);
        lv_obj_set_style_text_font(track_label, getThemeFonts()->font_body, 0);
        if (isComingSoon) {
            lv_obj_set_style_text_color(track_label, LV_COLOR_TEXT_SECONDARY, 0);
        }
        lv_obj_set_width(track_label, lv_pct(100));
        lv_label_set_long_mode(track_label, LV_LABEL_LONG_CLIP);
        lv_obj_center(track_label);
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

    // Focus the first button for keyboard navigation
    if (cwa_track_btns[0] != NULL) {
        lv_group_focus_obj(cwa_track_btns[0]);
    }

    cwa_screen = screen;
    return screen;
}

/*
 * Create CW Academy Session Select Screen (Mode 9)
 * Shows 16 sessions with character counts and descriptions
 */
lv_obj_t* createCWAcademySessionSelectScreen() {
    // NOTE: Don't call clearNavigationGroup() here - onLVGLMenuSelect() already does it

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    char title_text[48];
    snprintf(title_text, sizeof(title_text), "CW ACADEMY - %s", cwaTrackNames[cwaSelectedTrack]);
    lv_label_set_text(title, title_text);
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar
    createCompactStatusBar(screen);

    // Content - Scrollable session list
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, SCREEN_WIDTH - 20, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 10);
    lv_obj_set_pos(content, 10, HEADER_HEIGHT + 5);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 8, 0);
    lv_obj_set_style_pad_all(content, 8, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);

    // Session buttons (16 sessions) - use lv_btn for proper keyboard navigation
    for (int i = 0; i < 16; i++) {
        lv_obj_t* session_btn = lv_btn_create(content);
        lv_obj_set_size(session_btn, lv_pct(100), 50);
        lv_obj_add_style(session_btn, getStyleMenuCard(), 0);
        lv_obj_add_style(session_btn, getStyleMenuCardFocused(), LV_STATE_FOCUSED);

        // Store session number (1-based) and add event handlers
        lv_obj_set_user_data(session_btn, (void*)(intptr_t)(i + 1));
        lv_obj_add_event_cb(session_btn, cwa_session_select_handler, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(session_btn, cwa_linear_nav_handler, LV_EVENT_KEY, NULL);

        addNavigableWidget(session_btn);
        cwa_session_btns[i] = session_btn;

        // Session number and description - single line format
        lv_obj_t* session_label = lv_label_create(session_btn);
        char session_text[80];
        const struct CWASession& session = cwaSessionData[i];
        if (strlen(session.newChars) > 0) {
            snprintf(session_text, sizeof(session_text), "%d. %s  [+%s]", i + 1, session.description, session.newChars);
        } else {
            snprintf(session_text, sizeof(session_text), "%d. %s", i + 1, session.description);
        }
        lv_label_set_text(session_label, session_text);
        lv_obj_set_style_text_font(session_label, getThemeFonts()->font_body, 0);
        lv_obj_set_width(session_label, lv_pct(100));
        lv_label_set_long_mode(session_label, LV_LABEL_LONG_CLIP);
        lv_obj_center(session_label);
    }

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "UP/DN Select   ENTER Choose   ESC Back");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    // Focus the first button for keyboard navigation
    if (cwa_session_btns[0] != NULL) {
        lv_group_focus_obj(cwa_session_btns[0]);
    }

    return screen;
}

/*
 * Create CW Academy Practice Type Select Screen (Mode 10)
 * Shows Copy Practice, Sending Practice, Daily Drill with lock indicators
 */
lv_obj_t* createCWAcademyPracticeTypeSelectScreen() {
    // NOTE: Don't call clearNavigationGroup() here - onLVGLMenuSelect() already does it

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    char title_text[48];
    snprintf(title_text, sizeof(title_text), "SESSION %d - PRACTICE TYPE", cwaSelectedSession);
    lv_label_set_text(title, title_text);
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar
    createCompactStatusBar(screen);

    // Content area
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, SCREEN_WIDTH - 40, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 20);
    lv_obj_set_pos(content, 20, HEADER_HEIGHT + 10);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 12, 0);
    lv_obj_set_style_pad_all(content, 10, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);

    bool advancedLocked = (cwaSelectedSession <= 10);

    // Practice type buttons (3 types) - use lv_btn for proper keyboard navigation
    for (int i = 0; i < 3; i++) {
        bool isLocked = advancedLocked && (i != PRACTICE_COPY);

        lv_obj_t* type_btn = lv_btn_create(content);
        lv_obj_set_size(type_btn, lv_pct(100), 55);
        lv_obj_add_style(type_btn, getStyleMenuCard(), 0);
        lv_obj_add_style(type_btn, getStyleMenuCardFocused(), LV_STATE_FOCUSED);

        // Visual indication for locked items
        if (isLocked) {
            lv_obj_set_style_bg_opa(type_btn, LV_OPA_50, 0);
        }

        // Store practice type and add event handlers
        lv_obj_set_user_data(type_btn, (void*)(intptr_t)i);
        lv_obj_add_event_cb(type_btn, cwa_practice_type_select_handler, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(type_btn, cwa_linear_nav_handler, LV_EVENT_KEY, NULL);

        addNavigableWidget(type_btn);
        cwa_practice_type_btns[i] = type_btn;

        // Practice type - single line format with description
        lv_obj_t* type_label = lv_label_create(type_btn);
        char type_text[64];
        if (isLocked) {
            snprintf(type_text, sizeof(type_text), "[LOCKED] %s (Session 11+)", cwaPracticeTypeNames[i]);
        } else {
            snprintf(type_text, sizeof(type_text), "%s - %s", cwaPracticeTypeNames[i], cwaPracticeTypeDescriptions[i]);
        }
        lv_label_set_text(type_label, type_text);
        lv_obj_set_style_text_font(type_label, getThemeFonts()->font_body, 0);
        if (isLocked) {
            lv_obj_set_style_text_color(type_label, LV_COLOR_TEXT_SECONDARY, 0);
        }
        lv_obj_set_width(type_label, lv_pct(100));
        lv_label_set_long_mode(type_label, LV_LABEL_LONG_CLIP);
        lv_obj_center(type_label);
    }

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "UP/DN Select   ENTER Start   ESC Back");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    // Focus the first button for keyboard navigation
    if (cwa_practice_type_btns[0] != NULL) {
        lv_group_focus_obj(cwa_practice_type_btns[0]);
    }

    return screen;
}

/*
 * Create CW Academy Message Type Select Screen (Mode 11)
 * Shows 6 message types: Characters, Words, Abbreviations, Numbers, Callsigns, Phrases
 */
lv_obj_t* createCWAcademyMessageTypeSelectScreen() {
    // NOTE: Don't call clearNavigationGroup() here - onLVGLMenuSelect() already does it

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    char title_text[48];
    snprintf(title_text, sizeof(title_text), "%s - MESSAGE TYPE", cwaPracticeTypeNames[cwaSelectedPracticeType]);
    lv_label_set_text(title, title_text);
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar
    createCompactStatusBar(screen);

    // Content - Scrollable message type list
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, SCREEN_WIDTH - 20, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 10);
    lv_obj_set_pos(content, 10, HEADER_HEIGHT + 5);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 8, 0);
    lv_obj_set_style_pad_all(content, 8, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);

    // Message type buttons (6 types) - use lv_btn for proper keyboard navigation
    for (int i = 0; i < 6; i++) {
        lv_obj_t* msg_btn = lv_btn_create(content);
        lv_obj_set_size(msg_btn, lv_pct(100), 40);
        lv_obj_add_style(msg_btn, getStyleMenuCard(), 0);
        lv_obj_add_style(msg_btn, getStyleMenuCardFocused(), LV_STATE_FOCUSED);

        // Store message type and add event handlers
        lv_obj_set_user_data(msg_btn, (void*)(intptr_t)i);
        lv_obj_add_event_cb(msg_btn, cwa_message_type_select_handler, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(msg_btn, cwa_linear_nav_handler, LV_EVENT_KEY, NULL);

        addNavigableWidget(msg_btn);
        cwa_message_type_btns[i] = msg_btn;

        // Message type - single line format: "Name - Description"
        lv_obj_t* msg_label = lv_label_create(msg_btn);
        char msg_text[64];
        snprintf(msg_text, sizeof(msg_text), "%s - %s", cwaMessageTypeNames[i], cwaMessageTypeDescriptions[i]);
        lv_label_set_text(msg_label, msg_text);
        lv_obj_set_style_text_font(msg_label, getThemeFonts()->font_body, 0);
        lv_obj_set_width(msg_label, lv_pct(100));
        lv_label_set_long_mode(msg_label, LV_LABEL_LONG_CLIP);
        lv_obj_center(msg_label);
    }

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "UP/DN Select   ENTER Start   ESC Back");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    // Focus the first button for keyboard navigation
    if (cwa_message_type_btns[0] != NULL) {
        lv_group_focus_obj(cwa_message_type_btns[0]);
    }

    return screen;
}

// ============================================
// CW Academy Copy Practice Screen (Mode 12)
// ============================================

// External copy practice state (from training_cwa_copy_practice.h)
extern String cwaCopyTarget;
extern String cwaCopyInput;
extern int cwaCopyRound;
extern int cwaCopyCorrect;
extern int cwaCopyTotal;
extern int cwaCopyCharCount;
extern bool cwaCopyWaitingForInput;
extern bool cwaCopyShowingFeedback;
extern void startCWACopyRound(LGFX& tft);
extern String generateCWAContent();
extern void playMorseString(const char* text, int wpm, int tone);
extern LGFX tft;

// Static widgets for Copy Practice screen
static lv_obj_t* cwa_copy_screen = NULL;
static lv_obj_t* cwa_copy_round_label = NULL;
static lv_obj_t* cwa_copy_score_label = NULL;
static lv_obj_t* cwa_copy_chars_label = NULL;
static lv_obj_t* cwa_copy_prompt_label = NULL;
static lv_obj_t* cwa_copy_input_label = NULL;
static lv_obj_t* cwa_copy_feedback_label = NULL;
static lv_obj_t* cwa_copy_target_label = NULL;

// State machine for copy practice
enum CWACopyState {
    CWA_COPY_READY,        // Ready to start round
    CWA_COPY_PLAYING,      // Playing morse
    CWA_COPY_INPUT,        // Waiting for input
    CWA_COPY_FEEDBACK,     // Showing feedback
    CWA_COPY_COMPLETE      // Session complete
};
static CWACopyState cwaCopyUIState = CWA_COPY_READY;

/*
 * Initialize copy practice state for LVGL mode
 */
void initCWACopyPractice() {
    cwaCopyRound = 0;
    cwaCopyCorrect = 0;
    cwaCopyTotal = 0;
    cwaCopyTarget = "";
    cwaCopyInput = "";
    cwaCopyWaitingForInput = false;
    cwaCopyShowingFeedback = false;
    cwaCopyUIState = CWA_COPY_READY;

    // Use esp_random() instead of analogRead(0) to avoid breaking I2S audio
    randomSeed(esp_random());
}

/*
 * Update CW Academy Copy Practice UI elements
 */
void updateCWACopyPracticeUI() {
    if (!cwa_copy_screen) return;

    // Update round display
    if (cwa_copy_round_label) {
        lv_label_set_text_fmt(cwa_copy_round_label, "Round: %d/10", cwaCopyRound);
    }

    // Update score display
    if (cwa_copy_score_label) {
        lv_label_set_text_fmt(cwa_copy_score_label, "Score: %d/%d", cwaCopyCorrect, cwaCopyTotal);
    }

    // Update character count display
    if (cwa_copy_chars_label) {
        lv_label_set_text_fmt(cwa_copy_chars_label, "Chars: %d", cwaCopyCharCount);
    }

    // Update input display
    if (cwa_copy_input_label) {
        if (cwaCopyInput.length() > 0) {
            lv_label_set_text(cwa_copy_input_label, cwaCopyInput.c_str());
        } else {
            lv_label_set_text(cwa_copy_input_label, "_");
        }
    }

    // Update feedback based on state
    if (cwaCopyUIState == CWA_COPY_FEEDBACK && cwa_copy_feedback_label && cwa_copy_target_label) {
        bool correct = cwaCopyInput.equalsIgnoreCase(cwaCopyTarget);
        lv_label_set_text(cwa_copy_feedback_label, correct ? "CORRECT!" : "INCORRECT");
        lv_obj_set_style_text_color(cwa_copy_feedback_label, correct ? LV_COLOR_SUCCESS : LV_COLOR_ERROR, 0);
        lv_label_set_text_fmt(cwa_copy_target_label, "Answer: %s", cwaCopyTarget.c_str());
    }

    // Update prompt based on state
    if (cwa_copy_prompt_label) {
        switch (cwaCopyUIState) {
            case CWA_COPY_READY:
                lv_label_set_text(cwa_copy_prompt_label, "Press SPACE to hear morse");
                break;
            case CWA_COPY_PLAYING:
                lv_label_set_text(cwa_copy_prompt_label, "Listening...");
                break;
            case CWA_COPY_INPUT:
                lv_label_set_text(cwa_copy_prompt_label, "Type what you heard:");
                break;
            case CWA_COPY_FEEDBACK:
                lv_label_set_text(cwa_copy_prompt_label, "Press any key to continue");
                break;
            case CWA_COPY_COMPLETE:
                lv_label_set_text(cwa_copy_prompt_label, "Session Complete!");
                break;
        }
    }
}

/*
 * Key event handler for Copy Practice
 */
static void cwa_copy_key_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    Serial.printf("[CWACopy] Key event: %lu state: %d\n", key, cwaCopyUIState);

    // Handle ESC always
    if (key == LV_KEY_ESC) {
        // Cleanup is handled by back navigation
        return;
    }

    // Handle UP/DOWN for character count adjustment
    if (key == LV_KEY_UP) {
        if (cwaCopyCharCount < 10) {
            cwaCopyCharCount++;
            beep(TONE_MENU_NAV, BEEP_SHORT);
            updateCWACopyPracticeUI();
        }
        return;
    }
    if (key == LV_KEY_DOWN) {
        if (cwaCopyCharCount > 1) {
            cwaCopyCharCount--;
            beep(TONE_MENU_NAV, BEEP_SHORT);
            updateCWACopyPracticeUI();
        }
        return;
    }

    switch (cwaCopyUIState) {
        case CWA_COPY_READY:
            if (key == ' ') {
                // Start playing morse
                cwaCopyUIState = CWA_COPY_PLAYING;
                updateCWACopyPracticeUI();

                // Generate new content and play
                cwaCopyRound++;
                cwaCopyTarget = generateCWAContent();
                cwaCopyInput = "";
                Serial.printf("[CWACopy] Playing: %s\n", cwaCopyTarget.c_str());

                playMorseString(cwaCopyTarget.c_str(), cwSpeed, cwTone);

                // After playing, switch to input mode
                cwaCopyUIState = CWA_COPY_INPUT;
                updateCWACopyPracticeUI();
            }
            break;

        case CWA_COPY_INPUT:
            if (key == ' ') {
                // Replay
                playMorseString(cwaCopyTarget.c_str(), cwSpeed, cwTone);
            } else if (key == LV_KEY_ENTER) {
                // Submit answer
                cwaCopyTotal++;
                if (cwaCopyInput.equalsIgnoreCase(cwaCopyTarget)) {
                    cwaCopyCorrect++;
                    beep(1000, 200);  // Success beep
                } else {
                    beep(400, 300);  // Error beep
                }
                cwaCopyUIState = CWA_COPY_FEEDBACK;
                updateCWACopyPracticeUI();
            } else if (key == LV_KEY_BACKSPACE || key == 0x08) {
                // Delete last character
                if (cwaCopyInput.length() > 0) {
                    cwaCopyInput.remove(cwaCopyInput.length() - 1);
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                    updateCWACopyPracticeUI();
                }
            } else if (key >= 32 && key <= 126 && key != ' ') {
                // Printable character - add to input
                if (cwaCopyInput.length() < 20) {
                    cwaCopyInput += (char)toupper(key);
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                    updateCWACopyPracticeUI();
                }
            }
            break;

        case CWA_COPY_FEEDBACK:
            // Any key continues
            if (cwaCopyRound >= 10) {
                cwaCopyUIState = CWA_COPY_COMPLETE;
                updateCWACopyPracticeUI();
            } else {
                cwaCopyUIState = CWA_COPY_READY;
                cwaCopyInput = "";
                updateCWACopyPracticeUI();
            }
            break;

        case CWA_COPY_COMPLETE:
            // Any key exits - ESC handled at top
            break;

        default:
            break;
    }
}

/*
 * Create CW Academy Copy Practice Screen (Mode 12)
 */
lv_obj_t* createCWAcademyCopyPracticeScreen() {
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Initialize copy practice state
    cwaCopyRound = 0;
    cwaCopyCorrect = 0;
    cwaCopyTotal = 0;
    cwaCopyInput = "";
    cwaCopyUIState = CWA_COPY_READY;

    // Use esp_random() instead of analogRead(0) which breaks I2S audio
    randomSeed(esp_random());

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "CW ACADEMY - COPY PRACTICE");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    createCompactStatusBar(screen);

    // Stats bar
    lv_obj_t* stats_bar = lv_obj_create(screen);
    lv_obj_set_size(stats_bar, SCREEN_WIDTH - 40, 32);
    lv_obj_set_pos(stats_bar, 20, HEADER_HEIGHT + 5);
    applyCardStyle(stats_bar);
    lv_obj_set_style_pad_all(stats_bar, 5, 0);
    lv_obj_clear_flag(stats_bar, LV_OBJ_FLAG_SCROLLABLE);

    cwa_copy_round_label = lv_label_create(stats_bar);
    lv_label_set_text_fmt(cwa_copy_round_label, "Round: %d/10", cwaCopyRound);
    lv_obj_set_style_text_font(cwa_copy_round_label, getThemeFonts()->font_body, 0);
    lv_obj_align(cwa_copy_round_label, LV_ALIGN_LEFT_MID, 10, 0);

    cwa_copy_score_label = lv_label_create(stats_bar);
    lv_label_set_text_fmt(cwa_copy_score_label, "Score: %d/%d", cwaCopyCorrect, cwaCopyTotal);
    lv_obj_set_style_text_font(cwa_copy_score_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(cwa_copy_score_label, LV_COLOR_SUCCESS, 0);
    lv_obj_align(cwa_copy_score_label, LV_ALIGN_CENTER, 0, 0);

    cwa_copy_chars_label = lv_label_create(stats_bar);
    lv_label_set_text_fmt(cwa_copy_chars_label, "Chars: %d", cwaCopyCharCount);
    lv_obj_set_style_text_font(cwa_copy_chars_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(cwa_copy_chars_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(cwa_copy_chars_label, LV_ALIGN_RIGHT_MID, -10, 0);

    // Prompt label
    cwa_copy_prompt_label = lv_label_create(screen);
    lv_label_set_text(cwa_copy_prompt_label, "Press SPACE to hear morse");
    lv_obj_set_style_text_font(cwa_copy_prompt_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(cwa_copy_prompt_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_pos(cwa_copy_prompt_label, 20, HEADER_HEIGHT + 48);

    // Input card
    lv_obj_t* input_card = lv_obj_create(screen);
    lv_obj_set_size(input_card, SCREEN_WIDTH - 40, 60);
    lv_obj_set_pos(input_card, 20, HEADER_HEIGHT + 75);
    applyCardStyle(input_card);
    lv_obj_clear_flag(input_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(input_card, LV_OBJ_FLAG_CLICKABLE);

    // Add key event handler
    lv_obj_add_event_cb(input_card, cwa_copy_key_event_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(input_card);

    cwa_copy_input_label = lv_label_create(input_card);
    lv_label_set_text(cwa_copy_input_label, "_");
    lv_obj_set_style_text_font(cwa_copy_input_label, getThemeFonts()->font_title, 0);
    lv_obj_set_style_text_color(cwa_copy_input_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_center(cwa_copy_input_label);

    // Feedback label
    cwa_copy_feedback_label = lv_label_create(screen);
    lv_label_set_text(cwa_copy_feedback_label, "");
    lv_obj_set_style_text_font(cwa_copy_feedback_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_pos(cwa_copy_feedback_label, 20, HEADER_HEIGHT + 145);
    lv_obj_set_width(cwa_copy_feedback_label, SCREEN_WIDTH - 40);
    lv_obj_set_style_text_align(cwa_copy_feedback_label, LV_TEXT_ALIGN_CENTER, 0);

    // Target label (shows correct answer during feedback)
    cwa_copy_target_label = lv_label_create(screen);
    lv_label_set_text(cwa_copy_target_label, "");
    lv_obj_set_style_text_font(cwa_copy_target_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(cwa_copy_target_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_pos(cwa_copy_target_label, 20, HEADER_HEIGHT + 175);
    lv_obj_set_width(cwa_copy_target_label, SCREEN_WIDTH - 40);
    lv_obj_set_style_text_align(cwa_copy_target_label, LV_TEXT_ALIGN_CENTER, 0);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "SPACE: Play   UP/DN: Chars   ENTER: Submit   ESC: Exit");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    cwa_copy_screen = screen;
    return screen;
}

// ============================================
// CW Academy Sending Practice Screen (Mode 13)
// ============================================

// Sending practice state
enum CWASendState {
    CWA_SEND_READY,       // Ready to start sending
    CWA_SEND_SENDING,     // Student is sending morse
    CWA_SEND_FEEDBACK,    // Showing correct/incorrect feedback
    CWA_SEND_COMPLETE     // Session complete
};

static CWASendState cwa_send_state = CWA_SEND_READY;
static lv_obj_t* cwa_send_screen = NULL;
static lv_obj_t* cwa_send_round_label = NULL;
static lv_obj_t* cwa_send_score_label = NULL;
static lv_obj_t* cwa_send_target_label = NULL;
static lv_obj_t* cwa_send_decoded_label = NULL;
static lv_obj_t* cwa_send_feedback_label = NULL;
static lv_obj_t* cwa_send_hint_label = NULL;
static lv_obj_t* cwa_send_ref_toggle = NULL;

// Keyer state for dual-core operation
static bool cwa_send_last_tone_state = false;   // Track tone state changes
static unsigned long cwa_send_element_start = 0;
static unsigned long cwa_send_last_state_change = 0;
static unsigned long cwa_send_last_element = 0;
static bool cwa_send_sending_dit = false;
static bool cwa_send_sending_dah = false;
static bool cwa_send_keyer_active = false;
static bool cwa_send_in_spacing = false;
static bool cwa_send_dit_memory = false;
static bool cwa_send_dah_memory = false;
static int cwa_send_dit_duration = 0;
static MorseDecoderAdaptive* cwa_send_decoder_ptr = NULL;

/*
 * Initialize sending practice state for LVGL mode
 */
void initCWASendingPractice() {
    cwaSendRound = 0;
    cwaSendCorrect = 0;
    cwaSendTotal = 0;
    cwaSendTarget = "";
    cwaSendDecoded = "";
    cwaSendShowReference = true;
    cwa_send_state = CWA_SEND_READY;

    // Reset keyer state
    cwa_send_last_tone_state = false;
    cwa_send_element_start = 0;
    cwa_send_last_state_change = 0;
    cwa_send_last_element = 0;
    cwa_send_sending_dit = false;
    cwa_send_sending_dah = false;
    cwa_send_keyer_active = false;
    cwa_send_in_spacing = false;
    cwa_send_dit_memory = false;
    cwa_send_dah_memory = false;

    // Set up timing for 15 WPM sending speed
    cwa_send_dit_duration = DIT_DURATION(15);

    // Initialize decoder
    if (cwa_send_decoder_ptr == NULL) {
        cwa_send_decoder_ptr = new MorseDecoderAdaptive(15, 15, 30);
    }
    cwa_send_decoder_ptr->reset();
    cwa_send_decoder_ptr->setWPM(15);

    // Set up decoder callback - uses thread-safe queue
    cwa_send_decoder_ptr->messageCallback = [](String morse, String text) {
        for (int i = 0; i < text.length(); i++) {
            sendDecodedChar(text[i]);  // Thread-safe queue
        }
    };
}

/*
 * Start a new sending round (LVGL version)
 */
void startCWASendRoundLVGL() {
    cwaSendRound++;
    cwaSendTarget = generateCWAContent();
    cwaSendDecoded = "";
    cwa_send_state = CWA_SEND_SENDING;

    // Reset keyer state
    cwa_send_last_tone_state = false;
    cwa_send_element_start = 0;
    cwa_send_last_state_change = 0;
    cwa_send_last_element = 0;
    cwa_send_sending_dit = false;
    cwa_send_sending_dah = false;
    cwa_send_keyer_active = false;
    cwa_send_in_spacing = false;
    cwa_send_dit_memory = false;
    cwa_send_dah_memory = false;

    // Reset decoder
    if (cwa_send_decoder_ptr) {
        cwa_send_decoder_ptr->reset();
        cwa_send_decoder_ptr->flush();
    }

    // Ensure tone is stopped
    requestStopTone();
}

/*
 * Handle iambic keyer with dual-core audio
 */
void handleCWASendingKeyerDualCore() {
    unsigned long currentTime = millis();

    // Get paddle state from audio task (thread-safe)
    bool ditPressed = false;
    bool dahPressed = false;
    getPaddleState(&ditPressed, &dahPressed);

    // If not actively sending or spacing, check for new input
    if (!cwa_send_keyer_active && !cwa_send_in_spacing) {
        if (ditPressed || cwa_send_dit_memory) {
            // Start sending dit
            if (!cwa_send_last_tone_state) {
                if (cwa_send_last_state_change > 0) {
                    float silenceDuration = currentTime - cwa_send_last_state_change;
                    if (silenceDuration > 0 && cwa_send_decoder_ptr) {
                        cwa_send_decoder_ptr->addTiming(-silenceDuration);
                    }
                }
                cwa_send_last_state_change = currentTime;
                cwa_send_last_tone_state = true;
                requestStartTone(cwTone);  // Non-blocking request
            }

            cwa_send_keyer_active = true;
            cwa_send_sending_dit = true;
            cwa_send_sending_dah = false;
            cwa_send_in_spacing = false;
            cwa_send_element_start = currentTime;
            cwa_send_dit_memory = false;
        }
        else if (dahPressed || cwa_send_dah_memory) {
            // Start sending dah
            if (!cwa_send_last_tone_state) {
                if (cwa_send_last_state_change > 0) {
                    float silenceDuration = currentTime - cwa_send_last_state_change;
                    if (silenceDuration > 0 && cwa_send_decoder_ptr) {
                        cwa_send_decoder_ptr->addTiming(-silenceDuration);
                    }
                }
                cwa_send_last_state_change = currentTime;
                cwa_send_last_tone_state = true;
                requestStartTone(cwTone);  // Non-blocking request
            }

            cwa_send_keyer_active = true;
            cwa_send_sending_dit = false;
            cwa_send_sending_dah = true;
            cwa_send_in_spacing = false;
            cwa_send_element_start = currentTime;
            cwa_send_dah_memory = false;
        }
    }
    // Currently sending an element
    else if (cwa_send_keyer_active && !cwa_send_in_spacing) {
        int duration = cwa_send_sending_dit ? cwa_send_dit_duration : (cwa_send_dit_duration * 3);

        // Check for opposite paddle (iambic memory)
        if (cwa_send_sending_dit && dahPressed) {
            cwa_send_dah_memory = true;
        } else if (cwa_send_sending_dah && ditPressed) {
            cwa_send_dit_memory = true;
        }

        // Check if element is complete
        if (currentTime - cwa_send_element_start >= duration) {
            // Send tone duration to decoder
            if (cwa_send_last_tone_state) {
                float toneDuration = currentTime - cwa_send_last_state_change;
                if (toneDuration > 0 && cwa_send_decoder_ptr) {
                    cwa_send_decoder_ptr->addTiming(toneDuration);
                    cwa_send_last_element = currentTime;
                }
                cwa_send_last_state_change = currentTime;
                cwa_send_last_tone_state = false;
            }

            requestStopTone();  // Non-blocking request
            cwa_send_keyer_active = false;
            cwa_send_in_spacing = true;
            cwa_send_element_start = currentTime;
        }
    }
    // In inter-element spacing
    else if (cwa_send_in_spacing) {
        if (currentTime - cwa_send_element_start >= cwa_send_dit_duration) {
            cwa_send_in_spacing = false;
        }
    }
}

/*
 * Handle straight key with dual-core audio
 */
void handleCWASendingStraightKeyDualCore() {
    bool ditPressed = false;
    bool dahPressed = false;
    getPaddleState(&ditPressed, &dahPressed);  // Use dit paddle for straight key

    unsigned long currentTime = millis();
    bool keyDown = ditPressed;

    if (keyDown && !cwa_send_last_tone_state) {
        // Key just pressed
        if (cwa_send_last_state_change > 0) {
            float silenceDuration = currentTime - cwa_send_last_state_change;
            if (silenceDuration > 0 && cwa_send_decoder_ptr) {
                cwa_send_decoder_ptr->addTiming(-silenceDuration);
            }
        }
        cwa_send_last_state_change = currentTime;
        cwa_send_last_tone_state = true;
        requestStartTone(cwTone);
    }
    else if (!keyDown && cwa_send_last_tone_state) {
        // Key just released
        float toneDuration = currentTime - cwa_send_last_state_change;
        if (toneDuration > 0 && cwa_send_decoder_ptr) {
            cwa_send_decoder_ptr->addTiming(toneDuration);
            cwa_send_last_element = currentTime;
        }
        cwa_send_last_state_change = currentTime;
        cwa_send_last_tone_state = false;
        requestStopTone();
    }
}

/*
 * Update sending practice - called from main loop
 */
void updateCWASendingPracticeLVGL() {
    if (cwa_send_state != CWA_SEND_SENDING) return;
    if (!cwa_send_screen) return;

    // Handle keyer based on type
    if (cwKeyType == KEY_STRAIGHT) {
        handleCWASendingStraightKeyDualCore();
    } else {
        handleCWASendingKeyerDualCore();
    }

    // Check for decoded characters from thread-safe queue
    bool decoded_changed = false;
    while (hasDecodedChars()) {
        char c = getDecodedChar();
        if (c != 0) {
            cwaSendDecoded += c;
            decoded_changed = true;
        }
    }

    // Check for decoder timeout (flush after word gap)
    if (cwa_send_last_element > 0) {
        bool ditPressed = false, dahPressed = false;
        getPaddleState(&ditPressed, &dahPressed);

        if (!ditPressed && !dahPressed) {
            unsigned long timeSinceLastElement = millis() - cwa_send_last_element;
            float wordGapDuration = MorseWPM::wordGap(15);

            if (timeSinceLastElement > wordGapDuration) {
                if (cwa_send_decoder_ptr) {
                    cwa_send_decoder_ptr->flush();
                }
                cwa_send_last_element = 0;
            }
        }
    }

    // Update decoded label if changed
    if (decoded_changed && cwa_send_decoded_label) {
        String display = cwaSendDecoded.length() > 0 ? cwaSendDecoded : "_";
        lv_label_set_text(cwa_send_decoded_label, display.c_str());
    }
}

/*
 * Update sending practice UI
 */
void updateCWASendPracticeUI() {
    if (!cwa_send_screen) return;

    // Update round/score labels
    if (cwa_send_round_label) {
        lv_label_set_text_fmt(cwa_send_round_label, "Round: %d/10", cwaSendRound);
    }
    if (cwa_send_score_label) {
        lv_label_set_text_fmt(cwa_send_score_label, "Score: %d/%d", cwaSendCorrect, cwaSendTotal);
    }

    // Update target label
    if (cwa_send_target_label) {
        if (cwaSendShowReference) {
            String target = "Send: " + cwaSendTarget;
            lv_label_set_text(cwa_send_target_label, target.c_str());
            lv_obj_set_style_text_color(cwa_send_target_label, LV_COLOR_SUCCESS, 0);
        } else {
            lv_label_set_text(cwa_send_target_label, "(Reference hidden - press R to show)");
            lv_obj_set_style_text_color(cwa_send_target_label, LV_COLOR_TEXT_SECONDARY, 0);
        }
    }

    // Update decoded label
    if (cwa_send_decoded_label) {
        String display = cwaSendDecoded.length() > 0 ? cwaSendDecoded : "_";
        lv_label_set_text(cwa_send_decoded_label, display.c_str());
    }

    // Update feedback label based on state
    if (cwa_send_feedback_label) {
        switch (cwa_send_state) {
            case CWA_SEND_READY:
                lv_label_set_text(cwa_send_feedback_label, "Press SPACE to hear target, then key your response");
                lv_obj_set_style_text_color(cwa_send_feedback_label, LV_COLOR_ACCENT_CYAN, 0);
                break;
            case CWA_SEND_SENDING:
                lv_label_set_text(cwa_send_feedback_label, "");
                break;
            case CWA_SEND_FEEDBACK: {
                bool correct = cwaSendDecoded.equalsIgnoreCase(cwaSendTarget);
                if (correct) {
                    lv_label_set_text(cwa_send_feedback_label, "CORRECT!");
                    lv_obj_set_style_text_color(cwa_send_feedback_label, LV_COLOR_SUCCESS, 0);
                } else {
                    String fb = "Expected: " + cwaSendTarget;
                    lv_label_set_text(cwa_send_feedback_label, fb.c_str());
                    lv_obj_set_style_text_color(cwa_send_feedback_label, LV_COLOR_ERROR, 0);
                }
                break;
            }
            case CWA_SEND_COMPLETE: {
                int pct = cwaSendTotal > 0 ? (cwaSendCorrect * 100 / cwaSendTotal) : 0;
                char buf[64];
                snprintf(buf, sizeof(buf), "Session Complete! Score: %d%% - Press any key", pct);
                lv_label_set_text(cwa_send_feedback_label, buf);
                lv_obj_set_style_text_color(cwa_send_feedback_label, pct >= 70 ? LV_COLOR_SUCCESS : LV_COLOR_WARNING, 0);
                break;
            }
        }
    }
}

/*
 * Key event handler for sending practice
 */
static void cwa_send_key_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // ESC exits
    if (key == LV_KEY_ESC) {
        requestStopTone();
        if (cwa_send_decoder_ptr) {
            cwa_send_decoder_ptr->reset();
        }
        beep(TONE_MENU_NAV, BEEP_SHORT);
        onLVGLBackNavigation();
        return;
    }

    switch (cwa_send_state) {
        case CWA_SEND_READY:
            // Start first round
            startCWASendRoundLVGL();
            updateCWASendPracticeUI();
            break;

        case CWA_SEND_SENDING:
            if (key == ' ') {
                // Play target
                playMorseString(cwaSendTarget.c_str(), 15, cwTone);
            }
            else if (key == 'R' || key == 'r') {
                // Toggle reference
                cwaSendShowReference = !cwaSendShowReference;
                beep(TONE_MENU_NAV, BEEP_SHORT);
                updateCWASendPracticeUI();
            }
            else if (key == LV_KEY_ENTER) {
                // Submit
                if (cwa_send_decoder_ptr) {
                    cwa_send_decoder_ptr->flush();
                }

                // Check for any remaining decoded chars
                while (hasDecodedChars()) {
                    char c = getDecodedChar();
                    if (c != 0) cwaSendDecoded += c;
                }

                cwaSendTotal++;
                if (cwaSendDecoded.equalsIgnoreCase(cwaSendTarget)) {
                    cwaSendCorrect++;
                    beep(1000, 200);  // Success
                } else {
                    beep(400, 300);   // Error
                }

                requestStopTone();
                cwa_send_state = CWA_SEND_FEEDBACK;
                updateCWASendPracticeUI();
            }
            break;

        case CWA_SEND_FEEDBACK:
            if (cwaSendRound >= 10) {
                cwa_send_state = CWA_SEND_COMPLETE;
                updateCWASendPracticeUI();
            } else {
                startCWASendRoundLVGL();
                updateCWASendPracticeUI();
            }
            break;

        case CWA_SEND_COMPLETE:
            // Exit on any key
            beep(TONE_MENU_NAV, BEEP_SHORT);
            onLVGLBackNavigation();
            break;
    }
}

/*
 * Create CW Academy Sending Practice screen
 */
lv_obj_t* createCWAcademySendingPracticeScreen() {
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Header
    lv_obj_t* header = createHeader(screen, "CWA SENDING PRACTICE");

    // Session info label
    lv_obj_t* session_label = lv_label_create(screen);
    char session_text[64];
    snprintf(session_text, sizeof(session_text), "Session %d - %s",
             cwaSelectedSession, cwaMessageTypeNames[cwaSelectedMessageType]);
    lv_label_set_text(session_label, session_text);
    lv_obj_set_style_text_font(session_label, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(session_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_pos(session_label, 20, HEADER_HEIGHT + 5);

    // Stats bar
    lv_obj_t* stats_bar = lv_obj_create(screen);
    lv_obj_set_size(stats_bar, SCREEN_WIDTH - 40, 30);
    lv_obj_set_pos(stats_bar, 20, HEADER_HEIGHT + 25);
    lv_obj_set_style_bg_opa(stats_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(stats_bar, 0, 0);
    lv_obj_clear_flag(stats_bar, LV_OBJ_FLAG_SCROLLABLE);

    cwa_send_round_label = lv_label_create(stats_bar);
    lv_label_set_text(cwa_send_round_label, "Round: 0/10");
    lv_obj_set_style_text_font(cwa_send_round_label, getThemeFonts()->font_body, 0);
    lv_obj_align(cwa_send_round_label, LV_ALIGN_LEFT_MID, 0, 0);

    cwa_send_score_label = lv_label_create(stats_bar);
    lv_label_set_text(cwa_send_score_label, "Score: 0/0");
    lv_obj_set_style_text_font(cwa_send_score_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(cwa_send_score_label, LV_COLOR_SUCCESS, 0);
    lv_obj_align(cwa_send_score_label, LV_ALIGN_RIGHT_MID, 0, 0);

    // Target card (what to send)
    lv_obj_t* target_card = lv_obj_create(screen);
    lv_obj_set_size(target_card, SCREEN_WIDTH - 40, 50);
    lv_obj_set_pos(target_card, 20, HEADER_HEIGHT + 60);
    applyCardStyle(target_card);
    lv_obj_clear_flag(target_card, LV_OBJ_FLAG_SCROLLABLE);

    cwa_send_target_label = lv_label_create(target_card);
    lv_label_set_text(cwa_send_target_label, "Press any key to start");
    lv_obj_set_style_text_font(cwa_send_target_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(cwa_send_target_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_center(cwa_send_target_label);

    // Decoded display card
    lv_obj_t* decoded_card = lv_obj_create(screen);
    lv_obj_set_size(decoded_card, SCREEN_WIDTH - 40, 50);
    lv_obj_set_pos(decoded_card, 20, HEADER_HEIGHT + 120);
    applyCardStyle(decoded_card);
    lv_obj_clear_flag(decoded_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(decoded_card, LV_OBJ_FLAG_CLICKABLE);

    // Add key event handler
    lv_obj_add_event_cb(decoded_card, cwa_send_key_event_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(decoded_card);

    lv_obj_t* decoded_prefix = lv_label_create(decoded_card);
    lv_label_set_text(decoded_prefix, "You sent:");
    lv_obj_set_style_text_font(decoded_prefix, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(decoded_prefix, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(decoded_prefix, LV_ALIGN_LEFT_MID, 10, -10);

    cwa_send_decoded_label = lv_label_create(decoded_card);
    lv_label_set_text(cwa_send_decoded_label, "_");
    lv_obj_set_style_text_font(cwa_send_decoded_label, getThemeFonts()->font_title, 0);
    lv_obj_set_style_text_color(cwa_send_decoded_label, LV_COLOR_WARNING, 0);
    lv_obj_align(cwa_send_decoded_label, LV_ALIGN_LEFT_MID, 80, 0);

    // Feedback label
    cwa_send_feedback_label = lv_label_create(screen);
    lv_label_set_text(cwa_send_feedback_label, "Press any key to start");
    lv_obj_set_style_text_font(cwa_send_feedback_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(cwa_send_feedback_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_pos(cwa_send_feedback_label, 20, HEADER_HEIGHT + 180);
    lv_obj_set_width(cwa_send_feedback_label, SCREEN_WIDTH - 40);
    lv_obj_set_style_text_align(cwa_send_feedback_label, LV_TEXT_ALIGN_CENTER, 0);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "SPACE: Hear target   R: Toggle ref   ENTER: Submit   ESC: Exit");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    // Initialize state
    initCWASendingPractice();

    cwa_send_screen = screen;
    return screen;
}

// ============================================
// CW Academy QSO Practice Screen (Mode 14)
// ============================================

// QSO Practice state
enum CWAQSOState {
    CWA_QSO_READY,        // Ready to start
    CWA_QSO_PLAYING,      // Playing exchange
    CWA_QSO_INPUT,        // Waiting for user response
    CWA_QSO_FEEDBACK,     // Showing feedback
    CWA_QSO_COMPLETE      // Session complete
};

static CWAQSOState cwa_qso_state = CWA_QSO_READY;
static lv_obj_t* cwa_qso_screen = NULL;
static lv_obj_t* cwa_qso_round_label = NULL;
static lv_obj_t* cwa_qso_station_label = NULL;
static lv_obj_t* cwa_qso_prompt_label = NULL;
static lv_obj_t* cwa_qso_input_label = NULL;
static lv_obj_t* cwa_qso_feedback_label = NULL;

// QSO practice variables
static int cwa_qso_round = 0;
static int cwa_qso_correct = 0;
static int cwa_qso_total = 0;
static String cwa_qso_input = "";
static String cwa_qso_expected = "";
static String cwa_qso_station_call = "";
static String cwa_qso_station_name = "";
static int cwa_qso_rst = 599;

// Simulated station data
static const char* cwa_qso_callsigns[] = {"W1AW", "K3LR", "N6BV", "W5KFT", "K9LA", "W0AIH", "N2IC", "K5ZD"};
static const char* cwa_qso_names[] = {"JOHN", "BOB", "MARY", "TOM", "JANE", "MIKE", "SUE", "BILL"};
static const int cwa_qso_num_stations = 8;

/*
 * Initialize QSO practice
 */
void initCWAQSOPractice() {
    cwa_qso_round = 0;
    cwa_qso_correct = 0;
    cwa_qso_total = 0;
    cwa_qso_input = "";
    cwa_qso_expected = "";
    cwa_qso_state = CWA_QSO_READY;

    // Pick a random station
    randomSeed(esp_random());
    int idx = random(cwa_qso_num_stations);
    cwa_qso_station_call = cwa_qso_callsigns[idx];
    cwa_qso_station_name = cwa_qso_names[idx];
    cwa_qso_rst = 559 + random(4) * 10;  // 559, 569, 579, 589, or 599
}

/*
 * Start a new QSO round
 */
void startCWAQSORound() {
    cwa_qso_round++;
    cwa_qso_input = "";
    cwa_qso_state = CWA_QSO_PLAYING;

    // Generate exchange based on round
    // Rounds 1-3: CQ exchange, Rounds 4-6: RST exchange, Rounds 7-10: Full exchange
    if (cwa_qso_round <= 3) {
        // CQ exchange: Station calls CQ, expect your call
        cwa_qso_expected = vailCallsign.length() > 0 && vailCallsign != "GUEST" ? vailCallsign : "W1TEST";
    } else if (cwa_qso_round <= 6) {
        // RST exchange: Station sends RST, expect 599 back
        cwa_qso_expected = "599";
    } else {
        // Full exchange: Station sends name, expect your name
        cwa_qso_expected = "OP";  // Placeholder - user should send their name
    }
}

/*
 * Update QSO practice UI
 */
void updateCWAQSOPracticeUI() {
    if (!cwa_qso_screen) return;

    if (cwa_qso_round_label) {
        lv_label_set_text_fmt(cwa_qso_round_label, "Round: %d/10  Score: %d/%d",
                              cwa_qso_round, cwa_qso_correct, cwa_qso_total);
    }

    if (cwa_qso_station_label) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Station: %s (%s)",
                 cwa_qso_station_call.c_str(), cwa_qso_station_name.c_str());
        lv_label_set_text(cwa_qso_station_label, buf);
    }

    if (cwa_qso_prompt_label) {
        switch (cwa_qso_state) {
            case CWA_QSO_READY:
                lv_label_set_text(cwa_qso_prompt_label, "Press any key to start QSO practice");
                break;
            case CWA_QSO_PLAYING:
                lv_label_set_text(cwa_qso_prompt_label, "Listening to exchange...");
                break;
            case CWA_QSO_INPUT:
                lv_label_set_text(cwa_qso_prompt_label, "Type your response:");
                break;
            case CWA_QSO_FEEDBACK:
            case CWA_QSO_COMPLETE:
                break;
        }
    }

    if (cwa_qso_input_label) {
        String display = cwa_qso_input.length() > 0 ? cwa_qso_input : "_";
        lv_label_set_text(cwa_qso_input_label, display.c_str());
    }

    if (cwa_qso_feedback_label) {
        switch (cwa_qso_state) {
            case CWA_QSO_FEEDBACK: {
                bool correct = cwa_qso_input.equalsIgnoreCase(cwa_qso_expected);
                if (correct) {
                    lv_label_set_text(cwa_qso_feedback_label, "CORRECT! Press any key to continue");
                    lv_obj_set_style_text_color(cwa_qso_feedback_label, LV_COLOR_SUCCESS, 0);
                } else {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "Expected: %s - Press any key", cwa_qso_expected.c_str());
                    lv_label_set_text(cwa_qso_feedback_label, buf);
                    lv_obj_set_style_text_color(cwa_qso_feedback_label, LV_COLOR_ERROR, 0);
                }
                break;
            }
            case CWA_QSO_COMPLETE: {
                int pct = cwa_qso_total > 0 ? (cwa_qso_correct * 100 / cwa_qso_total) : 0;
                char buf[64];
                snprintf(buf, sizeof(buf), "QSO Complete! Score: %d%% - Press any key", pct);
                lv_label_set_text(cwa_qso_feedback_label, buf);
                lv_obj_set_style_text_color(cwa_qso_feedback_label,
                                            pct >= 70 ? LV_COLOR_SUCCESS : LV_COLOR_WARNING, 0);
                break;
            }
            default:
                lv_label_set_text(cwa_qso_feedback_label, "");
                break;
        }
    }
}

/*
 * Play QSO exchange audio
 */
void playCWAQSOExchange() {
    String exchange = "";

    if (cwa_qso_round <= 3) {
        // CQ exchange
        exchange = "CQ CQ CQ DE " + cwa_qso_station_call + " " + cwa_qso_station_call + " K";
    } else if (cwa_qso_round <= 6) {
        // RST exchange
        char rst[8];
        snprintf(rst, sizeof(rst), "%d", cwa_qso_rst);
        exchange = "UR RST " + String(rst) + " " + String(rst);
    } else {
        // Name exchange
        exchange = "NAME " + cwa_qso_station_name + " " + cwa_qso_station_name;
    }

    playMorseString(exchange.c_str(), cwSpeed, cwTone);
    cwa_qso_state = CWA_QSO_INPUT;
    updateCWAQSOPracticeUI();
}

/*
 * Key event handler for QSO practice
 */
static void cwa_qso_key_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // ESC exits
    if (key == LV_KEY_ESC) {
        beep(TONE_MENU_NAV, BEEP_SHORT);
        onLVGLBackNavigation();
        return;
    }

    switch (cwa_qso_state) {
        case CWA_QSO_READY:
            startCWAQSORound();
            playCWAQSOExchange();
            break;

        case CWA_QSO_PLAYING:
            // Wait for audio to finish
            break;

        case CWA_QSO_INPUT:
            if (key == ' ') {
                // Replay exchange
                playCWAQSOExchange();
            }
            else if (key == LV_KEY_ENTER) {
                // Submit answer
                cwa_qso_total++;
                if (cwa_qso_input.equalsIgnoreCase(cwa_qso_expected)) {
                    cwa_qso_correct++;
                    beep(1000, 200);
                } else {
                    beep(400, 300);
                }
                cwa_qso_state = CWA_QSO_FEEDBACK;
                updateCWAQSOPracticeUI();
            }
            else if (key == 0x08 || key == 0x7F) {
                // Backspace
                if (cwa_qso_input.length() > 0) {
                    cwa_qso_input.remove(cwa_qso_input.length() - 1);
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                    updateCWAQSOPracticeUI();
                }
            }
            else if (key >= 32 && key <= 126) {
                // Printable character
                if (cwa_qso_input.length() < 20) {
                    cwa_qso_input += (char)toupper(key);
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                    updateCWAQSOPracticeUI();
                }
            }
            break;

        case CWA_QSO_FEEDBACK:
            if (cwa_qso_round >= 10) {
                cwa_qso_state = CWA_QSO_COMPLETE;
                updateCWAQSOPracticeUI();
            } else {
                // Pick new station for variety
                int idx = random(cwa_qso_num_stations);
                cwa_qso_station_call = cwa_qso_callsigns[idx];
                cwa_qso_station_name = cwa_qso_names[idx];
                cwa_qso_rst = 559 + random(4) * 10;

                startCWAQSORound();
                playCWAQSOExchange();
            }
            break;

        case CWA_QSO_COMPLETE:
            // Exit on any key
            beep(TONE_MENU_NAV, BEEP_SHORT);
            onLVGLBackNavigation();
            break;
    }
}

/*
 * Create CW Academy QSO Practice screen
 */
lv_obj_t* createCWAcademyQSOPracticeScreen() {
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Header
    lv_obj_t* header = createHeader(screen, "CWA QSO PRACTICE");

    // Round/score label
    cwa_qso_round_label = lv_label_create(screen);
    lv_label_set_text(cwa_qso_round_label, "Round: 0/10  Score: 0/0");
    lv_obj_set_style_text_font(cwa_qso_round_label, getThemeFonts()->font_body, 0);
    lv_obj_set_pos(cwa_qso_round_label, 20, HEADER_HEIGHT + 10);

    // Station info
    cwa_qso_station_label = lv_label_create(screen);
    lv_label_set_text(cwa_qso_station_label, "Station: W1AW (JOHN)");
    lv_obj_set_style_text_font(cwa_qso_station_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(cwa_qso_station_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_pos(cwa_qso_station_label, 20, HEADER_HEIGHT + 35);

    // Prompt label
    cwa_qso_prompt_label = lv_label_create(screen);
    lv_label_set_text(cwa_qso_prompt_label, "Press any key to start QSO practice");
    lv_obj_set_style_text_font(cwa_qso_prompt_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(cwa_qso_prompt_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_pos(cwa_qso_prompt_label, 20, HEADER_HEIGHT + 65);

    // Input card
    lv_obj_t* input_card = lv_obj_create(screen);
    lv_obj_set_size(input_card, SCREEN_WIDTH - 40, 60);
    lv_obj_set_pos(input_card, 20, HEADER_HEIGHT + 95);
    applyCardStyle(input_card);
    lv_obj_clear_flag(input_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(input_card, LV_OBJ_FLAG_CLICKABLE);

    // Add key event handler
    lv_obj_add_event_cb(input_card, cwa_qso_key_event_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(input_card);

    cwa_qso_input_label = lv_label_create(input_card);
    lv_label_set_text(cwa_qso_input_label, "_");
    lv_obj_set_style_text_font(cwa_qso_input_label, getThemeFonts()->font_title, 0);
    lv_obj_set_style_text_color(cwa_qso_input_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_center(cwa_qso_input_label);

    // Feedback label
    cwa_qso_feedback_label = lv_label_create(screen);
    lv_label_set_text(cwa_qso_feedback_label, "");
    lv_obj_set_style_text_font(cwa_qso_feedback_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_pos(cwa_qso_feedback_label, 20, HEADER_HEIGHT + 165);
    lv_obj_set_width(cwa_qso_feedback_label, SCREEN_WIDTH - 40);
    lv_obj_set_style_text_align(cwa_qso_feedback_label, LV_TEXT_ALIGN_CENTER, 0);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "SPACE: Replay   ENTER: Submit   ESC: Exit");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    // Initialize state
    initCWAQSOPractice();
    updateCWAQSOPracticeUI();

    cwa_qso_screen = screen;
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
                // Normal state - reset all feedback styling to menu card defaults
                lv_obj_set_style_bg_color(license_answer_btns[i], LV_COLOR_CARD_TEAL, 0);
                lv_obj_set_style_bg_opa(license_answer_btns[i], LV_OPA_COVER, 0);
                lv_obj_set_style_border_color(license_answer_btns[i], LV_COLOR_BORDER_SUBTLE, 0);
                lv_obj_set_style_border_width(license_answer_btns[i], 2, 0);
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
    // Check for Vail Master modes first (70-74)
    lv_obj_t* vmScreen = createVailMasterScreenForMode(mode);
    if (vmScreen != NULL) return vmScreen;

    switch (mode) {
        case 6:  // MODE_PRACTICE
            return createPracticeScreen();
        case 2:  // MODE_HEAR_IT_MENU
        case 3:  // MODE_HEAR_IT_TYPE_IT
            return createHearItTypeItScreen();
        case 7:  // MODE_KOCH_METHOD
            return createKochMethodScreen();
        case 67: // MODE_KOCH_PRACTICE
            return createKochPracticeScreen();
        case 68: // MODE_KOCH_SETTINGS
            return createKochSettingsScreen();
        case 69: // MODE_KOCH_STATISTICS
            return createKochStatisticsScreen();
        case 75: // MODE_KOCH_HELP
            return createKochHelpScreen();
        case 76: // MODE_KOCH_CHAR_REF
            return createKochCharRefScreen();
        case 77: // MODE_KOCH_NEW_CHAR
            return createKochNewCharScreen();
        case 8:  // MODE_CW_ACADEMY_TRACK_SELECT
            return createCWAcademyTrackSelectScreen();
        case 9:  // MODE_CW_ACADEMY_SESSION_SELECT
            return createCWAcademySessionSelectScreen();
        case 10: // MODE_CW_ACADEMY_PRACTICE_TYPE_SELECT
            return createCWAcademyPracticeTypeSelectScreen();
        case 11: // MODE_CW_ACADEMY_MESSAGE_TYPE_SELECT
            return createCWAcademyMessageTypeSelectScreen();
        case 12: // MODE_CW_ACADEMY_COPY_PRACTICE
            return createCWAcademyCopyPracticeScreen();
        case 13: // MODE_CW_ACADEMY_SENDING_PRACTICE
            return createCWAcademySendingPracticeScreen();
        case 14: // MODE_CW_ACADEMY_QSO_PRACTICE
            return createCWAcademyQSOPracticeScreen();
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
            break;
    }

    // Check for LICW Training modes (120-133)
    lv_obj_t* licwScreen = createLICWScreenForMode(mode);
    if (licwScreen != NULL) return licwScreen;

    Serial.printf("[TrainingScreens] Unknown training mode: %d\n", mode);
    return NULL;
}

#endif // LV_TRAINING_SCREENS_H
