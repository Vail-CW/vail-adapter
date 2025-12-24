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

// ============================================
// Radio Output Screen
// ============================================

static lv_obj_t* radio_screen = NULL;
static lv_obj_t* radio_mode_label = NULL;
static lv_obj_t* radio_status_label = NULL;
static lv_obj_t* radio_wpm_label = NULL;

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

    // Mode card
    lv_obj_t* mode_card = lv_obj_create(screen);
    lv_obj_set_size(mode_card, SCREEN_WIDTH - 40, 80);
    lv_obj_set_pos(mode_card, 20, HEADER_HEIGHT + 20);
    lv_obj_set_layout(mode_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(mode_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mode_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    applyCardStyle(mode_card);

    lv_obj_t* mode_title = lv_label_create(mode_card);
    lv_label_set_text(mode_title, "Current Mode");
    lv_obj_add_style(mode_title, getStyleLabelBody(), 0);

    radio_mode_label = lv_label_create(mode_card);
    lv_label_set_text(radio_mode_label, "Summit Keyer");
    lv_obj_set_style_text_font(radio_mode_label, getThemeFonts()->font_title, 0);
    lv_obj_set_style_text_color(radio_mode_label, LV_COLOR_ACCENT_CYAN, 0);

    // Settings display
    lv_obj_t* settings_card = lv_obj_create(screen);
    lv_obj_set_size(settings_card, SCREEN_WIDTH - 40, 60);
    lv_obj_set_pos(settings_card, 20, HEADER_HEIGHT + 110);
    lv_obj_set_layout(settings_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(settings_card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(settings_card, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    applyCardStyle(settings_card);

    // WPM
    lv_obj_t* wpm_container = lv_obj_create(settings_card);
    lv_obj_set_size(wpm_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(wpm_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(wpm_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wpm_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(wpm_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wpm_container, 0, 0);
    lv_obj_set_style_pad_all(wpm_container, 0, 0);

    lv_obj_t* wpm_title = lv_label_create(wpm_container);
    lv_label_set_text(wpm_title, "Speed");
    lv_obj_set_style_text_color(wpm_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(wpm_title, getThemeFonts()->font_small, 0);

    radio_wpm_label = lv_label_create(wpm_container);
    lv_label_set_text(radio_wpm_label, "20 WPM");
    lv_obj_set_style_text_color(radio_wpm_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(radio_wpm_label, getThemeFonts()->font_input, 0);

    // Status
    radio_status_label = lv_label_create(screen);
    lv_label_set_text(radio_status_label, "Ready - Use paddle to key radio");
    lv_obj_add_style(radio_status_label, getStyleLabelBody(), 0);
    lv_obj_align(radio_status_label, LV_ALIGN_CENTER, 0, 50);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "M Toggle Mode   Use paddle to key   ESC Exit");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    radio_screen = screen;
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

    // Status card
    lv_obj_t* status_card = lv_obj_create(screen);
    lv_obj_set_size(status_card, SCREEN_WIDTH - 40, 120);
    lv_obj_center(status_card);
    lv_obj_set_layout(status_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(status_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(status_card, 15, 0);
    applyCardStyle(status_card);

    lv_obj_t* bt_icon = lv_label_create(status_card);
    lv_label_set_text(bt_icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_font(bt_icon, getThemeFonts()->font_large, 0);
    lv_obj_set_style_text_color(bt_icon, LV_COLOR_ACCENT_BLUE, 0);

    bt_hid_status_label = lv_label_create(status_card);
    lv_label_set_text(bt_hid_status_label, "Waiting for connection...");
    lv_obj_add_style(bt_hid_status_label, getStyleLabelSubtitle(), 0);

    lv_obj_t* hint = lv_label_create(status_card);
    lv_label_set_text(hint, "Paddle input will send keystrokes to connected device");
    lv_obj_add_style(hint, getStyleLabelBody(), 0);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* help = lv_label_create(footer);
    lv_label_set_text(help, "Use paddle to send keystrokes   ESC Exit");
    lv_obj_set_style_text_color(help, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(help, getThemeFonts()->font_small, 0);
    lv_obj_center(help);

    bt_hid_screen = screen;
    return screen;
}

void updateBTHIDStatus(const char* status, bool connected) {
    if (bt_hid_status_label != NULL) {
        lv_label_set_text(bt_hid_status_label, status);
        if (connected) {
            lv_obj_set_style_text_color(bt_hid_status_label, LV_COLOR_SUCCESS, 0);
        } else {
            lv_obj_set_style_text_color(bt_hid_status_label, LV_COLOR_TEXT_PRIMARY, 0);
        }
    }
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
