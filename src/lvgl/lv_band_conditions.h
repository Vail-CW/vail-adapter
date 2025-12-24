/*
 * VAIL SUMMIT - Band Conditions Screen
 * Displays solar/propagation data from hamqsl.com in a tabbed UI
 */

#ifndef LV_BAND_CONDITIONS_H
#define LV_BAND_CONDITIONS_H

#include <lvgl.h>
#include <WiFi.h>
#include "lv_theme_summit.h"
#include "lv_widgets_summit.h"
#include "lv_screen_manager.h"
#include "../core/config.h"
#include "../network/band_conditions.h"

// ============================================
// Screen State
// ============================================

static lv_obj_t* band_cond_screen = NULL;
static lv_obj_t* band_cond_content = NULL;
static lv_obj_t* band_cond_loading_bar = NULL;
static lv_obj_t* band_cond_loading_label = NULL;
static lv_obj_t* band_cond_tab_hf = NULL;
static lv_obj_t* band_cond_tab_vhf = NULL;
static lv_obj_t* band_cond_updated_label = NULL;

static int band_cond_selected_tab = 0;  // 0 = HF, 1 = VHF
static bool band_cond_is_loading = false;
static unsigned long band_cond_fetch_time = 0;  // millis() when data was fetched
static lv_timer_t* band_cond_update_timer = NULL;  // Timer to update "X min ago"

// ============================================
// Forward Declarations
// ============================================

void updateBandConditionsContent();
void refreshBandConditions();
void createHFTabContent(lv_obj_t* parent);
void createVHFTabContent(lv_obj_t* parent);
void updateTimestampLabel();
void stopBandConditionsTimer();

// ============================================
// Timer callback to update "X min ago" label
// ============================================

static void band_cond_timer_cb(lv_timer_t* timer) {
    updateTimestampLabel();
}

void updateTimestampLabel() {
    if (!band_cond_updated_label) return;

    if (band_cond_fetch_time == 0) {
        lv_label_set_text(band_cond_updated_label, "Press R to refresh");
        return;
    }

    unsigned long elapsed_ms = millis() - band_cond_fetch_time;
    unsigned long elapsed_min = elapsed_ms / 60000;

    char buf[64];
    if (elapsed_min == 0) {
        snprintf(buf, sizeof(buf), "Updated: just now");
    } else if (elapsed_min == 1) {
        snprintf(buf, sizeof(buf), "Updated: 1 min ago");
    } else if (elapsed_min < 60) {
        snprintf(buf, sizeof(buf), "Updated: %lu min ago", elapsed_min);
    } else {
        unsigned long elapsed_hr = elapsed_min / 60;
        if (elapsed_hr == 1) {
            snprintf(buf, sizeof(buf), "Updated: 1 hr ago");
        } else {
            snprintf(buf, sizeof(buf), "Updated: %lu hrs ago", elapsed_hr);
        }
    }

    lv_label_set_text(band_cond_updated_label, buf);
}

void stopBandConditionsTimer() {
    if (band_cond_update_timer) {
        lv_timer_del(band_cond_update_timer);
        band_cond_update_timer = NULL;
    }
}

// ============================================
// Tab Styling
// ============================================

void updateTabStyles() {
    if (!band_cond_tab_hf || !band_cond_tab_vhf) return;

    // HF Tab
    if (band_cond_selected_tab == 0) {
        lv_obj_set_style_bg_color(band_cond_tab_hf, LV_COLOR_ACCENT_CYAN, 0);
        lv_obj_set_style_text_color(lv_obj_get_child(band_cond_tab_hf, 0), lv_color_hex(0x1A1A2E), 0);
    } else {
        lv_obj_set_style_bg_color(band_cond_tab_hf, lv_color_hex(0x1A2A2A), 0);
        lv_obj_set_style_text_color(lv_obj_get_child(band_cond_tab_hf, 0), LV_COLOR_TEXT_SECONDARY, 0);
    }

    // VHF Tab
    if (band_cond_selected_tab == 1) {
        lv_obj_set_style_bg_color(band_cond_tab_vhf, LV_COLOR_ACCENT_CYAN, 0);
        lv_obj_set_style_text_color(lv_obj_get_child(band_cond_tab_vhf, 0), lv_color_hex(0x1A1A2E), 0);
    } else {
        lv_obj_set_style_bg_color(band_cond_tab_vhf, lv_color_hex(0x1A2A2A), 0);
        lv_obj_set_style_text_color(lv_obj_get_child(band_cond_tab_vhf, 0), LV_COLOR_TEXT_SECONDARY, 0);
    }
}

