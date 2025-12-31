/*
 * VAIL SUMMIT - Band Plans Screen
 * US Amateur Radio HF Band Allocations with license visualization
 */

#ifndef LV_BAND_PLANS_H
#define LV_BAND_PLANS_H

#include <lvgl.h>
#include "lv_theme_summit.h"
#include "lv_widgets_summit.h"
#include "lv_screen_manager.h"
#include "../core/config.h"
#include "../data/band_plan_data.h"
#include "../settings/settings_band_plan.h"

// ============================================
// View Mode
// ============================================

typedef enum {
    BP_VIEW_OVERVIEW = 0,
    BP_VIEW_DETAIL = 1
} BandPlanViewMode;

// ============================================
// Screen State
// ============================================

static lv_obj_t* bp_screen = NULL;
static lv_obj_t* bp_content = NULL;
static lv_obj_t* bp_license_label = NULL;
static lv_obj_t* bp_mode_label = NULL;
static lv_obj_t* bp_footer_text = NULL;

static BandPlanViewMode bp_view_mode = BP_VIEW_OVERVIEW;
static int bp_focused_band = 0;
static int bp_focused_row = 0;       // For detail view
static int bp_detail_scroll = 0;     // Scroll offset in detail view

// Band cards in overview
static lv_obj_t* bp_band_cards[10] = {NULL};

// ============================================
// Forward Declarations
// ============================================

void updateBandPlansOverview();
void updateBandPlansDetail();
void updateLicenseLabel();
void updateModeLabel();
void updateBandCardFocus();
lv_obj_t* createBandCard(lv_obj_t* parent, int band_index);
lv_obj_t* createLicenseColorBar(lv_obj_t* parent, const BandDefinition* band, int width);
void showBandDetail(int band_index);
void exitBandDetail();

// ============================================
// Color Helpers
// ============================================

static lv_color_t getLicenseColor(LicenseClass lic, bool user_can_operate) {
    if (user_can_operate) {
        return LV_COLOR_ACCENT_CYAN;  // User can operate here
    }
    switch (lic) {
        case LICENSE_EXTRA:      return LV_COLOR_SUCCESS;   // Green - Extra only
        case LICENSE_GENERAL:    return LV_COLOR_WARNING;   // Orange - General
        case LICENSE_TECHNICIAN: return lv_color_hex(0x666666);  // Gray - Tech
        default:                 return lv_color_hex(0x333333);
    }
}

// ============================================
// Update Label Functions
// ============================================

void updateLicenseLabel() {
    if (!bp_license_label) return;
    LicenseClass lic = getBPUserLicense();
    lv_label_set_text_fmt(bp_license_label, "< %s >", getLicenseClassName(lic));
}

void updateModeLabel() {
    if (!bp_mode_label) return;
    uint8_t filter = getBPModeFilter();
    lv_label_set_text_fmt(bp_mode_label, "< %s >", getModeFilterLabel(filter));
}

// ============================================
// Create License Color Bar
// ============================================

