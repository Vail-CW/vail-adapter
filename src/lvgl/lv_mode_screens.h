/*
 * VAIL SUMMIT - LVGL Mode Screens
 * Covers Radio, Vail Repeater, QSO Logger, Bluetooth, and other modes
 */

#ifndef LV_MODE_SCREENS_H
#define LV_MODE_SCREENS_H

#include <lvgl.h>
#include "lv_theme_summit.h"
#include "lv_widgets_summit.h"
#include "lv_screen_manager.h"
#include "../core/config.h"

// Forward declaration for back navigation
extern void onLVGLBackNavigation();

// ============================================
// Radio Output Screen
// ============================================

// External references for radio settings
extern int cwSpeed;
extern int cwTone;
extern KeyType cwKeyType;
extern void saveCWSettings();

// Forward declarations for radio functions (defined in radio_output.h)
extern bool queueRadioMessage(const char* message);

// Radio mode - use values from radio_output.h (already included before this file)
// RadioMode enum: RADIO_MODE_SUMMIT_KEYER, RADIO_MODE_RADIO_KEYER
extern RadioMode radioMode;
extern void saveRadioSettings();

// CW Memories - use values from radio_cw_memories.h (already included before this file)
// CWMemoryPreset struct and CW_MEMORY_MAX_SLOTS defined there
extern CWMemoryPreset cwMemories[];

// Radio screen state
static lv_obj_t* radio_screen = NULL;
static lv_obj_t* radio_mode_label = NULL;
static lv_obj_t* radio_status_label = NULL;
static lv_obj_t* radio_wpm_label = NULL;
static lv_obj_t* radio_tone_label = NULL;
static lv_obj_t* radio_keytype_label = NULL;

// Action bar buttons
static lv_obj_t* radio_btn_mode = NULL;
static lv_obj_t* radio_btn_settings = NULL;
static lv_obj_t* radio_btn_memories = NULL;
static int radio_action_focus = 0;  // 0=Mode, 1=Settings, 2=Memories

// Overlay state
static lv_obj_t* radio_overlay = NULL;
static bool radio_settings_active = false;
static bool radio_memories_active = false;
static int radio_settings_selection = 0;  // 0=WPM, 1=KeyType, 2=Tone
static int radio_memory_selection = 0;    // 0-9 for memory slots

// Forward declarations
void createRadioSettingsOverlay();
void createRadioMemoriesOverlay();
void closeRadioOverlay();
void updateRadioSettingsDisplay();
void updateRadioMemoriesDisplay();

// Helper to get key type string
const char* getKeyTypeString(KeyType type) {
    switch(type) {
        case KEY_STRAIGHT: return "Straight";
        case KEY_IAMBIC_A: return "Iambic A";
        case KEY_IAMBIC_B: return "Iambic B";
        default: return "Unknown";
    }
}

// Update action bar button focus styling
void updateRadioActionBarFocus() {
    // Reset all buttons
    if (radio_btn_mode) {
        lv_obj_set_style_border_color(radio_btn_mode, LV_COLOR_BORDER_SUBTLE, 0);
        lv_obj_set_style_border_width(radio_btn_mode, 1, 0);
    }
    if (radio_btn_settings) {
        lv_obj_set_style_border_color(radio_btn_settings, LV_COLOR_BORDER_SUBTLE, 0);
        lv_obj_set_style_border_width(radio_btn_settings, 1, 0);
    }
    if (radio_btn_memories) {
        lv_obj_set_style_border_color(radio_btn_memories, LV_COLOR_BORDER_SUBTLE, 0);
        lv_obj_set_style_border_width(radio_btn_memories, 1, 0);
    }

    // Highlight focused button
    lv_obj_t* focused = NULL;
    switch(radio_action_focus) {
        case 0: focused = radio_btn_mode; break;
        case 1: focused = radio_btn_settings; break;
        case 2: focused = radio_btn_memories; break;
    }
    if (focused) {
        lv_obj_set_style_border_color(focused, LV_COLOR_ACCENT_CYAN, 0);
        lv_obj_set_style_border_width(focused, 2, 0);
    }
}

// Settings overlay row labels for highlighting
static lv_obj_t* settings_row_wpm = NULL;
static lv_obj_t* settings_row_keytype = NULL;
static lv_obj_t* settings_row_tone = NULL;
static lv_obj_t* settings_val_wpm = NULL;
static lv_obj_t* settings_val_keytype = NULL;
static lv_obj_t* settings_val_tone = NULL;

// Memory overlay display elements
static lv_obj_t* memory_rows[5] = {NULL};  // Show 5 at a time
static lv_obj_t* memory_labels[5] = {NULL};
static int memory_scroll_offset = 0;

void updateRadioSettingsDisplay() {
    // Update selection highlight
    lv_color_t normal_bg = LV_COLOR_BG_LAYER2;
    lv_color_t selected_bg = getThemeColors()->card_secondary;  // Theme highlight

    if (settings_row_wpm) {
        lv_obj_set_style_bg_color(settings_row_wpm,
            radio_settings_selection == 0 ? selected_bg : normal_bg, 0);
    }
    if (settings_row_keytype) {
        lv_obj_set_style_bg_color(settings_row_keytype,
            radio_settings_selection == 1 ? selected_bg : normal_bg, 0);
    }
    if (settings_row_tone) {
        lv_obj_set_style_bg_color(settings_row_tone,
            radio_settings_selection == 2 ? selected_bg : normal_bg, 0);
    }

    // Update values
    if (settings_val_wpm) lv_label_set_text_fmt(settings_val_wpm, "%d", cwSpeed);
    if (settings_val_keytype) lv_label_set_text(settings_val_keytype, getKeyTypeString(cwKeyType));
    if (settings_val_tone) lv_label_set_text_fmt(settings_val_tone, "%d Hz", cwTone);
}

void updateRadioMemoriesDisplay() {
    // Calculate scroll offset
    if (radio_memory_selection >= memory_scroll_offset + 5) {
        memory_scroll_offset = radio_memory_selection - 4;
    } else if (radio_memory_selection < memory_scroll_offset) {
        memory_scroll_offset = radio_memory_selection;
    }

    for (int i = 0; i < 5; i++) {
        int slot = memory_scroll_offset + i;
        if (slot >= CW_MEMORY_MAX_SLOTS) break;

        bool isSelected = (slot == radio_memory_selection);

        if (memory_rows[i]) {
            lv_obj_set_style_bg_color(memory_rows[i],
                isSelected ? getThemeColors()->card_secondary : LV_COLOR_BG_LAYER2, 0);
        }

        if (memory_labels[i]) {
            char buf[32];
            if (cwMemories[slot].isEmpty) {
                snprintf(buf, sizeof(buf), "%d. (empty)", slot + 1);
                lv_obj_set_style_text_color(memory_labels[i],
                    isSelected ? LV_COLOR_TEXT_SECONDARY : LV_COLOR_TEXT_DISABLED, 0);
            } else {
                snprintf(buf, sizeof(buf), "%d. %s", slot + 1, cwMemories[slot].label);
                lv_obj_set_style_text_color(memory_labels[i],
                    isSelected ? LV_COLOR_TEXT_PRIMARY : LV_COLOR_ACCENT_CYAN, 0);
            }
            lv_label_set_text(memory_labels[i], buf);
        }
    }
}

void closeRadioOverlay() {
    if (radio_overlay) {
        lv_obj_del(radio_overlay);
        radio_overlay = NULL;
    }
    radio_settings_active = false;
    radio_memories_active = false;

    // Clear row references
    settings_row_wpm = NULL;
    settings_row_keytype = NULL;
    settings_row_tone = NULL;
    settings_val_wpm = NULL;
    settings_val_keytype = NULL;
    settings_val_tone = NULL;
    for (int i = 0; i < 5; i++) {
        memory_rows[i] = NULL;
        memory_labels[i] = NULL;
    }
}

