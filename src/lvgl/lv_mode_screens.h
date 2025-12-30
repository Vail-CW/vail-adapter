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
    lv_color_t selected_bg = lv_color_hex(0x1A4A4A);  // Teal highlight

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
                isSelected ? lv_color_hex(0x1A4A4A) : LV_COLOR_BG_LAYER2, 0);
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
    lv_obj_set_size(radio_overlay, 320, 200);
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
    lv_obj_set_style_bg_color(settings_row_wpm, lv_color_hex(0x1A4A4A), 0);
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
// CW Memories Screen
// ============================================

static lv_obj_t* memories_screen = NULL;
static lv_obj_t* memories_list = NULL;

lv_obj_t* createCWMemoriesScreen() {
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

    // Memory slots list
    memories_list = lv_obj_create(screen);
    lv_obj_set_size(memories_list, SCREEN_WIDTH - 40, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 20);
    lv_obj_set_pos(memories_list, 20, HEADER_HEIGHT + 10);
    lv_obj_set_layout(memories_list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(memories_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(memories_list, 5, 0);
    lv_obj_set_style_pad_all(memories_list, 10, 0);
    applyListStyle(memories_list);

    // Create 10 memory slot rows
    for (int i = 0; i < 10; i++) {
        lv_obj_t* slot = lv_obj_create(memories_list);
        lv_obj_set_size(slot, lv_pct(100), 40);
        lv_obj_set_layout(slot, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(slot, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(slot, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_hor(slot, 10, 0);
        lv_obj_set_style_pad_column(slot, 15, 0);
        applyCardStyle(slot);
        lv_obj_add_flag(slot, LV_OBJ_FLAG_CLICKABLE);
        addNavigableWidget(slot);

        // Slot number
        lv_obj_t* num = lv_label_create(slot);
        lv_label_set_text_fmt(num, "%d", i + 1);
        lv_obj_set_style_text_color(num, LV_COLOR_ACCENT_CYAN, 0);
        lv_obj_set_style_text_font(num, getThemeFonts()->font_input, 0);

        // Label (placeholder)
        lv_obj_t* label = lv_label_create(slot);
        lv_label_set_text(label, "(Empty)");
        lv_obj_set_style_text_color(label, LV_COLOR_TEXT_TERTIARY, 0);
        lv_obj_set_flex_grow(label, 1);
    }

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "ENTER Play   E Edit   D Delete   ESC Back");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    memories_screen = screen;
    return screen;
}

// ============================================
// Vail Repeater Screen
// ============================================

static lv_obj_t* vail_screen = NULL;
static lv_obj_t* vail_chat_textarea = NULL;
static lv_obj_t* vail_status_label = NULL;
static lv_obj_t* vail_callsign_label = NULL;

// Key event callback for Vail Repeater keyboard input
static void vail_key_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    Serial.printf("[Vail LVGL] Key event: %lu (0x%02lX)\n", key, key);

    switch(key) {
        case LV_KEY_ESC:
            onLVGLBackNavigation();
            lv_event_stop_processing(e);  // Prevent global ESC handler from also firing
            break;
        case 'c':
        case 'C':
            // Clear chat - just beep for now, functionality can be added later
            if (vail_chat_textarea != NULL) {
                lv_textarea_set_text(vail_chat_textarea, "");
            }
            beep(TONE_MENU_NAV, BEEP_SHORT);
            break;
    }
}

lv_obj_t* createVailRepeaterScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

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
    lv_label_set_text(title, "VAIL CHAT");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);

    vail_status_label = lv_label_create(title_bar);
    lv_label_set_text(vail_status_label, "Connecting...");
    lv_obj_set_style_text_color(vail_status_label, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(vail_status_label, getThemeFonts()->font_body, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Callsign display
    lv_obj_t* callsign_bar = lv_obj_create(screen);
    lv_obj_set_size(callsign_bar, SCREEN_WIDTH, 30);
    lv_obj_set_pos(callsign_bar, 0, HEADER_HEIGHT);
    lv_obj_set_layout(callsign_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(callsign_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(callsign_bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(callsign_bar, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_border_width(callsign_bar, 0, 0);
    lv_obj_clear_flag(callsign_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* callsign_title = lv_label_create(callsign_bar);
    lv_label_set_text(callsign_title, "Your callsign: ");
    lv_obj_set_style_text_color(callsign_title, LV_COLOR_TEXT_SECONDARY, 0);

    vail_callsign_label = lv_label_create(callsign_bar);
    lv_label_set_text(vail_callsign_label, "N0CALL");
    lv_obj_set_style_text_color(vail_callsign_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(vail_callsign_label, getThemeFonts()->font_input, 0);

    // Chat area
    vail_chat_textarea = lv_textarea_create(screen);
    lv_obj_set_size(vail_chat_textarea, SCREEN_WIDTH - 20, SCREEN_HEIGHT - HEADER_HEIGHT - 30 - FOOTER_HEIGHT - 20);
    lv_obj_set_pos(vail_chat_textarea, 10, HEADER_HEIGHT + 40);
    lv_textarea_set_text(vail_chat_textarea, "");
    lv_textarea_set_placeholder_text(vail_chat_textarea, "Chat messages will appear here...");
    lv_obj_add_style(vail_chat_textarea, getStyleTextarea(), 0);
    lv_obj_set_style_text_font(vail_chat_textarea, getThemeFonts()->font_body, 0);
    lv_obj_clear_flag(vail_chat_textarea, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "Use paddle to send   C Clear   ESC Exit");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    // Invisible focus container for keyboard input (C to clear, ESC to exit)
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
    return screen;
}

void updateVailStatus(const char* status, bool connected) {
    if (vail_status_label != NULL) {
        lv_label_set_text(vail_status_label, status);
        if (connected) {
            lv_obj_set_style_text_color(vail_status_label, LV_COLOR_SUCCESS, 0);
        } else {
            lv_obj_set_style_text_color(vail_status_label, LV_COLOR_WARNING, 0);
        }
    }
}

void updateVailCallsign(const char* callsign) {
    if (vail_callsign_label != NULL) {
        lv_label_set_text(vail_callsign_label, callsign);
    }
}

void appendVailMessage(const char* message) {
    if (vail_chat_textarea != NULL) {
        lv_textarea_add_text(vail_chat_textarea, message);
        lv_textarea_add_text(vail_chat_textarea, "\n");
    }
}

// ============================================
// QSO Logger Entry Screen
// ============================================

static lv_obj_t* qso_entry_screen = NULL;
static lv_obj_t* qso_callsign_input = NULL;
static lv_obj_t* qso_freq_input = NULL;
static lv_obj_t* qso_rst_sent_input = NULL;
static lv_obj_t* qso_rst_rcvd_input = NULL;

lv_obj_t* createQSOLogEntryScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

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

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Form container
    lv_obj_t* form = lv_obj_create(screen);
    lv_obj_set_size(form, SCREEN_WIDTH - 40, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 20);
    lv_obj_set_pos(form, 20, HEADER_HEIGHT + 10);
    lv_obj_set_layout(form, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(form, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(form, 12, 0);
    lv_obj_set_style_pad_all(form, 15, 0);
    applyCardStyle(form);

    // Callsign field
    lv_obj_t* call_row = lv_obj_create(form);
    lv_obj_set_size(call_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(call_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(call_row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(call_row, 5, 0);
    lv_obj_set_style_bg_opa(call_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(call_row, 0, 0);
    lv_obj_set_style_pad_all(call_row, 0, 0);

    lv_obj_t* call_label = lv_label_create(call_row);
    lv_label_set_text(call_label, "Callsign:");
    lv_obj_add_style(call_label, getStyleLabelBody(), 0);

    qso_callsign_input = lv_textarea_create(call_row);
    lv_obj_set_size(qso_callsign_input, lv_pct(100), 40);
    lv_textarea_set_one_line(qso_callsign_input, true);
    lv_textarea_set_max_length(qso_callsign_input, 12);
    lv_textarea_set_placeholder_text(qso_callsign_input, "e.g. W1ABC");
    lv_obj_add_style(qso_callsign_input, getStyleTextarea(), 0);
    addNavigableWidget(qso_callsign_input);

    // Frequency field
    lv_obj_t* freq_row = lv_obj_create(form);
    lv_obj_set_size(freq_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(freq_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(freq_row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(freq_row, 5, 0);
    lv_obj_set_style_bg_opa(freq_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(freq_row, 0, 0);
    lv_obj_set_style_pad_all(freq_row, 0, 0);

    lv_obj_t* freq_label = lv_label_create(freq_row);
    lv_label_set_text(freq_label, "Frequency (kHz):");
    lv_obj_add_style(freq_label, getStyleLabelBody(), 0);

    qso_freq_input = lv_textarea_create(freq_row);
    lv_obj_set_size(qso_freq_input, lv_pct(100), 40);
    lv_textarea_set_one_line(qso_freq_input, true);
    lv_textarea_set_max_length(qso_freq_input, 10);
    lv_textarea_set_placeholder_text(qso_freq_input, "e.g. 7030");
    lv_obj_add_style(qso_freq_input, getStyleTextarea(), 0);
    addNavigableWidget(qso_freq_input);

    // RST row (sent and received side by side)
    lv_obj_t* rst_row = lv_obj_create(form);
    lv_obj_set_size(rst_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(rst_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(rst_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(rst_row, 20, 0);
    lv_obj_set_style_bg_opa(rst_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rst_row, 0, 0);
    lv_obj_set_style_pad_all(rst_row, 0, 0);

    // RST Sent
    lv_obj_t* rst_sent_col = lv_obj_create(rst_row);
    lv_obj_set_size(rst_sent_col, lv_pct(45), LV_SIZE_CONTENT);
    lv_obj_set_layout(rst_sent_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(rst_sent_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(rst_sent_col, 5, 0);
    lv_obj_set_style_bg_opa(rst_sent_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rst_sent_col, 0, 0);
    lv_obj_set_style_pad_all(rst_sent_col, 0, 0);

    lv_obj_t* rst_sent_label = lv_label_create(rst_sent_col);
    lv_label_set_text(rst_sent_label, "RST Sent:");
    lv_obj_add_style(rst_sent_label, getStyleLabelBody(), 0);

    qso_rst_sent_input = lv_textarea_create(rst_sent_col);
    lv_obj_set_size(qso_rst_sent_input, lv_pct(100), 40);
    lv_textarea_set_one_line(qso_rst_sent_input, true);
    lv_textarea_set_max_length(qso_rst_sent_input, 3);
    lv_textarea_set_text(qso_rst_sent_input, "599");
    lv_obj_add_style(qso_rst_sent_input, getStyleTextarea(), 0);
    addNavigableWidget(qso_rst_sent_input);

    // RST Received
    lv_obj_t* rst_rcvd_col = lv_obj_create(rst_row);
    lv_obj_set_size(rst_rcvd_col, lv_pct(45), LV_SIZE_CONTENT);
    lv_obj_set_layout(rst_rcvd_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(rst_rcvd_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(rst_rcvd_col, 5, 0);
    lv_obj_set_style_bg_opa(rst_rcvd_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rst_rcvd_col, 0, 0);
    lv_obj_set_style_pad_all(rst_rcvd_col, 0, 0);

    lv_obj_t* rst_rcvd_label = lv_label_create(rst_rcvd_col);
    lv_label_set_text(rst_rcvd_label, "RST Rcvd:");
    lv_obj_add_style(rst_rcvd_label, getStyleLabelBody(), 0);

    qso_rst_rcvd_input = lv_textarea_create(rst_rcvd_col);
    lv_obj_set_size(qso_rst_rcvd_input, lv_pct(100), 40);
    lv_textarea_set_one_line(qso_rst_rcvd_input, true);
    lv_textarea_set_max_length(qso_rst_rcvd_input, 3);
    lv_textarea_set_text(qso_rst_rcvd_input, "599");
    lv_obj_add_style(qso_rst_rcvd_input, getStyleTextarea(), 0);
    addNavigableWidget(qso_rst_rcvd_input);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "TAB Next field   ENTER Save   ESC Cancel");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

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
    lv_obj_set_style_text_font(bt_icon, &lv_font_montserrat_28, 0);
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
        case 37: // MODE_QSO_LOG_ENTRY
            return createQSOLogEntryScreen();
        case 33: // MODE_BT_HID
            return createBTHIDScreen();
        default:
            Serial.printf("[ModeScreens] Unknown mode: %d\n", mode);
            return NULL;
    }
}

#endif // LV_MODE_SCREENS_H
