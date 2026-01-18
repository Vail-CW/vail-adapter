/*
 * VAIL SUMMIT - LVGL Settings Screens
 * Replaces LovyanGFX settings rendering with LVGL
 */

#ifndef LV_SETTINGS_SCREENS_H
#define LV_SETTINGS_SCREENS_H

#include <lvgl.h>
#include "lv_theme_summit.h"
#include "lv_widgets_summit.h"
#include "lv_screen_manager.h"
#include "../core/config.h"

// Note: Settings variables are accessed through the settings modules
// These extern declarations must match the actual types from the settings headers

// Volume settings (from settings_volume.h)
extern int getVolume();
extern void setVolume(int vol);

// Brightness settings (from settings_brightness.h)
extern int brightnessValue;
extern void applyBrightness(int val);
extern void saveBrightnessSettings();

// CW settings (from settings_cw.h)
extern int cwSpeed;
extern int cwTone;
extern void saveCWSettings();

// cwKeyType access functions (defined in main sketch to handle KeyType enum)
int getCwKeyTypeAsInt();
void setCwKeyTypeFromInt(int keyType);

// Callsign (from vail_repeater.h)
extern String vailCallsign;
extern void saveCallsign(String callsign);

// Web password settings (from settings_web_password.h)
extern String webPassword;
extern bool webAuthEnabled;
extern void saveWebPassword(String password);
extern void clearWebPassword();

// ============================================
// Volume Settings Screen
// ============================================

static lv_obj_t* volume_screen = NULL;
static lv_obj_t* volume_slider = NULL;
static lv_obj_t* volume_value_label = NULL;

// Forward declaration for key acceleration
extern int getKeyAccelerationStep();

static void volume_slider_event_cb(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    int value = lv_slider_get_value(slider);

    // Update label
    if (volume_value_label != NULL) {
        lv_label_set_text_fmt(volume_value_label, "%d%%", value);
    }

    // Apply volume immediately for feedback
    setVolume(value);

    // Play test tone so user can hear the new volume level
    beep(TONE_MENU_NAV, BEEP_SHORT);
}

// Key handler for volume slider - applies acceleration for faster adjustment
// Number keys 1-9 = 10%-90%, 0 = 100%
static void volume_slider_key_cb(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    lv_obj_t* slider = lv_event_get_target(e);

    // Number keys for quick percentage jumps
    if (key >= '0' && key <= '9') {
        int new_val;
        if (key == '0') {
            new_val = 100;  // 0 = 100%
        } else {
            new_val = (key - '0') * 10;  // 1=10%, 2=20%, ... 9=90%
        }
        lv_slider_set_value(slider, new_val, LV_ANIM_OFF);
        lv_event_send(slider, LV_EVENT_VALUE_CHANGED, NULL);
        lv_event_stop_bubbling(e);
        return;
    }

    if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT) {
        int step = getKeyAccelerationStep();
        int delta = (key == LV_KEY_RIGHT) ? step : -step;
        int current = lv_slider_get_value(slider);
        int new_val = current + delta;

        // Clamp to range
        int min_val = lv_slider_get_min_value(slider);
        int max_val = lv_slider_get_max_value(slider);
        if (new_val < min_val) new_val = min_val;
        if (new_val > max_val) new_val = max_val;

        lv_slider_set_value(slider, new_val, LV_ANIM_OFF);
        lv_event_send(slider, LV_EVENT_VALUE_CHANGED, NULL);

        // Prevent default slider handling
        lv_event_stop_bubbling(e);
    }
}

lv_obj_t* createVolumeSettingsScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "VOLUME");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Content area
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, SCREEN_WIDTH - 60, 160);
    lv_obj_center(content);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(content, 20, 0);
    applyCardStyle(content);

    // Volume value (large display) - use theme font
    volume_value_label = lv_label_create(content);
    lv_label_set_text_fmt(volume_value_label, "%d%%", getVolume());
    lv_obj_set_style_text_font(volume_value_label, getThemeFonts()->font_large, 0);
    lv_obj_set_style_text_color(volume_value_label, LV_COLOR_ACCENT_CYAN, 0);

    // Volume slider
    volume_slider = lv_slider_create(content);
    lv_obj_set_width(volume_slider, SCREEN_WIDTH - 120);
    lv_slider_set_range(volume_slider, VOLUME_MIN, VOLUME_MAX);
    lv_slider_set_value(volume_slider, getVolume(), LV_ANIM_OFF);
    applySliderStyle(volume_slider);
    lv_obj_add_event_cb(volume_slider, volume_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    // Add key handler for acceleration support
    lv_obj_add_event_cb(volume_slider, volume_slider_key_cb, LV_EVENT_KEY, NULL);

    // Make slider navigable
    addNavigableWidget(volume_slider);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, FOOTER_ADJUST_ESC);  // Use standardized footer
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    volume_screen = screen;
    return screen;
}

// ============================================
// Brightness Settings Screen
// ============================================

static lv_obj_t* brightness_screen = NULL;
static lv_obj_t* brightness_slider = NULL;
static lv_obj_t* brightness_value_label = NULL;

static void brightness_slider_event_cb(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    int value = lv_slider_get_value(slider);

    // Update label
    if (brightness_value_label != NULL) {
        lv_label_set_text_fmt(brightness_value_label, "%d%%", value);
    }

    // Apply brightness immediately
    applyBrightness(value);
    saveBrightnessSettings();
}

