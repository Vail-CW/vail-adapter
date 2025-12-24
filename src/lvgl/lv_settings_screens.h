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
}

// Key handler for volume slider - applies acceleration for faster adjustment
static void volume_slider_key_cb(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    lv_obj_t* slider = lv_event_get_target(e);

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
    lv_label_set_text(help, "LEFT/RIGHT Adjust   ESC Back (auto-saves)");
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
static void brightness_slider_key_cb(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    lv_obj_t* slider = lv_event_get_target(e);

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
    lv_label_set_text(help, "LEFT/RIGHT Adjust   ESC Back (auto-saves)");
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
static lv_obj_t* cw_keytype_dropdown = NULL;
static lv_obj_t* cw_speed_value = NULL;
static lv_obj_t* cw_tone_value = NULL;

static void cw_speed_event_cb(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    cwSpeed = lv_slider_get_value(slider);
    if (cw_speed_value != NULL) {
        lv_label_set_text_fmt(cw_speed_value, "%d WPM", cwSpeed);
    }
    saveCWSettings();
}

// Key handler for CW speed slider - applies acceleration
static void cw_speed_key_cb(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    lv_obj_t* slider = lv_event_get_target(e);

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

        lv_event_stop_bubbling(e);
    }
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

// Key handler for CW tone slider - applies acceleration (larger steps for Hz range)
static void cw_tone_key_cb(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    lv_obj_t* slider = lv_event_get_target(e);

    if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT) {
        // Tone range is 400-1200 Hz, so use larger base step (10 Hz)
        int accel = getKeyAccelerationStep();
        int step = 10 * accel;  // 10, 20, or 40 Hz steps
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

        lv_event_stop_bubbling(e);
    }
}

static void cw_keytype_event_cb(lv_event_t* e) {
    lv_obj_t* dd = lv_event_get_target(e);
    setCwKeyTypeFromInt(lv_dropdown_get_selected(dd));
    saveCWSettings();
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
    lv_obj_set_style_pad_row(content, 15, 0);
    lv_obj_set_style_pad_all(content, 15, 0);
    applyCardStyle(content);

    // Speed setting
    lv_obj_t* speed_row = lv_obj_create(content);
    lv_obj_set_size(speed_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(speed_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(speed_row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(speed_row, 5, 0);
    lv_obj_set_style_bg_opa(speed_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(speed_row, 0, 0);
    lv_obj_set_style_pad_all(speed_row, 0, 0);

    lv_obj_t* speed_header = lv_obj_create(speed_row);
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

    cw_speed_slider = lv_slider_create(speed_row);
    lv_obj_set_width(cw_speed_slider, lv_pct(100));
    lv_slider_set_range(cw_speed_slider, WPM_MIN, WPM_MAX);
    lv_slider_set_value(cw_speed_slider, cwSpeed, LV_ANIM_OFF);
    applySliderStyle(cw_speed_slider);
    lv_obj_add_event_cb(cw_speed_slider, cw_speed_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(cw_speed_slider, cw_speed_key_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(cw_speed_slider);

    // Tone setting
    lv_obj_t* tone_row = lv_obj_create(content);
    lv_obj_set_size(tone_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(tone_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tone_row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(tone_row, 5, 0);
    lv_obj_set_style_bg_opa(tone_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tone_row, 0, 0);
    lv_obj_set_style_pad_all(tone_row, 0, 0);

    lv_obj_t* tone_header = lv_obj_create(tone_row);
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

    cw_tone_slider = lv_slider_create(tone_row);
    lv_obj_set_width(cw_tone_slider, lv_pct(100));
    lv_slider_set_range(cw_tone_slider, 400, 1200);
    lv_slider_set_value(cw_tone_slider, cwTone, LV_ANIM_OFF);
    applySliderStyle(cw_tone_slider);
    lv_obj_add_event_cb(cw_tone_slider, cw_tone_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(cw_tone_slider, cw_tone_key_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(cw_tone_slider);

    // Key type setting
    lv_obj_t* keytype_row = lv_obj_create(content);
    lv_obj_set_size(keytype_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(keytype_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(keytype_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(keytype_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(keytype_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(keytype_row, 0, 0);
    lv_obj_set_style_pad_all(keytype_row, 0, 0);

    lv_obj_t* keytype_label = lv_label_create(keytype_row);
    lv_label_set_text(keytype_label, "Key Type");
    lv_obj_add_style(keytype_label, getStyleLabelSubtitle(), 0);

    cw_keytype_dropdown = lv_dropdown_create(keytype_row);
    lv_dropdown_set_options(cw_keytype_dropdown, "Straight\nIambic A\nIambic B");
    lv_dropdown_set_selected(cw_keytype_dropdown, getCwKeyTypeAsInt());
    lv_obj_set_width(cw_keytype_dropdown, 150);
    lv_obj_add_style(cw_keytype_dropdown, getStyleDropdown(), 0);
    lv_obj_add_event_cb(cw_keytype_dropdown, cw_keytype_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    addNavigableWidget(cw_keytype_dropdown);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "UP/DN Select   LEFT/RIGHT Adjust   ESC Back");
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

// Forward declaration for back navigation
extern void onLVGLBackNavigation();

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
    lv_label_set_text(help, "Type callsign   ENTER Save   ESC Cancel");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

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

// Key handler for web password textarea - handles ENTER to save based on switch state
static void web_password_key_handler(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    uint32_t key = lv_event_get_key(e);

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
                    // Invalid length - beep error but stay on screen
                    beep(TONE_ERROR, BEEP_MEDIUM);
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

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "Toggle switch to enable/disable   ENTER Save   ESC Cancel");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    web_password_screen = screen;

    // Auto-focus the password textarea for immediate input
    focusWidget(web_password_textarea);

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
        default:
            Serial.printf("[SettingsScreens] Unknown settings mode: %d\n", mode);
            return NULL;
    }
}

#endif // LV_SETTINGS_SCREENS_H