// ============================================
// Color Helper for Band Conditions
// ============================================

lv_color_t getBandColor(BandCondition cond) {
    switch (cond) {
        case BAND_GOOD:    return LV_COLOR_SUCCESS;
        case BAND_FAIR:    return LV_COLOR_WARNING;
        case BAND_POOR:    return lv_color_hex(0xFF8C00);  // Orange
        case BAND_CLOSED:  return LV_COLOR_ERROR;
        default:           return LV_COLOR_TEXT_TERTIARY;
    }
}

// ============================================
// Event Handlers
// ============================================

static void band_cond_key_handler(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // 'R' or 'r' for refresh
    if (key == 'R' || key == 'r') {
        if (!band_cond_is_loading) {
            refreshBandConditions();
        }
        lv_event_stop_processing(e);
        return;
    }

    // Tab key or Left/Right arrows to switch tabs
    if (key == '\t' || key == LV_KEY_LEFT || key == LV_KEY_RIGHT) {
        band_cond_selected_tab = 1 - band_cond_selected_tab;  // Toggle 0 <-> 1
        updateTabStyles();
        updateBandConditionsContent();
        lv_event_stop_processing(e);
        return;
    }
}

static void band_cond_tab_click_handler(lv_event_t* e) {
    int tab = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    if (tab >= 0 && tab < 2 && tab != band_cond_selected_tab) {
        band_cond_selected_tab = tab;
        updateTabStyles();
        updateBandConditionsContent();
    }
}

// ============================================
// Create Band Condition Cell
// ============================================

lv_obj_t* createConditionCell(lv_obj_t* parent, BandCondition cond, int width) {
    lv_obj_t* cell = lv_obj_create(parent);
    lv_obj_set_size(cell, width, 22);
    lv_obj_set_style_bg_color(cell, getBandColor(cond), 0);
    lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(cell, 4, 0);
    lv_obj_set_style_border_width(cell, 0, 0);
    lv_obj_set_style_pad_all(cell, 0, 0);
    lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lbl = lv_label_create(cell);
    lv_label_set_text(lbl, getBandConditionText(cond));
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x000000), 0);  // Black text on colored bg
    lv_obj_center(lbl);

    return cell;
}

// ============================================
// HF Tab Content
// ============================================