// Key handler for brightness slider - applies acceleration for faster adjustment
// Number keys 1-9 = 10%-90%, 0 = 100%
static void brightness_slider_key_cb(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    lv_obj_t* slider = lv_event_get_target(e);

    // Number keys for quick percentage jumps
    if (key >= '0' && key <= '9') {
        int new_val;
        if (key == '0') {
            new_val = 100;  // 0 = 100%
        } else {
            new_val = (key - '0') * 10;  // 1=10%, 2=20%, ... 9=90%
        }
        lv_slider_set_value(slider, new_val, LV_ANIM_OFF);
        lv_event_send(slider, LV_EVENT_VALUE_CHANGED, NULL);
        lv_event_stop_bubbling(e);
        return;
    }

    if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT) {
        int step = getKeyAccelerationStep();
        int delta = (key == LV_KEY_RIGHT) ? step : -step;
        int current = lv_slider_get_value(slider);
        int new_val = current + delta;

        // Clamp to range
        int min_val = lv_slider_get_min_value(slider);
        int max_val = lv_slider_get_max_value(slider);
        if (new_val < min_val) new_val = min_val;
        if (new_val > max_val) new_val = max_val;

        lv_slider_set_value(slider, new_val, LV_ANIM_OFF);
        lv_event_send(slider, LV_EVENT_VALUE_CHANGED, NULL);

        // Prevent default slider handling
        lv_event_stop_bubbling(e);
    }
}

lv_obj_t* createBrightnessSettingsScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "BRIGHTNESS");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Content area
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, SCREEN_WIDTH - 60, 160);
    lv_obj_center(content);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(content, 20, 0);
    applyCardStyle(content);

    // Brightness value - use theme font
    brightness_value_label = lv_label_create(content);
    lv_label_set_text_fmt(brightness_value_label, "%d%%", brightnessValue);
    lv_obj_set_style_text_font(brightness_value_label, getThemeFonts()->font_large, 0);
    lv_obj_set_style_text_color(brightness_value_label, LV_COLOR_ACCENT_CYAN, 0);

    // Brightness slider
    brightness_slider = lv_slider_create(content);
    lv_obj_set_width(brightness_slider, SCREEN_WIDTH - 120);
    lv_slider_set_range(brightness_slider, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
    lv_slider_set_value(brightness_slider, brightnessValue, LV_ANIM_OFF);
    applySliderStyle(brightness_slider);
    lv_obj_add_event_cb(brightness_slider, brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    // Add key handler for acceleration support
    lv_obj_add_event_cb(brightness_slider, brightness_slider_key_cb, LV_EVENT_KEY, NULL);

    addNavigableWidget(brightness_slider);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, FOOTER_ADJUST_ESC);  // Use standardized footer
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    brightness_screen = screen;
    return screen;
}

// ============================================
// CW Settings Screen
// ============================================

static lv_obj_t* cw_settings_screen = NULL;
static lv_obj_t* cw_speed_slider = NULL;
static lv_obj_t* cw_tone_slider = NULL;
static lv_obj_t* cw_keytype_value = NULL;  // Changed from dropdown to label
static lv_obj_t* cw_speed_value = NULL;
static lv_obj_t* cw_tone_value = NULL;

// CW Settings focus state and row references
static int cw_settings_focus = 0;  // 0=Speed, 1=Tone, 2=Key Type
static lv_obj_t* cw_focus_container = NULL;
static lv_obj_t* cw_speed_row = NULL;
static lv_obj_t* cw_tone_row = NULL;
static lv_obj_t* cw_keytype_row = NULL;

// Key type names for selector display
static const char* cw_keytype_names[] = {"Straight", "Iambic A", "Iambic B"};
static const int cw_keytype_count = 3;

// Musical note frequencies in the CW tone range (400-1200 Hz)
// A4 = 440 Hz standard tuning, includes all semitones (chromatic scale)
static const int cw_note_frequencies[] = {
    400,   // G4 (392 Hz rounded up)
    415,   // G#4/Ab4
    440,   // A4
    466,   // A#4/Bb4
    494,   // B4
    523,   // C5
    554,   // C#5/Db5
    587,   // D5
    622,   // D#5/Eb5
    659,   // E5
    698,   // F5
    740,   // F#5/Gb5
    784,   // G5
    831,   // G#5/Ab5
    880,   // A5
    932,   // A#5/Bb5
    988,   // B5
    1047,  // C6
    1109,  // C#6/Db6
    1175   // D6
};
static const int cw_note_count = sizeof(cw_note_frequencies) / sizeof(cw_note_frequencies[0]);

// Snap a frequency to the nearest musical note
static int snapToNearestNote(int freq) {
    int closest = cw_note_frequencies[0];
    int minDiff = abs(freq - closest);

    for (int i = 1; i < cw_note_count; i++) {
        int diff = abs(freq - cw_note_frequencies[i]);
        if (diff < minDiff) {
            minDiff = diff;
            closest = cw_note_frequencies[i];
        }
    }
    return closest;
}