void createRadioSettingsOverlay() {
    if (radio_overlay) return;  // Already open

    radio_settings_active = true;
    radio_settings_selection = 0;

    // Create overlay container
    radio_overlay = lv_obj_create(radio_screen);
    lv_obj_set_size(radio_overlay, 320, 220);
    lv_obj_center(radio_overlay);
    lv_obj_set_style_bg_color(radio_overlay, LV_COLOR_BG_DEEP, 0);
    lv_obj_set_style_bg_opa(radio_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(radio_overlay, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_border_width(radio_overlay, 2, 0);
    lv_obj_set_style_radius(radio_overlay, 12, 0);
    lv_obj_set_style_pad_all(radio_overlay, 15, 0);
    lv_obj_clear_flag(radio_overlay, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* title = lv_label_create(radio_overlay);
    lv_label_set_text(title, "KEYER SETTINGS");
    lv_obj_set_style_text_color(title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // WPM row
    settings_row_wpm = lv_obj_create(radio_overlay);
    lv_obj_set_size(settings_row_wpm, 280, 40);
    lv_obj_set_pos(settings_row_wpm, 5, 35);
    lv_obj_set_style_bg_color(settings_row_wpm, getThemeColors()->card_secondary, 0);
    lv_obj_set_style_bg_opa(settings_row_wpm, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(settings_row_wpm, 6, 0);
    lv_obj_set_style_border_width(settings_row_wpm, 0, 0);
    lv_obj_set_style_pad_hor(settings_row_wpm, 10, 0);
    lv_obj_clear_flag(settings_row_wpm, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* wpm_lbl = lv_label_create(settings_row_wpm);
    lv_label_set_text(wpm_lbl, "Speed (WPM)");
    lv_obj_set_style_text_color(wpm_lbl, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(wpm_lbl, LV_ALIGN_LEFT_MID, 0, 0);

    settings_val_wpm = lv_label_create(settings_row_wpm);
    lv_label_set_text_fmt(settings_val_wpm, "%d", cwSpeed);
    lv_obj_set_style_text_color(settings_val_wpm, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(settings_val_wpm, getThemeFonts()->font_input, 0);
    lv_obj_align(settings_val_wpm, LV_ALIGN_RIGHT_MID, 0, 0);

    // Key Type row
    settings_row_keytype = lv_obj_create(radio_overlay);
    lv_obj_set_size(settings_row_keytype, 280, 40);
    lv_obj_set_pos(settings_row_keytype, 5, 80);
    lv_obj_set_style_bg_color(settings_row_keytype, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_bg_opa(settings_row_keytype, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(settings_row_keytype, 6, 0);
    lv_obj_set_style_border_width(settings_row_keytype, 0, 0);
    lv_obj_set_style_pad_hor(settings_row_keytype, 10, 0);
    lv_obj_clear_flag(settings_row_keytype, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* keytype_lbl = lv_label_create(settings_row_keytype);
    lv_label_set_text(keytype_lbl, "Key Type");
    lv_obj_set_style_text_color(keytype_lbl, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(keytype_lbl, LV_ALIGN_LEFT_MID, 0, 0);

    settings_val_keytype = lv_label_create(settings_row_keytype);
    lv_label_set_text(settings_val_keytype, getKeyTypeString(cwKeyType));
    lv_obj_set_style_text_color(settings_val_keytype, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(settings_val_keytype, getThemeFonts()->font_input, 0);
    lv_obj_align(settings_val_keytype, LV_ALIGN_RIGHT_MID, 0, 0);

    // Tone row
    settings_row_tone = lv_obj_create(radio_overlay);
    lv_obj_set_size(settings_row_tone, 280, 40);
    lv_obj_set_pos(settings_row_tone, 5, 125);
    lv_obj_set_style_bg_color(settings_row_tone, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_bg_opa(settings_row_tone, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(settings_row_tone, 6, 0);
    lv_obj_set_style_border_width(settings_row_tone, 0, 0);
    lv_obj_set_style_pad_hor(settings_row_tone, 10, 0);
    lv_obj_clear_flag(settings_row_tone, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* tone_lbl = lv_label_create(settings_row_tone);
    lv_label_set_text(tone_lbl, "Sidetone");
    lv_obj_set_style_text_color(tone_lbl, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(tone_lbl, LV_ALIGN_LEFT_MID, 0, 0);

    settings_val_tone = lv_label_create(settings_row_tone);
    lv_label_set_text_fmt(settings_val_tone, "%d Hz", cwTone);
    lv_obj_set_style_text_color(settings_val_tone, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(settings_val_tone, getThemeFonts()->font_input, 0);
    lv_obj_align(settings_val_tone, LV_ALIGN_RIGHT_MID, 0, 0);

    // Footer hint
    lv_obj_t* hint = lv_label_create(radio_overlay);
    lv_label_set_text(hint, "UP/DN Select   L/R Adjust   ESC Close");
    lv_obj_set_style_text_color(hint, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(hint, getThemeFonts()->font_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, 0);
}

void createRadioMemoriesOverlay() {
    if (radio_overlay) return;  // Already open

    radio_memories_active = true;
    radio_memory_selection = 0;
    memory_scroll_offset = 0;

    // Create overlay container
    radio_overlay = lv_obj_create(radio_screen);
    lv_obj_set_size(radio_overlay, 320, 220);
    lv_obj_center(radio_overlay);
    lv_obj_set_style_bg_color(radio_overlay, LV_COLOR_BG_DEEP, 0);
    lv_obj_set_style_bg_opa(radio_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(radio_overlay, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_border_width(radio_overlay, 2, 0);
    lv_obj_set_style_radius(radio_overlay, 12, 0);
    lv_obj_set_style_pad_all(radio_overlay, 15, 0);
    lv_obj_clear_flag(radio_overlay, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* title = lv_label_create(radio_overlay);
    lv_label_set_text(title, "CW MEMORIES");
    lv_obj_set_style_text_color(title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // Create 5 visible rows
    for (int i = 0; i < 5; i++) {
        memory_rows[i] = lv_obj_create(radio_overlay);
        lv_obj_set_size(memory_rows[i], 280, 30);
        lv_obj_set_pos(memory_rows[i], 5, 30 + (i * 32));
        lv_obj_set_style_bg_color(memory_rows[i], LV_COLOR_BG_LAYER2, 0);
        lv_obj_set_style_bg_opa(memory_rows[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(memory_rows[i], 4, 0);
        lv_obj_set_style_border_width(memory_rows[i], 0, 0);
        lv_obj_set_style_pad_hor(memory_rows[i], 10, 0);
        lv_obj_clear_flag(memory_rows[i], LV_OBJ_FLAG_SCROLLABLE);

        memory_labels[i] = lv_label_create(memory_rows[i]);
        lv_label_set_text(memory_labels[i], "");
        lv_obj_set_style_text_font(memory_labels[i], getThemeFonts()->font_body, 0);
        lv_obj_align(memory_labels[i], LV_ALIGN_LEFT_MID, 0, 0);
    }

    // Footer hint
    lv_obj_t* hint = lv_label_create(radio_overlay);
    lv_label_set_text(hint, "UP/DN Select   ENTER Send   ESC Close");
    lv_obj_set_style_text_color(hint, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(hint, getThemeFonts()->font_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Initialize display
    updateRadioMemoriesDisplay();
}

// Key event callback for Radio Output - handles action bar and overlays
static void radio_key_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    Serial.printf("[Radio LVGL] Key event: %lu (0x%02lX)\n", key, key);

    // Handle settings overlay
    if (radio_settings_active) {
        switch(key) {
            case LV_KEY_ESC:
                closeRadioOverlay();
                beep(TONE_MENU_NAV, BEEP_SHORT);
                lv_event_stop_processing(e);
                return;
            case LV_KEY_UP:
            case LV_KEY_PREV:
                if (radio_settings_selection > 0) {
                    radio_settings_selection--;
                    updateRadioSettingsDisplay();
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                }
                lv_event_stop_processing(e);
                return;
            case LV_KEY_DOWN:
            case LV_KEY_NEXT:
                if (radio_settings_selection < 2) {
                    radio_settings_selection++;
                    updateRadioSettingsDisplay();
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                }
                lv_event_stop_processing(e);
                return;
            case LV_KEY_LEFT:
                // Decrease value
                if (radio_settings_selection == 0) {
                    // WPM
                    if (cwSpeed > 5) {
                        cwSpeed--;
                        saveCWSettings();
                        updateRadioSettingsDisplay();
                        if (radio_wpm_label) lv_label_set_text_fmt(radio_wpm_label, "%d WPM", cwSpeed);
                        beep(TONE_MENU_NAV, BEEP_SHORT);
                    }
                } else if (radio_settings_selection == 1) {
                    // Key Type
                    if (cwKeyType == KEY_STRAIGHT) cwKeyType = KEY_IAMBIC_B;
                    else if (cwKeyType == KEY_IAMBIC_A) cwKeyType = KEY_STRAIGHT;
                    else cwKeyType = KEY_IAMBIC_A;
                    saveCWSettings();
                    updateRadioSettingsDisplay();
                    if (radio_keytype_label) lv_label_set_text(radio_keytype_label, getKeyTypeString(cwKeyType));
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                } else if (radio_settings_selection == 2) {
                    // Tone
                    if (cwTone > 400) {
                        cwTone -= 50;
                        saveCWSettings();
                        updateRadioSettingsDisplay();
                        if (radio_tone_label) lv_label_set_text_fmt(radio_tone_label, "%d Hz", cwTone);
                        beep(TONE_MENU_NAV, BEEP_SHORT);
                    }
                }
                lv_event_stop_processing(e);
                return;
            case LV_KEY_RIGHT:
                // Increase value
                if (radio_settings_selection == 0) {
                    // WPM
                    if (cwSpeed < 40) {
                        cwSpeed++;
                        saveCWSettings();
                        updateRadioSettingsDisplay();
                        if (radio_wpm_label) lv_label_set_text_fmt(radio_wpm_label, "%d WPM", cwSpeed);
                        beep(TONE_MENU_NAV, BEEP_SHORT);
                    }
                } else if (radio_settings_selection == 1) {
                    // Key Type
                    if (cwKeyType == KEY_STRAIGHT) cwKeyType = KEY_IAMBIC_A;
                    else if (cwKeyType == KEY_IAMBIC_A) cwKeyType = KEY_IAMBIC_B;
                    else cwKeyType = KEY_STRAIGHT;
                    saveCWSettings();
                    updateRadioSettingsDisplay();
                    if (radio_keytype_label) lv_label_set_text(radio_keytype_label, getKeyTypeString(cwKeyType));
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                } else if (radio_settings_selection == 2) {
                    // Tone
                    if (cwTone < 1000) {
                        cwTone += 50;
                        saveCWSettings();
                        updateRadioSettingsDisplay();
                        if (radio_tone_label) lv_label_set_text_fmt(radio_tone_label, "%d Hz", cwTone);
                        beep(TONE_MENU_NAV, BEEP_SHORT);
                    }
                }
                lv_event_stop_processing(e);
                return;
        }
        return;
    }

    // Handle memories overlay
    if (radio_memories_active) {
        switch(key) {
            case LV_KEY_ESC:
                closeRadioOverlay();
                beep(TONE_MENU_NAV, BEEP_SHORT);
                lv_event_stop_processing(e);
                return;
            case LV_KEY_UP:
            case LV_KEY_PREV:
                if (radio_memory_selection > 0) {
                    radio_memory_selection--;
                    updateRadioMemoriesDisplay();
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                }
                lv_event_stop_processing(e);
                return;
            case LV_KEY_DOWN:
            case LV_KEY_NEXT:
                if (radio_memory_selection < CW_MEMORY_MAX_SLOTS - 1) {
                    radio_memory_selection++;
                    updateRadioMemoriesDisplay();
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                }
                lv_event_stop_processing(e);
                return;
            case LV_KEY_ENTER:
                // Send selected memory
                if (!cwMemories[radio_memory_selection].isEmpty) {
                    bool success = queueRadioMessage(cwMemories[radio_memory_selection].message);
                    closeRadioOverlay();
                    if (success) {
                        beep(TONE_SUCCESS, BEEP_MEDIUM);
                        if (radio_status_label) {
                            lv_label_set_text(radio_status_label, "Sending memory...");
                        }
                    } else {
                        beep(TONE_ERROR, BEEP_SHORT);
                    }
                } else {
                    beep(TONE_ERROR, BEEP_SHORT);
                }
                lv_event_stop_processing(e);
                return;
        }
        return;
    }

    // Handle main screen action bar
    switch(key) {
        case LV_KEY_ESC:
            onLVGLBackNavigation();
            lv_event_stop_processing(e);
            break;
        case LV_KEY_LEFT:
            if (radio_action_focus > 0) {
                radio_action_focus--;
                updateRadioActionBarFocus();
                beep(TONE_MENU_NAV, BEEP_SHORT);
            }
            lv_event_stop_processing(e);
            break;
        case LV_KEY_RIGHT:
            if (radio_action_focus < 2) {
                radio_action_focus++;
                updateRadioActionBarFocus();
                beep(TONE_MENU_NAV, BEEP_SHORT);
            }
            lv_event_stop_processing(e);
            break;
        case LV_KEY_ENTER:
            // Activate focused action
            if (radio_action_focus == 0) {
                // Toggle mode
                if (radioMode == RADIO_MODE_SUMMIT_KEYER) {
                    radioMode = RADIO_MODE_RADIO_KEYER;
                } else {
                    radioMode = RADIO_MODE_SUMMIT_KEYER;
                }
                saveRadioSettings();
                if (radio_mode_label) {
                    lv_label_set_text(radio_mode_label,
                        radioMode == RADIO_MODE_SUMMIT_KEYER ? "Summit Keyer" : "Radio Keyer");
                }
                beep(TONE_SUCCESS, BEEP_SHORT);
            } else if (radio_action_focus == 1) {
                // Open settings
                createRadioSettingsOverlay();
                beep(TONE_SELECT, BEEP_SHORT);
            } else if (radio_action_focus == 2) {
                // Open memories
                createRadioMemoriesOverlay();
                beep(TONE_SELECT, BEEP_SHORT);
            }
            lv_event_stop_processing(e);
            break;
    }
}

lv_obj_t* createRadioOutputScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "RADIO OUTPUT");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Mode card - shows current keyer mode
    lv_obj_t* mode_card = lv_obj_create(screen);
    lv_obj_set_size(mode_card, SCREEN_WIDTH - 40, 70);
    lv_obj_set_pos(mode_card, 20, HEADER_HEIGHT + 10);
    lv_obj_set_layout(mode_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(mode_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mode_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    applyCardStyle(mode_card);

    lv_obj_t* mode_title = lv_label_create(mode_card);
    lv_label_set_text(mode_title, "Keyer Mode");
    lv_obj_add_style(mode_title, getStyleLabelBody(), 0);

    radio_mode_label = lv_label_create(mode_card);
    lv_label_set_text(radio_mode_label, radioMode == RADIO_MODE_SUMMIT_KEYER ? "Summit Keyer" : "Radio Keyer");
    lv_obj_set_style_text_font(radio_mode_label, getThemeFonts()->font_title, 0);
    lv_obj_set_style_text_color(radio_mode_label, LV_COLOR_ACCENT_CYAN, 0);

    // Settings display row - WPM, Key Type, Tone
    lv_obj_t* settings_card = lv_obj_create(screen);
    lv_obj_set_size(settings_card, SCREEN_WIDTH - 40, 50);
    lv_obj_set_pos(settings_card, 20, HEADER_HEIGHT + 90);
    lv_obj_set_layout(settings_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(settings_card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(settings_card, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    applyCardStyle(settings_card);

    // WPM display
    lv_obj_t* wpm_container = lv_obj_create(settings_card);
    lv_obj_set_size(wpm_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(wpm_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wpm_container, 0, 0);
    lv_obj_set_style_pad_all(wpm_container, 0, 0);
    lv_obj_clear_flag(wpm_container, LV_OBJ_FLAG_SCROLLABLE);

    radio_wpm_label = lv_label_create(wpm_container);
    lv_label_set_text_fmt(radio_wpm_label, "%d WPM", cwSpeed);
    lv_obj_set_style_text_color(radio_wpm_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(radio_wpm_label, getThemeFonts()->font_body, 0);

    // Key Type display
    lv_obj_t* keytype_container = lv_obj_create(settings_card);
    lv_obj_set_size(keytype_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(keytype_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(keytype_container, 0, 0);
    lv_obj_set_style_pad_all(keytype_container, 0, 0);
    lv_obj_clear_flag(keytype_container, LV_OBJ_FLAG_SCROLLABLE);

    radio_keytype_label = lv_label_create(keytype_container);
    lv_label_set_text(radio_keytype_label, getKeyTypeString(cwKeyType));
    lv_obj_set_style_text_color(radio_keytype_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(radio_keytype_label, getThemeFonts()->font_body, 0);

    // Tone display
    lv_obj_t* tone_container = lv_obj_create(settings_card);
    lv_obj_set_size(tone_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(tone_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tone_container, 0, 0);
    lv_obj_set_style_pad_all(tone_container, 0, 0);
    lv_obj_clear_flag(tone_container, LV_OBJ_FLAG_SCROLLABLE);

    radio_tone_label = lv_label_create(tone_container);
    lv_label_set_text_fmt(radio_tone_label, "%d Hz", cwTone);
    lv_obj_set_style_text_color(radio_tone_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(radio_tone_label, getThemeFonts()->font_body, 0);

    // Status text
    radio_status_label = lv_label_create(screen);
    lv_label_set_text(radio_status_label, "Ready - Use paddle to key radio");
    lv_obj_add_style(radio_status_label, getStyleLabelBody(), 0);
    lv_obj_set_pos(radio_status_label, 20, HEADER_HEIGHT + 150);

    // Action bar container
    lv_obj_t* action_bar = lv_obj_create(screen);
    lv_obj_set_size(action_bar, SCREEN_WIDTH - 40, 50);
    lv_obj_set_pos(action_bar, 20, SCREEN_HEIGHT - FOOTER_HEIGHT - 60);
    lv_obj_set_layout(action_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(action_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(action_bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(action_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(action_bar, 0, 0);
    lv_obj_set_style_pad_all(action_bar, 0, 0);
    lv_obj_clear_flag(action_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Mode button
    radio_btn_mode = lv_obj_create(action_bar);
    lv_obj_set_size(radio_btn_mode, 120, 40);
    lv_obj_set_style_bg_color(radio_btn_mode, LV_COLOR_CARD_TEAL, 0);
    lv_obj_set_style_radius(radio_btn_mode, 8, 0);
    lv_obj_set_style_border_color(radio_btn_mode, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_border_width(radio_btn_mode, 2, 0);
    lv_obj_clear_flag(radio_btn_mode, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* mode_btn_lbl = lv_label_create(radio_btn_mode);
    lv_label_set_text(mode_btn_lbl, "Mode");
    lv_obj_set_style_text_color(mode_btn_lbl, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(mode_btn_lbl, getThemeFonts()->font_body, 0);
    lv_obj_center(mode_btn_lbl);

    // Settings button
    radio_btn_settings = lv_obj_create(action_bar);
    lv_obj_set_size(radio_btn_settings, 120, 40);
    lv_obj_set_style_bg_color(radio_btn_settings, LV_COLOR_CARD_TEAL, 0);
    lv_obj_set_style_radius(radio_btn_settings, 8, 0);
    lv_obj_set_style_border_color(radio_btn_settings, LV_COLOR_BORDER_SUBTLE, 0);
    lv_obj_set_style_border_width(radio_btn_settings, 1, 0);
    lv_obj_clear_flag(radio_btn_settings, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* settings_btn_lbl = lv_label_create(radio_btn_settings);
    lv_label_set_text(settings_btn_lbl, "Settings");
    lv_obj_set_style_text_color(settings_btn_lbl, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(settings_btn_lbl, getThemeFonts()->font_body, 0);
    lv_obj_center(settings_btn_lbl);

    // Memories button
    radio_btn_memories = lv_obj_create(action_bar);
    lv_obj_set_size(radio_btn_memories, 120, 40);
    lv_obj_set_style_bg_color(radio_btn_memories, LV_COLOR_CARD_TEAL, 0);
    lv_obj_set_style_radius(radio_btn_memories, 8, 0);
    lv_obj_set_style_border_color(radio_btn_memories, LV_COLOR_BORDER_SUBTLE, 0);
    lv_obj_set_style_border_width(radio_btn_memories, 1, 0);
    lv_obj_clear_flag(radio_btn_memories, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* memories_btn_lbl = lv_label_create(radio_btn_memories);
    lv_label_set_text(memories_btn_lbl, "Memories");
    lv_obj_set_style_text_color(memories_btn_lbl, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(memories_btn_lbl, getThemeFonts()->font_body, 0);
    lv_obj_center(memories_btn_lbl);

    // Reset action bar focus state
    radio_action_focus = 0;

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "L/R Select   ENTER Activate   ESC Exit");
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
    lv_obj_add_event_cb(focus_container, radio_key_event_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus_container);
    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }
    lv_group_focus_obj(focus_container);

    radio_screen = screen;

    // Reset overlay state
    radio_overlay = NULL;
    radio_settings_active = false;
    radio_memories_active = false;

    return screen;
}

void updateRadioMode(const char* mode) {
    if (radio_mode_label != NULL) {
        lv_label_set_text(radio_mode_label, mode);
    }
}

void updateRadioWPM(int wpm) {
    if (radio_wpm_label != NULL) {
        lv_label_set_text_fmt(radio_wpm_label, "%d WPM", wpm);
    }
}

void updateRadioStatus(const char* status) {
    if (radio_status_label != NULL) {
        lv_label_set_text(radio_status_label, status);
    }
}

void cleanupRadioOutputScreen() {
    closeRadioOverlay();
    radio_screen = NULL;
    radio_mode_label = NULL;
    radio_status_label = NULL;
    radio_wpm_label = NULL;
    radio_tone_label = NULL;
    radio_keytype_label = NULL;
    radio_btn_mode = NULL;
    radio_btn_settings = NULL;
    radio_btn_memories = NULL;
}

// ============================================
// CW Memories Screen - Full LVGL Implementation
// ============================================

// Forward declarations
extern void previewCWMemory(int slot);
extern void saveCWMemory(int slot);
extern void deleteCWMemory(int slot);
extern bool isValidMorseMessage(const char* message);

// CW Memories screen state
static lv_obj_t* cwmem_screen = NULL;
static lv_obj_t* cwmem_rows[5] = {NULL};      // Show 5 slots at a time
static lv_obj_t* cwmem_labels[5] = {NULL};    // Labels for each visible row
static int cwmem_selection = 0;               // Currently selected slot (0-9)
static int cwmem_scroll_offset = 0;           // Scroll offset for displaying

// Overlay state
static lv_obj_t* cwmem_overlay = NULL;
static bool cwmem_context_active = false;     // Context menu open
static bool cwmem_edit_active = false;        // Edit overlay open
static bool cwmem_delete_active = false;      // Delete confirmation open
static int cwmem_context_selection = 0;       // Context menu selection

// Edit state
static bool cwmem_editing_label = true;       // true=label, false=message
static char cwmem_edit_label[16] = {0};       // Label buffer (max 15 chars)
static char cwmem_edit_message[101] = {0};    // Message buffer (max 100 chars)
static lv_obj_t* cwmem_edit_textarea = NULL;
static lv_obj_t* cwmem_edit_title = NULL;
static lv_obj_t* cwmem_edit_prompt = NULL;
static lv_obj_t* cwmem_edit_counter = NULL;

// Forward declarations
void updateCWMemoriesDisplay();
void closeCWMemOverlay();
void createCWMemContextMenu();
void createCWMemEditOverlay();
void createCWMemDeleteConfirm();
static void cwmem_key_event_cb(lv_event_t* e);

// Update the visible memory slots display
void updateCWMemoriesDisplay() {
    // Adjust scroll offset if selection moves outside visible range
    if (cwmem_selection >= cwmem_scroll_offset + 5) {
        cwmem_scroll_offset = cwmem_selection - 4;
    } else if (cwmem_selection < cwmem_scroll_offset) {
        cwmem_scroll_offset = cwmem_selection;
    }

    for (int i = 0; i < 5; i++) {
        int slot = cwmem_scroll_offset + i;
        if (slot >= CW_MEMORY_MAX_SLOTS) break;

        bool isSelected = (slot == cwmem_selection);

        if (cwmem_rows[i]) {
            lv_obj_set_style_bg_color(cwmem_rows[i],
                isSelected ? getThemeColors()->card_secondary : LV_COLOR_BG_LAYER2, 0);
            lv_obj_set_style_border_color(cwmem_rows[i],
                isSelected ? LV_COLOR_ACCENT_CYAN : LV_COLOR_BORDER_SUBTLE, 0);
            lv_obj_set_style_border_width(cwmem_rows[i], isSelected ? 2 : 1, 0);
        }

        if (cwmem_labels[i]) {
            char buf[40];
            if (cwMemories[slot].isEmpty) {
                snprintf(buf, sizeof(buf), "%d.  (Empty)", slot + 1);
                lv_obj_set_style_text_color(cwmem_labels[i],
                    isSelected ? LV_COLOR_TEXT_SECONDARY : LV_COLOR_TEXT_DISABLED, 0);
            } else {
                snprintf(buf, sizeof(buf), "%d.  %s", slot + 1, cwMemories[slot].label);
                lv_obj_set_style_text_color(cwmem_labels[i],
                    isSelected ? LV_COLOR_TEXT_PRIMARY : LV_COLOR_ACCENT_CYAN, 0);
            }
            lv_label_set_text(cwmem_labels[i], buf);
        }
    }
}

// Close any open overlay
void closeCWMemOverlay() {
    if (cwmem_overlay) {
        lv_obj_del(cwmem_overlay);
        cwmem_overlay = NULL;
    }
    cwmem_context_active = false;
    cwmem_edit_active = false;
    cwmem_delete_active = false;
    cwmem_edit_textarea = NULL;
    cwmem_edit_title = NULL;
    cwmem_edit_prompt = NULL;
    cwmem_edit_counter = NULL;
}

// Create context menu overlay
void createCWMemContextMenu() {
    if (cwmem_overlay) return;

    cwmem_context_active = true;
    cwmem_context_selection = 0;

    bool isEmpty = cwMemories[cwmem_selection].isEmpty;
    int numOptions = isEmpty ? 2 : 4;
    int overlayHeight = isEmpty ? 120 : 180;

    // Create overlay
    cwmem_overlay = lv_obj_create(cwmem_screen);
    lv_obj_set_size(cwmem_overlay, 260, overlayHeight);
    lv_obj_center(cwmem_overlay);
    lv_obj_set_style_bg_color(cwmem_overlay, LV_COLOR_BG_DEEP, 0);
    lv_obj_set_style_bg_opa(cwmem_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(cwmem_overlay, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_border_width(cwmem_overlay, 2, 0);
    lv_obj_set_style_radius(cwmem_overlay, 12, 0);
    lv_obj_set_style_pad_all(cwmem_overlay, 15, 0);
    lv_obj_clear_flag(cwmem_overlay, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* title = lv_label_create(cwmem_overlay);
    if (isEmpty) {
        lv_label_set_text_fmt(title, "SLOT %d - EMPTY", cwmem_selection + 1);
    } else {
        lv_label_set_text_fmt(title, "SLOT %d", cwmem_selection + 1);
    }
    lv_obj_set_style_text_color(title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // Show label if not empty
    if (!isEmpty) {
        lv_obj_t* label = lv_label_create(cwmem_overlay);
        lv_label_set_text(label, cwMemories[cwmem_selection].label);
        lv_obj_set_style_text_color(label, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_style_text_font(label, getThemeFonts()->font_body, 0);
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 22);
    }

    // Menu options
    const char* options[4];
    if (isEmpty) {
        options[0] = "Create";
        options[1] = "Cancel";
    } else {
        options[0] = "Preview";
        options[1] = "Edit";
        options[2] = "Delete";
        options[3] = "Cancel";
    }

    int startY = isEmpty ? 35 : 50;
    for (int i = 0; i < numOptions; i++) {
        lv_obj_t* opt = lv_label_create(cwmem_overlay);
        lv_label_set_text(opt, options[i]);
        lv_obj_set_style_text_font(opt, getThemeFonts()->font_body, 0);
        lv_obj_set_style_text_color(opt,
            (i == cwmem_context_selection) ? LV_COLOR_TEXT_PRIMARY : LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_align(opt, LV_ALIGN_TOP_MID, 0, startY + (i * 28));
    }

    // Footer hint
    lv_obj_t* hint = lv_label_create(cwmem_overlay);
    lv_label_set_text(hint, LV_SYMBOL_UP LV_SYMBOL_DOWN " Select   ENTER Confirm   ESC Back");
    lv_obj_set_style_text_color(hint, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(hint, getThemeFonts()->font_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, 0);
}

// Update context menu selection display
void updateCWMemContextDisplay() {
    if (!cwmem_overlay || !cwmem_context_active) return;

    bool isEmpty = cwMemories[cwmem_selection].isEmpty;
    int numOptions = isEmpty ? 2 : 4;
    int startY = isEmpty ? 35 : 50;

    const char* options[4];
    if (isEmpty) {
        options[0] = "Create";
        options[1] = "Cancel";
    } else {
        options[0] = "Preview";
        options[1] = "Edit";
        options[2] = "Delete";
        options[3] = "Cancel";
    }

    // Recreate overlay to update highlighting
    closeCWMemOverlay();
    cwmem_context_active = true;

    int overlayHeight = isEmpty ? 120 : 180;

    cwmem_overlay = lv_obj_create(cwmem_screen);
    lv_obj_set_size(cwmem_overlay, 260, overlayHeight);
    lv_obj_center(cwmem_overlay);
    lv_obj_set_style_bg_color(cwmem_overlay, LV_COLOR_BG_DEEP, 0);
    lv_obj_set_style_bg_opa(cwmem_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(cwmem_overlay, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_border_width(cwmem_overlay, 2, 0);
    lv_obj_set_style_radius(cwmem_overlay, 12, 0);
    lv_obj_set_style_pad_all(cwmem_overlay, 15, 0);
    lv_obj_clear_flag(cwmem_overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(cwmem_overlay);
    if (isEmpty) {
        lv_label_set_text_fmt(title, "SLOT %d - EMPTY", cwmem_selection + 1);
    } else {
        lv_label_set_text_fmt(title, "SLOT %d", cwmem_selection + 1);
    }
    lv_obj_set_style_text_color(title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    if (!isEmpty) {
        lv_obj_t* label = lv_label_create(cwmem_overlay);
        lv_label_set_text(label, cwMemories[cwmem_selection].label);
        lv_obj_set_style_text_color(label, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_style_text_font(label, getThemeFonts()->font_body, 0);
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 22);
    }

    for (int i = 0; i < numOptions; i++) {
        lv_obj_t* opt = lv_label_create(cwmem_overlay);
        lv_label_set_text(opt, options[i]);
        lv_obj_set_style_text_font(opt, getThemeFonts()->font_body, 0);
        if (i == cwmem_context_selection) {
            lv_obj_set_style_text_color(opt, LV_COLOR_TEXT_PRIMARY, 0);
            // Add selection indicator
            lv_label_set_text_fmt(opt, "> %s", options[i]);
        } else {
            lv_obj_set_style_text_color(opt, LV_COLOR_TEXT_SECONDARY, 0);
        }
        lv_obj_align(opt, LV_ALIGN_TOP_MID, 0, startY + (i * 28));
    }

    lv_obj_t* hint = lv_label_create(cwmem_overlay);
    lv_label_set_text(hint, LV_SYMBOL_UP LV_SYMBOL_DOWN " Select   ENTER Confirm   ESC Back");
    lv_obj_set_style_text_color(hint, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(hint, getThemeFonts()->font_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, 0);
}

// Create edit overlay for label or message
void createCWMemEditOverlay() {
    if (cwmem_overlay) return;

    cwmem_edit_active = true;
    cwmem_editing_label = true;

    // If creating new, clear buffers; if editing, load existing
    if (cwMemories[cwmem_selection].isEmpty) {
        cwmem_edit_label[0] = '\0';
        cwmem_edit_message[0] = '\0';
    } else {
        strlcpy(cwmem_edit_label, cwMemories[cwmem_selection].label, sizeof(cwmem_edit_label));
        strlcpy(cwmem_edit_message, cwMemories[cwmem_selection].message, sizeof(cwmem_edit_message));
    }

    cwmem_overlay = lv_obj_create(cwmem_screen);
    lv_obj_set_size(cwmem_overlay, 380, 200);
    lv_obj_center(cwmem_overlay);
    lv_obj_set_style_bg_color(cwmem_overlay, LV_COLOR_BG_DEEP, 0);
    lv_obj_set_style_bg_opa(cwmem_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(cwmem_overlay, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_border_width(cwmem_overlay, 2, 0);
    lv_obj_set_style_radius(cwmem_overlay, 12, 0);
    lv_obj_set_style_pad_all(cwmem_overlay, 15, 0);
    lv_obj_clear_flag(cwmem_overlay, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    cwmem_edit_title = lv_label_create(cwmem_overlay);
    bool isNew = cwMemories[cwmem_selection].isEmpty;
    lv_label_set_text(cwmem_edit_title, isNew ? "CREATE PRESET" : "EDIT PRESET");
    lv_obj_set_style_text_color(cwmem_edit_title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(cwmem_edit_title, getThemeFonts()->font_subtitle, 0);
    lv_obj_align(cwmem_edit_title, LV_ALIGN_TOP_MID, 0, 0);

    // Prompt
    cwmem_edit_prompt = lv_label_create(cwmem_overlay);
    lv_label_set_text(cwmem_edit_prompt, "Label (max 15 chars):");
    lv_obj_set_style_text_color(cwmem_edit_prompt, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(cwmem_edit_prompt, getThemeFonts()->font_body, 0);
    lv_obj_align(cwmem_edit_prompt, LV_ALIGN_TOP_LEFT, 5, 30);

    // Text area
    cwmem_edit_textarea = lv_textarea_create(cwmem_overlay);
    lv_obj_set_size(cwmem_edit_textarea, 340, 70);
    lv_obj_align(cwmem_edit_textarea, LV_ALIGN_TOP_MID, 0, 55);
    lv_textarea_set_one_line(cwmem_edit_textarea, true);
    lv_textarea_set_max_length(cwmem_edit_textarea, 15);
    lv_textarea_set_text(cwmem_edit_textarea, cwmem_edit_label);
    lv_obj_set_style_bg_color(cwmem_edit_textarea, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_border_color(cwmem_edit_textarea, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_border_width(cwmem_edit_textarea, 1, 0);
    lv_obj_set_style_text_color(cwmem_edit_textarea, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(cwmem_edit_textarea, getThemeFonts()->font_input, 0);

    // Character counter
    cwmem_edit_counter = lv_label_create(cwmem_overlay);
    lv_label_set_text_fmt(cwmem_edit_counter, "%d / 15 chars", strlen(cwmem_edit_label));
    lv_obj_set_style_text_color(cwmem_edit_counter, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_style_text_font(cwmem_edit_counter, getThemeFonts()->font_small, 0);
    lv_obj_align(cwmem_edit_counter, LV_ALIGN_TOP_LEFT, 5, 130);

    // Footer hint
    lv_obj_t* hint = lv_label_create(cwmem_overlay);
    lv_label_set_text(hint, "Type text   ENTER Next/Save   ESC Cancel");
    lv_obj_set_style_text_color(hint, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(hint, getThemeFonts()->font_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, 0);
}

// Switch to message editing
void switchToMessageEdit() {
    if (!cwmem_overlay || !cwmem_edit_active) return;

    cwmem_editing_label = false;

    // Update prompt
    if (cwmem_edit_prompt) {
        lv_label_set_text(cwmem_edit_prompt, "Message (max 100 chars):");
    }

    // Update textarea
    if (cwmem_edit_textarea) {
        lv_textarea_set_one_line(cwmem_edit_textarea, false);
        lv_textarea_set_max_length(cwmem_edit_textarea, 100);
        lv_textarea_set_text(cwmem_edit_textarea, cwmem_edit_message);
        lv_obj_set_size(cwmem_edit_textarea, 340, 70);
    }

    // Update counter
    if (cwmem_edit_counter) {
        lv_label_set_text_fmt(cwmem_edit_counter, "%d / 100 chars", strlen(cwmem_edit_message));
    }
}

// Update character counter during editing
void updateCWMemEditCounter() {
    if (!cwmem_edit_counter || !cwmem_edit_textarea) return;

    const char* text = lv_textarea_get_text(cwmem_edit_textarea);
    int len = strlen(text);

    if (cwmem_editing_label) {
        lv_label_set_text_fmt(cwmem_edit_counter, "%d / 15 chars", len);
    } else {
        lv_label_set_text_fmt(cwmem_edit_counter, "%d / 100 chars", len);
    }
}

// Create delete confirmation overlay
void createCWMemDeleteConfirm() {
    if (cwmem_overlay) return;

    cwmem_delete_active = true;
    cwmem_context_selection = 1;  // Default to "No"

    cwmem_overlay = lv_obj_create(cwmem_screen);
    lv_obj_set_size(cwmem_overlay, 280, 160);
    lv_obj_center(cwmem_overlay);
    lv_obj_set_style_bg_color(cwmem_overlay, LV_COLOR_BG_DEEP, 0);
    lv_obj_set_style_bg_opa(cwmem_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(cwmem_overlay, LV_COLOR_ERROR, 0);
    lv_obj_set_style_border_width(cwmem_overlay, 2, 0);
    lv_obj_set_style_radius(cwmem_overlay, 12, 0);
    lv_obj_set_style_pad_all(cwmem_overlay, 15, 0);
    lv_obj_clear_flag(cwmem_overlay, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* title = lv_label_create(cwmem_overlay);
    lv_label_set_text(title, "DELETE PRESET?");
    lv_obj_set_style_text_color(title, LV_COLOR_ERROR, 0);
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // Label being deleted
    lv_obj_t* label = lv_label_create(cwmem_overlay);
    lv_label_set_text_fmt(label, "\"%s\"", cwMemories[cwmem_selection].label);
    lv_obj_set_style_text_color(label, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(label, getThemeFonts()->font_body, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 28);

    // Warning
    lv_obj_t* warn = lv_label_create(cwmem_overlay);
    lv_label_set_text(warn, "This cannot be undone");
    lv_obj_set_style_text_color(warn, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_style_text_font(warn, getThemeFonts()->font_small, 0);
    lv_obj_align(warn, LV_ALIGN_TOP_MID, 0, 50);

    // Yes button
    lv_obj_t* yes = lv_label_create(cwmem_overlay);
    lv_label_set_text(yes, cwmem_context_selection == 0 ? "> Yes, Delete" : "  Yes, Delete");
    lv_obj_set_style_text_color(yes,
        cwmem_context_selection == 0 ? LV_COLOR_ERROR : LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(yes, getThemeFonts()->font_body, 0);
    lv_obj_align(yes, LV_ALIGN_TOP_MID, 0, 75);

    // No button
    lv_obj_t* no = lv_label_create(cwmem_overlay);
    lv_label_set_text(no, cwmem_context_selection == 1 ? "> No, Cancel" : "  No, Cancel");
    lv_obj_set_style_text_color(no,
        cwmem_context_selection == 1 ? LV_COLOR_TEXT_PRIMARY : LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(no, getThemeFonts()->font_body, 0);
    lv_obj_align(no, LV_ALIGN_TOP_MID, 0, 100);

    // Footer hint
    lv_obj_t* hint = lv_label_create(cwmem_overlay);
    lv_label_set_text(hint, LV_SYMBOL_UP LV_SYMBOL_DOWN " Select   ENTER Confirm");
    lv_obj_set_style_text_color(hint, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(hint, getThemeFonts()->font_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, 0);
}

// Update delete confirmation display
void updateCWMemDeleteDisplay() {
    // Recreate the overlay with updated selection
    closeCWMemOverlay();
    createCWMemDeleteConfirm();
}

// Key event handler for CW Memories screen
static void cwmem_key_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    Serial.printf("[CWMem LVGL] Key event: %lu (0x%02lX)\n", key, key);

    // Handle delete confirmation
    if (cwmem_delete_active) {
        switch(key) {
            case LV_KEY_ESC:
                closeCWMemOverlay();
                beep(TONE_MENU_NAV, BEEP_SHORT);
                lv_event_stop_processing(e);
                return;
            case LV_KEY_UP:
            case LV_KEY_PREV:
            case LV_KEY_DOWN:
            case LV_KEY_NEXT:
                cwmem_context_selection = (cwmem_context_selection == 0) ? 1 : 0;
                updateCWMemDeleteDisplay();
                beep(TONE_MENU_NAV, BEEP_SHORT);
                lv_event_stop_processing(e);
                return;
            case LV_KEY_ENTER:
                if (cwmem_context_selection == 0) {
                    // Delete confirmed
                    deleteCWMemory(cwmem_selection);
                    beep(TONE_SUCCESS, BEEP_MEDIUM);
                } else {
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                }
                closeCWMemOverlay();
                updateCWMemoriesDisplay();
                lv_event_stop_processing(e);
                return;
        }
        return;
    }

    // Handle edit mode
    if (cwmem_edit_active) {
        switch(key) {
            case LV_KEY_ESC:
                closeCWMemOverlay();
                beep(TONE_MENU_NAV, BEEP_SHORT);
                lv_event_stop_processing(e);
                return;
            case LV_KEY_ENTER:
                if (cwmem_editing_label) {
                    // Save label and switch to message
                    const char* text = lv_textarea_get_text(cwmem_edit_textarea);
                    if (strlen(text) == 0) {
                        beep(TONE_ERROR, BEEP_SHORT);
                        lv_event_stop_processing(e);
                        return;
                    }
                    strlcpy(cwmem_edit_label, text, sizeof(cwmem_edit_label));
                    switchToMessageEdit();
                    beep(TONE_SELECT, BEEP_SHORT);
                } else {
                    // Save message and complete
                    const char* text = lv_textarea_get_text(cwmem_edit_textarea);
                    if (strlen(text) == 0) {
                        beep(TONE_ERROR, BEEP_SHORT);
                        lv_event_stop_processing(e);
                        return;
                    }
                    if (!isValidMorseMessage(text)) {
                        beep(TONE_ERROR, BEEP_LONG);
                        lv_event_stop_processing(e);
                        return;
                    }
                    strlcpy(cwmem_edit_message, text, sizeof(cwmem_edit_message));

                    // Save to memory slot
                    strlcpy(cwMemories[cwmem_selection].label, cwmem_edit_label, 16);
                    strlcpy(cwMemories[cwmem_selection].message, cwmem_edit_message, 101);
                    cwMemories[cwmem_selection].isEmpty = false;
                    saveCWMemory(cwmem_selection);

                    closeCWMemOverlay();
                    updateCWMemoriesDisplay();
                    beep(TONE_SUCCESS, BEEP_MEDIUM);
                }
                lv_event_stop_processing(e);
                return;
            case LV_KEY_BACKSPACE:
                // Handle backspace
                if (cwmem_edit_textarea) {
                    lv_textarea_del_char(cwmem_edit_textarea);
                    updateCWMemEditCounter();
                }
                lv_event_stop_processing(e);
                return;
            default:
                // Handle printable characters (32-126)
                if (key >= 32 && key <= 126) {
                    if (cwmem_edit_textarea) {
                        // Auto-uppercase for morse consistency
                        char c = (char)key;
                        if (c >= 'a' && c <= 'z') {
                            c = c - 'a' + 'A';
                        }
                        char str[2] = {c, '\0'};
                        lv_textarea_add_text(cwmem_edit_textarea, str);
                        updateCWMemEditCounter();
                    }
                    lv_event_stop_processing(e);
                }
                return;
        }
        return;
    }

    // Handle context menu
    if (cwmem_context_active) {
        bool isEmpty = cwMemories[cwmem_selection].isEmpty;
        int maxOptions = isEmpty ? 2 : 4;

        switch(key) {
            case LV_KEY_ESC:
                closeCWMemOverlay();
                beep(TONE_MENU_NAV, BEEP_SHORT);
                lv_event_stop_processing(e);
                return;
            case LV_KEY_UP:
            case LV_KEY_PREV:
                if (cwmem_context_selection > 0) {
                    cwmem_context_selection--;
                    updateCWMemContextDisplay();
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                }
                lv_event_stop_processing(e);
                return;
            case LV_KEY_DOWN:
            case LV_KEY_NEXT:
                if (cwmem_context_selection < maxOptions - 1) {
                    cwmem_context_selection++;
                    updateCWMemContextDisplay();
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                }
                lv_event_stop_processing(e);
                return;
            case LV_KEY_ENTER:
                if (isEmpty) {
                    // Empty slot: Create or Cancel
                    if (cwmem_context_selection == 0) {
                        closeCWMemOverlay();
                        createCWMemEditOverlay();
                        beep(TONE_SELECT, BEEP_SHORT);
                    } else {
                        closeCWMemOverlay();
                        beep(TONE_MENU_NAV, BEEP_SHORT);
                    }
                } else {
                    // Occupied slot: Preview, Edit, Delete, Cancel
                    switch(cwmem_context_selection) {
                        case 0:  // Preview
                            closeCWMemOverlay();
                            previewCWMemory(cwmem_selection);
                            beep(TONE_SELECT, BEEP_SHORT);
                            break;
                        case 1:  // Edit
                            closeCWMemOverlay();
                            createCWMemEditOverlay();
                            beep(TONE_SELECT, BEEP_SHORT);
                            break;
                        case 2:  // Delete
                            closeCWMemOverlay();
                            createCWMemDeleteConfirm();
                            beep(TONE_MENU_NAV, BEEP_SHORT);
                            break;
                        case 3:  // Cancel
                            closeCWMemOverlay();
                            beep(TONE_MENU_NAV, BEEP_SHORT);
                            break;
                    }
                }
                lv_event_stop_processing(e);
                return;
        }
        return;
    }

    // Handle main screen navigation
    switch(key) {
        case LV_KEY_ESC:
            onLVGLBackNavigation();
            lv_event_stop_processing(e);
            break;
        case LV_KEY_UP:
        case LV_KEY_PREV:
            if (cwmem_selection > 0) {
                cwmem_selection--;
                updateCWMemoriesDisplay();
                beep(TONE_MENU_NAV, BEEP_SHORT);
            }
            lv_event_stop_processing(e);
            break;
        case LV_KEY_DOWN:
        case LV_KEY_NEXT:
            if (cwmem_selection < CW_MEMORY_MAX_SLOTS - 1) {
                cwmem_selection++;
                updateCWMemoriesDisplay();
                beep(TONE_MENU_NAV, BEEP_SHORT);
            }
            lv_event_stop_processing(e);
            break;
        case LV_KEY_ENTER:
            createCWMemContextMenu();
            beep(TONE_SELECT, BEEP_SHORT);
            lv_event_stop_processing(e);
            break;
    }
}

lv_obj_t* createCWMemoriesScreen() {
    // Reset state
    cwmem_selection = 0;
    cwmem_scroll_offset = 0;
    cwmem_context_active = false;
    cwmem_edit_active = false;
    cwmem_delete_active = false;
    cwmem_overlay = NULL;

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "CW MEMORIES");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Memory slots container (5 visible)
    lv_obj_t* list = lv_obj_create(screen);
    lv_obj_set_size(list, SCREEN_WIDTH - 40, 200);  // Height for 5 rows
    lv_obj_set_pos(list, 20, HEADER_HEIGHT + 10);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 5, 0);
    lv_obj_set_style_pad_all(list, 5, 0);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_clear_flag(list, LV_OBJ_FLAG_SCROLLABLE);

    // Create 5 visible slot rows
    for (int i = 0; i < 5; i++) {
        cwmem_rows[i] = lv_obj_create(list);
        lv_obj_set_size(cwmem_rows[i], SCREEN_WIDTH - 60, 36);
        lv_obj_set_style_bg_color(cwmem_rows[i], LV_COLOR_BG_LAYER2, 0);
        lv_obj_set_style_bg_opa(cwmem_rows[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(cwmem_rows[i], 6, 0);
        lv_obj_set_style_border_color(cwmem_rows[i], LV_COLOR_BORDER_SUBTLE, 0);
        lv_obj_set_style_border_width(cwmem_rows[i], 1, 0);
        lv_obj_set_style_pad_hor(cwmem_rows[i], 12, 0);
        lv_obj_clear_flag(cwmem_rows[i], LV_OBJ_FLAG_SCROLLABLE);

        cwmem_labels[i] = lv_label_create(cwmem_rows[i]);
        lv_label_set_text(cwmem_labels[i], "");
        lv_obj_set_style_text_font(cwmem_labels[i], getThemeFonts()->font_body, 0);
        lv_obj_align(cwmem_labels[i], LV_ALIGN_LEFT_MID, 0, 0);
    }

    // Focus container for key events
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);
    lv_obj_set_pos(focus_container, 0, 0);
    lv_obj_set_style_bg_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_container, 0, 0);
    lv_obj_add_flag(focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(focus_container, cwmem_key_event_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus_container);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, LV_SYMBOL_UP LV_SYMBOL_DOWN " Navigate   ENTER Menu   ESC Back");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    cwmem_screen = screen;

    // Initialize display with actual memory data
    updateCWMemoriesDisplay();

    return screen;
}

// ============================================
// Vail Repeater Screen - Full LVGL Implementation
// ============================================

// External references from vail_repeater.h
extern VailState vailState;
extern String vailChannel;
extern String vailCallsign;
extern int connectedClients;
extern std::vector<ChatMessage> chatHistory;
extern std::vector<RoomInfo> activeRooms;
extern std::vector<UserInfo> connectedUsers;
extern String chatInput;
extern String roomInput;
extern int cwSpeed;
extern void connectToVail(String channel);
extern void disconnectFromVail();
extern void sendChatMessage(String message);
extern void saveCWSettings();

// Vail screen static variables
static lv_obj_t* vail_screen = NULL;
static lv_obj_t* vail_chat_textarea = NULL;
static lv_obj_t* vail_status_label = NULL;
static lv_obj_t* vail_callsign_label = NULL;
static lv_obj_t* vail_room_label = NULL;
static lv_obj_t* vail_users_label = NULL;
static lv_obj_t* vail_wpm_label = NULL;
static lv_obj_t* vail_footer_label = NULL;

// Overlay elements
static lv_obj_t* vail_room_overlay = NULL;
static lv_obj_t* vail_room_list = NULL;
static lv_obj_t* vail_room_input_textarea = NULL;
static lv_obj_t* vail_chat_input_overlay = NULL;
static lv_obj_t* vail_chat_input_textarea = NULL;
static lv_obj_t* vail_user_list_overlay = NULL;
static lv_obj_t* vail_user_list = NULL;

// View state: 0=main, 1=rooms, 2=chat input, 3=users, 4=callsign required
static int vail_view_mode = 0;
static int vail_room_selection = 0;
static int vail_user_scroll = 0;
static size_t vail_last_chat_count = 0;
static bool vail_custom_room_mode = false;
static bool vail_callsign_required = false;  // True if user needs to set callsign
static lv_obj_t* vail_callsign_overlay = NULL;

// Forward declarations
static void showVailRoomOverlay();
static void hideVailRoomOverlay();
static void showVailChatInputOverlay();
static void hideVailChatInputOverlay();
static void showVailUserListOverlay();
static void hideVailUserListOverlay();
static void updateVailRoomList();
static void updateVailUserList();
static void showVailCallsignRequiredOverlay();
static bool checkVailCallsignRequired();

// Key event callback for Vail Repeater keyboard input
static void vail_key_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    Serial.printf("[Vail LVGL] Key event: %lu (0x%02lX), view_mode: %d\n", key, key, vail_view_mode);

    // Handle based on current view mode
    switch (vail_view_mode) {
        case 0: // Main view
            switch(key) {
                case LV_KEY_ESC:
                    // Disconnect and exit
                    disconnectFromVail();
                    onLVGLBackNavigation();
                    lv_event_stop_processing(e);
                    break;
                case LV_KEY_UP:
                case LV_KEY_PREV:
                    // Show room selection
                    showVailRoomOverlay();
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                    break;
                case LV_KEY_DOWN:
                case LV_KEY_NEXT:
                    // Show chat input
                    showVailChatInputOverlay();
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                    break;
                case LV_KEY_LEFT:
                    // Decrease WPM
                    if (cwSpeed > 5) {
                        cwSpeed--;
                        saveCWSettings();
                        if (vail_wpm_label) {
                            char wpm_str[16];
                            snprintf(wpm_str, sizeof(wpm_str), "WPM: %d", cwSpeed);
                            lv_label_set_text(vail_wpm_label, wpm_str);
                        }
                        beep(TONE_MENU_NAV, BEEP_SHORT);
                    }
                    break;
                case LV_KEY_RIGHT:
                    // Increase WPM
                    if (cwSpeed < 40) {
                        cwSpeed++;
                        saveCWSettings();
                        if (vail_wpm_label) {
                            char wpm_str[16];
                            snprintf(wpm_str, sizeof(wpm_str), "WPM: %d", cwSpeed);
                            lv_label_set_text(vail_wpm_label, wpm_str);
                        }
                        beep(TONE_MENU_NAV, BEEP_SHORT);
                    }
                    break;
                case 'u':
                case 'U':
                    // Show user list
                    showVailUserListOverlay();
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                    break;
                case 'c':
                case 'C':
                    // Clear chat display
                    if (vail_chat_textarea != NULL) {
                        lv_textarea_set_text(vail_chat_textarea, "");
                        vail_last_chat_count = chatHistory.size(); // Mark all as "seen"
                    }
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                    break;
            }
            break;

        case 1: // Room selection overlay
            switch(key) {
                case LV_KEY_ESC:
                    if (vail_custom_room_mode) {
                        // Exit custom room input, back to room list
                        vail_custom_room_mode = false;
                        roomInput = "";
                        if (vail_room_input_textarea) {
                            lv_obj_add_flag(vail_room_input_textarea, LV_OBJ_FLAG_HIDDEN);
                        }
                        if (vail_room_list) {
                            lv_obj_clear_flag(vail_room_list, LV_OBJ_FLAG_HIDDEN);
                        }
                    } else {
                        hideVailRoomOverlay();
                    }
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                    break;
                case LV_KEY_UP:
                case LV_KEY_PREV:
                    if (!vail_custom_room_mode && vail_room_selection > 0) {
                        vail_room_selection--;
                        updateVailRoomList();
                        beep(TONE_MENU_NAV, BEEP_SHORT);
                    }
                    break;
                case LV_KEY_DOWN:
                case LV_KEY_NEXT:
                    if (!vail_custom_room_mode) {
                        int maxSelection = activeRooms.size(); // +1 for custom room option, but 0-indexed
                        if (vail_room_selection < maxSelection) {
                            vail_room_selection++;
                            updateVailRoomList();
                            beep(TONE_MENU_NAV, BEEP_SHORT);
                        }
                    }
                    break;
                case LV_KEY_ENTER:
                    if (vail_custom_room_mode) {
                        // Connect to custom room
                        if (roomInput.length() > 0) {
                            hideVailRoomOverlay();
                            connectToVail(roomInput);
                            roomInput = "";
                            beep(TONE_SUCCESS, BEEP_SHORT);
                        }
                    } else if (vail_room_selection == (int)activeRooms.size()) {
                        // Selected "Custom room..." option
                        vail_custom_room_mode = true;
                        roomInput = "";
                        if (vail_room_list) {
                            lv_obj_add_flag(vail_room_list, LV_OBJ_FLAG_HIDDEN);
                        }
                        if (vail_room_input_textarea) {
                            lv_obj_clear_flag(vail_room_input_textarea, LV_OBJ_FLAG_HIDDEN);
                            lv_textarea_set_text(vail_room_input_textarea, "");
                        }
                        beep(TONE_MENU_NAV, BEEP_SHORT);
                    } else if (vail_room_selection < (int)activeRooms.size()) {
                        // Connect to selected room
                        String roomName = activeRooms[vail_room_selection].name;
                        hideVailRoomOverlay();
                        connectToVail(roomName);
                        beep(TONE_SUCCESS, BEEP_SHORT);
                    }
                    break;
                default:
                    // Handle text input for custom room
                    if (vail_custom_room_mode) {
                        if (key == LV_KEY_BACKSPACE) {
                            if (roomInput.length() > 0) {
                                roomInput.remove(roomInput.length() - 1);
                                if (vail_room_input_textarea) {
                                    lv_textarea_set_text(vail_room_input_textarea, roomInput.c_str());
                                }
                            }
                        } else if (key >= 32 && key < 127 && roomInput.length() < 30) {
                            roomInput += (char)key;
                            if (vail_room_input_textarea) {
                                lv_textarea_set_text(vail_room_input_textarea, roomInput.c_str());
                            }
                        }
                    }
                    break;
            }
            break;

        case 2: // Chat input overlay
            switch(key) {
                case LV_KEY_ESC:
                    hideVailChatInputOverlay();
                    chatInput = "";
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                    break;
                case LV_KEY_ENTER:
                    // Send chat message
                    if (chatInput.length() > 0) {
                        sendChatMessage(chatInput);
                        chatInput = "";
                        hideVailChatInputOverlay();
                        beep(TONE_SUCCESS, BEEP_SHORT);
                    }
                    break;
                case LV_KEY_BACKSPACE:
                    if (chatInput.length() > 0) {
                        chatInput.remove(chatInput.length() - 1);
                        if (vail_chat_input_textarea) {
                            lv_textarea_set_text(vail_chat_input_textarea, chatInput.c_str());
                        }
                    }
                    break;
                default:
                    // Add character to chat input
                    if (key >= 32 && key < 127 && chatInput.length() < 40) {
                        chatInput += (char)key;
                        if (vail_chat_input_textarea) {
                            lv_textarea_set_text(vail_chat_input_textarea, chatInput.c_str());
                        }
                    }
                    break;
            }
            break;

        case 3: // User list overlay
            switch(key) {
                case LV_KEY_ESC:
                    hideVailUserListOverlay();
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                    break;
                case LV_KEY_UP:
                case LV_KEY_PREV:
                    if (vail_user_scroll > 0) {
                        vail_user_scroll--;
                        updateVailUserList();
                        beep(TONE_MENU_NAV, BEEP_SHORT);
                    }
                    break;
                case LV_KEY_DOWN:
                case LV_KEY_NEXT:
                    if (vail_user_scroll < (int)connectedUsers.size() - 1) {
                        vail_user_scroll++;
                        updateVailUserList();
                        beep(TONE_MENU_NAV, BEEP_SHORT);
                    }
                    break;
            }
            break;

        case 4: // Callsign required overlay - only ESC to exit
            switch(key) {
                case LV_KEY_ESC:
                    // Exit back to menu - don't connect without callsign
                    onLVGLBackNavigation();
                    lv_event_stop_processing(e);
                    break;
                default:
                    // Ignore all other keys - user must exit and set callsign first
                    break;
            }
            break;
    }
}

// Create overlay for room selection
static void showVailRoomOverlay() {
    if (vail_room_overlay != NULL) {
        lv_obj_clear_flag(vail_room_overlay, LV_OBJ_FLAG_HIDDEN);
        vail_view_mode = 1;
        vail_room_selection = 0;
        vail_custom_room_mode = false;
        updateVailRoomList();
        return;
    }

    // Create overlay background
    vail_room_overlay = lv_obj_create(vail_screen);
    lv_obj_set_size(vail_room_overlay, 360, 220);
    lv_obj_center(vail_room_overlay);
    lv_obj_set_style_bg_color(vail_room_overlay, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_border_color(vail_room_overlay, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_border_width(vail_room_overlay, 2, 0);
    lv_obj_set_style_radius(vail_room_overlay, 8, 0);
    lv_obj_set_style_pad_all(vail_room_overlay, 10, 0);
    lv_obj_clear_flag(vail_room_overlay, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* title = lv_label_create(vail_room_overlay);
    lv_label_set_text(title, "SELECT ROOM");
    lv_obj_set_style_text_font(title, getThemeFonts()->font_title, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // Room list container
    vail_room_list = lv_obj_create(vail_room_overlay);
    lv_obj_set_size(vail_room_list, 340, 140);
    lv_obj_align(vail_room_list, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_bg_opa(vail_room_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(vail_room_list, 0, 0);
    lv_obj_set_style_pad_all(vail_room_list, 5, 0);
    lv_obj_set_flex_flow(vail_room_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(vail_room_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Custom room input textarea (initially hidden)
    vail_room_input_textarea = lv_textarea_create(vail_room_overlay);
    lv_obj_set_size(vail_room_input_textarea, 300, 40);
    lv_obj_align(vail_room_input_textarea, LV_ALIGN_TOP_MID, 0, 80);
    lv_textarea_set_placeholder_text(vail_room_input_textarea, "Enter room name...");
    lv_textarea_set_one_line(vail_room_input_textarea, true);
    lv_obj_add_style(vail_room_input_textarea, getStyleTextarea(), 0);
    lv_obj_add_flag(vail_room_input_textarea, LV_OBJ_FLAG_HIDDEN);

    // Footer help text
    lv_obj_t* help = lv_label_create(vail_room_overlay);
    lv_label_set_text(help, "UP/DN Navigate  ENTER Select  ESC Back");
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_align(help, LV_ALIGN_BOTTOM_MID, 0, -5);

    vail_view_mode = 1;
    vail_room_selection = 0;
    updateVailRoomList();
}

static void hideVailRoomOverlay() {
    if (vail_room_overlay != NULL) {
        lv_obj_add_flag(vail_room_overlay, LV_OBJ_FLAG_HIDDEN);
    }
    vail_view_mode = 0;
    vail_custom_room_mode = false;
}

static void updateVailRoomList() {
    if (vail_room_list == NULL) return;

    // Clear existing items
    lv_obj_clean(vail_room_list);

    // Add room items
    for (size_t i = 0; i < activeRooms.size() && i < 5; i++) {
        lv_obj_t* item = lv_label_create(vail_room_list);
        char item_text[64];
        snprintf(item_text, sizeof(item_text), "%s %s (%d users)",
                 (i == (size_t)vail_room_selection) ? ">" : " ",
                 activeRooms[i].name.c_str(),
                 activeRooms[i].users);
        lv_label_set_text(item, item_text);
        lv_obj_set_style_text_font(item, getThemeFonts()->font_body, 0);
        if (i == (size_t)vail_room_selection) {
            lv_obj_set_style_text_color(item, LV_COLOR_ACCENT_CYAN, 0);
        } else {
            lv_obj_set_style_text_color(item, LV_COLOR_TEXT_PRIMARY, 0);
        }
    }

    // Add "Custom room..." option
    lv_obj_t* custom_item = lv_label_create(vail_room_list);
    char custom_text[32];
    snprintf(custom_text, sizeof(custom_text), "%s Custom room...",
             (vail_room_selection == (int)activeRooms.size()) ? ">" : " ");
    lv_label_set_text(custom_item, custom_text);
    lv_obj_set_style_text_font(custom_item, getThemeFonts()->font_body, 0);
    if (vail_room_selection == (int)activeRooms.size()) {
        lv_obj_set_style_text_color(custom_item, LV_COLOR_ACCENT_CYAN, 0);
    } else {
        lv_obj_set_style_text_color(custom_item, LV_COLOR_TEXT_SECONDARY, 0);
    }
}

// Create overlay for chat input
static void showVailChatInputOverlay() {
    if (vail_chat_input_overlay != NULL) {
        lv_obj_clear_flag(vail_chat_input_overlay, LV_OBJ_FLAG_HIDDEN);
        if (vail_chat_input_textarea) {
            lv_textarea_set_text(vail_chat_input_textarea, "");
        }
        chatInput = "";
        vail_view_mode = 2;
        return;
    }

    // Create overlay background
    vail_chat_input_overlay = lv_obj_create(vail_screen);
    lv_obj_set_size(vail_chat_input_overlay, 400, 140);
    lv_obj_center(vail_chat_input_overlay);
    lv_obj_set_style_bg_color(vail_chat_input_overlay, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_border_color(vail_chat_input_overlay, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_border_width(vail_chat_input_overlay, 2, 0);
    lv_obj_set_style_radius(vail_chat_input_overlay, 8, 0);
    lv_obj_set_style_pad_all(vail_chat_input_overlay, 15, 0);
    lv_obj_clear_flag(vail_chat_input_overlay, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* title = lv_label_create(vail_chat_input_overlay);
    lv_label_set_text(title, "SEND MESSAGE");
    lv_obj_set_style_text_font(title, getThemeFonts()->font_title, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // Chat input textarea
    vail_chat_input_textarea = lv_textarea_create(vail_chat_input_overlay);
    lv_obj_set_size(vail_chat_input_textarea, 360, 45);
    lv_obj_align(vail_chat_input_textarea, LV_ALIGN_TOP_MID, 0, 35);
    lv_textarea_set_placeholder_text(vail_chat_input_textarea, "Type your message...");
    lv_textarea_set_one_line(vail_chat_input_textarea, true);
    lv_textarea_set_max_length(vail_chat_input_textarea, 40);
    lv_obj_add_style(vail_chat_input_textarea, getStyleTextarea(), 0);

    // Footer help text
    lv_obj_t* help = lv_label_create(vail_chat_input_overlay);
    lv_label_set_text(help, "Type message   ENTER Send   ESC Cancel");
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_align(help, LV_ALIGN_BOTTOM_MID, 0, 0);

    chatInput = "";
    vail_view_mode = 2;
}

static void hideVailChatInputOverlay() {
    if (vail_chat_input_overlay != NULL) {
        lv_obj_add_flag(vail_chat_input_overlay, LV_OBJ_FLAG_HIDDEN);
    }
    vail_view_mode = 0;
}

// Create overlay for user list
static void showVailUserListOverlay() {
    if (vail_user_list_overlay != NULL) {
        lv_obj_clear_flag(vail_user_list_overlay, LV_OBJ_FLAG_HIDDEN);
        vail_view_mode = 3;
        vail_user_scroll = 0;
        updateVailUserList();
        return;
    }

    // Create overlay background
    vail_user_list_overlay = lv_obj_create(vail_screen);
    lv_obj_set_size(vail_user_list_overlay, 320, 200);
    lv_obj_center(vail_user_list_overlay);
    lv_obj_set_style_bg_color(vail_user_list_overlay, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_border_color(vail_user_list_overlay, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_border_width(vail_user_list_overlay, 2, 0);
    lv_obj_set_style_radius(vail_user_list_overlay, 8, 0);
    lv_obj_set_style_pad_all(vail_user_list_overlay, 10, 0);
    lv_obj_clear_flag(vail_user_list_overlay, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* title = lv_label_create(vail_user_list_overlay);
    lv_label_set_text(title, "CONNECTED USERS");
    lv_obj_set_style_text_font(title, getThemeFonts()->font_title, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // User list container
    vail_user_list = lv_obj_create(vail_user_list_overlay);
    lv_obj_set_size(vail_user_list, 300, 120);
    lv_obj_align(vail_user_list, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_bg_opa(vail_user_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(vail_user_list, 0, 0);
    lv_obj_set_style_pad_all(vail_user_list, 5, 0);
    lv_obj_set_flex_flow(vail_user_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(vail_user_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Footer help text
    lv_obj_t* help = lv_label_create(vail_user_list_overlay);
    lv_label_set_text(help, "UP/DN Scroll   ESC Close");
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_align(help, LV_ALIGN_BOTTOM_MID, 0, -5);

    vail_view_mode = 3;
    vail_user_scroll = 0;
    updateVailUserList();
}

static void hideVailUserListOverlay() {
    if (vail_user_list_overlay != NULL) {
        lv_obj_add_flag(vail_user_list_overlay, LV_OBJ_FLAG_HIDDEN);
    }
    vail_view_mode = 0;
}

static void updateVailUserList() {
    if (vail_user_list == NULL) return;

    // Clear existing items
    lv_obj_clean(vail_user_list);

    if (connectedUsers.empty()) {
        lv_obj_t* empty_label = lv_label_create(vail_user_list);
        lv_label_set_text(empty_label, "No users connected");
        lv_obj_set_style_text_color(empty_label, LV_COLOR_TEXT_SECONDARY, 0);
        return;
    }

    // Show up to 5 users starting from scroll position
    for (size_t i = vail_user_scroll; i < connectedUsers.size() && i < (size_t)(vail_user_scroll + 5); i++) {
        lv_obj_t* item = lv_label_create(vail_user_list);
        char item_text[48];
        // Convert MIDI note to approximate Hz for display
        int freq = (int)(440.0 * pow(2.0, (connectedUsers[i].txTone - 69) / 12.0));
        snprintf(item_text, sizeof(item_text), "%s (%d Hz)", connectedUsers[i].callsign.c_str(), freq);
        lv_label_set_text(item, item_text);
        lv_obj_set_style_text_font(item, getThemeFonts()->font_body, 0);
        lv_obj_set_style_text_color(item, LV_COLOR_TEXT_PRIMARY, 0);
    }
}

// Check if callsign is required (returns true if user needs to set callsign)
static bool checkVailCallsignRequired() {
    // Check if callsign is empty or still the default "GUEST"
    return (vailCallsign.length() == 0 || vailCallsign == "GUEST");
}

// Show overlay prompting user to set callsign
static void showVailCallsignRequiredOverlay() {
    if (vail_callsign_overlay != NULL) {
        lv_obj_clear_flag(vail_callsign_overlay, LV_OBJ_FLAG_HIDDEN);
        vail_view_mode = 4;
        return;
    }

    // Create overlay background
    vail_callsign_overlay = lv_obj_create(vail_screen);
    lv_obj_set_size(vail_callsign_overlay, 380, 180);
    lv_obj_center(vail_callsign_overlay);
    lv_obj_set_style_bg_color(vail_callsign_overlay, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_border_color(vail_callsign_overlay, LV_COLOR_WARNING, 0);
    lv_obj_set_style_border_width(vail_callsign_overlay, 2, 0);
    lv_obj_set_style_radius(vail_callsign_overlay, 8, 0);
    lv_obj_set_style_pad_all(vail_callsign_overlay, 15, 0);
    lv_obj_clear_flag(vail_callsign_overlay, LV_OBJ_FLAG_SCROLLABLE);

    // Warning icon and title
    lv_obj_t* title = lv_label_create(vail_callsign_overlay);
    lv_label_set_text(title, LV_SYMBOL_WARNING "  CALLSIGN REQUIRED");
    lv_obj_set_style_text_font(title, getThemeFonts()->font_title, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_WARNING, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // Message
    lv_obj_t* msg1 = lv_label_create(vail_callsign_overlay);
    lv_label_set_text(msg1, "You must set your callsign before");
    lv_obj_set_style_text_font(msg1, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(msg1, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(msg1, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t* msg2 = lv_label_create(vail_callsign_overlay);
    lv_label_set_text(msg2, "using the Vail Repeater.");
    lv_obj_set_style_text_font(msg2, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(msg2, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(msg2, LV_ALIGN_TOP_MID, 0, 60);

    // Instructions
    lv_obj_t* instr = lv_label_create(vail_callsign_overlay);
    lv_label_set_text(instr, "Go to: Settings > General > Callsign");
    lv_obj_set_style_text_font(instr, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(instr, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(instr, LV_ALIGN_TOP_MID, 0, 95);

    // Footer help text
    lv_obj_t* help = lv_label_create(vail_callsign_overlay);
    lv_label_set_text(help, "ESC  Exit");
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_align(help, LV_ALIGN_BOTTOM_MID, 0, -5);

    vail_view_mode = 4;
    vail_callsign_required = true;
}

lv_obj_t* createVailRepeaterScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Reset state
    vail_view_mode = 0;
    vail_last_chat_count = 0;
    vail_room_overlay = NULL;
    vail_chat_input_overlay = NULL;
    vail_user_list_overlay = NULL;
    vail_callsign_overlay = NULL;
    vail_callsign_required = false;

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
    lv_label_set_text(title, "VAIL REPEATER");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);

    vail_status_label = lv_label_create(title_bar);
    lv_label_set_text(vail_status_label, "Connecting...");
    lv_obj_set_style_text_color(vail_status_label, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(vail_status_label, getThemeFonts()->font_body, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Info bar with room, users, callsign
    lv_obj_t* info_bar = lv_obj_create(screen);
    lv_obj_set_size(info_bar, SCREEN_WIDTH, 32);
    lv_obj_set_pos(info_bar, 0, HEADER_HEIGHT);
    lv_obj_set_layout(info_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(info_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(info_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(info_bar, 15, 0);
    lv_obj_set_style_bg_color(info_bar, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_border_width(info_bar, 0, 0);
    lv_obj_clear_flag(info_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Left side: Room and users
    lv_obj_t* left_info = lv_obj_create(info_bar);
    lv_obj_set_size(left_info, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(left_info, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left_info, 0, 0);
    lv_obj_set_style_pad_all(left_info, 0, 0);
    lv_obj_set_layout(left_info, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left_info, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(left_info, 5, 0);
    lv_obj_clear_flag(left_info, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* room_prefix = lv_label_create(left_info);
    lv_label_set_text(room_prefix, "Room:");
    lv_obj_set_style_text_color(room_prefix, LV_COLOR_TEXT_SECONDARY, 0);

    vail_room_label = lv_label_create(left_info);
    lv_label_set_text(vail_room_label, "General");
    lv_obj_set_style_text_color(vail_room_label, LV_COLOR_ACCENT_CYAN, 0);

    vail_users_label = lv_label_create(left_info);
    lv_label_set_text(vail_users_label, "(0)");
    lv_obj_set_style_text_color(vail_users_label, LV_COLOR_TEXT_SECONDARY, 0);

    // Right side: Callsign
    lv_obj_t* right_info = lv_obj_create(info_bar);
    lv_obj_set_size(right_info, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(right_info, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right_info, 0, 0);
    lv_obj_set_style_pad_all(right_info, 0, 0);
    lv_obj_set_layout(right_info, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right_info, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(right_info, 5, 0);
    lv_obj_clear_flag(right_info, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* callsign_prefix = lv_label_create(right_info);
    lv_label_set_text(callsign_prefix, "Callsign:");
    lv_obj_set_style_text_color(callsign_prefix, LV_COLOR_TEXT_SECONDARY, 0);

    vail_callsign_label = lv_label_create(right_info);
    lv_label_set_text(vail_callsign_label, vailCallsign.c_str());
    lv_obj_set_style_text_color(vail_callsign_label, LV_COLOR_ACCENT_CYAN, 0);

    // Chat area
    int chat_top = HEADER_HEIGHT + 32 + 5;
    int chat_height = SCREEN_HEIGHT - chat_top - FOOTER_HEIGHT - 5;
    vail_chat_textarea = lv_textarea_create(screen);
    lv_obj_set_size(vail_chat_textarea, SCREEN_WIDTH - 20, chat_height);
    lv_obj_set_pos(vail_chat_textarea, 10, chat_top);
    lv_textarea_set_text(vail_chat_textarea, "");
    lv_textarea_set_placeholder_text(vail_chat_textarea, "Messages will appear here...\nUse paddle to send morse code.");
    lv_obj_add_style(vail_chat_textarea, getStyleTextarea(), 0);
    lv_obj_set_style_text_font(vail_chat_textarea, getThemeFonts()->font_body, 0);
    lv_obj_clear_flag(vail_chat_textarea, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    // Make scrollable but not editable
    lv_textarea_set_cursor_click_pos(vail_chat_textarea, false);

    // Footer with key hints
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(footer, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(footer, 10, 0);

    // Left side footer - navigation hints
    vail_footer_label = lv_label_create(footer);
    lv_label_set_text(vail_footer_label, "UP Rooms  DN Chat  U Users  C Clear");
    lv_obj_set_style_text_color(vail_footer_label, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(vail_footer_label, getThemeFonts()->font_small, 0);

    // Right side footer - WPM display
    vail_wpm_label = lv_label_create(footer);
    char wpm_str[16];
    snprintf(wpm_str, sizeof(wpm_str), "WPM: %d", cwSpeed);
    lv_label_set_text(vail_wpm_label, wpm_str);
    lv_obj_set_style_text_color(vail_wpm_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(vail_wpm_label, getThemeFonts()->font_small, 0);

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
    lv_obj_add_event_cb(focus_container, vail_key_event_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus_container);
    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }
    lv_group_focus_obj(focus_container);

    vail_screen = screen;

    // Check if callsign is required - show overlay if user hasn't set callsign
    if (checkVailCallsignRequired()) {
        showVailCallsignRequiredOverlay();
    }

    return screen;
}

// Update Vail screen elements from global state
// Call this from main loop when in Vail mode
void updateVailScreenLVGL() {
    if (vail_screen == NULL) return;

    // Update status label based on connection state
    if (vail_status_label != NULL) {
        const char* status_text;
        lv_color_t status_color;
        switch (vailState) {
            case VAIL_DISCONNECTED:
                status_text = "Disconnected";
                status_color = LV_COLOR_TEXT_SECONDARY;
                break;
            case VAIL_CONNECTING:
                status_text = "Connecting...";
                status_color = LV_COLOR_WARNING;
                break;
            case VAIL_CONNECTED:
                status_text = "Connected";
                status_color = LV_COLOR_SUCCESS;
                break;
            case VAIL_ERROR:
                status_text = "Error";
                status_color = LV_COLOR_ERROR;
                break;
            default:
                status_text = "Unknown";
                status_color = LV_COLOR_TEXT_SECONDARY;
        }
        lv_label_set_text(vail_status_label, status_text);
        lv_obj_set_style_text_color(vail_status_label, status_color, 0);
    }

    // Update room name
    if (vail_room_label != NULL) {
        lv_label_set_text(vail_room_label, vailChannel.c_str());
    }

    // Update user count
    if (vail_users_label != NULL) {
        char users_str[16];
        snprintf(users_str, sizeof(users_str), "(%d)", connectedClients);
        lv_label_set_text(vail_users_label, users_str);
    }

    // Update callsign if changed
    if (vail_callsign_label != NULL) {
        lv_label_set_text(vail_callsign_label, vailCallsign.c_str());
    }

    // Sync chat messages if new ones arrived
    if (chatHistory.size() != vail_last_chat_count && vail_chat_textarea != NULL) {
        // Rebuild chat text from history
        String chatText = "";
        for (size_t i = 0; i < chatHistory.size(); i++) {
            chatText += chatHistory[i].callsign;
            chatText += ": ";
            chatText += chatHistory[i].message;
            chatText += "\n";
        }
        lv_textarea_set_text(vail_chat_textarea, chatText.c_str());
        // Scroll to bottom
        lv_textarea_set_cursor_pos(vail_chat_textarea, LV_TEXTAREA_CURSOR_LAST);
        vail_last_chat_count = chatHistory.size();
    }
}

// Legacy compatibility functions
void updateVailStatus(const char* status, bool connected) {
    // Now handled by updateVailScreenLVGL()
}

void updateVailCallsign(const char* callsign) {
    // Now handled by updateVailScreenLVGL()
}

void appendVailMessage(const char* message) {
    // Messages are now synced from chatHistory in updateVailScreenLVGL()
}

// ============================================
// QSO Logger Entry Screen (Enhanced)
// Full form with all QSO fields
// ============================================

static lv_obj_t* qso_entry_screen = NULL;
static lv_obj_t* qso_entry_focus_container = NULL;
static lv_obj_t* qso_callsign_input = NULL;
static lv_obj_t* qso_freq_input = NULL;
static lv_obj_t* qso_mode_row = NULL;
static lv_obj_t* qso_mode_label = NULL;
static lv_obj_t* qso_rst_sent_input = NULL;
static lv_obj_t* qso_rst_rcvd_input = NULL;
static lv_obj_t* qso_date_input = NULL;
static lv_obj_t* qso_time_input = NULL;
static lv_obj_t* qso_my_grid_input = NULL;
static lv_obj_t* qso_my_pota_input = NULL;
static lv_obj_t* qso_notes_input = NULL;

static int qso_entry_mode_index = 0;  // 0=CW, 1=SSB, etc.
static const char* qso_mode_names[] = {"CW", "SSB", "FM", "AM", "FT8", "FT4", "RTTY", "PSK31"};
static const int qso_mode_count = 8;

// Focus state for arrow key navigation
static int qso_entry_focus = 0;  // Current focused field index
static const int QSO_ENTRY_FIELD_COUNT = 10;  // Total navigable fields

// Forward declarations for QSO operations
extern Preferences qsoPrefs;
extern bool saveQSO(const QSO& qso);
extern String frequencyToBand(float freq);
extern String getDefaultRST(const char* mode);
extern void formatCurrentDateTime(char* dateOut, char* timeOut);

// Get current date/time string
static void getCurrentDateTimeStrings(char* dateOut, char* timeOut) {
    // Use formatCurrentDateTime from qso_logger_validation.h
    formatCurrentDateTime(dateOut, timeOut);
}

// Load operator settings (grid, pota) into form
static void loadOperatorSettingsToForm() {
    extern Preferences qsoPrefs;
    qsoPrefs.begin("qso_operator", true);  // Read-only

    char grid[9] = "";
    char pota[11] = "";
    qsoPrefs.getString("grid", grid, sizeof(grid));
    qsoPrefs.getString("pota_ref", pota, sizeof(pota));

    qsoPrefs.end();

    if (qso_my_grid_input != NULL && strlen(grid) > 0) {
        lv_textarea_set_text(qso_my_grid_input, grid);
    }
    if (qso_my_pota_input != NULL && strlen(pota) > 0) {
        lv_textarea_set_text(qso_my_pota_input, pota);
    }
}

// Update mode display and RST defaults
static void updateQSOEntryMode() {
    if (qso_mode_label != NULL) {
        lv_label_set_text_fmt(qso_mode_label, "< %s >", qso_mode_names[qso_entry_mode_index]);
    }

    // Update RST defaults based on mode
    String defaultRST = getDefaultRST(qso_mode_names[qso_entry_mode_index]);
    if (qso_rst_sent_input != NULL) {
        lv_textarea_set_text(qso_rst_sent_input, defaultRST.c_str());
    }
    if (qso_rst_rcvd_input != NULL) {
        lv_textarea_set_text(qso_rst_rcvd_input, defaultRST.c_str());
    }
}

// Update focus to the current field
static void qso_entry_update_focus() {
    // Array of all navigable widgets in order
    lv_obj_t* fields[] = {
        qso_callsign_input,
        qso_freq_input,
        qso_mode_row,
        qso_rst_sent_input,
        qso_rst_rcvd_input,
        qso_date_input,
        qso_time_input,
        qso_my_grid_input,
        qso_my_pota_input,
        qso_notes_input
    };

    // Focus the correct widget in LVGL's group
    lv_group_t* group = getLVGLInputGroup();
    if (group && qso_entry_focus >= 0 && qso_entry_focus < QSO_ENTRY_FIELD_COUNT) {
        lv_obj_t* target = fields[qso_entry_focus];
        if (target != NULL) {
            lv_group_focus_obj(target);
        }
    }
}

// Key handler for QSO entry form navigation
static void qso_entry_key_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // ESC - cancel and exit
    if (key == LV_KEY_ESC) {
        onLVGLBackNavigation();
        lv_event_stop_bubbling(e);
        return;
    }

    // Handle UP - previous field
    if (key == LV_KEY_UP) {
        lv_event_stop_bubbling(e);
        if (qso_entry_focus > 0) {
            qso_entry_focus--;
            qso_entry_update_focus();
            beep(TONE_MENU_NAV, BEEP_SHORT);
        }
        return;
    }

    // Handle DOWN / TAB - next field
    if (key == LV_KEY_DOWN || key == LV_KEY_NEXT) {
        lv_event_stop_bubbling(e);
        if (qso_entry_focus < QSO_ENTRY_FIELD_COUNT - 1) {
            qso_entry_focus++;
            qso_entry_update_focus();
            beep(TONE_MENU_NAV, BEEP_SHORT);
        }
        return;
    }

    // LEFT/RIGHT on mode selector (field index 2)
    if (qso_entry_focus == 2) {
        if (key == LV_KEY_LEFT) {
            qso_entry_mode_index = (qso_entry_mode_index - 1 + qso_mode_count) % qso_mode_count;
            updateQSOEntryMode();
            lv_event_stop_bubbling(e);
            return;
        }
        if (key == LV_KEY_RIGHT) {
            qso_entry_mode_index = (qso_entry_mode_index + 1) % qso_mode_count;
            updateQSOEntryMode();
            lv_event_stop_bubbling(e);
            return;
        }
    }

    // ENTER - save QSO
    if (key == LV_KEY_ENTER) {
        // Collect all field values
        const char* callsign = lv_textarea_get_text(qso_callsign_input);
        const char* freqStr = lv_textarea_get_text(qso_freq_input);
        const char* rst_sent = lv_textarea_get_text(qso_rst_sent_input);
        const char* rst_rcvd = lv_textarea_get_text(qso_rst_rcvd_input);
        const char* dateStr = lv_textarea_get_text(qso_date_input);
        const char* timeStr = lv_textarea_get_text(qso_time_input);
        const char* my_grid = lv_textarea_get_text(qso_my_grid_input);
        const char* my_pota = lv_textarea_get_text(qso_my_pota_input);
        const char* notes = lv_textarea_get_text(qso_notes_input);

        // Validate required fields
        if (strlen(callsign) < 3) {
            beep(600, 100);  // Error - need callsign
            lv_group_focus_obj(qso_callsign_input);
            lv_event_stop_bubbling(e);
            return;
        }

        // Create QSO struct
        QSO qso;
        memset(&qso, 0, sizeof(QSO));

        qso.id = millis();
        strlcpy(qso.callsign, callsign, sizeof(qso.callsign));

        // Convert frequency to MHz
        float freqKHz = atof(freqStr);
        qso.frequency = freqKHz / 1000.0;  // kHz to MHz

        // Get band from frequency
        String band = frequencyToBand(qso.frequency);
        strlcpy(qso.band, band.c_str(), sizeof(qso.band));

        strlcpy(qso.mode, qso_mode_names[qso_entry_mode_index], sizeof(qso.mode));
        strlcpy(qso.rst_sent, rst_sent, sizeof(qso.rst_sent));
        strlcpy(qso.rst_rcvd, rst_rcvd, sizeof(qso.rst_rcvd));
        strlcpy(qso.date, dateStr, sizeof(qso.date));
        strlcpy(qso.time_on, timeStr, sizeof(qso.time_on));
        strlcpy(qso.my_gridsquare, my_grid, sizeof(qso.my_gridsquare));
        strlcpy(qso.my_pota_ref, my_pota, sizeof(qso.my_pota_ref));
        strlcpy(qso.notes, notes, sizeof(qso.notes));

        // Save QSO
        saveQSO(qso);

        beep(1000, 100);  // Success
        onLVGLBackNavigation();
        lv_event_stop_bubbling(e);
        return;
    }

    lv_event_stop_bubbling(e);
}

lv_obj_t* createQSOLogEntryScreen() {
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Reset mode index
    qso_entry_mode_index = 0;

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "NEW QSO");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    createCompactStatusBar(screen);

    // Scrollable form container
    lv_obj_t* form = lv_obj_create(screen);
    lv_obj_set_size(form, SCREEN_WIDTH - 20, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 10);
    lv_obj_set_pos(form, 10, HEADER_HEIGHT + 5);
    lv_obj_set_layout(form, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(form, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(form, 8, 0);
    lv_obj_set_style_pad_all(form, 10, 0);
    lv_obj_set_style_bg_opa(form, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(form, 0, 0);
    lv_obj_add_flag(form, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(form, LV_SCROLLBAR_MODE_AUTO);

    // Row 1: Callsign
    lv_obj_t* call_label = lv_label_create(form);
    lv_label_set_text(call_label, "Callsign *");
    lv_obj_set_style_text_color(call_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(call_label, getThemeFonts()->font_small, 0);

    qso_callsign_input = lv_textarea_create(form);
    lv_obj_set_size(qso_callsign_input, lv_pct(100), 35);
    lv_textarea_set_one_line(qso_callsign_input, true);
    lv_textarea_set_max_length(qso_callsign_input, 12);
    lv_textarea_set_placeholder_text(qso_callsign_input, "W1ABC");
    lv_obj_add_style(qso_callsign_input, getStyleTextarea(), 0);
    addNavigableWidget(qso_callsign_input);

    // Row 2: Frequency + Mode (side by side)
    lv_obj_t* freq_mode_row = lv_obj_create(form);
    lv_obj_set_size(freq_mode_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(freq_mode_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(freq_mode_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(freq_mode_row, 15, 0);
    lv_obj_set_style_bg_opa(freq_mode_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(freq_mode_row, 0, 0);
    lv_obj_set_style_pad_all(freq_mode_row, 0, 0);
    lv_obj_clear_flag(freq_mode_row, LV_OBJ_FLAG_SCROLLABLE);

    // Frequency column
    lv_obj_t* freq_col = lv_obj_create(freq_mode_row);
    lv_obj_set_size(freq_col, lv_pct(55), LV_SIZE_CONTENT);
    lv_obj_set_layout(freq_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(freq_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(freq_col, 3, 0);
    lv_obj_set_style_bg_opa(freq_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(freq_col, 0, 0);
    lv_obj_set_style_pad_all(freq_col, 0, 0);
    lv_obj_clear_flag(freq_col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* freq_label = lv_label_create(freq_col);
    lv_label_set_text(freq_label, "Frequency (kHz)");
    lv_obj_set_style_text_color(freq_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(freq_label, getThemeFonts()->font_small, 0);

    qso_freq_input = lv_textarea_create(freq_col);
    lv_obj_set_size(qso_freq_input, lv_pct(100), 35);
    lv_textarea_set_one_line(qso_freq_input, true);
    lv_textarea_set_max_length(qso_freq_input, 10);
    lv_textarea_set_text(qso_freq_input, "7030");
    lv_obj_add_style(qso_freq_input, getStyleTextarea(), 0);
    addNavigableWidget(qso_freq_input);

    // Mode column
    lv_obj_t* mode_col = lv_obj_create(freq_mode_row);
    lv_obj_set_size(mode_col, lv_pct(40), LV_SIZE_CONTENT);
    lv_obj_set_layout(mode_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(mode_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(mode_col, 3, 0);
    lv_obj_set_style_bg_opa(mode_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mode_col, 0, 0);
    lv_obj_set_style_pad_all(mode_col, 0, 0);
    lv_obj_clear_flag(mode_col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* mode_title = lv_label_create(mode_col);
    lv_label_set_text(mode_title, "Mode");
    lv_obj_set_style_text_color(mode_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(mode_title, getThemeFonts()->font_small, 0);

    qso_mode_row = lv_obj_create(mode_col);
    lv_obj_set_size(qso_mode_row, lv_pct(100), 35);
    lv_obj_set_style_bg_color(qso_mode_row, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_radius(qso_mode_row, 6, 0);
    lv_obj_set_style_border_width(qso_mode_row, 1, 0);
    lv_obj_set_style_border_color(qso_mode_row, LV_COLOR_BORDER_SUBTLE, 0);
    lv_obj_set_style_pad_all(qso_mode_row, 5, 0);
    lv_obj_clear_flag(qso_mode_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(qso_mode_row, LV_OBJ_FLAG_CLICKABLE);
    addNavigableWidget(qso_mode_row);

    qso_mode_label = lv_label_create(qso_mode_row);
    lv_label_set_text_fmt(qso_mode_label, "< %s >", qso_mode_names[qso_entry_mode_index]);
    lv_obj_set_style_text_font(qso_mode_label, getThemeFonts()->font_body, 0);
    lv_obj_center(qso_mode_label);

    // Row 3: RST Sent + RST Rcvd (side by side)
    lv_obj_t* rst_row = lv_obj_create(form);
    lv_obj_set_size(rst_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(rst_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(rst_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(rst_row, 15, 0);
    lv_obj_set_style_bg_opa(rst_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rst_row, 0, 0);
    lv_obj_set_style_pad_all(rst_row, 0, 0);
    lv_obj_clear_flag(rst_row, LV_OBJ_FLAG_SCROLLABLE);

    // RST Sent column
    lv_obj_t* rst_sent_col = lv_obj_create(rst_row);
    lv_obj_set_size(rst_sent_col, lv_pct(45), LV_SIZE_CONTENT);
    lv_obj_set_layout(rst_sent_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(rst_sent_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(rst_sent_col, 3, 0);
    lv_obj_set_style_bg_opa(rst_sent_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rst_sent_col, 0, 0);
    lv_obj_set_style_pad_all(rst_sent_col, 0, 0);
    lv_obj_clear_flag(rst_sent_col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* rst_sent_label = lv_label_create(rst_sent_col);
    lv_label_set_text(rst_sent_label, "RST Sent");
    lv_obj_set_style_text_color(rst_sent_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(rst_sent_label, getThemeFonts()->font_small, 0);

    qso_rst_sent_input = lv_textarea_create(rst_sent_col);
    lv_obj_set_size(qso_rst_sent_input, lv_pct(100), 35);
    lv_textarea_set_one_line(qso_rst_sent_input, true);
    lv_textarea_set_max_length(qso_rst_sent_input, 3);
    lv_textarea_set_text(qso_rst_sent_input, "599");
    lv_obj_add_style(qso_rst_sent_input, getStyleTextarea(), 0);
    addNavigableWidget(qso_rst_sent_input);

    // RST Rcvd column
    lv_obj_t* rst_rcvd_col = lv_obj_create(rst_row);
    lv_obj_set_size(rst_rcvd_col, lv_pct(45), LV_SIZE_CONTENT);
    lv_obj_set_layout(rst_rcvd_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(rst_rcvd_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(rst_rcvd_col, 3, 0);
    lv_obj_set_style_bg_opa(rst_rcvd_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rst_rcvd_col, 0, 0);
    lv_obj_set_style_pad_all(rst_rcvd_col, 0, 0);
    lv_obj_clear_flag(rst_rcvd_col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* rst_rcvd_label = lv_label_create(rst_rcvd_col);
    lv_label_set_text(rst_rcvd_label, "RST Rcvd");
    lv_obj_set_style_text_color(rst_rcvd_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(rst_rcvd_label, getThemeFonts()->font_small, 0);

    qso_rst_rcvd_input = lv_textarea_create(rst_rcvd_col);
    lv_obj_set_size(qso_rst_rcvd_input, lv_pct(100), 35);
    lv_textarea_set_one_line(qso_rst_rcvd_input, true);
    lv_textarea_set_max_length(qso_rst_rcvd_input, 3);
    lv_textarea_set_text(qso_rst_rcvd_input, "599");
    lv_obj_add_style(qso_rst_rcvd_input, getStyleTextarea(), 0);
    addNavigableWidget(qso_rst_rcvd_input);

    // Row 4: Date + Time (side by side)
    lv_obj_t* datetime_row = lv_obj_create(form);
    lv_obj_set_size(datetime_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(datetime_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(datetime_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(datetime_row, 15, 0);
    lv_obj_set_style_bg_opa(datetime_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(datetime_row, 0, 0);
    lv_obj_set_style_pad_all(datetime_row, 0, 0);
    lv_obj_clear_flag(datetime_row, LV_OBJ_FLAG_SCROLLABLE);

    // Date column
    lv_obj_t* date_col = lv_obj_create(datetime_row);
    lv_obj_set_size(date_col, lv_pct(55), LV_SIZE_CONTENT);
    lv_obj_set_layout(date_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(date_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(date_col, 3, 0);
    lv_obj_set_style_bg_opa(date_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(date_col, 0, 0);
    lv_obj_set_style_pad_all(date_col, 0, 0);
    lv_obj_clear_flag(date_col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* date_label = lv_label_create(date_col);
    lv_label_set_text(date_label, "Date (YYYYMMDD)");
    lv_obj_set_style_text_color(date_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(date_label, getThemeFonts()->font_small, 0);

    qso_date_input = lv_textarea_create(date_col);
    lv_obj_set_size(qso_date_input, lv_pct(100), 35);
    lv_textarea_set_one_line(qso_date_input, true);
    lv_textarea_set_max_length(qso_date_input, 8);
    lv_obj_add_style(qso_date_input, getStyleTextarea(), 0);
    addNavigableWidget(qso_date_input);

    // Time column
    lv_obj_t* time_col = lv_obj_create(datetime_row);
    lv_obj_set_size(time_col, lv_pct(40), LV_SIZE_CONTENT);
    lv_obj_set_layout(time_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(time_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(time_col, 3, 0);
    lv_obj_set_style_bg_opa(time_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(time_col, 0, 0);
    lv_obj_set_style_pad_all(time_col, 0, 0);
    lv_obj_clear_flag(time_col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* time_label = lv_label_create(time_col);
    lv_label_set_text(time_label, "Time UTC");
    lv_obj_set_style_text_color(time_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(time_label, getThemeFonts()->font_small, 0);

    qso_time_input = lv_textarea_create(time_col);
    lv_obj_set_size(qso_time_input, lv_pct(100), 35);
    lv_textarea_set_one_line(qso_time_input, true);
    lv_textarea_set_max_length(qso_time_input, 4);
    lv_obj_add_style(qso_time_input, getStyleTextarea(), 0);
    addNavigableWidget(qso_time_input);

    // Auto-fill date and time
    char dateStr[12], timeStr[8];
    getCurrentDateTimeStrings(dateStr, timeStr);
    lv_textarea_set_text(qso_date_input, dateStr);
    lv_textarea_set_text(qso_time_input, timeStr);

    // Row 5: My Grid + My POTA (side by side)
    lv_obj_t* my_loc_row = lv_obj_create(form);
    lv_obj_set_size(my_loc_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(my_loc_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(my_loc_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(my_loc_row, 15, 0);
    lv_obj_set_style_bg_opa(my_loc_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(my_loc_row, 0, 0);
    lv_obj_set_style_pad_all(my_loc_row, 0, 0);
    lv_obj_clear_flag(my_loc_row, LV_OBJ_FLAG_SCROLLABLE);

    // My Grid column
    lv_obj_t* my_grid_col = lv_obj_create(my_loc_row);
    lv_obj_set_size(my_grid_col, lv_pct(45), LV_SIZE_CONTENT);
    lv_obj_set_layout(my_grid_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(my_grid_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(my_grid_col, 3, 0);
    lv_obj_set_style_bg_opa(my_grid_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(my_grid_col, 0, 0);
    lv_obj_set_style_pad_all(my_grid_col, 0, 0);
    lv_obj_clear_flag(my_grid_col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* my_grid_label = lv_label_create(my_grid_col);
    lv_label_set_text(my_grid_label, "My Grid");
    lv_obj_set_style_text_color(my_grid_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(my_grid_label, getThemeFonts()->font_small, 0);

    qso_my_grid_input = lv_textarea_create(my_grid_col);
    lv_obj_set_size(qso_my_grid_input, lv_pct(100), 35);
    lv_textarea_set_one_line(qso_my_grid_input, true);
    lv_textarea_set_max_length(qso_my_grid_input, 6);
    lv_textarea_set_placeholder_text(qso_my_grid_input, "EN52wa");
    lv_obj_add_style(qso_my_grid_input, getStyleTextarea(), 0);
    addNavigableWidget(qso_my_grid_input);

    // My POTA column
    lv_obj_t* my_pota_col = lv_obj_create(my_loc_row);
    lv_obj_set_size(my_pota_col, lv_pct(50), LV_SIZE_CONTENT);
    lv_obj_set_layout(my_pota_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(my_pota_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(my_pota_col, 3, 0);
    lv_obj_set_style_bg_opa(my_pota_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(my_pota_col, 0, 0);
    lv_obj_set_style_pad_all(my_pota_col, 0, 0);
    lv_obj_clear_flag(my_pota_col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* my_pota_label = lv_label_create(my_pota_col);
    lv_label_set_text(my_pota_label, "My POTA");
    lv_obj_set_style_text_color(my_pota_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(my_pota_label, getThemeFonts()->font_small, 0);

    qso_my_pota_input = lv_textarea_create(my_pota_col);
    lv_obj_set_size(qso_my_pota_input, lv_pct(100), 35);
    lv_textarea_set_one_line(qso_my_pota_input, true);
    lv_textarea_set_max_length(qso_my_pota_input, 10);
    lv_textarea_set_placeholder_text(qso_my_pota_input, "US-2256");
    lv_obj_add_style(qso_my_pota_input, getStyleTextarea(), 0);
    addNavigableWidget(qso_my_pota_input);

    // Row 6: Notes
    lv_obj_t* notes_label = lv_label_create(form);
    lv_label_set_text(notes_label, "Notes");
    lv_obj_set_style_text_color(notes_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(notes_label, getThemeFonts()->font_small, 0);

    qso_notes_input = lv_textarea_create(form);
    lv_obj_set_size(qso_notes_input, lv_pct(100), 35);
    lv_textarea_set_one_line(qso_notes_input, true);
    lv_textarea_set_max_length(qso_notes_input, 60);
    lv_textarea_set_placeholder_text(qso_notes_input, "Optional notes");
    lv_obj_add_style(qso_notes_input, getStyleTextarea(), 0);
    addNavigableWidget(qso_notes_input);

    // Load operator settings (grid, pota) from preferences
    loadOperatorSettingsToForm();

    // Add key handler to all textarea inputs for ENTER/ESC handling
    // LVGL textareas consume key events, so we must attach our handler to each
    lv_obj_t* textareas[] = {
        qso_callsign_input, qso_freq_input, qso_rst_sent_input, qso_rst_rcvd_input,
        qso_date_input, qso_time_input, qso_my_grid_input, qso_my_pota_input, qso_notes_input
    };
    for (int i = 0; i < 9; i++) {
        lv_obj_add_event_cb(textareas[i], qso_entry_key_cb, LV_EVENT_KEY, NULL);
    }

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, LV_SYMBOL_UP LV_SYMBOL_DOWN " Navigate   " LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " Mode   ENTER Save   ESC Cancel");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    // Add key handler to all navigable widgets for UP/DOWN navigation
    lv_obj_add_event_cb(qso_callsign_input, qso_entry_key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(qso_freq_input, qso_entry_key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(qso_mode_row, qso_entry_key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(qso_rst_sent_input, qso_entry_key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(qso_rst_rcvd_input, qso_entry_key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(qso_date_input, qso_entry_key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(qso_time_input, qso_entry_key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(qso_my_grid_input, qso_entry_key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(qso_my_pota_input, qso_entry_key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(qso_notes_input, qso_entry_key_cb, LV_EVENT_KEY, NULL);

    // Reset focus state and focus first field
    qso_entry_focus = 0;
    lv_group_t* group = getLVGLInputGroup();
    if (group) {
        lv_group_focus_obj(qso_callsign_input);
    }

    qso_entry_screen = screen;
    return screen;
}

// ============================================
// Bluetooth HID Screen
// ============================================

static lv_obj_t* bt_hid_screen = NULL;
static lv_obj_t* bt_hid_status_label = NULL;
static lv_obj_t* bt_hid_device_name_label = NULL;
static lv_obj_t* bt_hid_dit_indicator = NULL;
static lv_obj_t* bt_hid_dah_indicator = NULL;
static lv_obj_t* bt_hid_keyer_label = NULL;

// Forward declaration for keyer mode cycling (defined in ble_hid.h)
extern void cycleBTHIDKeyerMode(int direction);

// Key event callback for BT HID keyboard input (arrows to change keyer, ESC to exit)
static void bt_hid_key_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ESC) {
        onLVGLBackNavigation();
        lv_event_stop_processing(e);
    } else if (key == LV_KEY_LEFT) {
        cycleBTHIDKeyerMode(-1);  // Previous keyer mode
        lv_event_stop_processing(e);
    } else if (key == LV_KEY_RIGHT) {
        cycleBTHIDKeyerMode(1);   // Next keyer mode
        lv_event_stop_processing(e);
    }
}

lv_obj_t* createBTHIDScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "BT KEYBOARD");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // === Single Centered Card ===
    lv_obj_t* card = lv_obj_create(screen);
    lv_obj_set_size(card, 400, 210);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 5);
    applyCardStyle(card);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(card, 15, 0);

    // Row 1: Bluetooth icon + Device name (top, centered)
    lv_obj_t* name_row = lv_obj_create(card);
    lv_obj_set_size(name_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(name_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(name_row, 0, 0);
    lv_obj_set_style_pad_all(name_row, 0, 0);
    lv_obj_align(name_row, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(name_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* bt_icon = lv_label_create(name_row);
    lv_label_set_text(bt_icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_font(bt_icon, getThemeFonts()->font_large, 0);  // Theme font includes symbols
    lv_obj_set_style_text_color(bt_icon, LV_COLOR_ACCENT_BLUE, 0);
    lv_obj_align(bt_icon, LV_ALIGN_LEFT_MID, 100, 0);

    bt_hid_device_name_label = lv_label_create(name_row);
    lv_label_set_text(bt_hid_device_name_label, "VAIL-SUMMIT-XXXXXX");
    lv_obj_set_style_text_color(bt_hid_device_name_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(bt_hid_device_name_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_align(bt_hid_device_name_label, LV_ALIGN_LEFT_MID, 140, 0);

    // Row 2: Connection status (centered, colored)
    bt_hid_status_label = lv_label_create(card);
    lv_label_set_text(bt_hid_status_label, "Advertising...");
    lv_obj_set_style_text_font(bt_hid_status_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(bt_hid_status_label, LV_COLOR_WARNING, 0);
    lv_obj_align(bt_hid_status_label, LV_ALIGN_TOP_MID, 0, 35);

    // Row 3: Keyer mode selector (< Passthrough >)
    lv_obj_t* keyer_row = lv_obj_create(card);
    lv_obj_set_size(keyer_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(keyer_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(keyer_row, 0, 0);
    lv_obj_set_style_pad_all(keyer_row, 0, 0);
    lv_obj_align(keyer_row, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_clear_flag(keyer_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* keyer_title = lv_label_create(keyer_row);
    lv_label_set_text(keyer_title, "Keyer:");
    lv_obj_set_style_text_color(keyer_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(keyer_title, getThemeFonts()->font_body, 0);
    lv_obj_align(keyer_title, LV_ALIGN_LEFT_MID, 70, 0);

    bt_hid_keyer_label = lv_label_create(keyer_row);
    lv_label_set_text(bt_hid_keyer_label, "< Passthrough >");
    lv_obj_set_style_text_color(bt_hid_keyer_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(bt_hid_keyer_label, getThemeFonts()->font_subtitle, 0);
    lv_obj_align(bt_hid_keyer_label, LV_ALIGN_LEFT_MID, 145, 0);

    // Row 4: Key mapping (DIT -> Left Ctrl, DAH -> Right Ctrl)
    lv_obj_t* mapping_row = lv_obj_create(card);
    lv_obj_set_size(mapping_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(mapping_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mapping_row, 0, 0);
    lv_obj_set_style_pad_all(mapping_row, 0, 0);
    lv_obj_align(mapping_row, LV_ALIGN_TOP_MID, 0, 95);
    lv_obj_clear_flag(mapping_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* dit_mapping = lv_label_create(mapping_row);
    lv_label_set_text(dit_mapping, "DIT " LV_SYMBOL_RIGHT " Left Ctrl");
    lv_obj_set_style_text_color(dit_mapping, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(dit_mapping, getThemeFonts()->font_small, 0);
    lv_obj_align(dit_mapping, LV_ALIGN_LEFT_MID, 50, 0);

    lv_obj_t* dah_mapping = lv_label_create(mapping_row);
    lv_label_set_text(dah_mapping, "DAH " LV_SYMBOL_RIGHT " Right Ctrl");
    lv_obj_set_style_text_color(dah_mapping, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(dah_mapping, getThemeFonts()->font_small, 0);
    lv_obj_align(dah_mapping, LV_ALIGN_LEFT_MID, 220, 0);

    // Row 5: Paddle LED indicators at bottom
    lv_obj_t* indicator_row = lv_obj_create(card);
    lv_obj_set_size(indicator_row, lv_pct(100), 50);
    lv_obj_set_style_bg_opa(indicator_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(indicator_row, 0, 0);
    lv_obj_set_style_pad_all(indicator_row, 0, 0);
    lv_obj_align(indicator_row, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(indicator_row, LV_OBJ_FLAG_SCROLLABLE);

    // DIT indicator (left side)
    bt_hid_dit_indicator = lv_led_create(indicator_row);
    lv_led_set_color(bt_hid_dit_indicator, lv_color_hex(0x00FF00));
    lv_obj_set_size(bt_hid_dit_indicator, 30, 30);
    lv_obj_align(bt_hid_dit_indicator, LV_ALIGN_LEFT_MID, 100, 0);
    lv_led_off(bt_hid_dit_indicator);

    lv_obj_t* dit_label = lv_label_create(indicator_row);
    lv_label_set_text(dit_label, "DIT");
    lv_obj_set_style_text_color(dit_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(dit_label, getThemeFonts()->font_body, 0);
    lv_obj_align(dit_label, LV_ALIGN_LEFT_MID, 140, 0);

    // DAH indicator (right side)
    bt_hid_dah_indicator = lv_led_create(indicator_row);
    lv_led_set_color(bt_hid_dah_indicator, lv_color_hex(0x00FF00));
    lv_obj_set_size(bt_hid_dah_indicator, 30, 30);
    lv_obj_align(bt_hid_dah_indicator, LV_ALIGN_LEFT_MID, 210, 0);
    lv_led_off(bt_hid_dah_indicator);

    lv_obj_t* dah_label = lv_label_create(indicator_row);
    lv_label_set_text(dah_label, "DAH");
    lv_obj_set_style_text_color(dah_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(dah_label, getThemeFonts()->font_body, 0);
    lv_obj_align(dah_label, LV_ALIGN_LEFT_MID, 250, 0);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " Change Keyer    Paddle to key    ESC Exit");
    lv_obj_set_style_text_color(help, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    // Invisible focus container for keyboard handling
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);
    lv_obj_set_pos(focus_container, -10, -10);
    lv_obj_set_style_bg_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, LV_STATE_FOCUSED);
    lv_obj_clear_flag(focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(focus_container, bt_hid_key_event_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus_container);
    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }
    lv_group_focus_obj(focus_container);

    bt_hid_screen = screen;
    return screen;
}

void updateBTHIDStatus(const char* status, bool connected) {
    if (bt_hid_status_label != NULL) {
        lv_label_set_text(bt_hid_status_label, status);
        if (connected) {
            lv_obj_set_style_text_color(bt_hid_status_label, LV_COLOR_SUCCESS, 0);
        } else {
            lv_obj_set_style_text_color(bt_hid_status_label, LV_COLOR_WARNING, 0);
        }
    }
}

void updateBTHIDDeviceName(const char* name) {
    if (bt_hid_device_name_label != NULL) {
        lv_label_set_text(bt_hid_device_name_label, name);
    }
}

void updateBTHIDPaddleIndicators(bool ditPressed, bool dahPressed) {
    if (bt_hid_dit_indicator != NULL) {
        if (ditPressed) {
            lv_led_on(bt_hid_dit_indicator);
        } else {
            lv_led_off(bt_hid_dit_indicator);
        }
    }
    if (bt_hid_dah_indicator != NULL) {
        if (dahPressed) {
            lv_led_on(bt_hid_dah_indicator);
        } else {
            lv_led_off(bt_hid_dah_indicator);
        }
    }
}

void updateBTHIDKeyerMode(const char* mode) {
    if (bt_hid_keyer_label != NULL) {
        char buf[32];
        snprintf(buf, sizeof(buf), "< %s >", mode);
        lv_label_set_text(bt_hid_keyer_label, buf);
    }
}

void cleanupBTHIDScreen() {
    bt_hid_screen = NULL;
    bt_hid_status_label = NULL;
    bt_hid_device_name_label = NULL;
    bt_hid_dit_indicator = NULL;
    bt_hid_dah_indicator = NULL;
    bt_hid_keyer_label = NULL;
}

// ============================================
// QSO Logger Settings Screen (Mode 40)
// Configure location (Grid Square or POTA Park)
// ============================================

static lv_obj_t* logger_settings_screen = NULL;
static lv_obj_t* logger_settings_focus_container = NULL;
static lv_obj_t* logger_mode_row = NULL;
static lv_obj_t* logger_mode_value = NULL;
static lv_obj_t* logger_location_row = NULL;
static lv_obj_t* logger_location_input = NULL;
static lv_obj_t* logger_qth_row = NULL;
static lv_obj_t* logger_qth_input = NULL;
static lv_obj_t* logger_pota_status_card = NULL;
static lv_obj_t* logger_pota_name_label = NULL;
static lv_obj_t* logger_pota_location_label = NULL;
static lv_obj_t* logger_pota_grid_label = NULL;
static lv_obj_t* logger_footer_label = NULL;

static int logger_settings_focus = 0;  // 0=mode, 1=location, 2=qth (grid mode only)
static int logger_location_mode = 0;   // 0=Grid Square, 1=POTA Park

static const char* logger_mode_names[] = {"Grid Square", "POTA Park"};

// Forward declarations from qso_logger_settings.h
extern LoggerSettingsState loggerSettings;
extern void saveLoggerLocation();
extern void loadLoggerLocation();

// Forward declaration from qso_logger_validation.h
extern bool validateGridSquare(const char* grid);
extern bool validatePOTAReference(const char* ref);

// Forward declaration from pota_api.h
extern bool lookupPOTAPark(const char* reference, POTAPark& park);

// Update visual focus indicators for logger settings
static void updateLoggerSettingsFocus() {
    // Mode row styling
    if (logger_settings_focus == 0) {
        lv_obj_set_style_bg_color(logger_mode_row, LV_COLOR_CARD_TEAL, 0);
        lv_obj_set_style_bg_opa(logger_mode_row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(logger_mode_row, LV_COLOR_ACCENT_CYAN, 0);
        lv_obj_set_style_border_width(logger_mode_row, 2, 0);
        lv_obj_set_style_text_color(logger_mode_value, LV_COLOR_ACCENT_CYAN, 0);
    } else {
        lv_obj_set_style_bg_color(logger_mode_row, LV_COLOR_BG_LAYER2, 0);
        lv_obj_set_style_bg_opa(logger_mode_row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(logger_mode_row, LV_COLOR_BORDER_SUBTLE, 0);
        lv_obj_set_style_border_width(logger_mode_row, 1, 0);
        lv_obj_set_style_text_color(logger_mode_value, LV_COLOR_TEXT_PRIMARY, 0);
    }

    // Location row styling
    if (logger_settings_focus == 1) {
        lv_obj_set_style_border_color(logger_location_row, LV_COLOR_ACCENT_CYAN, 0);
        lv_obj_set_style_border_width(logger_location_row, 2, 0);
    } else {
        lv_obj_set_style_border_color(logger_location_row, LV_COLOR_BORDER_SUBTLE, 0);
        lv_obj_set_style_border_width(logger_location_row, 1, 0);
    }

    // QTH row styling (only visible in Grid mode)
    if (logger_qth_row != NULL && logger_location_mode == 0) {
        if (logger_settings_focus == 2) {
            lv_obj_set_style_border_color(logger_qth_row, LV_COLOR_ACCENT_CYAN, 0);
            lv_obj_set_style_border_width(logger_qth_row, 2, 0);
        } else {
            lv_obj_set_style_border_color(logger_qth_row, LV_COLOR_BORDER_SUBTLE, 0);
            lv_obj_set_style_border_width(logger_qth_row, 1, 0);
        }
    }
}

// Update footer text based on current focus and mode
static void updateLoggerSettingsFooter() {
    if (logger_footer_label == NULL) return;

    if (logger_settings_focus == 0) {
        lv_label_set_text(logger_footer_label, "L/R Change Mode   UP/DN Navigate   ESC Back");
    } else if (logger_settings_focus == 1) {
        if (logger_location_mode == 1) { // POTA mode
            lv_label_set_text(logger_footer_label, "Type ref   ENTER Lookup   UP/DN Navigate   ESC Back");
        } else { // Grid mode
            lv_label_set_text(logger_footer_label, "Type grid   UP/DN Navigate   ESC Back (auto-saves)");
        }
    } else if (logger_settings_focus == 2) {
        lv_label_set_text(logger_footer_label, "Type QTH   UP/DN Navigate   ESC Back (auto-saves)");
    }
}

// Update POTA status card visibility and content
static void updateLoggerPOTAStatus() {
    if (logger_pota_status_card == NULL) return;

    if (logger_location_mode == 0) {
        // Grid mode - hide POTA status, show QTH row
        lv_obj_add_flag(logger_pota_status_card, LV_OBJ_FLAG_HIDDEN);
        if (logger_qth_row != NULL) {
            lv_obj_clear_flag(logger_qth_row, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        // POTA mode - show POTA status, hide QTH row
        lv_obj_clear_flag(logger_pota_status_card, LV_OBJ_FLAG_HIDDEN);
        if (logger_qth_row != NULL) {
            lv_obj_add_flag(logger_qth_row, LV_OBJ_FLAG_HIDDEN);
        }

        // Update status card content
        if (loggerSettings.potaLookupDone) {
            if (loggerSettings.potaLookupSuccess && loggerSettings.potaPark.valid) {
                // Success - green card
                lv_obj_set_style_bg_color(logger_pota_status_card, lv_color_hex(0x0A3020), 0);
                lv_obj_set_style_border_color(logger_pota_status_card, LV_COLOR_SUCCESS, 0);
                lv_label_set_text_fmt(logger_pota_name_label, "Park Found: %s", loggerSettings.potaPark.name);
                lv_obj_set_style_text_color(logger_pota_name_label, LV_COLOR_SUCCESS, 0);
                lv_label_set_text_fmt(logger_pota_location_label, "Location: %s", loggerSettings.potaPark.locationDesc);
                lv_label_set_text_fmt(logger_pota_grid_label, "Grid: %s", loggerSettings.potaPark.grid6);
            } else {
                // Failed - red card
                lv_obj_set_style_bg_color(logger_pota_status_card, lv_color_hex(0x300A0A), 0);
                lv_obj_set_style_border_color(logger_pota_status_card, LV_COLOR_ERROR, 0);
                lv_label_set_text(logger_pota_name_label, "Park Not Found");
                lv_obj_set_style_text_color(logger_pota_name_label, LV_COLOR_ERROR, 0);
                lv_label_set_text(logger_pota_location_label, "Check reference or try again");
                lv_label_set_text(logger_pota_grid_label, "");
            }
        } else {
            // Not looked up yet - neutral card
            lv_obj_set_style_bg_color(logger_pota_status_card, LV_COLOR_BG_LAYER2, 0);
            lv_obj_set_style_border_color(logger_pota_status_card, LV_COLOR_BORDER_SUBTLE, 0);
            lv_label_set_text(logger_pota_name_label, "Enter POTA reference and press ENTER");
            lv_obj_set_style_text_color(logger_pota_name_label, LV_COLOR_TEXT_SECONDARY, 0);
            lv_label_set_text(logger_pota_location_label, "Format: US-1234 or K-1234");
            lv_label_set_text(logger_pota_grid_label, "");
        }
    }
}

// Key event handler for logger settings
static void logger_settings_key_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // ESC - save and exit
    if (key == LV_KEY_ESC) {
        saveLoggerLocation();
        onLVGLBackNavigation();
        lv_event_stop_bubbling(e);
        return;
    }

    // UP/DOWN navigation
    if (key == LV_KEY_UP || key == LV_KEY_PREV) {
        if (logger_settings_focus > 0) {
            logger_settings_focus--;
            // In POTA mode, skip QTH field (field 2)
            if (logger_location_mode == 1 && logger_settings_focus == 2) {
                logger_settings_focus = 1;
            }
        }
        updateLoggerSettingsFocus();
        updateLoggerSettingsFooter();
        lv_event_stop_bubbling(e);
        return;
    }

    if (key == LV_KEY_DOWN || key == LV_KEY_NEXT) {
        int maxField = (logger_location_mode == 0) ? 2 : 1;  // Grid has 3 fields, POTA has 2
        if (logger_settings_focus < maxField) {
            logger_settings_focus++;
        }
        updateLoggerSettingsFocus();
        updateLoggerSettingsFooter();
        lv_event_stop_bubbling(e);
        return;
    }

    // LEFT/RIGHT - change mode (only on mode row)
    if (logger_settings_focus == 0 && (key == LV_KEY_LEFT || key == LV_KEY_RIGHT)) {
        logger_location_mode = (logger_location_mode + 1) % 2;
        loggerSettings.inputMode = (LocationInputMode)logger_location_mode;
        loggerSettings.potaLookupDone = false;  // Reset lookup on mode change
        lv_label_set_text_fmt(logger_mode_value, "< %s >", logger_mode_names[logger_location_mode]);
        updateLoggerPOTAStatus();
        updateLoggerSettingsFooter();
        lv_event_stop_bubbling(e);
        return;
    }

    // ENTER - lookup POTA park (only on location field in POTA mode)
    if (key == LV_KEY_ENTER && logger_settings_focus == 1 && logger_location_mode == 1) {
        const char* ref = lv_textarea_get_text(logger_location_input);
        strlcpy(loggerSettings.potaInput, ref, sizeof(loggerSettings.potaInput));

        if (validatePOTAReference(loggerSettings.potaInput)) {
            Serial.println("Looking up POTA park...");
            loggerSettings.potaLookupSuccess = lookupPOTAPark(loggerSettings.potaInput, loggerSettings.potaPark);
            loggerSettings.potaLookupDone = true;

            if (loggerSettings.potaLookupSuccess) {
                beep(1000, 100);  // Success beep
                saveLoggerLocation();
            } else {
                beep(600, 100);  // Error beep
            }
            updateLoggerPOTAStatus();
        } else {
            beep(600, 100);  // Invalid format beep
        }
        lv_event_stop_bubbling(e);
        return;
    }

    lv_event_stop_bubbling(e);
}

// Text area event handler to sync input with state
static void logger_location_input_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        const char* text = lv_textarea_get_text(logger_location_input);
        if (logger_location_mode == 0) {
            strlcpy(loggerSettings.gridInput, text, sizeof(loggerSettings.gridInput));
        } else {
            strlcpy(loggerSettings.potaInput, text, sizeof(loggerSettings.potaInput));
            loggerSettings.potaLookupDone = false;  // Reset lookup when text changes
            updateLoggerPOTAStatus();
        }
    }
}

static void logger_qth_input_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        const char* text = lv_textarea_get_text(logger_qth_input);
        strlcpy(loggerSettings.qthInput, text, sizeof(loggerSettings.qthInput));
    }
}

lv_obj_t* createQSOLoggerSettingsScreen() {
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Load saved settings
    loadLoggerLocation();
    logger_location_mode = loggerSettings.inputMode;
    logger_settings_focus = 0;

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "LOGGER SETTINGS");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar
    createCompactStatusBar(screen);

    // Content container
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, SCREEN_WIDTH - 40, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 20);
    lv_obj_set_pos(content, 20, HEADER_HEIGHT + 10);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 10, 0);
    lv_obj_set_style_pad_all(content, 10, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Row 1: Location Mode selector
    lv_obj_t* mode_label = lv_label_create(content);
    lv_label_set_text(mode_label, "Location Mode");
    lv_obj_set_style_text_color(mode_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(mode_label, getThemeFonts()->font_small, 0);

    logger_mode_row = lv_obj_create(content);
    lv_obj_set_size(logger_mode_row, lv_pct(100), 40);
    lv_obj_set_style_bg_color(logger_mode_row, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_radius(logger_mode_row, 8, 0);
    lv_obj_set_style_border_width(logger_mode_row, 1, 0);
    lv_obj_set_style_border_color(logger_mode_row, LV_COLOR_BORDER_SUBTLE, 0);
    lv_obj_set_style_pad_all(logger_mode_row, 8, 0);
    lv_obj_clear_flag(logger_mode_row, LV_OBJ_FLAG_SCROLLABLE);

    logger_mode_value = lv_label_create(logger_mode_row);
    lv_label_set_text_fmt(logger_mode_value, "< %s >", logger_mode_names[logger_location_mode]);
    lv_obj_set_style_text_font(logger_mode_value, getThemeFonts()->font_subtitle, 0);
    lv_obj_center(logger_mode_value);

    // Row 2: Location input (Grid or POTA ref)
    lv_obj_t* location_label = lv_label_create(content);
    if (logger_location_mode == 0) {
        lv_label_set_text(location_label, "Grid Square (e.g., EN52wa)");
    } else {
        lv_label_set_text(location_label, "POTA Reference (e.g., US-2256)");
    }
    lv_obj_set_style_text_color(location_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(location_label, getThemeFonts()->font_small, 0);

    logger_location_row = lv_obj_create(content);
    lv_obj_set_size(logger_location_row, lv_pct(100), 45);
    lv_obj_set_style_bg_color(logger_location_row, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_radius(logger_location_row, 8, 0);
    lv_obj_set_style_border_width(logger_location_row, 1, 0);
    lv_obj_set_style_border_color(logger_location_row, LV_COLOR_BORDER_SUBTLE, 0);
    lv_obj_set_style_pad_all(logger_location_row, 5, 0);
    lv_obj_clear_flag(logger_location_row, LV_OBJ_FLAG_SCROLLABLE);

    logger_location_input = lv_textarea_create(logger_location_row);
    lv_obj_set_size(logger_location_input, lv_pct(100), 35);
    lv_textarea_set_one_line(logger_location_input, true);
    lv_textarea_set_max_length(logger_location_input, logger_location_mode == 0 ? 6 : 10);
    if (logger_location_mode == 0) {
        lv_textarea_set_text(logger_location_input, loggerSettings.gridInput);
        lv_textarea_set_placeholder_text(logger_location_input, "EN52wa");
    } else {
        lv_textarea_set_text(logger_location_input, loggerSettings.potaInput);
        lv_textarea_set_placeholder_text(logger_location_input, "US-2256");
    }
    lv_obj_add_style(logger_location_input, getStyleTextarea(), 0);
    lv_obj_add_event_cb(logger_location_input, logger_location_input_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_center(logger_location_input);

    // Row 3: QTH input (Grid mode only)
    lv_obj_t* qth_label = lv_label_create(content);
    lv_label_set_text(qth_label, "QTH (Optional)");
    lv_obj_set_style_text_color(qth_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(qth_label, getThemeFonts()->font_small, 0);

    logger_qth_row = lv_obj_create(content);
    lv_obj_set_size(logger_qth_row, lv_pct(100), 45);
    lv_obj_set_style_bg_color(logger_qth_row, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_radius(logger_qth_row, 8, 0);
    lv_obj_set_style_border_width(logger_qth_row, 1, 0);
    lv_obj_set_style_border_color(logger_qth_row, LV_COLOR_BORDER_SUBTLE, 0);
    lv_obj_set_style_pad_all(logger_qth_row, 5, 0);
    lv_obj_clear_flag(logger_qth_row, LV_OBJ_FLAG_SCROLLABLE);

    logger_qth_input = lv_textarea_create(logger_qth_row);
    lv_obj_set_size(logger_qth_input, lv_pct(100), 35);
    lv_textarea_set_one_line(logger_qth_input, true);
    lv_textarea_set_max_length(logger_qth_input, 40);
    lv_textarea_set_text(logger_qth_input, loggerSettings.qthInput);
    lv_textarea_set_placeholder_text(logger_qth_input, "City, State");
    lv_obj_add_style(logger_qth_input, getStyleTextarea(), 0);
    lv_obj_add_event_cb(logger_qth_input, logger_qth_input_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_center(logger_qth_input);

    // Row 4: POTA status card (POTA mode only)
    logger_pota_status_card = lv_obj_create(content);
    lv_obj_set_size(logger_pota_status_card, lv_pct(100), 70);
    lv_obj_set_style_bg_color(logger_pota_status_card, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_radius(logger_pota_status_card, 8, 0);
    lv_obj_set_style_border_width(logger_pota_status_card, 1, 0);
    lv_obj_set_style_border_color(logger_pota_status_card, LV_COLOR_BORDER_SUBTLE, 0);
    lv_obj_set_style_pad_all(logger_pota_status_card, 8, 0);
    lv_obj_clear_flag(logger_pota_status_card, LV_OBJ_FLAG_SCROLLABLE);

    logger_pota_name_label = lv_label_create(logger_pota_status_card);
    lv_label_set_text(logger_pota_name_label, "");
    lv_obj_set_style_text_font(logger_pota_name_label, getThemeFonts()->font_body, 0);
    lv_obj_align(logger_pota_name_label, LV_ALIGN_TOP_LEFT, 0, 0);

    logger_pota_location_label = lv_label_create(logger_pota_status_card);
    lv_label_set_text(logger_pota_location_label, "");
    lv_obj_set_style_text_font(logger_pota_location_label, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(logger_pota_location_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(logger_pota_location_label, LV_ALIGN_TOP_LEFT, 0, 20);

    logger_pota_grid_label = lv_label_create(logger_pota_status_card);
    lv_label_set_text(logger_pota_grid_label, "");
    lv_obj_set_style_text_font(logger_pota_grid_label, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(logger_pota_grid_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(logger_pota_grid_label, LV_ALIGN_TOP_LEFT, 0, 38);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    logger_footer_label = lv_label_create(footer);
    lv_label_set_text(logger_footer_label, "L/R Change Mode   UP/DN Navigate   ESC Back");
    lv_obj_set_style_text_color(logger_footer_label, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(logger_footer_label, getThemeFonts()->font_small, 0);
    lv_obj_center(logger_footer_label);

    // Invisible focus container for key handling
    logger_settings_focus_container = lv_obj_create(screen);
    lv_obj_set_size(logger_settings_focus_container, 0, 0);
    lv_obj_set_style_bg_opa(logger_settings_focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(logger_settings_focus_container, 0, 0);
    lv_obj_add_flag(logger_settings_focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(logger_settings_focus_container, logger_settings_key_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(logger_settings_focus_container);

    // Add textareas to nav group (allows keyboard input to them)
    addNavigableWidget(logger_location_input);
    addNavigableWidget(logger_qth_input);

    // Enable edit mode for direct key handling
    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }

    // Initialize display state
    updateLoggerSettingsFocus();
    updateLoggerPOTAStatus();
    updateLoggerSettingsFooter();

    logger_settings_screen = screen;
    return screen;
}

// ============================================
// QSO Statistics Screen (Mode 39)
// Display QSO logging statistics
// ============================================

static lv_obj_t* qso_stats_screen = NULL;
static lv_obj_t* qso_stats_focus_container = NULL;
static lv_obj_t* qso_stats_scroll_container = NULL;
static lv_obj_t* qso_stats_total_label = NULL;
static lv_obj_t* qso_stats_unique_label = NULL;
static lv_obj_t* qso_stats_active_label = NULL;
static lv_obj_t* qso_stats_last_label = NULL;

// Forward declarations from qso_logger_statistics.h
extern QSOStatistics stats;
extern void calculateStatistics();

// Key event handler for statistics screen (scroll + ESC)
static void qso_stats_key_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    if (key == LV_KEY_ESC) {
        onLVGLBackNavigation();
        lv_event_stop_bubbling(e);
        return;
    }

    // Scroll with UP/DOWN
    if (qso_stats_scroll_container != NULL) {
        if (key == LV_KEY_UP || key == LV_KEY_PREV) {
            lv_obj_scroll_by(qso_stats_scroll_container, 0, 30, LV_ANIM_ON);
            lv_event_stop_bubbling(e);
            return;
        }
        if (key == LV_KEY_DOWN || key == LV_KEY_NEXT) {
            lv_obj_scroll_by(qso_stats_scroll_container, 0, -30, LV_ANIM_ON);
            lv_event_stop_bubbling(e);
            return;
        }
    }

    lv_event_stop_bubbling(e);
}

// Format date from YYYYMMDD to MM/DD/YY
static void formatDateShort(const char* yyyymmdd, char* out, size_t outSize) {
    if (strlen(yyyymmdd) >= 8) {
        snprintf(out, outSize, "%c%c/%c%c/%c%c",
                 yyyymmdd[4], yyyymmdd[5],   // MM
                 yyyymmdd[6], yyyymmdd[7],   // DD
                 yyyymmdd[2], yyyymmdd[3]);  // YY
    } else {
        strlcpy(out, yyyymmdd, outSize);
    }
}

lv_obj_t* createQSOStatisticsScreen() {
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Calculate statistics from QSO files
    calculateStatistics();

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "QSO STATISTICS");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar
    createCompactStatusBar(screen);

    // Scrollable content container
    qso_stats_scroll_container = lv_obj_create(screen);
    lv_obj_set_size(qso_stats_scroll_container, SCREEN_WIDTH - 20, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 10);
    lv_obj_set_pos(qso_stats_scroll_container, 10, HEADER_HEIGHT + 5);
    lv_obj_set_layout(qso_stats_scroll_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(qso_stats_scroll_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(qso_stats_scroll_container, 8, 0);
    lv_obj_set_style_pad_all(qso_stats_scroll_container, 5, 0);
    lv_obj_set_style_bg_opa(qso_stats_scroll_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(qso_stats_scroll_container, 0, 0);
    lv_obj_add_flag(qso_stats_scroll_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(qso_stats_scroll_container, LV_SCROLLBAR_MODE_AUTO);

    if (stats.totalQSOs == 0) {
        // No data message
        lv_obj_t* no_data = lv_label_create(qso_stats_scroll_container);
        lv_label_set_text(no_data, "No QSO data available");
        lv_obj_set_style_text_color(no_data, LV_COLOR_WARNING, 0);
        lv_obj_set_style_text_font(no_data, getThemeFonts()->font_subtitle, 0);
        lv_obj_set_width(no_data, lv_pct(100));
        lv_obj_set_style_text_align(no_data, LV_TEXT_ALIGN_CENTER, 0);
    } else {
        // Stats cards row (2x2 grid)
        lv_obj_t* cards_row = lv_obj_create(qso_stats_scroll_container);
        lv_obj_set_size(cards_row, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_layout(cards_row, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(cards_row, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_style_pad_column(cards_row, 10, 0);
        lv_obj_set_style_pad_row(cards_row, 8, 0);
        lv_obj_set_style_pad_all(cards_row, 0, 0);
        lv_obj_set_style_bg_opa(cards_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(cards_row, 0, 0);
        lv_obj_clear_flag(cards_row, LV_OBJ_FLAG_SCROLLABLE);

        // Card 1: Total QSOs
        lv_obj_t* total_card = lv_obj_create(cards_row);
        lv_obj_set_size(total_card, 210, 55);
        applyCardStyle(total_card);
        lv_obj_set_style_pad_all(total_card, 8, 0);
        lv_obj_clear_flag(total_card, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* total_title = lv_label_create(total_card);
        lv_label_set_text(total_title, "Total QSOs");
        lv_obj_set_style_text_color(total_title, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_style_text_font(total_title, getThemeFonts()->font_small, 0);
        lv_obj_align(total_title, LV_ALIGN_TOP_LEFT, 0, 0);

        qso_stats_total_label = lv_label_create(total_card);
        lv_label_set_text_fmt(qso_stats_total_label, "%d", stats.totalQSOs);
        lv_obj_set_style_text_color(qso_stats_total_label, LV_COLOR_ACCENT_CYAN, 0);
        lv_obj_set_style_text_font(qso_stats_total_label, getThemeFonts()->font_large, 0);
        lv_obj_align(qso_stats_total_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);

        // Card 2: Unique Callsigns
        lv_obj_t* unique_card = lv_obj_create(cards_row);
        lv_obj_set_size(unique_card, 210, 55);
        applyCardStyle(unique_card);
        lv_obj_set_style_pad_all(unique_card, 8, 0);
        lv_obj_clear_flag(unique_card, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* unique_title = lv_label_create(unique_card);
        lv_label_set_text(unique_title, "Unique Calls");
        lv_obj_set_style_text_color(unique_title, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_style_text_font(unique_title, getThemeFonts()->font_small, 0);
        lv_obj_align(unique_title, LV_ALIGN_TOP_LEFT, 0, 0);

        qso_stats_unique_label = lv_label_create(unique_card);
        lv_label_set_text_fmt(qso_stats_unique_label, "%d", stats.uniqueCallsigns);
        lv_obj_set_style_text_color(qso_stats_unique_label, LV_COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_style_text_font(qso_stats_unique_label, getThemeFonts()->font_large, 0);
        lv_obj_align(qso_stats_unique_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);

        // Card 3: Most Active Date
        lv_obj_t* active_card = lv_obj_create(cards_row);
        lv_obj_set_size(active_card, 210, 55);
        applyCardStyle(active_card);
        lv_obj_set_style_pad_all(active_card, 8, 0);
        lv_obj_clear_flag(active_card, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* active_title = lv_label_create(active_card);
        lv_label_set_text(active_title, "Most Active Day");
        lv_obj_set_style_text_color(active_title, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_style_text_font(active_title, getThemeFonts()->font_small, 0);
        lv_obj_align(active_title, LV_ALIGN_TOP_LEFT, 0, 0);

        qso_stats_active_label = lv_label_create(active_card);
        if (strlen(stats.mostActiveDate) > 0) {
            char dateStr[12];
            formatDateShort(stats.mostActiveDate, dateStr, sizeof(dateStr));
            lv_label_set_text_fmt(qso_stats_active_label, "%s (%d)", dateStr, stats.mostActiveDateCount);
        } else {
            lv_label_set_text(qso_stats_active_label, "-");
        }
        lv_obj_set_style_text_color(qso_stats_active_label, LV_COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_style_text_font(qso_stats_active_label, getThemeFonts()->font_body, 0);
        lv_obj_align(qso_stats_active_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);

        // Card 4: Last QSO
        lv_obj_t* last_card = lv_obj_create(cards_row);
        lv_obj_set_size(last_card, 210, 55);
        applyCardStyle(last_card);
        lv_obj_set_style_pad_all(last_card, 8, 0);
        lv_obj_clear_flag(last_card, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* last_title = lv_label_create(last_card);
        lv_label_set_text(last_title, "Last QSO");
        lv_obj_set_style_text_color(last_title, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_style_text_font(last_title, getThemeFonts()->font_small, 0);
        lv_obj_align(last_title, LV_ALIGN_TOP_LEFT, 0, 0);

        qso_stats_last_label = lv_label_create(last_card);
        if (strlen(stats.lastQSODate) > 0) {
            char dateStr[12];
            formatDateShort(stats.lastQSODate, dateStr, sizeof(dateStr));
            lv_label_set_text(qso_stats_last_label, dateStr);
        } else {
            lv_label_set_text(qso_stats_last_label, "-");
        }
        lv_obj_set_style_text_color(qso_stats_last_label, LV_COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_style_text_font(qso_stats_last_label, getThemeFonts()->font_body, 0);
        lv_obj_align(qso_stats_last_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);

        // Bands section
        if (stats.bandCount > 0) {
            lv_obj_t* bands_section = lv_obj_create(qso_stats_scroll_container);
            lv_obj_set_size(bands_section, lv_pct(100), LV_SIZE_CONTENT);
            lv_obj_set_layout(bands_section, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(bands_section, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_pad_row(bands_section, 4, 0);
            lv_obj_set_style_pad_all(bands_section, 8, 0);
            applyCardStyle(bands_section);
            lv_obj_clear_flag(bands_section, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t* bands_title = lv_label_create(bands_section);
            lv_label_set_text(bands_title, "Bands");
            lv_obj_set_style_text_color(bands_title, LV_COLOR_ACCENT_CYAN, 0);
            lv_obj_set_style_text_font(bands_title, getThemeFonts()->font_body, 0);

            for (int i = 0; i < stats.bandCount && i < 10; i++) {
                lv_obj_t* band_row = lv_obj_create(bands_section);
                lv_obj_set_size(band_row, lv_pct(100), 20);
                lv_obj_set_style_bg_opa(band_row, LV_OPA_TRANSP, 0);
                lv_obj_set_style_border_width(band_row, 0, 0);
                lv_obj_set_style_pad_all(band_row, 0, 0);
                lv_obj_clear_flag(band_row, LV_OBJ_FLAG_SCROLLABLE);

                lv_obj_t* band_name = lv_label_create(band_row);
                lv_label_set_text_fmt(band_name, "%s:", stats.bandStats[i].band);
                lv_obj_set_style_text_font(band_name, getThemeFonts()->font_small, 0);
                lv_obj_align(band_name, LV_ALIGN_LEFT_MID, 0, 0);

                // Bar graph
                int barWidth = (stats.bandStats[i].count * 200) / stats.totalQSOs;
                if (barWidth < 4 && stats.bandStats[i].count > 0) barWidth = 4;

                lv_obj_t* bar = lv_obj_create(band_row);
                lv_obj_set_size(bar, barWidth, 12);
                lv_obj_set_style_bg_color(bar, LV_COLOR_ACCENT_CYAN, 0);
                lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
                lv_obj_set_style_radius(bar, 2, 0);
                lv_obj_set_style_border_width(bar, 0, 0);
                lv_obj_align(bar, LV_ALIGN_LEFT_MID, 45, 0);

                lv_obj_t* band_count = lv_label_create(band_row);
                lv_label_set_text_fmt(band_count, "%d", stats.bandStats[i].count);
                lv_obj_set_style_text_font(band_count, getThemeFonts()->font_small, 0);
                lv_obj_align(band_count, LV_ALIGN_LEFT_MID, 50 + barWidth + 5, 0);
            }
        }

        // Modes section
        if (stats.modeCount > 0) {
            lv_obj_t* modes_section = lv_obj_create(qso_stats_scroll_container);
            lv_obj_set_size(modes_section, lv_pct(100), LV_SIZE_CONTENT);
            lv_obj_set_layout(modes_section, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(modes_section, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_pad_row(modes_section, 4, 0);
            lv_obj_set_style_pad_all(modes_section, 8, 0);
            applyCardStyle(modes_section);
            lv_obj_clear_flag(modes_section, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t* modes_title = lv_label_create(modes_section);
            lv_label_set_text(modes_title, "Modes");
            lv_obj_set_style_text_color(modes_title, LV_COLOR_SUCCESS, 0);
            lv_obj_set_style_text_font(modes_title, getThemeFonts()->font_body, 0);

            for (int i = 0; i < stats.modeCount && i < 8; i++) {
                lv_obj_t* mode_row = lv_obj_create(modes_section);
                lv_obj_set_size(mode_row, lv_pct(100), 20);
                lv_obj_set_style_bg_opa(mode_row, LV_OPA_TRANSP, 0);
                lv_obj_set_style_border_width(mode_row, 0, 0);
                lv_obj_set_style_pad_all(mode_row, 0, 0);
                lv_obj_clear_flag(mode_row, LV_OBJ_FLAG_SCROLLABLE);

                lv_obj_t* mode_name = lv_label_create(mode_row);
                lv_label_set_text_fmt(mode_name, "%s:", stats.modeStats[i].mode);
                lv_obj_set_style_text_font(mode_name, getThemeFonts()->font_small, 0);
                lv_obj_align(mode_name, LV_ALIGN_LEFT_MID, 0, 0);

                // Bar graph
                int barWidth = (stats.modeStats[i].count * 200) / stats.totalQSOs;
                if (barWidth < 4 && stats.modeStats[i].count > 0) barWidth = 4;

                lv_obj_t* bar = lv_obj_create(mode_row);
                lv_obj_set_size(bar, barWidth, 12);
                lv_obj_set_style_bg_color(bar, LV_COLOR_SUCCESS, 0);
                lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
                lv_obj_set_style_radius(bar, 2, 0);
                lv_obj_set_style_border_width(bar, 0, 0);
                lv_obj_align(bar, LV_ALIGN_LEFT_MID, 45, 0);

                lv_obj_t* mode_count = lv_label_create(mode_row);
                lv_label_set_text_fmt(mode_count, "%d", stats.modeStats[i].count);
                lv_obj_set_style_text_font(mode_count, getThemeFonts()->font_small, 0);
                lv_obj_align(mode_count, LV_ALIGN_LEFT_MID, 50 + barWidth + 5, 0);
            }
        }
    }

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* footer_label = lv_label_create(footer);
    lv_label_set_text(footer_label, "UP/DN Scroll   ESC Back");
    lv_obj_set_style_text_color(footer_label, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(footer_label, getThemeFonts()->font_small, 0);
    lv_obj_center(footer_label);

    // Invisible focus container for key handling
    qso_stats_focus_container = lv_obj_create(screen);
    lv_obj_set_size(qso_stats_focus_container, 0, 0);
    lv_obj_set_style_bg_opa(qso_stats_focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(qso_stats_focus_container, 0, 0);
    lv_obj_add_flag(qso_stats_focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(qso_stats_focus_container, qso_stats_key_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(qso_stats_focus_container);

    // Enable edit mode
    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }

    qso_stats_screen = screen;
    return screen;
}

// ============================================
// QSO View Logs Screen (Mode 38)
// Browse and view saved QSO logs
// ============================================

static lv_obj_t* view_logs_screen = NULL;
static lv_obj_t* view_logs_focus_container = NULL;
static lv_obj_t* view_logs_list_container = NULL;
static lv_obj_t* view_logs_detail_overlay = NULL;
static lv_obj_t* view_logs_count_label = NULL;
static lv_obj_t** view_logs_rows = NULL;
static int view_logs_row_count = 0;
static int view_logs_selected = 0;
static int view_logs_scroll_offset = 0;
static bool view_logs_detail_mode = false;
static bool view_logs_delete_confirm = false;

#define VIEW_LOGS_MAX_VISIBLE 6
#define VIEW_LOGS_ROW_HEIGHT 40

// Forward declarations from qso_logger_view.h
extern ViewState viewState;
extern void loadQSOsForView();
extern void freeQSOsFromView();
extern bool deleteCurrentQSO();

// Update row visual styling based on selection
static void updateViewLogsRowStyles() {
    for (int i = 0; i < view_logs_row_count; i++) {
        int qsoIndex = view_logs_scroll_offset + i;
        if (view_logs_rows[i] == NULL) continue;

        if (qsoIndex == view_logs_selected) {
            lv_obj_set_style_bg_color(view_logs_rows[i], LV_COLOR_CARD_TEAL, 0);
            lv_obj_set_style_border_color(view_logs_rows[i], LV_COLOR_ACCENT_CYAN, 0);
            lv_obj_set_style_border_width(view_logs_rows[i], 2, 0);
        } else {
            lv_obj_set_style_bg_color(view_logs_rows[i], LV_COLOR_BG_LAYER2, 0);
            lv_obj_set_style_border_color(view_logs_rows[i], LV_COLOR_BORDER_SUBTLE, 0);
            lv_obj_set_style_border_width(view_logs_rows[i], 1, 0);
        }
    }
}

// Rebuild the visible rows based on scroll offset
static void rebuildViewLogsList();

// Show detail overlay for selected QSO
static void showQSODetailOverlay() {
    if (viewState.selectedIndex < 0 || viewState.selectedIndex >= viewState.totalQSOs) return;

    QSO& qso = viewState.qsos[viewState.selectedIndex];
    view_logs_detail_mode = true;
    view_logs_delete_confirm = false;

    // Create semi-transparent overlay
    view_logs_detail_overlay = lv_obj_create(view_logs_screen);
    lv_obj_set_size(view_logs_detail_overlay, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(view_logs_detail_overlay, 0, 0);
    lv_obj_set_style_bg_color(view_logs_detail_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(view_logs_detail_overlay, LV_OPA_50, 0);
    lv_obj_clear_flag(view_logs_detail_overlay, LV_OBJ_FLAG_SCROLLABLE);

    // Detail card
    lv_obj_t* card = lv_obj_create(view_logs_detail_overlay);
    lv_obj_set_size(card, 380, 220);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    applyCardStyle(card);
    lv_obj_set_style_pad_all(card, 15, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* title = lv_label_create(card);
    lv_label_set_text(title, "QSO DETAILS");
    lv_obj_set_style_text_color(title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // Callsign (large)
    lv_obj_t* callsign_label = lv_label_create(card);
    lv_label_set_text(callsign_label, qso.callsign);
    lv_obj_set_style_text_color(callsign_label, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(callsign_label, getThemeFonts()->font_large, 0);
    lv_obj_align(callsign_label, LV_ALIGN_TOP_LEFT, 0, 30);

    // Date/Time
    char dateTimeStr[32];
    if (strlen(qso.date) >= 8) {
        snprintf(dateTimeStr, sizeof(dateTimeStr), "%c%c%c%c-%c%c-%c%c",
                 qso.date[0], qso.date[1], qso.date[2], qso.date[3],
                 qso.date[4], qso.date[5], qso.date[6], qso.date[7]);
    } else {
        strlcpy(dateTimeStr, qso.date, sizeof(dateTimeStr));
    }
    if (strlen(qso.time_on) >= 4) {
        char timeStr[16];
        snprintf(timeStr, sizeof(timeStr), " %c%c:%c%c UTC",
                 qso.time_on[0], qso.time_on[1], qso.time_on[2], qso.time_on[3]);
        strlcat(dateTimeStr, timeStr, sizeof(dateTimeStr));
    }

    lv_obj_t* datetime_label = lv_label_create(card);
    lv_label_set_text_fmt(datetime_label, "Date: %s", dateTimeStr);
    lv_obj_set_style_text_font(datetime_label, getThemeFonts()->font_small, 0);
    lv_obj_align(datetime_label, LV_ALIGN_TOP_LEFT, 0, 60);

    // Frequency/Band
    lv_obj_t* freq_label = lv_label_create(card);
    lv_label_set_text_fmt(freq_label, "Freq: %.3f MHz (%s)", qso.frequency, qso.band);
    lv_obj_set_style_text_font(freq_label, getThemeFonts()->font_small, 0);
    lv_obj_align(freq_label, LV_ALIGN_TOP_LEFT, 0, 78);

    // Mode
    lv_obj_t* mode_label = lv_label_create(card);
    lv_label_set_text_fmt(mode_label, "Mode: %s", qso.mode);
    lv_obj_set_style_text_font(mode_label, getThemeFonts()->font_small, 0);
    lv_obj_align(mode_label, LV_ALIGN_TOP_LEFT, 0, 96);

    // RST
    lv_obj_t* rst_label = lv_label_create(card);
    lv_label_set_text_fmt(rst_label, "RST: %s / %s", qso.rst_sent, qso.rst_rcvd);
    lv_obj_set_style_text_font(rst_label, getThemeFonts()->font_small, 0);
    lv_obj_align(rst_label, LV_ALIGN_TOP_LEFT, 0, 114);

    // My Grid (if set)
    int yOffset = 132;
    if (strlen(qso.my_gridsquare) > 0) {
        lv_obj_t* grid_label = lv_label_create(card);
        lv_label_set_text_fmt(grid_label, "My Grid: %s", qso.my_gridsquare);
        lv_obj_set_style_text_font(grid_label, getThemeFonts()->font_small, 0);
        lv_obj_align(grid_label, LV_ALIGN_TOP_LEFT, 0, yOffset);
        yOffset += 18;
    }

    // My POTA (if set)
    if (strlen(qso.my_pota_ref) > 0) {
        lv_obj_t* pota_label = lv_label_create(card);
        lv_label_set_text_fmt(pota_label, "My POTA: %s", qso.my_pota_ref);
        lv_obj_set_style_text_font(pota_label, getThemeFonts()->font_small, 0);
        lv_obj_align(pota_label, LV_ALIGN_TOP_LEFT, 0, yOffset);
        yOffset += 18;
    }

    // Notes (if set)
    if (strlen(qso.notes) > 0) {
        lv_obj_t* notes_label = lv_label_create(card);
        lv_label_set_text_fmt(notes_label, "Notes: %s", qso.notes);
        lv_obj_set_style_text_font(notes_label, getThemeFonts()->font_small, 0);
        lv_obj_set_style_text_color(notes_label, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_width(notes_label, 350);
        lv_label_set_long_mode(notes_label, LV_LABEL_LONG_WRAP);
        lv_obj_align(notes_label, LV_ALIGN_TOP_LEFT, 0, yOffset);
    }

    // Footer in card
    lv_obj_t* card_footer = lv_label_create(card);
    lv_label_set_text(card_footer, "D Delete   ESC Close");
    lv_obj_set_style_text_color(card_footer, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(card_footer, getThemeFonts()->font_small, 0);
    lv_obj_align(card_footer, LV_ALIGN_BOTTOM_MID, 0, 0);
}

// Show delete confirmation in overlay
static void showDeleteConfirmation() {
    view_logs_delete_confirm = true;

    // Clear the overlay content and recreate for delete confirmation
    lv_obj_clean(view_logs_detail_overlay);

    QSO& qso = viewState.qsos[viewState.selectedIndex];

    // Confirmation card
    lv_obj_t* card = lv_obj_create(view_logs_detail_overlay);
    lv_obj_set_size(card, 320, 160);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x300A0A), 0);
    lv_obj_set_style_border_color(card, LV_COLOR_ERROR, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 20, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // Warning title
    lv_obj_t* title = lv_label_create(card);
    lv_label_set_text(title, "DELETE QSO?");
    lv_obj_set_style_text_color(title, LV_COLOR_ERROR, 0);
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // QSO info
    char dateStr[12];
    formatDateShort(qso.date, dateStr, sizeof(dateStr));
    lv_obj_t* info = lv_label_create(card);
    lv_label_set_text_fmt(info, "%s - %s", qso.callsign, dateStr);
    lv_obj_set_style_text_color(info, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(info, getThemeFonts()->font_body, 0);
    lv_obj_align(info, LV_ALIGN_CENTER, 0, -10);

    // Warning
    lv_obj_t* warning = lv_label_create(card);
    lv_label_set_text(warning, "This cannot be undone!");
    lv_obj_set_style_text_color(warning, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(warning, getThemeFonts()->font_small, 0);
    lv_obj_align(warning, LV_ALIGN_CENTER, 0, 15);

    // Footer
    lv_obj_t* footer = lv_label_create(card);
    lv_label_set_text(footer, "Y = Confirm   N = Cancel");
    lv_obj_set_style_text_color(footer, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(footer, getThemeFonts()->font_small, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, 0);
}

// Close the detail overlay
static void closeDetailOverlay() {
    if (view_logs_detail_overlay != NULL) {
        lv_obj_del(view_logs_detail_overlay);
        view_logs_detail_overlay = NULL;
    }
    view_logs_detail_mode = false;
    view_logs_delete_confirm = false;
}

// Key event handler for view logs
static void view_logs_key_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // Handle delete confirmation mode
    if (view_logs_delete_confirm) {
        if (key == 'Y' || key == 'y') {
            viewState.selectedIndex = view_logs_selected;
            if (deleteCurrentQSO()) {
                // Reload QSOs
                freeQSOsFromView();
                loadQSOsForView();

                // Adjust selection
                if (view_logs_selected >= viewState.totalQSOs) {
                    view_logs_selected = viewState.totalQSOs - 1;
                }
                if (view_logs_selected < 0) view_logs_selected = 0;

                closeDetailOverlay();
                rebuildViewLogsList();
                lv_label_set_text_fmt(view_logs_count_label, "VIEW LOGS (%d)", viewState.totalQSOs);
                beep(1000, 100);  // Success
            } else {
                closeDetailOverlay();
                beep(600, 200);  // Error
            }
            lv_event_stop_bubbling(e);
            return;
        }
        if (key == 'N' || key == 'n' || key == LV_KEY_ESC) {
            // Go back to detail view
            closeDetailOverlay();
            showQSODetailOverlay();
            lv_event_stop_bubbling(e);
            return;
        }
        lv_event_stop_bubbling(e);
        return;
    }

    // Handle detail view mode
    if (view_logs_detail_mode) {
        if (key == LV_KEY_ESC) {
            closeDetailOverlay();
            lv_event_stop_bubbling(e);
            return;
        }
        if (key == 'D' || key == 'd') {
            showDeleteConfirmation();
            lv_event_stop_bubbling(e);
            return;
        }
        lv_event_stop_bubbling(e);
        return;
    }

    // List view navigation
    if (key == LV_KEY_ESC) {
        freeQSOsFromView();
        if (view_logs_rows != NULL) {
            delete[] view_logs_rows;
            view_logs_rows = NULL;
        }
        view_logs_row_count = 0;
        onLVGLBackNavigation();
        lv_event_stop_bubbling(e);
        return;
    }

    if (key == LV_KEY_UP || key == LV_KEY_PREV) {
        if (view_logs_selected > 0) {
            view_logs_selected--;
            if (view_logs_selected < view_logs_scroll_offset) {
                view_logs_scroll_offset = view_logs_selected;
                rebuildViewLogsList();
            } else {
                viewState.selectedIndex = view_logs_selected;
                updateViewLogsRowStyles();
            }
        }
        lv_event_stop_bubbling(e);
        return;
    }

    if (key == LV_KEY_DOWN || key == LV_KEY_NEXT) {
        if (view_logs_selected < viewState.totalQSOs - 1) {
            view_logs_selected++;
            if (view_logs_selected >= view_logs_scroll_offset + VIEW_LOGS_MAX_VISIBLE) {
                view_logs_scroll_offset = view_logs_selected - VIEW_LOGS_MAX_VISIBLE + 1;
                rebuildViewLogsList();
            } else {
                viewState.selectedIndex = view_logs_selected;
                updateViewLogsRowStyles();
            }
        }
        lv_event_stop_bubbling(e);
        return;
    }

    if (key == LV_KEY_ENTER) {
        if (viewState.totalQSOs > 0) {
            viewState.selectedIndex = view_logs_selected;
            showQSODetailOverlay();
        }
        lv_event_stop_bubbling(e);
        return;
    }

    lv_event_stop_bubbling(e);
}

// Rebuild the visible list rows
static void rebuildViewLogsList() {
    // Clear existing rows
    if (view_logs_list_container != NULL) {
        lv_obj_clean(view_logs_list_container);
    }

    // Determine how many rows to show
    int visibleCount = min(VIEW_LOGS_MAX_VISIBLE, viewState.totalQSOs - view_logs_scroll_offset);
    if (visibleCount <= 0) return;

    // Allocate row pointers
    if (view_logs_rows != NULL) {
        delete[] view_logs_rows;
    }
    view_logs_rows = new lv_obj_t*[visibleCount];
    view_logs_row_count = visibleCount;

    for (int i = 0; i < visibleCount; i++) {
        int qsoIndex = view_logs_scroll_offset + i;
        QSO& qso = viewState.qsos[qsoIndex];

        // Create row container
        lv_obj_t* row = lv_obj_create(view_logs_list_container);
        lv_obj_set_size(row, lv_pct(100), VIEW_LOGS_ROW_HEIGHT);
        lv_obj_set_style_bg_color(row, LV_COLOR_BG_LAYER2, 0);
        lv_obj_set_style_radius(row, 6, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_color(row, LV_COLOR_BORDER_SUBTLE, 0);
        lv_obj_set_style_pad_all(row, 6, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        view_logs_rows[i] = row;

        // Date (left)
        char dateStr[12];
        formatDateShort(qso.date, dateStr, sizeof(dateStr));
        lv_obj_t* date_label = lv_label_create(row);
        lv_label_set_text(date_label, dateStr);
        lv_obj_set_style_text_font(date_label, getThemeFonts()->font_small, 0);
        lv_obj_set_style_text_color(date_label, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_align(date_label, LV_ALIGN_LEFT_MID, 0, 0);

        // Callsign (center)
        lv_obj_t* call_label = lv_label_create(row);
        lv_label_set_text(call_label, qso.callsign);
        lv_obj_set_style_text_font(call_label, getThemeFonts()->font_subtitle, 0);
        lv_obj_set_style_text_color(call_label, LV_COLOR_ACCENT_CYAN, 0);
        lv_obj_align(call_label, LV_ALIGN_LEFT_MID, 70, 0);

        // Band/Mode (right)
        lv_obj_t* band_label = lv_label_create(row);
        lv_label_set_text_fmt(band_label, "%s %s", qso.band, qso.mode);
        lv_obj_set_style_text_font(band_label, getThemeFonts()->font_small, 0);
        lv_obj_set_style_text_color(band_label, LV_COLOR_WARNING, 0);
        lv_obj_align(band_label, LV_ALIGN_RIGHT_MID, 0, 0);
    }

    viewState.selectedIndex = view_logs_selected;
    updateViewLogsRowStyles();
}

lv_obj_t* createQSOViewLogsScreen() {
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Initialize state
    view_logs_selected = 0;
    view_logs_scroll_offset = 0;
    view_logs_detail_mode = false;
    view_logs_delete_confirm = false;
    view_logs_detail_overlay = NULL;
    view_logs_rows = NULL;
    view_logs_row_count = 0;

    // Load QSOs from storage
    loadQSOsForView();

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    view_logs_count_label = lv_label_create(title_bar);
    lv_label_set_text_fmt(view_logs_count_label, "VIEW LOGS (%d)", viewState.totalQSOs);
    lv_obj_add_style(view_logs_count_label, getStyleLabelTitle(), 0);
    lv_obj_align(view_logs_count_label, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar
    createCompactStatusBar(screen);

    // List container
    view_logs_list_container = lv_obj_create(screen);
    lv_obj_set_size(view_logs_list_container, SCREEN_WIDTH - 20, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 10);
    lv_obj_set_pos(view_logs_list_container, 10, HEADER_HEIGHT + 5);
    lv_obj_set_layout(view_logs_list_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(view_logs_list_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(view_logs_list_container, 5, 0);
    lv_obj_set_style_pad_all(view_logs_list_container, 5, 0);
    lv_obj_set_style_bg_opa(view_logs_list_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(view_logs_list_container, 0, 0);
    lv_obj_clear_flag(view_logs_list_container, LV_OBJ_FLAG_SCROLLABLE);

    if (viewState.totalQSOs == 0) {
        // No logs message
        lv_obj_t* no_logs = lv_label_create(view_logs_list_container);
        lv_label_set_text(no_logs, "No QSO logs found");
        lv_obj_set_style_text_color(no_logs, LV_COLOR_WARNING, 0);
        lv_obj_set_style_text_font(no_logs, getThemeFonts()->font_subtitle, 0);
        lv_obj_set_width(no_logs, lv_pct(100));
        lv_obj_set_style_text_align(no_logs, LV_TEXT_ALIGN_CENTER, 0);
    } else {
        rebuildViewLogsList();
    }

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* footer_label = lv_label_create(footer);
    lv_label_set_text(footer_label, "UP/DN Select   ENTER View   ESC Back");
    lv_obj_set_style_text_color(footer_label, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(footer_label, getThemeFonts()->font_small, 0);
    lv_obj_center(footer_label);

    // Invisible focus container for key handling
    view_logs_focus_container = lv_obj_create(screen);
    lv_obj_set_size(view_logs_focus_container, 0, 0);
    lv_obj_set_style_bg_opa(view_logs_focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(view_logs_focus_container, 0, 0);
    lv_obj_add_flag(view_logs_focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(view_logs_focus_container, view_logs_key_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(view_logs_focus_container);

    // Enable edit mode
    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }

    view_logs_screen = screen;
    return screen;
}

// ============================================
// Screen Selector
// Mode values MUST match MenuMode enum in menu_ui.h
// ============================================

lv_obj_t* createModeScreenForMode(int mode) {
    switch (mode) {
        case 19: // MODE_RADIO_OUTPUT
            return createRadioOutputScreen();
        case 20: // MODE_CW_MEMORIES
            return createCWMemoriesScreen();
        case 31: // MODE_VAIL_REPEATER
            return createVailRepeaterScreen();
        case 33: // MODE_BT_HID
            return createBTHIDScreen();
        case 37: // MODE_QSO_LOG_ENTRY
            return createQSOLogEntryScreen();
        case 38: // MODE_QSO_VIEW_LOGS
            return createQSOViewLogsScreen();
        case 39: // MODE_QSO_STATISTICS
            return createQSOStatisticsScreen();
        case 40: // MODE_QSO_LOGGER_SETTINGS
            return createQSOLoggerSettingsScreen();
        default:
            Serial.printf("[ModeScreens] Unknown mode: %d\n", mode);
            return NULL;
    }
}

#endif // LV_MODE_SCREENS_H
