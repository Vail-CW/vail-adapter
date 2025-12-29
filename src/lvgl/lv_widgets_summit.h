/*
 * VAIL SUMMIT - LVGL Widget Factory
 * Reusable widget creation functions for consistent UI
 */

#ifndef LV_WIDGETS_SUMMIT_H
#define LV_WIDGETS_SUMMIT_H

#include <lvgl.h>
#include <WiFi.h>
#include "lv_theme_summit.h"
#include "lv_screen_manager.h"
#include "../core/config.h"

// External battery state from status_bar.h
extern int batteryPercent;

// ============================================
// Standard Footer Text Constants
// Using text labels since LVGL fonts don't include arrow symbols
// ============================================

// Menu navigation (up/down to move, enter to select)
#define FOOTER_NAV_ENTER_ESC        "UP/DN Navigate   ENTER Select   ESC Back"

// Slider/value adjustment (left/right to change)
#define FOOTER_ADJUST_ESC           "L/R Adjust   ESC Back (auto-saves)"

// Combined navigation + adjustment
#define FOOTER_NAV_ADJUST_ESC       "UP/DN Navigate   L/R Adjust   ESC Back"

// Text input screens
#define FOOTER_TYPE_ENTER_ESC       "Type text   ENTER Save   ESC Cancel"

// Game screens
#define FOOTER_GAME_ESC             "SPACE Pause   ESC Exit"
#define FOOTER_GAME_SETTINGS        "UP/DN Navigate   L/R Adjust   ENTER Save   ESC Cancel"

// Context menu/confirmation
#define FOOTER_CONTEXT_MENU         "UP/DN Select   ENTER Confirm   ESC Cancel"
#define FOOTER_CONFIRM_DIALOG       "L/R Select   ENTER Confirm   ESC Cancel"

// Training modes
#define FOOTER_TRAINING_ACTIVE      "ENTER Submit   LEFT Replay   TAB Skip   ESC Exit"
#define FOOTER_TRAINING_WAIT        "Keying in progress...   ESC Exit"

// ============================================
// Forward Declarations
// ============================================

// Widget creation functions
lv_obj_t* createMenuCard(lv_obj_t* parent, const char* icon, const char* title, lv_event_cb_t click_cb, void* user_data);
lv_obj_t* createSettingsRow(lv_obj_t* parent, const char* label, const char* value);
lv_obj_t* createValueSlider(lv_obj_t* parent, const char* label, int min, int max, int current, lv_event_cb_t change_cb);
lv_obj_t* createTextInput(lv_obj_t* parent, const char* placeholder, const char* initial_text, int max_length);
lv_obj_t* createStatsCard(lv_obj_t* parent, const char* title, const char** labels, const char** values, int count);
lv_obj_t* createStatusBar(lv_obj_t* parent);
lv_obj_t* createCompactStatusBar(lv_obj_t* parent);
lv_obj_t* createScrollableList(lv_obj_t* parent, int height);
lv_obj_t* createConfirmDialog(const char* title, const char* message, lv_event_cb_t confirm_cb, lv_event_cb_t cancel_cb);

// ============================================
// Menu Card Widget
// ============================================

/*
 * Create a menu card with icon circle, title, and right arrow
 * Used for main menu navigation
 *
 * Parameters:
 *   parent - Parent object to add card to
 *   icon - Single character for icon (e.g., "T" for Training)
 *   title - Menu item title text
 *   click_cb - Callback when card is selected
 *   user_data - User data passed to callback
 */