// Update visual focus indicator for CW settings rows
static void cw_update_focus() {
    // Speed row styling (focus == 0)
    if (cw_speed_row) {
        if (cw_settings_focus == 0) {
            lv_obj_set_style_bg_color(cw_speed_row, LV_COLOR_CARD_TEAL, 0);
            lv_obj_set_style_bg_opa(cw_speed_row, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(cw_speed_row, LV_COLOR_ACCENT_CYAN, 0);
            lv_obj_set_style_border_width(cw_speed_row, 2, 0);
        } else {
            lv_obj_set_style_bg_opa(cw_speed_row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(cw_speed_row, 0, 0);
        }
    }
    if (cw_speed_slider) {
        if (cw_settings_focus == 0) {
            lv_obj_add_state(cw_speed_slider, LV_STATE_FOCUSED);
        } else {
            lv_obj_clear_state(cw_speed_slider, LV_STATE_FOCUSED);
        }
    }

    // Tone row styling (focus == 1)
    if (cw_tone_row) {
        if (cw_settings_focus == 1) {
            lv_obj_set_style_bg_color(cw_tone_row, LV_COLOR_CARD_TEAL, 0);
            lv_obj_set_style_bg_opa(cw_tone_row, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(cw_tone_row, LV_COLOR_ACCENT_CYAN, 0);
            lv_obj_set_style_border_width(cw_tone_row, 2, 0);
        } else {
            lv_obj_set_style_bg_opa(cw_tone_row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(cw_tone_row, 0, 0);
        }
    }
    if (cw_tone_slider) {
        if (cw_settings_focus == 1) {
            lv_obj_add_state(cw_tone_slider, LV_STATE_FOCUSED);
        } else {
            lv_obj_clear_state(cw_tone_slider, LV_STATE_FOCUSED);
        }
    }

    // Key type row styling (focus == 2)
    if (cw_keytype_row) {
        if (cw_settings_focus == 2) {
            lv_obj_set_style_bg_color(cw_keytype_row, LV_COLOR_CARD_TEAL, 0);
            lv_obj_set_style_bg_opa(cw_keytype_row, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(cw_keytype_row, LV_COLOR_ACCENT_CYAN, 0);
            lv_obj_set_style_border_width(cw_keytype_row, 2, 0);
        } else {
            lv_obj_set_style_bg_opa(cw_keytype_row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(cw_keytype_row, 0, 0);
        }
    }
    if (cw_keytype_value) {
        lv_obj_set_style_text_color(cw_keytype_value,
            cw_settings_focus == 2 ? LV_COLOR_ACCENT_CYAN : LV_COLOR_TEXT_SECONDARY, 0);
    }
}

// Forward declaration for back navigation
extern void onLVGLBackNavigation();

// Unified key handler for CW settings - handles all navigation
static void cw_settings_key_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // Block Tab key - we don't want LVGL's default group navigation
    if (key == LV_KEY_NEXT || key == LV_KEY_PREV) {
        lv_event_stop_bubbling(e);
        return;
    }

    // Handle ESC for back navigation
    if (key == LV_KEY_ESC) {
        lv_event_stop_bubbling(e);  // Prevent double-navigation
        onLVGLBackNavigation();
        return;
    }

    // Handle UP/DOWN for navigation between settings
    if (key == LV_KEY_UP) {
        lv_event_stop_bubbling(e);
        if (cw_settings_focus > 0) {
            cw_settings_focus--;
            cw_update_focus();
        }
        return;
    }
    if (key == LV_KEY_DOWN) {
        lv_event_stop_bubbling(e);
        if (cw_settings_focus < 2) {
            cw_settings_focus++;
            cw_update_focus();
        }
        return;
    }

    // Handle LEFT/RIGHT for value adjustment based on current focus
    if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT) {
        lv_event_stop_bubbling(e);
        if (cw_settings_focus == 0 && cw_speed_slider) {
            // Speed slider - adjust WPM with acceleration
            int step = getKeyAccelerationStep();
            int delta = (key == LV_KEY_RIGHT) ? step : -step;
            int current = lv_slider_get_value(cw_speed_slider);
            int new_val = current + delta;

            // Clamp to range
            if (new_val < WPM_MIN) new_val = WPM_MIN;
            if (new_val > WPM_MAX) new_val = WPM_MAX;

            lv_slider_set_value(cw_speed_slider, new_val, LV_ANIM_OFF);
            lv_event_send(cw_speed_slider, LV_EVENT_VALUE_CHANGED, NULL);
        }
        else if (cw_settings_focus == 1 && cw_tone_slider) {
            // Tone slider - move to next/previous musical note
            int current = lv_slider_get_value(cw_tone_slider);
            int new_val = current;

            // Find current note index
            int current_idx = 0;
            int minDiff = abs(current - cw_note_frequencies[0]);
            for (int i = 1; i < cw_note_count; i++) {
                int diff = abs(current - cw_note_frequencies[i]);
                if (diff < minDiff) {
                    minDiff = diff;
                    current_idx = i;
                }
            }

            // Move to next/previous note
            if (key == LV_KEY_RIGHT && current_idx < cw_note_count - 1) {
                new_val = cw_note_frequencies[current_idx + 1];
            } else if (key == LV_KEY_LEFT && current_idx > 0) {
                new_val = cw_note_frequencies[current_idx - 1];
            }

            if (new_val != current) {
                lv_slider_set_value(cw_tone_slider, new_val, LV_ANIM_OFF);
                lv_event_send(cw_tone_slider, LV_EVENT_VALUE_CHANGED, NULL);
            }
        }
        else if (cw_settings_focus == 2 && cw_keytype_value) {
            // Key type - cycle through options using arrow selector
            int current = getCwKeyTypeAsInt();

            if (key == LV_KEY_RIGHT) {
                current = (current + 1) % cw_keytype_count;
            } else {
                current = (current - 1 + cw_keytype_count) % cw_keytype_count;
            }

            // Update display and save
            lv_label_set_text_fmt(cw_keytype_value, "< %s >", cw_keytype_names[current]);
            setCwKeyTypeFromInt(current);
            saveCWSettings();
        }
        return;
    }

    // Handle ENTER - no action needed, all settings use LEFT/RIGHT
    if (key == LV_KEY_ENTER) {
        lv_event_stop_bubbling(e);
        return;
    }
}

static void cw_speed_event_cb(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    cwSpeed = lv_slider_get_value(slider);
    if (cw_speed_value != NULL) {
        lv_label_set_text_fmt(cw_speed_value, "%d WPM", cwSpeed);
    }
    saveCWSettings();
    // Play preview tone at current CW frequency to confirm change
    beep(cwTone, 100);
}


static void cw_tone_event_cb(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    cwTone = lv_slider_get_value(slider);
    if (cw_tone_value != NULL) {
        lv_label_set_text_fmt(cw_tone_value, "%d Hz", cwTone);
    }
    saveCWSettings();
    // Play preview tone
    beep(cwTone, 100);
}


lv_obj_t* createCWSettingsScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "CW SETTINGS");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Content container
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, SCREEN_WIDTH - 40, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 20);
    lv_obj_set_pos(content, 20, HEADER_HEIGHT + 10);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 8, 0);
    lv_obj_set_style_pad_all(content, 10, 0);
    applyCardStyle(content);
    lv_obj_add_flag(content, LV_OBJ_FLAG_OVERFLOW_VISIBLE);  // Prevent slider clipping

    // Create an invisible focus container to receive all key events
    // This bypasses LVGL's widget-level key handling
    cw_focus_container = lv_obj_create(content);
    lv_obj_set_size(cw_focus_container, 0, 0);
    lv_obj_set_style_bg_opa(cw_focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cw_focus_container, 0, 0);
    lv_obj_clear_flag(cw_focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cw_focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(cw_focus_container, cw_settings_key_handler, LV_EVENT_KEY, NULL);
    addNavigableWidget(cw_focus_container);

    // Put group in edit mode - this makes UP/DOWN keys go to the widget instead of being consumed by LVGL
    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }

    // Ensure focus is on our container
    lv_group_focus_obj(cw_focus_container);

    // Reset focus state
    cw_settings_focus = 0;

    // Speed setting row (with styling for focus highlight)
    cw_speed_row = lv_obj_create(content);
    lv_obj_set_size(cw_speed_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(cw_speed_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cw_speed_row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(cw_speed_row, 5, 0);
    lv_obj_set_style_bg_opa(cw_speed_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cw_speed_row, 0, 0);
    lv_obj_set_style_pad_all(cw_speed_row, 8, 0);
    lv_obj_set_style_radius(cw_speed_row, 6, 0);
    lv_obj_clear_flag(cw_speed_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cw_speed_row, LV_OBJ_FLAG_OVERFLOW_VISIBLE);  // Prevent slider knob clipping

    lv_obj_t* speed_header = lv_obj_create(cw_speed_row);
    lv_obj_set_size(speed_header, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(speed_header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(speed_header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(speed_header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(speed_header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(speed_header, 0, 0);
    lv_obj_set_style_pad_all(speed_header, 0, 0);

    lv_obj_t* speed_label = lv_label_create(speed_header);
    lv_label_set_text(speed_label, "Speed");
    lv_obj_add_style(speed_label, getStyleLabelSubtitle(), 0);

    cw_speed_value = lv_label_create(speed_header);
    lv_label_set_text_fmt(cw_speed_value, "%d WPM", cwSpeed);
    lv_obj_set_style_text_color(cw_speed_value, LV_COLOR_ACCENT_CYAN, 0);

    cw_speed_slider = lv_slider_create(cw_speed_row);
    lv_obj_set_width(cw_speed_slider, lv_pct(100));
    lv_slider_set_range(cw_speed_slider, WPM_MIN, WPM_MAX);
    lv_slider_set_value(cw_speed_slider, cwSpeed, LV_ANIM_OFF);
    applySliderStyle(cw_speed_slider);
    lv_obj_add_event_cb(cw_speed_slider, cw_speed_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    // No addNavigableWidget - focus container handles all navigation

    // Tone setting row (with styling for focus highlight)
    cw_tone_row = lv_obj_create(content);
    lv_obj_set_size(cw_tone_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(cw_tone_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cw_tone_row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(cw_tone_row, 5, 0);
    lv_obj_set_style_bg_opa(cw_tone_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cw_tone_row, 0, 0);
    lv_obj_set_style_pad_all(cw_tone_row, 8, 0);
    lv_obj_set_style_radius(cw_tone_row, 6, 0);
    lv_obj_clear_flag(cw_tone_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cw_tone_row, LV_OBJ_FLAG_OVERFLOW_VISIBLE);  // Prevent slider knob clipping

    lv_obj_t* tone_header = lv_obj_create(cw_tone_row);
    lv_obj_set_size(tone_header, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(tone_header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tone_header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tone_header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(tone_header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tone_header, 0, 0);
    lv_obj_set_style_pad_all(tone_header, 0, 0);

    lv_obj_t* tone_label = lv_label_create(tone_header);
    lv_label_set_text(tone_label, "Tone");
    lv_obj_add_style(tone_label, getStyleLabelSubtitle(), 0);

    cw_tone_value = lv_label_create(tone_header);
    lv_label_set_text_fmt(cw_tone_value, "%d Hz", cwTone);
    lv_obj_set_style_text_color(cw_tone_value, LV_COLOR_ACCENT_CYAN, 0);

    cw_tone_slider = lv_slider_create(cw_tone_row);
    lv_obj_set_width(cw_tone_slider, lv_pct(100));
    lv_slider_set_range(cw_tone_slider, 400, 1200);
    lv_slider_set_value(cw_tone_slider, cwTone, LV_ANIM_OFF);
    applySliderStyle(cw_tone_slider);
    lv_obj_add_event_cb(cw_tone_slider, cw_tone_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    // No addNavigableWidget - focus container handles all navigation

    // Key type setting row (with styling for focus highlight)
    cw_keytype_row = lv_obj_create(content);
    lv_obj_set_size(cw_keytype_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(cw_keytype_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cw_keytype_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cw_keytype_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(cw_keytype_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cw_keytype_row, 0, 0);
    lv_obj_set_style_pad_all(cw_keytype_row, 8, 0);
    lv_obj_set_style_radius(cw_keytype_row, 6, 0);
    lv_obj_clear_flag(cw_keytype_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* keytype_label = lv_label_create(cw_keytype_row);
    lv_label_set_text(keytype_label, "Key Type");
    lv_obj_add_style(keytype_label, getStyleLabelSubtitle(), 0);

    // Key type value - shows "< Straight >" style selector (like Hear It Type It)
    cw_keytype_value = lv_label_create(cw_keytype_row);
    lv_label_set_text_fmt(cw_keytype_value, "< %s >", cw_keytype_names[getCwKeyTypeAsInt()]);
    lv_obj_set_style_text_color(cw_keytype_value, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(cw_keytype_value, getThemeFonts()->font_subtitle, 0);

    // Set initial focus styling
    cw_update_focus();

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, FOOTER_NAV_ADJUST_ESC);  // Use standardized footer
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    cw_settings_screen = screen;
    return screen;
}

// ============================================
// Callsign Settings Screen
// ============================================

static lv_obj_t* callsign_screen = NULL;
static lv_obj_t* callsign_textarea = NULL;

// Key handler for callsign textarea - handles ENTER to save
static void callsign_textarea_key_handler(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    uint32_t key = lv_event_get_key(e);

    if (key == LV_KEY_ENTER) {
        // Save callsign
        if (callsign_textarea != NULL) {
            const char* text = lv_textarea_get_text(callsign_textarea);
            if (text != NULL && strlen(text) > 0) {
                // Convert to uppercase
                String callsign = String(text);
                callsign.toUpperCase();

                // Save using existing function
                saveCallsign(callsign);
                vailCallsign = callsign;  // Update global

                beep(TONE_SELECT, BEEP_MEDIUM);
                Serial.printf("[Callsign] Saved: %s\n", callsign.c_str());
            }
        }
        // Navigate back
        onLVGLBackNavigation();
        lv_event_stop_bubbling(e);
    }
    // ESC is handled by the global back navigation system
}

lv_obj_t* createCallsignSettingsScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "CALLSIGN");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Content
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, SCREEN_WIDTH - 60, 140);
    lv_obj_center(content);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(content, 15, 0);
    applyCardStyle(content);

    // Label
    lv_obj_t* label = lv_label_create(content);
    lv_label_set_text(label, "Enter your callsign:");
    lv_obj_add_style(label, getStyleLabelSubtitle(), 0);

    // Text area
    callsign_textarea = lv_textarea_create(content);
    lv_obj_set_size(callsign_textarea, 250, 50);
    lv_textarea_set_one_line(callsign_textarea, true);
    lv_textarea_set_max_length(callsign_textarea, 12);
    lv_textarea_set_placeholder_text(callsign_textarea, "e.g. W1ABC");
    lv_textarea_set_text(callsign_textarea, vailCallsign.c_str());
    lv_obj_add_style(callsign_textarea, getStyleTextarea(), 0);
    lv_obj_set_style_text_font(callsign_textarea, getThemeFonts()->font_subtitle, 0);
    // Add key handler to process ENTER for save
    lv_obj_add_event_cb(callsign_textarea, callsign_textarea_key_handler, LV_EVENT_KEY, NULL);
    addNavigableWidget(callsign_textarea);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, FOOTER_TYPE_ENTER_ESC);  // Use standardized footer
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    // Auto-focus the callsign textarea for immediate input
    focusWidget(callsign_textarea);

    callsign_screen = screen;
    return screen;
}

// Get callsign from text area (call before leaving screen)
const char* getCallsignFromTextarea() {
    if (callsign_textarea != NULL) {
        return lv_textarea_get_text(callsign_textarea);
    }
    return "";
}

// ============================================
// Web Password Settings Screen
// ============================================

static lv_obj_t* web_password_screen = NULL;
static lv_obj_t* web_password_textarea = NULL;
static lv_obj_t* web_password_enable_switch = NULL;
static lv_obj_t* web_password_error_label = NULL;

// Key handler for web password switch - uses LEFT/RIGHT to toggle, UP/DOWN to navigate
// NOTE: Must stop processing on all keys to prevent LVGL's default switch behavior
// from toggling on PREV/NEXT keys
static void web_password_switch_key_handler(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    uint32_t key = lv_event_get_key(e);
    lv_obj_t* obj = lv_event_get_target(e);

    // LEFT/RIGHT toggles the switch
    if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT) {
        if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
            lv_obj_clear_state(obj, LV_STATE_CHECKED);
        } else {
            lv_obj_add_state(obj, LV_STATE_CHECKED);
        }
        beep(TONE_SELECT, BEEP_SHORT);
        lv_event_stop_processing(e);
        return;
    }

    // UP/DOWN should navigate - prevent default switch toggle behavior
    if (key == LV_KEY_UP || key == LV_KEY_PREV) {
        // Can't go up from switch (it's the first widget)
        lv_event_stop_processing(e);
        return;
    }
    if (key == LV_KEY_DOWN || key == LV_KEY_NEXT) {
        // Move to password textarea
        lv_group_focus_next(getLVGLInputGroup());
        lv_event_stop_processing(e);
        return;
    }

    // ENTER on switch toggles it
    if (key == LV_KEY_ENTER) {
        if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
            lv_obj_clear_state(obj, LV_STATE_CHECKED);
        } else {
            lv_obj_add_state(obj, LV_STATE_CHECKED);
        }
        beep(TONE_SELECT, BEEP_SHORT);
        lv_event_stop_processing(e);
        return;
    }
}

// Key handler for web password textarea - handles navigation and ENTER to save
static void web_password_key_handler(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    uint32_t key = lv_event_get_key(e);

    // Handle UP/DOWN navigation from textarea
    if (key == LV_KEY_UP || key == LV_KEY_PREV) {
        // Move focus back to switch
        if (web_password_error_label != NULL) {
            lv_obj_add_flag(web_password_error_label, LV_OBJ_FLAG_HIDDEN);
        }
        lv_group_focus_prev(getLVGLInputGroup());
        lv_event_stop_processing(e);
        return;
    }

    if (key == LV_KEY_DOWN || key == LV_KEY_NEXT) {
        // Can't go down from textarea (it's the last navigable widget)
        lv_event_stop_processing(e);
        return;
    }

    if (key == LV_KEY_ENTER) {
        // Check switch state to determine if protection should be enabled
        bool enableProtection = (web_password_enable_switch != NULL) &&
                                lv_obj_has_state(web_password_enable_switch, LV_STATE_CHECKED);

        if (enableProtection) {
            // Protection enabled - validate and save password
            if (web_password_textarea != NULL) {
                const char* text = lv_textarea_get_text(web_password_textarea);
                String password = String(text);

                if (password.length() >= 8 && password.length() <= 16) {
                    // Valid password - save it
                    webPassword = password;
                    webAuthEnabled = true;
                    saveWebPassword(password);

                    beep(TONE_SELECT, BEEP_MEDIUM);
                    Serial.printf("[WebPW] Password saved, auth enabled\n");
                    onLVGLBackNavigation();
                } else {
                    // Invalid length - show error message
                    beep(TONE_ERROR, BEEP_MEDIUM);
                    if (web_password_error_label != NULL) {
                        lv_label_set_text(web_password_error_label, "Must be 8-16 characters");
                        lv_obj_clear_flag(web_password_error_label, LV_OBJ_FLAG_HIDDEN);
                    }
                    Serial.println("[WebPW] Invalid password length (need 8-16 chars)");
                }
            }
        } else {
            // Protection disabled via switch - clear password
            webPassword = "";
            webAuthEnabled = false;
            clearWebPassword();

            beep(TONE_SELECT, BEEP_MEDIUM);
            Serial.println("[WebPW] Password protection disabled via switch");
            onLVGLBackNavigation();
        }
        lv_event_stop_bubbling(e);
    }
    // ESC is handled by the global back navigation system
}

lv_obj_t* createWebPasswordSettingsScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "WEB PASSWORD");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Content
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, SCREEN_WIDTH - 60, 180);
    lv_obj_center(content);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 15, 0);
    lv_obj_set_style_pad_all(content, 20, 0);
    applyCardStyle(content);

    // Enable switch row
    lv_obj_t* enable_row = lv_obj_create(content);
    lv_obj_set_size(enable_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(enable_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(enable_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(enable_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(enable_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(enable_row, 0, 0);
    lv_obj_set_style_pad_all(enable_row, 0, 0);

    lv_obj_t* enable_label = lv_label_create(enable_row);
    lv_label_set_text(enable_label, "Password Protection");
    lv_obj_add_style(enable_label, getStyleLabelSubtitle(), 0);

    web_password_enable_switch = lv_switch_create(enable_row);
    if (webAuthEnabled) {
        lv_obj_add_state(web_password_enable_switch, LV_STATE_CHECKED);
    }
    lv_obj_add_style(web_password_enable_switch, getStyleSwitch(), 0);
    lv_obj_add_style(web_password_enable_switch, getStyleSwitchChecked(), LV_STATE_CHECKED);
    // Add key handler for switch: LEFT/RIGHT toggles, UP/DOWN navigates
    lv_obj_add_event_cb(web_password_enable_switch, web_password_switch_key_handler, LV_EVENT_KEY, NULL);
    addNavigableWidget(web_password_enable_switch);

    // Password label
    lv_obj_t* pw_label = lv_label_create(content);
    lv_label_set_text(pw_label, "Password (8-16 characters):");
    lv_obj_add_style(pw_label, getStyleLabelBody(), 0);

    // Password text area
    web_password_textarea = lv_textarea_create(content);
    lv_obj_set_size(web_password_textarea, lv_pct(100), 45);
    lv_textarea_set_one_line(web_password_textarea, true);
    lv_textarea_set_max_length(web_password_textarea, 16);
    lv_textarea_set_placeholder_text(web_password_textarea, "Enter password");
    lv_textarea_set_password_mode(web_password_textarea, true);
    if (webPassword.length() > 0) {
        lv_textarea_set_text(web_password_textarea, webPassword.c_str());
    }
    lv_obj_add_style(web_password_textarea, getStyleTextarea(), 0);
    // Add key handler to process ENTER for save (checks switch state for enable/disable)
    lv_obj_add_event_cb(web_password_textarea, web_password_key_handler, LV_EVENT_KEY, NULL);
    addNavigableWidget(web_password_textarea);

    // Error message label (hidden by default)
    web_password_error_label = lv_label_create(content);
    lv_label_set_text(web_password_error_label, "");
    lv_obj_set_style_text_color(web_password_error_label, LV_COLOR_ERROR, 0);
    lv_obj_set_style_text_font(web_password_error_label, getThemeFonts()->font_small, 0);
    lv_obj_add_flag(web_password_error_label, LV_OBJ_FLAG_HIDDEN);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "UP/DN Navigate   L/R Toggle   ENTER Save   ESC Back");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    web_password_screen = screen;

    // Auto-focus the switch first for navigation flow
    focusWidget(web_password_enable_switch);

    return screen;
}

// ============================================
// WiFi Settings Screen
// ============================================

// Include the full WiFi setup screen implementation
#include "lv_wifi_screen.h"

// Forward declaration for back navigation
extern void onLVGLBackNavigation();

// Delegate to the new WiFi setup screen
lv_obj_t* createWiFiSettingsScreen() {
    return createWiFiSetupScreen();
}

// ============================================
// Theme Settings Screen
// ============================================

#include "../settings/settings_theme.h"

static lv_obj_t* theme_settings_screen = NULL;
static lv_obj_t* theme_dropdown = NULL;

static void theme_dropdown_event_cb(lv_event_t* e) {
    lv_obj_t* dd = lv_event_get_target(e);
    int selected = lv_dropdown_get_selected(dd);

    ThemeType newTheme = (selected == 0) ? THEME_SUMMIT : THEME_ENIGMA;

    // Save and apply theme
    saveThemeSetting(newTheme);
    setTheme(newTheme);
}

lv_obj_t* createThemeSettingsScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "UI THEME");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Content card
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, SCREEN_WIDTH - 60, 200);
    lv_obj_center(content);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(content, 20, 0);
    applyCardStyle(content);

    // Label
    lv_obj_t* label = lv_label_create(content);
    lv_label_set_text(label, "Select UI Theme:");
    lv_obj_add_style(label, getStyleLabelSubtitle(), 0);

    // Theme dropdown
    theme_dropdown = lv_dropdown_create(content);
    lv_dropdown_set_options(theme_dropdown, "Summit (Default)\nEnigma (Military)");
    lv_dropdown_set_selected(theme_dropdown, getCurrentTheme() == THEME_SUMMIT ? 0 : 1);
    lv_obj_set_width(theme_dropdown, 280);
    lv_obj_add_style(theme_dropdown, getStyleDropdown(), 0);
    lv_obj_add_event_cb(theme_dropdown, theme_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    addNavigableWidget(theme_dropdown);

    // Theme description
    lv_obj_t* desc = lv_label_create(content);
    if (getCurrentTheme() == THEME_SUMMIT) {
        lv_label_set_text(desc, "Modern dark theme with cyan accents");
    } else {
        lv_label_set_text(desc, "Military-inspired with brass accents\nand typewriter font");
    }
    lv_obj_add_style(desc, getStyleLabelBody(), 0);
    lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, 0);

    // Hint
    lv_obj_t* hint = lv_label_create(content);
    lv_label_set_text(hint, "Theme applies immediately");
    lv_obj_set_style_text_color(hint, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_style_text_font(hint, getThemeFonts()->font_small, 0);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "UP/DN Select   ENTER Apply   ESC Back");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    theme_settings_screen = screen;
    return screen;
}

// ============================================
// System Info Screen
// ============================================

static lv_obj_t* system_info_screen = NULL;
static lv_obj_t* system_info_scroll_container = NULL;
static lv_obj_t* system_info_focus_container = NULL;

// Helper to format uptime as HH:MM:SS
static void formatUptime(unsigned long ms, char* buf, size_t bufSize) {
    unsigned long totalSecs = ms / 1000;
    unsigned long hours = totalSecs / 3600;
    unsigned long minutes = (totalSecs % 3600) / 60;
    unsigned long seconds = totalSecs % 60;
    snprintf(buf, bufSize, "%lu:%02lu:%02lu", hours, minutes, seconds);
}

// Helper to create an info row (label: value)
static void createInfoRow(lv_obj_t* parent, const char* label, const char* value) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_ver(row, 4, 0);

    lv_obj_t* lbl = lv_label_create(row);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_color(lbl, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(lbl, getThemeFonts()->font_body, 0);

    lv_obj_t* val = lv_label_create(row);
    lv_label_set_text(val, value);
    lv_obj_set_style_text_color(val, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(val, getThemeFonts()->font_body, 0);
}

// Key callback for System Info screen scrolling
static void system_info_key_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    if (key == LV_KEY_ESC) {
        onLVGLBackNavigation();
        lv_event_stop_bubbling(e);
        return;
    }

    // Scroll with UP/DOWN
    if (system_info_scroll_container != NULL) {
        if (key == LV_KEY_UP || key == LV_KEY_PREV) {
            lv_obj_scroll_by(system_info_scroll_container, 0, 30, LV_ANIM_ON);
            lv_event_stop_bubbling(e);
            return;
        }
        if (key == LV_KEY_DOWN || key == LV_KEY_NEXT) {
            lv_obj_scroll_by(system_info_scroll_container, 0, -30, LV_ANIM_ON);
            lv_event_stop_bubbling(e);
            return;
        }
    }

    lv_event_stop_bubbling(e);
}

lv_obj_t* createSystemInfoScreen() {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "SYSTEM INFO");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Content card (scrollable)
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, SCREEN_WIDTH - 40, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 20);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, HEADER_HEIGHT + 10);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 8, 0);
    lv_obj_set_style_pad_all(content, 20, 0);
    applyCardStyle(content);
    lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
    system_info_scroll_container = content;

    // Firmware version (prominent)
    lv_obj_t* version_label = lv_label_create(content);
    lv_label_set_text_fmt(version_label, "v%s", FIRMWARE_VERSION);
    lv_obj_set_style_text_color(version_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(version_label, getThemeFonts()->font_title, 0);
    lv_obj_set_width(version_label, lv_pct(100));
    lv_obj_set_style_text_align(version_label, LV_TEXT_ALIGN_CENTER, 0);

    // Build date
    lv_obj_t* date_label = lv_label_create(content);
    lv_label_set_text_fmt(date_label, "Built: %s", FIRMWARE_DATE);
    lv_obj_set_style_text_color(date_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(date_label, getThemeFonts()->font_small, 0);
    lv_obj_set_width(date_label, lv_pct(100));
    lv_obj_set_style_text_align(date_label, LV_TEXT_ALIGN_CENTER, 0);

    // Spacer
    lv_obj_t* spacer = lv_obj_create(content);
    lv_obj_set_size(spacer, lv_pct(100), 10);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);

    // System info rows
    createInfoRow(content, "Device:", FIRMWARE_NAME);
    createInfoRow(content, "Chip:", "ESP32-S3");

    // Free heap
    char heapBuf[32];
    snprintf(heapBuf, sizeof(heapBuf), "%lu KB", ESP.getFreeHeap() / 1024);
    createInfoRow(content, "Free Heap:", heapBuf);

    // PSRAM
    if (psramFound()) {
        char psramBuf[32];
        snprintf(psramBuf, sizeof(psramBuf), "%lu KB", ESP.getFreePsram() / 1024);
        createInfoRow(content, "Free PSRAM:", psramBuf);
    } else {
        createInfoRow(content, "PSRAM:", "Not available");
    }

    // Uptime
    char uptimeBuf[32];
    formatUptime(millis(), uptimeBuf, sizeof(uptimeBuf));
    createInfoRow(content, "Uptime:", uptimeBuf);

    // Invisible focus container for key handling
    system_info_focus_container = lv_obj_create(screen);
    lv_obj_set_size(system_info_focus_container, 0, 0);
    lv_obj_set_style_bg_opa(system_info_focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(system_info_focus_container, 0, 0);
    lv_obj_add_flag(system_info_focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(system_info_focus_container, system_info_key_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(system_info_focus_container);

    // Enable edit mode to receive key events
    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "UP/DN Scroll   ESC Back");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    system_info_screen = screen;
    return screen;
}

// ============================================
// Screen Selector
// Mode values MUST match MenuMode enum in menu_ui.h
// ============================================

lv_obj_t* createSettingsScreenForMode(int mode) {
    switch (mode) {
        case 27: // MODE_VOLUME_SETTINGS
            return createVolumeSettingsScreen();
        case 28: // MODE_BRIGHTNESS_SETTINGS
            return createBrightnessSettingsScreen();
        case 26: // MODE_CW_SETTINGS
            return createCWSettingsScreen();
        case 29: // MODE_CALLSIGN_SETTINGS
            return createCallsignSettingsScreen();
        case 30: // MODE_WEB_PASSWORD_SETTINGS
            return createWebPasswordSettingsScreen();
        case 25: // MODE_WIFI_SETTINGS
            return createWiFiSettingsScreen();
        case 59: // MODE_THEME_SETTINGS
            return createThemeSettingsScreen();
        case 61: // MODE_SYSTEM_INFO
            return createSystemInfoScreen();
        default:
            Serial.printf("[SettingsScreens] Unknown settings mode: %d\n", mode);
            return NULL;
    }
}

#endif // LV_SETTINGS_SCREENS_H