lv_obj_t* createLicenseColorBar(lv_obj_t* parent, const BandDefinition* band, int width) {
    lv_obj_t* bar = lv_obj_create(parent);
    lv_obj_set_size(bar, width, 14);
    lv_obj_set_style_bg_color(bar, getThemeColors()->bg_deep, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bar, 3, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 1, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    LicenseClass user_lic = getBPUserLicense();
    uint8_t mode_filter = getBPModeFilter();
    float total_span = band->end_mhz - band->start_mhz;
    int available_width = width - 2;  // Account for padding

    for (int i = 0; i < band->entry_count; i++) {
        const BandPlanEntry* entry = &band->entries[i];

        // Skip if mode filter doesn't match
        if (!modeMatchesFilter(entry->modes, mode_filter)) continue;

        float seg_span = entry->end_mhz - entry->start_mhz;
        int seg_width = (int)((seg_span / total_span) * available_width);
        if (seg_width < 2) seg_width = 2;  // Minimum visibility

        // Calculate position
        float seg_offset = entry->start_mhz - band->start_mhz;
        int x_pos = (int)((seg_offset / total_span) * available_width) + 1;

        lv_obj_t* seg = lv_obj_create(bar);
        lv_obj_set_size(seg, seg_width, 10);
        lv_obj_set_pos(seg, x_pos, 1);

        bool can_operate = canOperate(entry, user_lic);
        lv_color_t color = getLicenseColor(entry->license, can_operate);

        lv_obj_set_style_bg_color(seg, color, 0);
        lv_obj_set_style_bg_opa(seg, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(seg, 2, 0);
        lv_obj_set_style_border_width(seg, 0, 0);
        lv_obj_clear_flag(seg, LV_OBJ_FLAG_SCROLLABLE);
    }

    return bar;
}

// ============================================
// Create Band Card (Overview)
// ============================================

lv_obj_t* createBandCard(lv_obj_t* parent, int band_index) {
    const BandDefinition* band = getBandByIndex(band_index);
    if (!band) return NULL;

    // Full-width single-column card
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, 440, 56);
    lv_obj_set_style_bg_color(card, getThemeColors()->bg_layer2, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, getThemeColors()->card_border, 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // Band name (short) - left side
    lv_obj_t* name = lv_label_create(card);
    lv_label_set_text(name, band->short_name);
    lv_obj_set_style_text_font(name, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(name, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_pos(name, 0, 0);

    // Frequency range - next to name
    char freq_buf[32];
    if (band->start_mhz >= 10.0f) {
        snprintf(freq_buf, sizeof(freq_buf), "%.2f-%.2f MHz", band->start_mhz, band->end_mhz);
    } else {
        snprintf(freq_buf, sizeof(freq_buf), "%.3f-%.3f MHz", band->start_mhz, band->end_mhz);
    }
    lv_obj_t* freq = lv_label_create(card);
    lv_label_set_text(freq, freq_buf);
    lv_obj_set_style_text_font(freq, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(freq, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_pos(freq, 65, 2);

    // WARC indicator - middle area
    if (band->warc_band) {
        lv_obj_t* warc = lv_label_create(card);
        lv_label_set_text(warc, "WARC");
        lv_obj_set_style_text_font(warc, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(warc, LV_COLOR_WARNING, 0);
        lv_obj_set_pos(warc, 220, 4);
    }

    // License color bar - bottom, full width
    lv_obj_t* color_bar = createLicenseColorBar(card, band, 420);
    lv_obj_set_pos(color_bar, 0, 26);

    return card;
}

// ============================================
// Update Band Card Focus
// ============================================

void updateBandCardFocus() {
    for (int i = 0; i < 10; i++) {
        if (bp_band_cards[i]) {
            if (i == bp_focused_band) {
                lv_obj_set_style_border_color(bp_band_cards[i], LV_COLOR_ACCENT_CYAN, 0);
                lv_obj_set_style_border_width(bp_band_cards[i], 2, 0);
                lv_obj_set_style_bg_color(bp_band_cards[i], getThemeColors()->card_secondary, 0);
            } else {
                lv_obj_set_style_border_color(bp_band_cards[i], getThemeColors()->card_border, 0);
                lv_obj_set_style_border_width(bp_band_cards[i], 1, 0);
                lv_obj_set_style_bg_color(bp_band_cards[i], getThemeColors()->bg_layer2, 0);
            }
        }
    }
}

// ============================================
// Update Overview Content
// ============================================

void updateBandPlansOverview() {
    if (!bp_content) return;

    // Clear existing content
    lv_obj_clean(bp_content);

    // Reset card pointers
    for (int i = 0; i < 10; i++) {
        bp_band_cards[i] = NULL;
    }

    // Single-column layout with full-width cards
    int card_height = 56;
    int v_gap = 6;
    int start_x = 0;
    int start_y = 0;

    int band_count = getBandCount();
    for (int i = 0; i < band_count && i < 10; i++) {
        lv_obj_t* card = createBandCard(bp_content, i);
        if (card) {
            int y = start_y + i * (card_height + v_gap);
            lv_obj_set_pos(card, start_x, y);
            bp_band_cards[i] = card;
        }
    }

    // Update focus state and scroll to focused band
    updateBandCardFocus();

    // Scroll focused card into view
    if (bp_band_cards[bp_focused_band]) {
        lv_obj_scroll_to_view(bp_band_cards[bp_focused_band], LV_ANIM_ON);
    }
}

// ============================================
// Create Detail Row
// ============================================

static lv_obj_t* createDetailRow(lv_obj_t* parent, const BandPlanEntry* entry, int y_pos, bool can_operate, bool is_focused) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, 440, 28);
    lv_obj_set_pos(row, 0, y_pos);

    if (is_focused) {
        lv_obj_set_style_bg_color(row, getThemeColors()->card_secondary, 0);
        lv_obj_set_style_border_color(row, LV_COLOR_ACCENT_CYAN, 0);
        lv_obj_set_style_border_width(row, 1, 0);
    } else {
        lv_obj_set_style_bg_color(row, getThemeColors()->bg_layer2, 0);
        lv_obj_set_style_border_width(row, 0, 0);
    }
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, 4, 0);
    lv_obj_set_style_pad_all(row, 4, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    // Frequency range
    char freq_buf[32];
    snprintf(freq_buf, sizeof(freq_buf), "%.3f-%.3f", entry->start_mhz, entry->end_mhz);
    lv_obj_t* freq_lbl = lv_label_create(row);
    lv_label_set_text(freq_lbl, freq_buf);
    lv_obj_set_style_text_font(freq_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(freq_lbl, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_pos(freq_lbl, 5, 4);

    // Mode
    lv_obj_t* mode_lbl = lv_label_create(row);
    lv_label_set_text(mode_lbl, getModeLabel(entry->modes));
    lv_obj_set_style_text_font(mode_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(mode_lbl, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_pos(mode_lbl, 150, 4);

    // License class
    lv_obj_t* lic_lbl = lv_label_create(row);
    lv_label_set_text(lic_lbl, getLicenseClassShort(entry->license));
    lv_obj_set_style_text_font(lic_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lic_lbl, 280, 3);

    // Color based on whether user can operate
    if (can_operate) {
        lv_obj_set_style_text_color(lic_lbl, LV_COLOR_ACCENT_CYAN, 0);
    } else {
        lv_obj_set_style_text_color(lic_lbl, getLicenseColor(entry->license, false), 0);
    }

    // Status indicator (small colored box)
    lv_obj_t* status = lv_obj_create(row);
    lv_obj_set_size(status, 60, 16);
    lv_obj_set_pos(status, 320, 4);
    lv_obj_set_style_radius(status, 4, 0);
    lv_obj_set_style_border_width(status, 0, 0);
    lv_obj_clear_flag(status, LV_OBJ_FLAG_SCROLLABLE);

    if (can_operate) {
        lv_obj_set_style_bg_color(status, LV_COLOR_ACCENT_CYAN, 0);
    } else {
        lv_obj_set_style_bg_color(status, lv_color_hex(0x444444), 0);
    }
    lv_obj_set_style_bg_opa(status, LV_OPA_COVER, 0);

    // Status text
    lv_obj_t* status_txt = lv_label_create(status);
    lv_label_set_text(status_txt, can_operate ? "OK" : "--");
    lv_obj_set_style_text_font(status_txt, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(status_txt, can_operate ? lv_color_hex(0x000000) : LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_center(status_txt);

    return row;
}

// ============================================
// Update Detail Content
// ============================================

void updateBandPlansDetail() {
    if (!bp_content) return;

    const BandDefinition* band = getBandByIndex(bp_focused_band);
    if (!band) return;

    // Clear existing content
    lv_obj_clean(bp_content);

    LicenseClass user_lic = getBPUserLicense();
    uint8_t mode_filter = getBPModeFilter();

    // Band header info card
    lv_obj_t* header_card = lv_obj_create(bp_content);
    lv_obj_set_size(header_card, 440, 50);
    lv_obj_set_pos(header_card, 0, 0);
    lv_obj_set_style_bg_color(header_card, getThemeColors()->bg_layer2, 0);
    lv_obj_set_style_bg_opa(header_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(header_card, 8, 0);
    lv_obj_set_style_border_width(header_card, 1, 0);
    lv_obj_set_style_border_color(header_card, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_pad_all(header_card, 8, 0);
    lv_obj_clear_flag(header_card, LV_OBJ_FLAG_SCROLLABLE);

    // Band name
    lv_obj_t* name_lbl = lv_label_create(header_card);
    lv_label_set_text(name_lbl, band->name);
    lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(name_lbl, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_pos(name_lbl, 0, 0);

    // Frequency range
    char freq_buf[48];
    snprintf(freq_buf, sizeof(freq_buf), "%.3f - %.3f MHz", band->start_mhz, band->end_mhz);
    lv_obj_t* freq_lbl = lv_label_create(header_card);
    lv_label_set_text(freq_lbl, freq_buf);
    lv_obj_set_style_text_font(freq_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(freq_lbl, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_pos(freq_lbl, 0, 22);

    // Power limit
    char power_buf[32];
    snprintf(power_buf, sizeof(power_buf), "%dW Max", band->max_power_watts);
    lv_obj_t* power_lbl = lv_label_create(header_card);
    lv_label_set_text(power_lbl, power_buf);
    lv_obj_set_style_text_font(power_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(power_lbl, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_pos(power_lbl, 200, 0);

    // WARC indicator
    if (band->warc_band) {
        lv_obj_t* warc_lbl = lv_label_create(header_card);
        lv_label_set_text(warc_lbl, "WARC (No Contests)");
        lv_obj_set_style_text_font(warc_lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(warc_lbl, LV_COLOR_WARNING, 0);
        lv_obj_set_pos(warc_lbl, 200, 22);
    }

    // Your license
    lv_obj_t* your_lic = lv_label_create(header_card);
    char lic_buf[32];
    snprintf(lic_buf, sizeof(lic_buf), "Your: %s", getLicenseClassShort(user_lic));
    lv_label_set_text(your_lic, lic_buf);
    lv_obj_set_style_text_font(your_lic, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(your_lic, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(your_lic, LV_ALIGN_TOP_RIGHT, 0, 10);

    // Column headers
    lv_obj_t* hdr_freq = lv_label_create(bp_content);
    lv_label_set_text(hdr_freq, "FREQUENCY");
    lv_obj_set_style_text_font(hdr_freq, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hdr_freq, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_pos(hdr_freq, 10, 55);

    lv_obj_t* hdr_mode = lv_label_create(bp_content);
    lv_label_set_text(hdr_mode, "MODES");
    lv_obj_set_style_text_font(hdr_mode, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hdr_mode, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_pos(hdr_mode, 155, 55);

    lv_obj_t* hdr_lic = lv_label_create(bp_content);
    lv_label_set_text(hdr_lic, "LIC");
    lv_obj_set_style_text_font(hdr_lic, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hdr_lic, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_pos(hdr_lic, 285, 55);

    lv_obj_t* hdr_status = lv_label_create(bp_content);
    lv_label_set_text(hdr_status, "STATUS");
    lv_obj_set_style_text_font(hdr_status, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hdr_status, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_pos(hdr_status, 325, 55);

    // Sub-band rows
    int y_pos = 72;
    int visible_count = 0;

    for (int i = 0; i < band->entry_count; i++) {
        const BandPlanEntry* entry = &band->entries[i];

        // Filter by mode if needed
        if (!modeMatchesFilter(entry->modes, mode_filter)) continue;

        bool can_operate = canOperate(entry, user_lic);
        bool is_focused = (visible_count == bp_focused_row);

        createDetailRow(bp_content, entry, y_pos, can_operate, is_focused);
        y_pos += 32;
        visible_count++;
    }

    // Clamp focused row to valid range
    if (bp_focused_row >= visible_count && visible_count > 0) {
        bp_focused_row = visible_count - 1;
    }

    // Legend at bottom
    lv_obj_t* legend = lv_label_create(bp_content);
    lv_label_set_text(legend, "E=Extra  G=General  T=Technician  |  Cyan=You can operate");
    lv_obj_set_style_text_font(legend, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(legend, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_pos(legend, 5, y_pos + 5);
}

// ============================================
// Show/Exit Detail View
// ============================================

void showBandDetail(int band_index) {
    bp_focused_band = band_index;
    bp_focused_row = 0;
    bp_view_mode = BP_VIEW_DETAIL;

    updateBandPlansDetail();

    // Update footer
    if (bp_footer_text) {
        lv_label_set_text(bp_footer_text, LV_SYMBOL_UP LV_SYMBOL_DOWN " Scroll   " LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " Prev/Next Band   ESC Back");
    }
}

void exitBandDetail() {
    bp_view_mode = BP_VIEW_OVERVIEW;
    bp_focused_row = 0;

    updateBandPlansOverview();

    // Update footer
    if (bp_footer_text) {
        lv_label_set_text(bp_footer_text, LV_SYMBOL_UP LV_SYMBOL_DOWN " Navigate   ENTER Detail   L License   M Mode   ESC Back");
    }
}

// ============================================
// Key Handler
// ============================================

static void bp_key_handler(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    int band_count = getBandCount();

    if (bp_view_mode == BP_VIEW_OVERVIEW) {
        // Overview navigation - simple UP/DOWN for single-column list
        switch (key) {
            case LV_KEY_UP:
                if (bp_focused_band > 0) {
                    bp_focused_band--;
                    updateBandCardFocus();
                    // Scroll focused card into view
                    if (bp_band_cards[bp_focused_band]) {
                        lv_obj_scroll_to_view(bp_band_cards[bp_focused_band], LV_ANIM_ON);
                    }
                }
                lv_event_stop_processing(e);
                break;

            case LV_KEY_DOWN:
                if (bp_focused_band < band_count - 1) {
                    bp_focused_band++;
                    updateBandCardFocus();
                    // Scroll focused card into view
                    if (bp_band_cards[bp_focused_band]) {
                        lv_obj_scroll_to_view(bp_band_cards[bp_focused_band], LV_ANIM_ON);
                    }
                }
                lv_event_stop_processing(e);
                break;

            case LV_KEY_LEFT:
                // Cycle license class backward
                cycleBPLicensePrev();
                updateLicenseLabel();
                updateBandPlansOverview();
                lv_event_stop_processing(e);
                break;

            case LV_KEY_RIGHT:
                // Cycle mode filter forward
                cycleBPModeFilterNext();
                updateModeLabel();
                updateBandPlansOverview();
                lv_event_stop_processing(e);
                break;

            case LV_KEY_ENTER:
                showBandDetail(bp_focused_band);
                lv_event_stop_processing(e);
                break;

            case LV_KEY_ESC:
                // Let ESC fall through to global handler for back navigation
                // Don't stop processing - global handler will navigate back to Ham Tools
                break;

            case 'L':
            case 'l':
                // Quick license cycle forward
                cycleBPLicenseNext();
                updateLicenseLabel();
                updateBandPlansOverview();
                lv_event_stop_processing(e);
                break;

            case 'M':
            case 'm':
                // Quick mode filter cycle forward
                cycleBPModeFilterNext();
                updateModeLabel();
                updateBandPlansOverview();
                lv_event_stop_processing(e);
                break;

            default:
                // Unknown key - don't stop processing
                break;
        }
    } else {
        // Detail view navigation
        const BandDefinition* band = getBandByIndex(bp_focused_band);
        int entry_count = band ? band->entry_count : 0;

        switch (key) {
            case LV_KEY_UP:
                if (bp_focused_row > 0) {
                    bp_focused_row--;
                    updateBandPlansDetail();
                }
                lv_event_stop_processing(e);
                break;

            case LV_KEY_DOWN:
                if (bp_focused_row < entry_count - 1) {
                    bp_focused_row++;
                    updateBandPlansDetail();
                }
                lv_event_stop_processing(e);
                break;

            case LV_KEY_LEFT:
                // Previous band
                if (bp_focused_band > 0) {
                    bp_focused_band--;
                    bp_focused_row = 0;
                    updateBandPlansDetail();
                }
                lv_event_stop_processing(e);
                break;

            case LV_KEY_RIGHT:
                // Next band
                if (bp_focused_band < band_count - 1) {
                    bp_focused_band++;
                    bp_focused_row = 0;
                    updateBandPlansDetail();
                }
                lv_event_stop_processing(e);
                break;

            case LV_KEY_ESC:
                // ESC in detail view goes back to overview, NOT to parent menu
                exitBandDetail();
                lv_event_stop_processing(e);
                return;

            case LV_KEY_ENTER:
                // ENTER does nothing in detail view, but stop processing to prevent bubbling
                lv_event_stop_processing(e);
                break;

            case 'L':
            case 'l':
                // Quick license cycle
                cycleBPLicenseNext();
                updateLicenseLabel();
                updateBandPlansDetail();
                lv_event_stop_processing(e);
                break;

            case 'M':
            case 'm':
                // Quick mode filter cycle
                cycleBPModeFilterNext();
                updateModeLabel();
                updateBandPlansDetail();
                lv_event_stop_processing(e);
                break;

            default:
                // Unknown key - don't stop processing
                break;
        }
    }
    // NOTE: Don't add lv_event_stop_processing(e) here!
    // In overview mode, ESC must bubble up to global handler for back navigation
}

// ============================================
// Create Band Plans Screen
// ============================================

lv_obj_t* createBandPlansScreen() {
    // Clear navigation group first
    clearNavigationGroup();

    // Load settings
    loadBandPlanSettings();

    // Reset state
    bp_view_mode = BP_VIEW_OVERVIEW;
    bp_focused_band = 0;
    bp_focused_row = 0;

    // Create screen
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    bp_screen = screen;

    // ===== Header Bar =====
    lv_obj_t* header = lv_obj_create(screen);
    lv_obj_set_size(header, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_add_style(header, getStyleStatusBar(), 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "US BAND PLANS");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on right
    createCompactStatusBar(screen);

    // ===== Control Row (License + Mode Filter) =====
    lv_obj_t* control_row = lv_obj_create(screen);
    lv_obj_set_size(control_row, SCREEN_WIDTH - 20, 32);
    lv_obj_set_pos(control_row, 10, HEADER_HEIGHT + 2);
    lv_obj_set_style_bg_opa(control_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(control_row, 0, 0);
    lv_obj_set_style_pad_all(control_row, 0, 0);
    lv_obj_clear_flag(control_row, LV_OBJ_FLAG_SCROLLABLE);

    // License selector with "L" key hint
    lv_obj_t* lic_container = lv_obj_create(control_row);
    lv_obj_set_size(lic_container, 210, 28);
    lv_obj_set_pos(lic_container, 0, 0);
    lv_obj_set_style_bg_color(lic_container, getThemeColors()->bg_layer2, 0);
    lv_obj_set_style_bg_opa(lic_container, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(lic_container, 6, 0);
    lv_obj_set_style_border_width(lic_container, 1, 0);
    lv_obj_set_style_border_color(lic_container, getThemeColors()->card_border, 0);
    lv_obj_set_style_pad_all(lic_container, 0, 0);
    lv_obj_clear_flag(lic_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lic_key = lv_label_create(lic_container);
    lv_label_set_text(lic_key, "[L]");
    lv_obj_set_style_text_font(lic_key, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lic_key, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_pos(lic_key, 6, 6);

    lv_obj_t* lic_prefix = lv_label_create(lic_container);
    lv_label_set_text(lic_prefix, "License:");
    lv_obj_set_style_text_font(lic_prefix, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lic_prefix, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_pos(lic_prefix, 30, 6);

    bp_license_label = lv_label_create(lic_container);
    lv_obj_set_style_text_font(bp_license_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(bp_license_label, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_pos(bp_license_label, 90, 6);
    updateLicenseLabel();

    // Mode filter selector with "M" key hint
    lv_obj_t* mode_container = lv_obj_create(control_row);
    lv_obj_set_size(mode_container, 210, 28);
    lv_obj_set_pos(mode_container, 220, 0);
    lv_obj_set_style_bg_color(mode_container, getThemeColors()->bg_layer2, 0);
    lv_obj_set_style_bg_opa(mode_container, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(mode_container, 6, 0);
    lv_obj_set_style_border_width(mode_container, 1, 0);
    lv_obj_set_style_border_color(mode_container, getThemeColors()->card_border, 0);
    lv_obj_set_style_pad_all(mode_container, 0, 0);
    lv_obj_clear_flag(mode_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* mode_key = lv_label_create(mode_container);
    lv_label_set_text(mode_key, "[M]");
    lv_obj_set_style_text_font(mode_key, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(mode_key, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_pos(mode_key, 6, 6);

    lv_obj_t* mode_prefix = lv_label_create(mode_container);
    lv_label_set_text(mode_prefix, "Mode:");
    lv_obj_set_style_text_font(mode_prefix, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(mode_prefix, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_pos(mode_prefix, 32, 6);

    bp_mode_label = lv_label_create(mode_container);
    lv_obj_set_style_text_font(bp_mode_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(bp_mode_label, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_pos(bp_mode_label, 72, 6);
    updateModeLabel();

    // ===== Content Area (scrollable) =====
    bp_content = lv_obj_create(screen);
    lv_obj_set_size(bp_content, SCREEN_WIDTH - 20, 180);
    lv_obj_set_pos(bp_content, 10, HEADER_HEIGHT + 38);
    lv_obj_set_style_bg_opa(bp_content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bp_content, 0, 0);
    lv_obj_set_style_pad_all(bp_content, 0, 0);
    // Enable vertical scrolling for band list
    lv_obj_add_flag(bp_content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(bp_content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(bp_content, LV_SCROLLBAR_MODE_AUTO);

    // Populate content
    updateBandPlansOverview();

    // ===== Footer =====
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_color(footer, getThemeColors()->bg_deep, 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    bp_footer_text = lv_label_create(footer);
    lv_label_set_text(bp_footer_text, LV_SYMBOL_UP LV_SYMBOL_DOWN " Navigate   ENTER Detail   L License   M Mode   ESC Back");
    lv_obj_set_style_text_font(bp_footer_text, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(bp_footer_text, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_align(bp_footer_text, LV_ALIGN_CENTER, 0, 0);

    // ===== Navigation Widget for Key Input =====
    lv_obj_t* key_receiver = lv_obj_create(screen);
    lv_obj_set_size(key_receiver, 1, 1);
    lv_obj_set_pos(key_receiver, -10, -10);  // Off screen
    lv_obj_set_style_bg_opa(key_receiver, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(key_receiver, 0, 0);
    lv_obj_add_flag(key_receiver, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(key_receiver, bp_key_handler, LV_EVENT_KEY, NULL);
    addNavigableWidget(key_receiver);

    return screen;
}

// ============================================
// Start Band Plans Mode
// ============================================

void startBandPlans() {
    Serial.println("[BandPlans] Starting Band Plans mode");
}

// ============================================
// Cleanup
// ============================================

void cleanupBandPlans() {
    bp_screen = NULL;
    bp_content = NULL;
    bp_license_label = NULL;
    bp_mode_label = NULL;
    bp_footer_text = NULL;

    for (int i = 0; i < 10; i++) {
        bp_band_cards[i] = NULL;
    }
}

#endif // LV_BAND_PLANS_H