void createHFTabContent(lv_obj_t* parent) {
    // Main container - horizontal layout
    lv_obj_t* main_container = lv_obj_create(parent);
    lv_obj_set_size(main_container, 460, 190);
    lv_obj_set_style_bg_opa(main_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(main_container, 0, 0);
    lv_obj_set_style_pad_all(main_container, 0, 0);
    lv_obj_set_layout(main_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(main_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(main_container, 10, 0);
    lv_obj_clear_flag(main_container, LV_OBJ_FLAG_SCROLLABLE);

    // ===== Solar Data Card (left side) =====
    lv_obj_t* solar_card = lv_obj_create(main_container);
    lv_obj_set_size(solar_card, 125, 180);  // Narrower to give band card more room
    applyCardStyle(solar_card);
    lv_obj_set_layout(solar_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(solar_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(solar_card, 8, 0);
    lv_obj_set_style_pad_row(solar_card, 4, 0);
    lv_obj_clear_flag(solar_card, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* solar_title = lv_label_create(solar_card);
    lv_label_set_text(solar_title, "Solar Data");
    lv_obj_set_style_text_font(solar_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(solar_title, LV_COLOR_ACCENT_CYAN, 0);

    // Solar Flux
    char buf[32];
    snprintf(buf, sizeof(buf), "SFI: %d", bandConditionsData.solar.solarFlux);
    lv_obj_t* sfi_lbl = lv_label_create(solar_card);
    lv_label_set_text(sfi_lbl, buf);
    lv_obj_set_style_text_font(sfi_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sfi_lbl, LV_COLOR_TEXT_PRIMARY, 0);

    // A and K indices on same line
    snprintf(buf, sizeof(buf), "A: %d   K: %d", bandConditionsData.solar.aIndex, bandConditionsData.solar.kIndex);
    lv_obj_t* ak_lbl = lv_label_create(solar_card);
    lv_label_set_text(ak_lbl, buf);
    lv_obj_set_style_text_font(ak_lbl, &lv_font_montserrat_14, 0);
    // Color K index
    lv_obj_set_style_text_color(ak_lbl, lv_color_hex(getKIndexColorHex(bandConditionsData.solar.kIndex)), 0);

    // X-Ray
    snprintf(buf, sizeof(buf), "X-Ray: %s", bandConditionsData.solar.xray);
    lv_obj_t* xray_lbl = lv_label_create(solar_card);
    lv_label_set_text(xray_lbl, buf);
    lv_obj_set_style_text_font(xray_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(xray_lbl, LV_COLOR_TEXT_SECONDARY, 0);

    // Sunspots
    snprintf(buf, sizeof(buf), "Spots: %d", bandConditionsData.solar.sunspots);
    lv_obj_t* spots_lbl = lv_label_create(solar_card);
    lv_label_set_text(spots_lbl, buf);
    lv_obj_set_style_text_font(spots_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(spots_lbl, LV_COLOR_TEXT_SECONDARY, 0);

    // Geomag Field
    snprintf(buf, sizeof(buf), "Geo: %s", bandConditionsData.solar.geomagField);
    lv_obj_t* geo_lbl = lv_label_create(solar_card);
    lv_label_set_text(geo_lbl, buf);
    lv_obj_set_style_text_font(geo_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(geo_lbl, lv_color_hex(getGeomagColorHex(bandConditionsData.solar.geomagField)), 0);

    // Signal Noise
    snprintf(buf, sizeof(buf), "Noise: %s", bandConditionsData.solar.signalNoise);
    lv_obj_t* noise_lbl = lv_label_create(solar_card);
    lv_label_set_text(noise_lbl, buf);
    lv_obj_set_style_text_font(noise_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(noise_lbl, LV_COLOR_TEXT_SECONDARY, 0);

    // ===== Band Conditions Grid (right side) =====
    lv_obj_t* band_card = lv_obj_create(main_container);
    lv_obj_set_size(band_card, 325, 180);  // Wider to prevent clipping
    applyCardStyle(band_card);
    lv_obj_set_style_pad_all(band_card, 8, 0);
    lv_obj_clear_flag(band_card, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* band_title = lv_label_create(band_card);
    lv_label_set_text(band_title, "HF Band Conditions");
    lv_obj_set_style_text_font(band_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(band_title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_pos(band_title, 0, 0);

    // Column headers
    lv_obj_t* hdr_band = lv_label_create(band_card);
    lv_label_set_text(hdr_band, "BAND");
    lv_obj_set_style_text_font(hdr_band, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hdr_band, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_pos(hdr_band, 5, 22);

    lv_obj_t* hdr_day = lv_label_create(band_card);
    lv_label_set_text(hdr_day, "DAY");
    lv_obj_set_style_text_font(hdr_day, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hdr_day, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_pos(hdr_day, 60, 22);

    lv_obj_t* hdr_night = lv_label_create(band_card);
    lv_label_set_text(hdr_night, "NIGHT");
    lv_obj_set_style_text_font(hdr_night, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hdr_night, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_pos(hdr_night, 115, 22);

    // Second column headers (shifted right to use extra width)
    lv_obj_t* hdr_band2 = lv_label_create(band_card);
    lv_label_set_text(hdr_band2, "BAND");
    lv_obj_set_style_text_font(hdr_band2, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hdr_band2, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_pos(hdr_band2, 168, 22);

    lv_obj_t* hdr_day2 = lv_label_create(band_card);
    lv_label_set_text(hdr_day2, "DAY");
    lv_obj_set_style_text_font(hdr_day2, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hdr_day2, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_pos(hdr_day2, 215, 22);

    lv_obj_t* hdr_night2 = lv_label_create(band_card);
    lv_label_set_text(hdr_night2, "NIGHT");
    lv_obj_set_style_text_font(hdr_night2, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hdr_night2, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_pos(hdr_night2, 263, 22);

    // Band data - Column 1 (80m, 40m, 30m, 20m)
    const char* bands_col1[] = {"80m", "40m", "30m", "20m"};
    BandCondition day_col1[] = {bandConditionsData.hf_80m_40m.day, bandConditionsData.hf_80m_40m.day,
                                 bandConditionsData.hf_30m_20m.day, bandConditionsData.hf_30m_20m.day};
    BandCondition night_col1[] = {bandConditionsData.hf_80m_40m.night, bandConditionsData.hf_80m_40m.night,
                                   bandConditionsData.hf_30m_20m.night, bandConditionsData.hf_30m_20m.night};

    int y_start = 40;
    int row_height = 28;

    for (int i = 0; i < 4; i++) {
        int y = y_start + i * row_height;

        // Band label
        lv_obj_t* band_lbl = lv_label_create(band_card);
        lv_label_set_text(band_lbl, bands_col1[i]);
        lv_obj_set_style_text_font(band_lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(band_lbl, LV_COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_pos(band_lbl, 8, y);

        // Day condition cell
        lv_obj_t* day_cell = createConditionCell(band_card, day_col1[i], 48);
        lv_obj_set_pos(day_cell, 50, y - 2);

        // Night condition cell
        lv_obj_t* night_cell = createConditionCell(band_card, night_col1[i], 48);
        lv_obj_set_pos(night_cell, 105, y - 2);
    }

    // Band data - Column 2 (17m, 15m, 12m, 10m)
    const char* bands_col2[] = {"17m", "15m", "12m", "10m"};
    BandCondition day_col2[] = {bandConditionsData.hf_17m_15m.day, bandConditionsData.hf_17m_15m.day,
                                 bandConditionsData.hf_12m_10m.day, bandConditionsData.hf_12m_10m.day};
    BandCondition night_col2[] = {bandConditionsData.hf_17m_15m.night, bandConditionsData.hf_17m_15m.night,
                                   bandConditionsData.hf_12m_10m.night, bandConditionsData.hf_12m_10m.night};

    for (int i = 0; i < 4; i++) {
        int y = y_start + i * row_height;

        // Band label
        lv_obj_t* band_lbl = lv_label_create(band_card);
        lv_label_set_text(band_lbl, bands_col2[i]);
        lv_obj_set_style_text_font(band_lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(band_lbl, LV_COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_pos(band_lbl, 168, y);

        // Day condition cell (same width as column 1)
        lv_obj_t* day_cell = createConditionCell(band_card, day_col2[i], 48);
        lv_obj_set_pos(day_cell, 208, y - 2);

        // Night condition cell (same width as column 1)
        lv_obj_t* night_cell = createConditionCell(band_card, night_col2[i], 48);
        lv_obj_set_pos(night_cell, 260, y - 2);
    }
}

// ============================================
// VHF Tab Content
// ============================================

void createVHFTabContent(lv_obj_t* parent) {
    lv_obj_t* vhf_card = lv_obj_create(parent);
    lv_obj_set_size(vhf_card, 460, 190);
    applyCardStyle(vhf_card);
    lv_obj_set_style_pad_all(vhf_card, 10, 0);
    lv_obj_clear_flag(vhf_card, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* title = lv_label_create(vhf_card);
    lv_label_set_text(title, "VHF Conditions");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_pos(title, 0, 0);

    // Column headers
    lv_obj_t* hdr_phenom = lv_label_create(vhf_card);
    lv_label_set_text(hdr_phenom, "PHENOMENON");
    lv_obj_set_style_text_font(hdr_phenom, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hdr_phenom, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_pos(hdr_phenom, 5, 28);

    lv_obj_t* hdr_region = lv_label_create(vhf_card);
    lv_label_set_text(hdr_region, "REGION");
    lv_obj_set_style_text_font(hdr_region, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hdr_region, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_pos(hdr_region, 180, 28);

    lv_obj_t* hdr_status = lv_label_create(vhf_card);
    lv_label_set_text(hdr_status, "STATUS");
    lv_obj_set_style_text_font(hdr_status, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hdr_status, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_pos(hdr_status, 380, 28);

    // VHF Phenomena rows
    int y_start = 48;
    int row_height = 24;
    int max_rows = 6;  // Limit to fit screen

    int count = min(bandConditionsData.vhfCount, max_rows);

    if (count == 0) {
        lv_obj_t* no_data = lv_label_create(vhf_card);
        lv_label_set_text(no_data, "No VHF data available");
        lv_obj_set_style_text_font(no_data, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(no_data, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_pos(no_data, 5, y_start);
    } else {
        for (int i = 0; i < count; i++) {
            int y = y_start + i * row_height;

            // Phenomenon name
            lv_obj_t* name_lbl = lv_label_create(vhf_card);
            lv_label_set_text(name_lbl, bandConditionsData.vhf[i].name);
            lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_12, 0);
            lv_obj_set_style_text_color(name_lbl, LV_COLOR_TEXT_PRIMARY, 0);
            lv_obj_set_pos(name_lbl, 5, y);

            // Region/Location
            lv_obj_t* region_lbl = lv_label_create(vhf_card);
            lv_label_set_text(region_lbl, bandConditionsData.vhf[i].location);
            lv_obj_set_style_text_font(region_lbl, &lv_font_montserrat_12, 0);
            lv_obj_set_style_text_color(region_lbl, LV_COLOR_TEXT_SECONDARY, 0);
            lv_obj_set_pos(region_lbl, 180, y);

            // Status (colored)
            lv_obj_t* status_lbl = lv_label_create(vhf_card);
            if (bandConditionsData.vhf[i].closed) {
                lv_label_set_text(status_lbl, "Closed");
                lv_obj_set_style_text_color(status_lbl, LV_COLOR_ERROR, 0);
            } else {
                lv_label_set_text(status_lbl, "Open");
                lv_obj_set_style_text_color(status_lbl, LV_COLOR_SUCCESS, 0);
            }
            lv_obj_set_style_text_font(status_lbl, &lv_font_montserrat_12, 0);
            lv_obj_set_pos(status_lbl, 380, y);
        }
    }
}

// ============================================
// Update Content Based on Tab
// ============================================

void updateBandConditionsContent() {
    if (!band_cond_content) return;

    // Clear existing content
    lv_obj_clean(band_cond_content);

    if (!bandConditionsData.valid) {
        // Show "No data" message
        lv_obj_t* no_data = lv_label_create(band_cond_content);
        lv_label_set_text(no_data, "No data available.\nPress R to refresh.");
        lv_obj_set_style_text_font(no_data, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(no_data, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_center(no_data);
        return;
    }

    // Create content based on selected tab
    if (band_cond_selected_tab == 0) {
        createHFTabContent(band_cond_content);
    } else {
        createVHFTabContent(band_cond_content);
    }

    // Update timestamp label (shows "X min ago")
    updateTimestampLabel();
}

// ============================================
// Show Loading State
// ============================================

void showLoadingState(bool show) {
    band_cond_is_loading = show;

    if (band_cond_loading_bar) {
        if (show) {
            lv_obj_clear_flag(band_cond_loading_bar, LV_OBJ_FLAG_HIDDEN);
            lv_bar_set_value(band_cond_loading_bar, 0, LV_ANIM_OFF);
        } else {
            lv_obj_add_flag(band_cond_loading_bar, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (band_cond_loading_label) {
        if (show) {
            lv_obj_clear_flag(band_cond_loading_label, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(band_cond_loading_label, "Fetching band conditions...");
        } else {
            lv_obj_add_flag(band_cond_loading_label, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

// ============================================
// Refresh Band Conditions
// ============================================

void refreshBandConditions() {
    if (band_cond_is_loading) return;

    // Check WiFi
    if (WiFi.status() != WL_CONNECTED) {
        if (band_cond_loading_label) {
            lv_obj_clear_flag(band_cond_loading_label, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(band_cond_loading_label, "WiFi not connected!");
            lv_obj_set_style_text_color(band_cond_loading_label, LV_COLOR_ERROR, 0);
        }
        return;
    }

    showLoadingState(true);

    // Animate progress bar (simulated since fetch is blocking)
    if (band_cond_loading_bar) {
        lv_bar_set_value(band_cond_loading_bar, 30, LV_ANIM_ON);
    }

    // Force LVGL to update display before blocking fetch
    lv_timer_handler();

    // Fetch data (blocking)
    bool success = fetchBandConditions(bandConditionsData);

    showLoadingState(false);

    if (success) {
        if (band_cond_loading_label) {
            lv_obj_set_style_text_color(band_cond_loading_label, LV_COLOR_TEXT_SECONDARY, 0);
        }
        // Record fetch time and update timestamp
        band_cond_fetch_time = millis();
        updateTimestampLabel();
        updateBandConditionsContent();
    } else {
        if (band_cond_loading_label) {
            lv_obj_clear_flag(band_cond_loading_label, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(band_cond_loading_label, "Failed to fetch data. Press R to retry.");
            lv_obj_set_style_text_color(band_cond_loading_label, LV_COLOR_WARNING, 0);
        }
    }
}

// ============================================
// Create Band Conditions Screen
// ============================================

lv_obj_t* createBandConditionsScreen() {
    // Clear navigation group first
    clearNavigationGroup();

    // Create screen
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    band_cond_screen = screen;

    // Reset state
    band_cond_selected_tab = 0;
    band_cond_is_loading = false;

    // ===== Header Bar =====
    lv_obj_t* header = lv_obj_create(screen);
    lv_obj_set_size(header, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_add_style(header, getStyleStatusBar(), 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    // Title (shortened to fit)
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "CONDITIONS");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Tab buttons in header (center-right)
    band_cond_tab_hf = lv_obj_create(header);
    lv_obj_set_size(band_cond_tab_hf, 50, 28);
    lv_obj_align(band_cond_tab_hf, LV_ALIGN_CENTER, 40, 0);
    lv_obj_set_style_radius(band_cond_tab_hf, 6, 0);
    lv_obj_set_style_border_width(band_cond_tab_hf, 1, 0);
    lv_obj_set_style_border_color(band_cond_tab_hf, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_pad_all(band_cond_tab_hf, 0, 0);
    lv_obj_clear_flag(band_cond_tab_hf, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(band_cond_tab_hf, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_user_data(band_cond_tab_hf, (void*)0);
    lv_obj_add_event_cb(band_cond_tab_hf, band_cond_tab_click_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t* hf_lbl = lv_label_create(band_cond_tab_hf);
    lv_label_set_text(hf_lbl, "HF");
    lv_obj_set_style_text_font(hf_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(hf_lbl);

    band_cond_tab_vhf = lv_obj_create(header);
    lv_obj_set_size(band_cond_tab_vhf, 50, 28);
    lv_obj_align(band_cond_tab_vhf, LV_ALIGN_CENTER, 95, 0);
    lv_obj_set_style_radius(band_cond_tab_vhf, 6, 0);
    lv_obj_set_style_border_width(band_cond_tab_vhf, 1, 0);
    lv_obj_set_style_border_color(band_cond_tab_vhf, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_pad_all(band_cond_tab_vhf, 0, 0);
    lv_obj_clear_flag(band_cond_tab_vhf, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(band_cond_tab_vhf, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_user_data(band_cond_tab_vhf, (void*)1);
    lv_obj_add_event_cb(band_cond_tab_vhf, band_cond_tab_click_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t* vhf_lbl = lv_label_create(band_cond_tab_vhf);
    lv_label_set_text(vhf_lbl, "VHF");
    lv_obj_set_style_text_font(vhf_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(vhf_lbl);

    // Apply initial tab styles
    updateTabStyles();

    // Status bar (WiFi + battery) on right
    createCompactStatusBar(screen);

    // ===== Content Area =====
    band_cond_content = lv_obj_create(screen);
    lv_obj_set_size(band_cond_content, SCREEN_WIDTH - 20, 195);
    lv_obj_set_pos(band_cond_content, 10, HEADER_HEIGHT + 5);
    lv_obj_set_style_bg_opa(band_cond_content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(band_cond_content, 0, 0);
    lv_obj_set_style_pad_all(band_cond_content, 0, 0);
    lv_obj_clear_flag(band_cond_content, LV_OBJ_FLAG_SCROLLABLE);

    // Loading bar (hidden initially)
    band_cond_loading_bar = lv_bar_create(screen);
    lv_obj_set_size(band_cond_loading_bar, 200, 10);
    lv_obj_align(band_cond_loading_bar, LV_ALIGN_CENTER, 0, -20);
    lv_bar_set_range(band_cond_loading_bar, 0, 100);
    lv_bar_set_value(band_cond_loading_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(band_cond_loading_bar, lv_color_hex(0x2A2A3E), 0);
    lv_obj_set_style_bg_color(band_cond_loading_bar, LV_COLOR_ACCENT_CYAN, LV_PART_INDICATOR);
    lv_obj_add_flag(band_cond_loading_bar, LV_OBJ_FLAG_HIDDEN);

    // Loading label (hidden initially)
    band_cond_loading_label = lv_label_create(screen);
    lv_label_set_text(band_cond_loading_label, "");
    lv_obj_set_style_text_font(band_cond_loading_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(band_cond_loading_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(band_cond_loading_label, LV_ALIGN_CENTER, 0, 10);
    lv_obj_add_flag(band_cond_loading_label, LV_OBJ_FLAG_HIDDEN);

    // ===== Updated Timestamp =====
    band_cond_updated_label = lv_label_create(screen);
    lv_obj_set_style_text_font(band_cond_updated_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(band_cond_updated_label, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_pos(band_cond_updated_label, 15, SCREEN_HEIGHT - FOOTER_HEIGHT - 18);
    updateTimestampLabel();  // Set initial text based on fetch state

    // ===== Footer =====
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_color(footer, lv_color_hex(0x0A0A14), 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* footer_text = lv_label_create(footer);
    lv_label_set_text(footer_text, "R: Refresh    " LV_SYMBOL_LEFT LV_SYMBOL_RIGHT ": Switch HF/VHF    ESC: Back");
    lv_obj_set_style_text_font(footer_text, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(footer_text, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_align(footer_text, LV_ALIGN_CENTER, 0, 0);

    // ===== Navigation Widget for Key Input =====
    // Create an invisible focusable object to receive key events
    lv_obj_t* key_receiver = lv_obj_create(screen);
    lv_obj_set_size(key_receiver, 1, 1);
    lv_obj_set_pos(key_receiver, -10, -10);  // Off screen
    lv_obj_set_style_bg_opa(key_receiver, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(key_receiver, 0, 0);
    lv_obj_add_flag(key_receiver, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(key_receiver, band_cond_key_handler, LV_EVENT_KEY, NULL);
    addNavigableWidget(key_receiver);

    // Initial content (will show "No data" until refreshed)
    updateBandConditionsContent();

    return screen;
}

// ============================================
// Start Band Conditions Mode
// ============================================

void startBandConditions(LGFX& display) {
    Serial.println("[BandConditions] Starting Band Conditions mode");

    // Start timer to update "X min ago" label every minute
    stopBandConditionsTimer();  // Clean up any existing timer
    band_cond_update_timer = lv_timer_create(band_cond_timer_cb, 60000, NULL);  // 60 seconds

    // Auto-refresh on entry if WiFi connected and no data yet
    if (WiFi.status() == WL_CONNECTED && !bandConditionsData.valid) {
        // Small delay to let screen render first
        delay(100);
        refreshBandConditions();
    }
}

/*
 * Clean up when leaving Band Conditions mode
 * Should be called from mode exit handler
 */
void cleanupBandConditions() {
    stopBandConditionsTimer();
    band_cond_screen = NULL;
    band_cond_content = NULL;
    band_cond_loading_bar = NULL;
    band_cond_loading_label = NULL;
    band_cond_tab_hf = NULL;
    band_cond_tab_vhf = NULL;
    band_cond_updated_label = NULL;
}

#endif // LV_BAND_CONDITIONS_H