lv_obj_t* createMenuCard(lv_obj_t* parent, const char* icon, const char* title, lv_event_cb_t click_cb, void* user_data) {
    // Create card container
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, CARD_MAIN_WIDTH, CARD_MAIN_HEIGHT);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(card, 15, 0);

    // Apply menu card style
    applyMenuCardStyle(card);

    // Icon circle
    lv_obj_t* icon_circle = lv_obj_create(card);
    lv_obj_set_size(icon_circle, ICON_RADIUS * 2, ICON_RADIUS * 2);
    lv_obj_add_style(icon_circle, getStyleIconCircle(), 0);
    lv_obj_clear_flag(icon_circle, LV_OBJ_FLAG_SCROLLABLE);

    // Icon letter - use theme font for consistency
    lv_obj_t* icon_label = lv_label_create(icon_circle);
    lv_label_set_text(icon_label, icon);
    lv_obj_set_style_text_font(icon_label, getThemeFonts()->font_title, 0);
    lv_obj_set_style_text_color(icon_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_center(icon_label);

    // Title label
    lv_obj_t* title_label = lv_label_create(card);
    lv_label_set_text(title_label, title);
    lv_obj_add_style(title_label, getStyleLabelSubtitle(), 0);
    lv_obj_set_flex_grow(title_label, 1);

    // Right arrow indicator
    lv_obj_t* arrow = lv_label_create(card);
    lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(arrow, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_style_text_font(arrow, &lv_font_montserrat_18, 0);

    // Make card focusable and add to navigation group
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    addNavigableWidget(card);

    // Add click event
    if (click_cb != NULL) {
        lv_obj_add_event_cb(card, click_cb, LV_EVENT_CLICKED, user_data);
    }

    return card;
}

// ============================================
// Settings Row Widget
// ============================================

/*
 * Create a settings row with label and value display
 * Used for showing current settings with option to edit
 *
 * Parameters:
 *   parent - Parent object
 *   label - Setting name
 *   value - Current value text
 */
lv_obj_t* createSettingsRow(lv_obj_t* parent, const char* label, const char* value) {
    // Row container
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(row, 12, 0);
    lv_obj_set_style_pad_column(row, 10, 0);

    // Apply card style
    applyCardStyle(row);

    // Label
    lv_obj_t* lbl = lv_label_create(row);
    lv_label_set_text(lbl, label);
    lv_obj_add_style(lbl, getStyleLabelBody(), 0);

    // Value (highlighted) - use theme font
    lv_obj_t* val = lv_label_create(row);
    lv_label_set_text(val, value);
    lv_obj_set_style_text_color(val, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(val, getThemeFonts()->font_input, 0);

    // Make focusable
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    addNavigableWidget(row);

    return row;
}

// ============================================
// Value Slider Widget
// ============================================

/*
 * Create a labeled slider for adjusting values
 * Used for volume, brightness, speed settings
 *
 * Parameters:
 *   parent - Parent object
 *   label - Setting name
 *   min - Minimum value
 *   max - Maximum value
 *   current - Current value
 *   change_cb - Callback when value changes
 */
lv_obj_t* createValueSlider(lv_obj_t* parent, const char* label, int min, int max, int current, lv_event_cb_t change_cb) {
    // Container
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_size(container, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(container, 15, 0);
    lv_obj_set_style_pad_row(container, 10, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    // Header row (label + value)
    lv_obj_t* header = lv_obj_create(container);
    lv_obj_set_size(header, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    // Label
    lv_obj_t* lbl = lv_label_create(header);
    lv_label_set_text(lbl, label);
    lv_obj_add_style(lbl, getStyleLabelSubtitle(), 0);

    // Value display - use theme font
    lv_obj_t* val = lv_label_create(header);
    lv_label_set_text_fmt(val, "%d", current);
    lv_obj_set_style_text_color(val, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(val, getThemeFonts()->font_subtitle, 0);

    // Slider
    lv_obj_t* slider = lv_slider_create(container);
    lv_obj_set_width(slider, lv_pct(100));
    lv_slider_set_range(slider, min, max);
    lv_slider_set_value(slider, current, LV_ANIM_OFF);
    applySliderStyle(slider);

    // Store value label reference in slider user data for updates
    lv_obj_set_user_data(slider, val);

    // Add change callback
    if (change_cb != NULL) {
        lv_obj_add_event_cb(slider, change_cb, LV_EVENT_VALUE_CHANGED, NULL);
    }

    // Update value label on change (internal handler)
    lv_obj_add_event_cb(slider, [](lv_event_t* e) {
        lv_obj_t* slider = lv_event_get_target(e);
        lv_obj_t* val = (lv_obj_t*)lv_obj_get_user_data(slider);
        if (val != NULL) {
            lv_label_set_text_fmt(val, "%d", lv_slider_get_value(slider));
        }
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Make slider focusable
    addNavigableWidget(slider);

    return container;
}

// ============================================
// Text Input Widget
// ============================================

/*
 * Create a text input field with cursor
 * Used for callsign, password, notes
 *
 * Parameters:
 *   parent - Parent object
 *   placeholder - Placeholder text when empty
 *   initial_text - Initial text content
 *   max_length - Maximum character length
 */
lv_obj_t* createTextInput(lv_obj_t* parent, const char* placeholder, const char* initial_text, int max_length) {
    lv_obj_t* ta = lv_textarea_create(parent);
    lv_obj_set_size(ta, lv_pct(100), LV_SIZE_CONTENT);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, max_length);
    lv_textarea_set_placeholder_text(ta, placeholder);

    if (initial_text != NULL && strlen(initial_text) > 0) {
        lv_textarea_set_text(ta, initial_text);
    }

    lv_obj_add_style(ta, getStyleTextarea(), 0);

    // Make focusable
    addNavigableWidget(ta);

    return ta;
}

// ============================================
// Stats Card Widget
// ============================================

/*
 * Create a statistics display card
 * Used for training stats, QSO stats overlay
 *
 * Parameters:
 *   parent - Parent object
 *   title - Card title
 *   labels - Array of stat labels
 *   values - Array of stat values
 *   count - Number of stats
 */
lv_obj_t* createStatsCard(lv_obj_t* parent, const char* title, const char** labels, const char** values, int count) {
    // Card container
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, 300, LV_SIZE_CONTENT);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(card, 15, 0);
    lv_obj_set_style_pad_row(card, 8, 0);
    applyCardStyle(card);

    // Title
    lv_obj_t* title_lbl = lv_label_create(card);
    lv_label_set_text(title_lbl, title);
    lv_obj_add_style(title_lbl, getStyleLabelTitle(), 0);

    // Stats rows
    for (int i = 0; i < count; i++) {
        lv_obj_t* row = lv_obj_create(card);
        lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_layout(row, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);

        lv_obj_t* lbl = lv_label_create(row);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_add_style(lbl, getStyleLabelBody(), 0);

        lv_obj_t* val = lv_label_create(row);
        lv_label_set_text(val, values[i]);
        lv_obj_set_style_text_color(val, LV_COLOR_ACCENT_CYAN, 0);
    }

    return card;
}

// ============================================
// Status Bar Widget
// ============================================

/*
 * Create a full-width status bar with title, WiFi, and battery icons
 * Used at top of screen for main views
 *
 * Parameters:
 *   parent - Parent screen object
 */
lv_obj_t* createStatusBar(lv_obj_t* parent) {
    lv_obj_t* bar = lv_obj_create(parent);
    lv_obj_set_size(bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_layout(bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_style(bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    // Left section (title or mode indicator)
    lv_obj_t* left = lv_obj_create(bar);
    lv_obj_set_size(left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left, 0, 0);
    lv_obj_set_style_pad_all(left, 0, 0);

    lv_obj_t* title = lv_label_create(left);
    lv_label_set_text(title, "VAIL SUMMIT");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);

    // Right section - battery icon only
    lv_obj_t* battery_icon = lv_label_create(bar);
    lv_obj_set_style_text_font(battery_icon, &lv_font_montserrat_20, 0);

    if (batteryPercent > 80) {
        lv_label_set_text(battery_icon, LV_SYMBOL_BATTERY_FULL);
        lv_obj_set_style_text_color(battery_icon, LV_COLOR_SUCCESS, 0);
    } else if (batteryPercent > 60) {
        lv_label_set_text(battery_icon, LV_SYMBOL_BATTERY_3);
        lv_obj_set_style_text_color(battery_icon, LV_COLOR_SUCCESS, 0);
    } else if (batteryPercent > 40) {
        lv_label_set_text(battery_icon, LV_SYMBOL_BATTERY_2);
        lv_obj_set_style_text_color(battery_icon, LV_COLOR_ACCENT_CYAN, 0);
    } else if (batteryPercent > 20) {
        lv_label_set_text(battery_icon, LV_SYMBOL_BATTERY_1);
        lv_obj_set_style_text_color(battery_icon, LV_COLOR_WARNING, 0);
    } else {
        lv_label_set_text(battery_icon, LV_SYMBOL_BATTERY_EMPTY);
        lv_obj_set_style_text_color(battery_icon, LV_COLOR_ERROR, 0);
    }

    return bar;
}

/*
 * Create status icons (WiFi + battery) for top-right of any screen
 * Matches the header icons exactly for consistency
 *
 * Parameters:
 *   parent - Parent screen or header object
 */
lv_obj_t* createCompactStatusBar(lv_obj_t* parent) {
    // WiFi icon - same pattern as createHeader()
    lv_obj_t* wifi_icon = lv_label_create(parent);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_20, 0);
    if (WiFi.status() == WL_CONNECTED) {
        lv_obj_set_style_text_color(wifi_icon, LV_COLOR_SUCCESS, 0);
    } else {
        lv_obj_set_style_text_color(wifi_icon, LV_COLOR_ERROR, 0);
    }
    lv_obj_align(wifi_icon, LV_ALIGN_TOP_RIGHT, -50, 8);

    // Battery icon - same pattern as createHeader()
    lv_obj_t* batt_icon = lv_label_create(parent);
    lv_obj_set_style_text_font(batt_icon, &lv_font_montserrat_20, 0);
    lv_obj_align(batt_icon, LV_ALIGN_TOP_RIGHT, -10, 8);

    if (batteryPercent > 80) {
        lv_label_set_text(batt_icon, LV_SYMBOL_BATTERY_FULL);
        lv_obj_set_style_text_color(batt_icon, LV_COLOR_SUCCESS, 0);
    } else if (batteryPercent > 60) {
        lv_label_set_text(batt_icon, LV_SYMBOL_BATTERY_3);
        lv_obj_set_style_text_color(batt_icon, LV_COLOR_SUCCESS, 0);
    } else if (batteryPercent > 40) {
        lv_label_set_text(batt_icon, LV_SYMBOL_BATTERY_2);
        lv_obj_set_style_text_color(batt_icon, LV_COLOR_ACCENT_CYAN, 0);
    } else if (batteryPercent > 20) {
        lv_label_set_text(batt_icon, LV_SYMBOL_BATTERY_1);
        lv_obj_set_style_text_color(batt_icon, LV_COLOR_WARNING, 0);
    } else {
        lv_label_set_text(batt_icon, LV_SYMBOL_BATTERY_EMPTY);
        lv_obj_set_style_text_color(batt_icon, LV_COLOR_ERROR, 0);
    }

    return batt_icon;
}

// ============================================
// Scrollable List Widget
// ============================================

/*
 * Create a scrollable list container
 * Used for menu lists, settings lists, log entries
 *
 * Parameters:
 *   parent - Parent object
 *   height - List height in pixels
 */
lv_obj_t* createScrollableList(lv_obj_t* parent, int height) {
    lv_obj_t* list = lv_obj_create(parent);
    lv_obj_set_size(list, lv_pct(100), height);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(list, 5, 0);
    lv_obj_set_style_pad_row(list, 5, 0);
    applyListStyle(list);

    // Enable scrolling
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);

    return list;
}

// ============================================
// Confirm Dialog Widget
// ============================================

/*
 * Create a confirmation dialog with Yes/No buttons
 *
 * Parameters:
 *   title - Dialog title
 *   message - Message text
 *   confirm_cb - Callback for Yes button
 *   cancel_cb - Callback for No button
 */
lv_obj_t* createConfirmDialog(const char* title, const char* message, lv_event_cb_t confirm_cb, lv_event_cb_t cancel_cb) {
    static const char* btns[] = {"Yes", "No", ""};

    lv_obj_t* mbox = lv_msgbox_create(NULL, title, message, btns, false);
    lv_obj_center(mbox);
    lv_obj_add_style(mbox, getStyleMsgbox(), 0);

    // Get button matrix and add to navigation
    lv_obj_t* btns_obj = lv_msgbox_get_btns(mbox);
    addNavigableWidget(btns_obj);

    // Add event handler for button clicks
    lv_obj_add_event_cb(mbox, [](lv_event_t* e) {
        lv_obj_t* obj = lv_event_get_current_target(e);
        const char* txt = lv_msgbox_get_active_btn_text(obj);

        if (txt != NULL) {
            if (strcmp(txt, "Yes") == 0) {
                // Get confirm callback from user data
                void** cbs = (void**)lv_obj_get_user_data(obj);
                if (cbs != NULL && cbs[0] != NULL) {
                    ((lv_event_cb_t)cbs[0])(e);
                }
            } else if (strcmp(txt, "No") == 0) {
                // Get cancel callback from user data
                void** cbs = (void**)lv_obj_get_user_data(obj);
                if (cbs != NULL && cbs[1] != NULL) {
                    ((lv_event_cb_t)cbs[1])(e);
                }
            }
            lv_msgbox_close(obj);
        }
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Store callbacks in user data
    static void* callbacks[2];
    callbacks[0] = (void*)confirm_cb;
    callbacks[1] = (void*)cancel_cb;
    lv_obj_set_user_data(mbox, callbacks);

    return mbox;
}

// ============================================
// Specialized Widget Helpers
// ============================================

/*
 * Create a decoder display box (monospace text for decoded morse)
 */
lv_obj_t* createDecoderBox(lv_obj_t* parent, int width, int height) {
    lv_obj_t* box = lv_obj_create(parent);
    lv_obj_set_size(box, width, height);
    lv_obj_set_style_bg_color(box, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_border_color(box, LV_COLOR_BORDER_SUBTLE, 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_radius(box, 6, 0);
    lv_obj_set_style_pad_all(box, 10, 0);

    lv_obj_t* text = lv_label_create(box);
    lv_label_set_text(text, "");
    lv_obj_set_style_text_font(text, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(text, LV_COLOR_ACCENT_GREEN, 0);
    lv_label_set_long_mode(text, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(text, width - 20);

    return box;
}

/*
 * Create a WPM display indicator
 */
lv_obj_t* createWPMIndicator(lv_obj_t* parent, int initial_wpm) {
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_size(container, 100, 50);
    lv_obj_set_style_bg_color(container, LV_COLOR_CARD_TEAL, 0);
    lv_obj_set_style_radius(container, 8, 0);
    lv_obj_set_style_pad_all(container, 5, 0);

    lv_obj_t* label = lv_label_create(container);
    lv_label_set_text(label, "WPM");
    lv_obj_set_style_text_font(label, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* value = lv_label_create(container);
    lv_label_set_text_fmt(value, "%d", initial_wpm);
    lv_obj_set_style_text_font(value, getThemeFonts()->font_title, 0);
    lv_obj_set_style_text_color(value, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(value, LV_ALIGN_BOTTOM_MID, 0, 0);

    return container;
}

/*
 * Create a game score display
 */
lv_obj_t* createScoreDisplay(lv_obj_t* parent, const char* label, int initial_score) {
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(container, 8, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);

    lv_obj_t* lbl = lv_label_create(container);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_color(lbl, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(lbl, getThemeFonts()->font_body, 0);

    lv_obj_t* val = lv_label_create(container);
    lv_label_set_text_fmt(val, "%d", initial_score);
    lv_obj_set_style_text_color(val, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(val, getThemeFonts()->font_subtitle, 0);

    return container;
}

/*
 * Create lives display (hearts)
 */
lv_obj_t* createLivesDisplay(lv_obj_t* parent, int lives, int max_lives) {
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(container, 4, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);

    for (int i = 0; i < max_lives; i++) {
        lv_obj_t* heart = lv_label_create(container);
        if (i < lives) {
            lv_label_set_text(heart, LV_SYMBOL_OK);  // Filled heart substitute - keep Montserrat for symbols
            lv_obj_set_style_text_color(heart, LV_COLOR_ERROR, 0);
        } else {
            lv_label_set_text(heart, LV_SYMBOL_CLOSE);  // Empty heart substitute
            lv_obj_set_style_text_color(heart, LV_COLOR_TEXT_DISABLED, 0);
        }
        lv_obj_set_style_text_font(heart, &lv_font_montserrat_18, 0);  // Keep Montserrat for symbols
    }

    return container;
}

/*
 * Create a character progress grid (for Koch method or Hear It Type It)
 */
lv_obj_t* createCharacterGrid(lv_obj_t* parent, const char* characters, bool* unlocked, int count) {
    lv_obj_t* grid = lv_obj_create(parent);
    lv_obj_set_size(grid, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(grid, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_all(grid, 10, 0);
    lv_obj_set_style_pad_gap(grid, 8, 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);

    for (int i = 0; i < count; i++) {
        lv_obj_t* cell = lv_obj_create(grid);
        lv_obj_set_size(cell, 40, 40);
        lv_obj_set_style_radius(cell, 6, 0);

        if (unlocked[i]) {
            lv_obj_set_style_bg_color(cell, LV_COLOR_SUCCESS, 0);
        } else {
            lv_obj_set_style_bg_color(cell, LV_COLOR_BG_LAYER2, 0);
        }

        char ch[2] = {characters[i], '\0'};
        lv_obj_t* lbl = lv_label_create(cell);
        lv_label_set_text(lbl, ch);
        lv_obj_center(lbl);
        lv_obj_set_style_text_font(lbl, getThemeFonts()->font_subtitle, 0);

        if (unlocked[i]) {
            lv_obj_set_style_text_color(lbl, LV_COLOR_BG_DEEP, 0);
        } else {
            lv_obj_set_style_text_color(lbl, LV_COLOR_TEXT_DISABLED, 0);
        }
    }

    return grid;
}

#endif // LV_WIDGETS_SUMMIT_H
