/*
 * VAIL SUMMIT - POTA Screens
 * Parks On The Air feature screens
 */

#ifndef LV_POTA_SCREENS_H
#define LV_POTA_SCREENS_H

#include <lvgl.h>
#include <WiFi.h>
#include "lv_theme_summit.h"
#include "lv_widgets_summit.h"
#include "lv_screen_manager.h"
#include "../core/config.h"
#include "../network/pota_spots.h"
#include "../qso/qso_logger.h"

// Forward declarations
extern void onLVGLMenuSelect(int target_mode);
extern int getCurrentModeAsInt();
extern void setCurrentModeFromInt(int mode);

// Mode constants (must match lv_mode_integration.h)
#define POTA_MODE_MENU              62
#define POTA_MODE_ACTIVE_SPOTS      63
#define POTA_MODE_SPOT_DETAIL       64
#define POTA_MODE_FILTERS           65
#define POTA_MODE_ACTIVATE          66

// ============================================
// Screen State
// ============================================

static lv_obj_t* pota_screen = NULL;
static lv_obj_t* pota_spots_table = NULL;
static lv_obj_t* pota_loading_bar = NULL;
static lv_obj_t* pota_loading_label = NULL;      // Loading container
static lv_obj_t* pota_loading_text = NULL;       // Loading text label inside container
static lv_obj_t* pota_updated_label = NULL;
static lv_obj_t* pota_filter_label = NULL;
static lv_obj_t* pota_count_label = NULL;

// Detail screen state
static lv_obj_t* pota_detail_content = NULL;
static lv_obj_t* pota_detail_tabs[4] = {NULL};
static int pota_detail_selected_tab = 0;

// Filter screen state
static int pota_filter_band_idx = 0;
static int pota_filter_mode_idx = 0;
static int pota_filter_region_idx = 0;
static int pota_filter_focus_row = 0;  // 0=band, 1=mode, 2=region, 3=apply, 4=clear

// Spots list state
static int pota_spots_scroll_pos = 0;
static int pota_spots_selected_row = 0;
static bool pota_is_loading = false;

// Filtered spots indices (dynamically allocated alongside spots cache)
static int* filteredSpotIndices = NULL;
static int filteredSpotCount = 0;
static bool filteredIndicesInitialized = false;

// Auto-refresh timer
static lv_timer_t* pota_refresh_timer = NULL;
static lv_timer_t* pota_timestamp_timer = NULL;
static lv_timer_t* pota_autoload_timer = NULL;

// ============================================
// Forward Declarations
// ============================================

lv_obj_t* createPOTAMenuScreen();
lv_obj_t* createPOTAActiveSpotsScreen();
lv_obj_t* createPOTASpotDetailScreen();
lv_obj_t* createPOTAFilterScreen();
void refreshPOTASpotsDisplay();
void updatePOTATimestampLabel();
void cleanupPOTAScreen();

// ============================================
// Timer Callbacks
// ============================================

static void pota_auto_refresh_cb(lv_timer_t* timer) {
    if (getCurrentModeAsInt() == POTA_MODE_ACTIVE_SPOTS && !potaSpotsCache.fetching) {
        // Trigger refresh
        fetchActiveSpots(potaSpotsCache);
        refreshPOTASpotsDisplay();
    }
}

static void pota_timestamp_cb(lv_timer_t* timer) {
    updatePOTATimestampLabel();
}

void startPOTATimers() {
    // Stop existing timers only (don't clear screen pointers)
    if (pota_refresh_timer) {
        lv_timer_del(pota_refresh_timer);
        pota_refresh_timer = NULL;
    }
    if (pota_timestamp_timer) {
        lv_timer_del(pota_timestamp_timer);
        pota_timestamp_timer = NULL;
    }

    // Start auto-refresh timer (60 seconds)
    pota_refresh_timer = lv_timer_create(pota_auto_refresh_cb, POTA_REFRESH_INTERVAL, NULL);

    // Start timestamp update timer (1 minute)
    pota_timestamp_timer = lv_timer_create(pota_timestamp_cb, 60000, NULL);
}

void cleanupPOTAScreen() {
    if (pota_refresh_timer) {
        lv_timer_del(pota_refresh_timer);
        pota_refresh_timer = NULL;
    }
    if (pota_timestamp_timer) {
        lv_timer_del(pota_timestamp_timer);
        pota_timestamp_timer = NULL;
    }
    if (pota_autoload_timer) {
        lv_timer_del(pota_autoload_timer);
        pota_autoload_timer = NULL;
    }
    pota_screen = NULL;
    pota_spots_table = NULL;
    pota_loading_bar = NULL;
    pota_loading_label = NULL;
    pota_loading_text = NULL;
    pota_updated_label = NULL;
    pota_filter_label = NULL;
    pota_count_label = NULL;
    pota_detail_content = NULL;
    for (int i = 0; i < 4; i++) pota_detail_tabs[i] = NULL;
}

// ============================================
// Helper Functions
// ============================================

void updatePOTATimestampLabel() {
    if (!pota_updated_label) {
        Serial.println("[POTA] updatePOTATimestampLabel: label is NULL");
        return;
    }

    // Verify label is still valid (parent exists)
    if (!lv_obj_is_valid(pota_updated_label)) {
        Serial.println("[POTA] updatePOTATimestampLabel: label invalid");
        pota_updated_label = NULL;
        return;
    }

    if (potaSpotsCache.fetchTime == 0 || !potaSpotsCache.valid) {
        lv_label_set_text(pota_updated_label, "Press R to refresh");
        return;
    }

    int mins = getCacheAgeMinutes();
    char buf[32];
    if (mins < 0) {
        snprintf(buf, sizeof(buf), "Press R to refresh");
    } else if (mins == 0) {
        snprintf(buf, sizeof(buf), "Updated: just now");
    } else if (mins == 1) {
        snprintf(buf, sizeof(buf), "Updated: 1 min ago");
    } else if (mins < 60) {
        snprintf(buf, sizeof(buf), "Updated: %d min ago", mins);
    } else {
        snprintf(buf, sizeof(buf), "Updated: %d hr ago", mins / 60);
    }
    lv_label_set_text(pota_updated_label, buf);
}

// Initialize filtered indices array (call after spots cache is initialized)
bool initFilteredIndices() {
    if (filteredIndicesInitialized && filteredSpotIndices != NULL) {
        return true;
    }

    size_t size = sizeof(int) * MAX_POTA_SPOTS;
    filteredSpotIndices = (int*)malloc(size);
    if (filteredSpotIndices) {
        memset(filteredSpotIndices, 0, size);
        filteredIndicesInitialized = true;
        return true;
    }

    Serial.println("[POTA] ERROR: Failed to allocate filtered indices!");
    return false;
}

void updateFilteredSpots() {
    // Safety check - ensure cache is valid
    if (!potaSpotsCache.spots || !potaSpotsCache.initialized) {
        filteredSpotCount = 0;
        return;
    }

    // Initialize filtered indices if needed
    if (!filteredIndicesInitialized || !filteredSpotIndices) {
        if (!initFilteredIndices()) {
            filteredSpotCount = 0;
            return;
        }
    }

    if (potaSpotFilter.active) {
        filteredSpotCount = filterSpots(potaSpotsCache, potaSpotFilter,
                                         filteredSpotIndices, potaSpotsCache.maxSpots);
    } else {
        // No filter - show all spots
        filteredSpotCount = potaSpotsCache.count;
        for (int i = 0; i < filteredSpotCount; i++) {
            filteredSpotIndices[i] = i;
        }
    }
}

void updateFilterLabel() {
    if (!pota_filter_label) {
        return;
    }

    if (!lv_obj_is_valid(pota_filter_label)) {
        pota_filter_label = NULL;
        return;
    }

    if (!potaSpotFilter.active) {
        lv_obj_add_flag(pota_filter_label, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    char buf[64];
    snprintf(buf, sizeof(buf), "BAND: %s  MODE: %s  REGION: %s",
             potaSpotFilter.band, potaSpotFilter.mode, potaSpotFilter.region);
    lv_label_set_text(pota_filter_label, buf);
    lv_obj_clear_flag(pota_filter_label, LV_OBJ_FLAG_HIDDEN);
}

void updateCountLabel() {
    if (!pota_count_label) {
        Serial.println("[POTA] updateCountLabel: label is NULL");
        return;
    }

    if (!lv_obj_is_valid(pota_count_label)) {
        Serial.println("[POTA] updateCountLabel: label invalid");
        pota_count_label = NULL;
        return;
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "SPOTS (%d)", filteredSpotCount);
    lv_label_set_text(pota_count_label, buf);
}

// ============================================
// POTA Menu Screen
// ============================================

static void pota_menu_key_handler(lv_event_t* e);

static const struct {
    const char* icon;
    const char* title;
    int target_mode;
} potaMenuItems[] = {
    {LV_SYMBOL_GPS, "Active Spots", POTA_MODE_ACTIVE_SPOTS},
    {LV_SYMBOL_HOME, "Activate a Park", POTA_MODE_ACTIVATE}
};
#define POTA_MENU_COUNT 2

static lv_obj_t* pota_menu_buttons[2] = {NULL};

lv_obj_t* createPOTAMenuScreen() {
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    pota_screen = screen;

    // Header
    lv_obj_t* header = lv_obj_create(screen);
    lv_obj_set_size(header, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_add_style(header, getStyleStatusBar(), 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "POTA");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    createCompactStatusBar(screen);

    // Content area with menu buttons
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, SCREEN_WIDTH - 20, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 10);
    lv_obj_set_pos(content, 10, HEADER_HEIGHT + 5);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(content, 20, 0);
    lv_obj_set_style_pad_row(content, 15, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Create menu buttons
    for (int i = 0; i < POTA_MENU_COUNT; i++) {
        lv_obj_t* btn = lv_obj_create(content);
        lv_obj_set_size(btn, 200, 100);
        applyCardStyle(btn);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_user_data(btn, (void*)(intptr_t)potaMenuItems[i].target_mode);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        // Focus style
        lv_obj_set_style_border_color(btn, LV_COLOR_ACCENT_CYAN, LV_STATE_FOCUSED);
        lv_obj_set_style_border_width(btn, 2, LV_STATE_FOCUSED);
        lv_obj_set_style_shadow_color(btn, LV_COLOR_ACCENT_CYAN, LV_STATE_FOCUSED);
        lv_obj_set_style_shadow_width(btn, 15, LV_STATE_FOCUSED);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_30, LV_STATE_FOCUSED);

        // Icon
        lv_obj_t* icon = lv_label_create(btn);
        lv_label_set_text(icon, potaMenuItems[i].icon);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(icon, LV_COLOR_ACCENT_CYAN, 0);
        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 15);

        // Title
        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text(label, potaMenuItems[i].title);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(label, LV_COLOR_TEXT_PRIMARY, 0);
        lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -15);

        lv_obj_add_event_cb(btn, pota_menu_key_handler, LV_EVENT_KEY, NULL);
        pota_menu_buttons[i] = btn;
        addNavigableWidget(btn);
    }

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_color(footer, getThemeColors()->bg_deep, 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* footer_text = lv_label_create(footer);
    lv_label_set_text(footer_text, LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " Navigate   ENTER Select   ESC Back");
    lv_obj_set_style_text_font(footer_text, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(footer_text, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_align(footer_text, LV_ALIGN_CENTER, 0, 0);

    return screen;
}

static void pota_menu_key_handler(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    lv_obj_t* target = lv_event_get_target(e);

    if (key == LV_KEY_ENTER) {
        int target_mode = (int)(intptr_t)lv_obj_get_user_data(target);
        onLVGLMenuSelect(target_mode);
        lv_event_stop_processing(e);
    }
}

// ============================================
// Active Spots List Screen
// ============================================

static void pota_spots_key_handler(lv_event_t* e);


void refreshPOTASpotsDisplay() {
    if (!pota_spots_table || !lv_obj_is_valid(pota_spots_table)) {
        pota_spots_table = NULL;
        return;
    }

    // Update filtered list
    updateFilteredSpots();
    updateFilterLabel();
    updateCountLabel();

    if (filteredSpotCount == 0) {
        // Show "No spots" message
        lv_table_set_row_cnt(pota_spots_table, 1);
        lv_table_set_cell_value(pota_spots_table, 0, 0, "No spots found");
        lv_table_set_cell_value(pota_spots_table, 0, 1, "");
        lv_table_set_cell_value(pota_spots_table, 0, 2, "");
        lv_table_set_cell_value(pota_spots_table, 0, 3, "");
        updatePOTATimestampLabel();
        return;
    }

    // Set row count (data only - header is separate)
    lv_table_set_row_cnt(pota_spots_table, filteredSpotCount);

    // Populate data rows (starting at row 0)
    for (int i = 0; i < filteredSpotCount; i++) {
        int spotIdx = filteredSpotIndices[i];

        // Bounds check
        if (spotIdx < 0 || spotIdx >= potaSpotsCache.count) {
            continue;
        }

        POTASpot& spot = potaSpotsCache.spots[spotIdx];

        // Column 0: Activator callsign
        lv_table_set_cell_value(pota_spots_table, i, 0, spot.activator);

        // Column 1: Park reference
        lv_table_set_cell_value(pota_spots_table, i, 1, spot.reference);

        // Column 2: Frequency + Mode
        char freqMode[24];
        float freq = parseFrequency(spot.frequency);
        snprintf(freqMode, sizeof(freqMode), "%.3f %s", freq, spot.mode);
        lv_table_set_cell_value(pota_spots_table, i, 2, freqMode);

        // Column 3: Age
        char age[16];
        getSpotAge(spot.spotTime, age, sizeof(age));
        lv_table_set_cell_value(pota_spots_table, i, 3, age);
    }

    updatePOTATimestampLabel();

    // Reset selection if out of bounds
    if (pota_spots_selected_row >= filteredSpotCount) {
        pota_spots_selected_row = filteredSpotCount > 0 ? filteredSpotCount - 1 : 0;
    }
}

void showSpotsLoadingState(bool loading, const char* message = NULL, lv_color_t color = LV_COLOR_TEXT_PRIMARY) {
    pota_is_loading = loading;

    // Show/hide loading container in the table area
    if (pota_loading_label && lv_obj_is_valid(pota_loading_label)) {
        if (loading) {
            lv_obj_clear_flag(pota_loading_label, LV_OBJ_FLAG_HIDDEN);
            // Update text message if provided
            if (message && pota_loading_text && lv_obj_is_valid(pota_loading_text)) {
                lv_label_set_text(pota_loading_text, message);
                lv_obj_set_style_text_color(pota_loading_text, color, 0);
            }
        } else {
            lv_obj_add_flag(pota_loading_label, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Hide table while loading to show loading message clearly
    if (pota_spots_table && lv_obj_is_valid(pota_spots_table)) {
        if (loading) {
            lv_obj_add_flag(pota_spots_table, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(pota_spots_table, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

lv_obj_t* createPOTAActiveSpotsScreen() {
    Serial.println("[POTA] Creating Active Spots screen...");

    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    if (!screen) {
        Serial.println("[POTA] ERROR: Failed to create screen!");
        return NULL;
    }
    applyScreenStyle(screen);
    pota_screen = screen;

    Serial.println("[POTA] Screen created, building UI...");

    // Reset state
    pota_spots_selected_row = 0;
    pota_spots_scroll_pos = 0;

    // Header
    lv_obj_t* header = lv_obj_create(screen);
    lv_obj_set_size(header, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_add_style(header, getStyleStatusBar(), 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    // Count label (left side)
    pota_count_label = lv_label_create(header);
    lv_label_set_text(pota_count_label, "SPOTS (0)");
    lv_obj_add_style(pota_count_label, getStyleLabelTitle(), 0);
    lv_obj_align(pota_count_label, LV_ALIGN_LEFT_MID, 15, 0);

    // Filter indicator
    lv_obj_t* filter_btn = lv_label_create(header);
    lv_label_set_text(filter_btn, "[F]ilter");
    lv_obj_set_style_text_font(filter_btn, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(filter_btn, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(filter_btn, LV_ALIGN_CENTER, 20, 0);

    createCompactStatusBar(screen);

    // Filter bar (hidden by default)
    pota_filter_label = lv_label_create(screen);
    lv_label_set_text(pota_filter_label, "");
    lv_obj_set_style_text_font(pota_filter_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(pota_filter_label, LV_COLOR_WARNING, 0);
    lv_obj_set_pos(pota_filter_label, 15, HEADER_HEIGHT + 2);
    lv_obj_add_flag(pota_filter_label, LV_OBJ_FLAG_HIDDEN);

    // ========================================
    // Fixed Header Row (separate from table)
    // ========================================
    int header_row_y = HEADER_HEIGHT + 8;
    int header_row_height = 28;

    // Header background bar
    lv_obj_t* header_bar = lv_obj_create(screen);
    lv_obj_set_size(header_bar, SCREEN_WIDTH - 20, header_row_height);
    lv_obj_set_pos(header_bar, 10, header_row_y);
    lv_obj_set_style_bg_color(header_bar, getThemeColors()->bg_layer2, 0);
    lv_obj_set_style_bg_opa(header_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header_bar, 0, 0);
    lv_obj_set_style_radius(header_bar, 0, 0);
    lv_obj_set_style_pad_all(header_bar, 0, 0);
    lv_obj_clear_flag(header_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Header labels - match table column positions
    lv_obj_t* h1 = lv_label_create(header_bar);
    lv_label_set_text(h1, "CALL");
    lv_obj_set_style_text_font(h1, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(h1, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_pos(h1, 8, 5);

    lv_obj_t* h2 = lv_label_create(header_bar);
    lv_label_set_text(h2, "PARK");
    lv_obj_set_style_text_font(h2, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(h2, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_pos(h2, 98, 5);

    lv_obj_t* h3 = lv_label_create(header_bar);
    lv_label_set_text(h3, "FREQ / MODE");
    lv_obj_set_style_text_font(h3, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(h3, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_pos(h3, 198, 5);

    lv_obj_t* h4 = lv_label_create(header_bar);
    lv_label_set_text(h4, "AGE");
    lv_obj_set_style_text_font(h4, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(h4, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_pos(h4, 358, 5);

    // ========================================
    // Table (data only, no header row)
    // ========================================
    int table_y = header_row_y + header_row_height + 2;
    int table_height = SCREEN_HEIGHT - table_y - FOOTER_HEIGHT - 25;

    Serial.printf("[POTA] Layout: header_y=%d, table_y=%d, table_h=%d\n", header_row_y, table_y, table_height);
    Serial.println("[POTA] Creating table...");

    pota_spots_table = lv_table_create(screen);
    if (!pota_spots_table) {
        Serial.println("[POTA] ERROR: Failed to create table!");
        return screen;
    }
    lv_obj_set_size(pota_spots_table, SCREEN_WIDTH - 20, table_height);
    lv_obj_set_pos(pota_spots_table, 10, table_y);

    // Table styling
    lv_obj_set_style_bg_color(pota_spots_table, getThemeColors()->bg_deep, 0);
    lv_obj_set_style_bg_opa(pota_spots_table, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(pota_spots_table, 0, 0);
    lv_obj_set_style_pad_all(pota_spots_table, 0, 0);

    // Cell styling
    lv_obj_set_style_text_font(pota_spots_table, &lv_font_montserrat_14, LV_PART_ITEMS);
    lv_obj_set_style_text_color(pota_spots_table, LV_COLOR_TEXT_PRIMARY, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(pota_spots_table, getThemeColors()->bg_deep, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(pota_spots_table, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_pad_top(pota_spots_table, 6, LV_PART_ITEMS);
    lv_obj_set_style_pad_bottom(pota_spots_table, 6, LV_PART_ITEMS);
    lv_obj_set_style_pad_left(pota_spots_table, 4, LV_PART_ITEMS);

    // Selected row styling
    lv_obj_set_style_bg_color(pota_spots_table, LV_COLOR_ACCENT_CYAN, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(pota_spots_table, getThemeColors()->text_on_accent, LV_PART_ITEMS | LV_STATE_PRESSED);

    // Set column widths to match header
    lv_table_set_col_cnt(pota_spots_table, 4);
    lv_table_set_col_width(pota_spots_table, 0, 90);   // Callsign
    lv_table_set_col_width(pota_spots_table, 1, 100);  // Reference
    lv_table_set_col_width(pota_spots_table, 2, 150);  // Freq+Mode
    lv_table_set_col_width(pota_spots_table, 3, 80);   // Age

    // Initialize with empty row
    lv_table_set_row_cnt(pota_spots_table, 1);
    lv_table_set_cell_value(pota_spots_table, 0, 0, "");
    lv_table_set_cell_value(pota_spots_table, 0, 1, "");
    lv_table_set_cell_value(pota_spots_table, 0, 2, "");
    lv_table_set_cell_value(pota_spots_table, 0, 3, "");

    Serial.println("[POTA] Table created successfully");

    // Make table navigable and add key handler
    lv_obj_add_flag(pota_spots_table, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(pota_spots_table, pota_spots_key_handler, LV_EVENT_KEY, NULL);
    addNavigableWidget(pota_spots_table);

    // ========================================
    // Loading Indicator (with spinner)
    // ========================================
    int loading_center_y = table_y + (table_height / 2) - 50;

    lv_obj_t* loading_container = lv_obj_create(screen);
    lv_obj_set_size(loading_container, 280, 100);
    lv_obj_set_pos(loading_container, (SCREEN_WIDTH - 280) / 2, loading_center_y);
    lv_obj_set_style_bg_color(loading_container, getThemeColors()->bg_layer2, 0);
    lv_obj_set_style_bg_opa(loading_container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(loading_container, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_border_width(loading_container, 2, 0);
    lv_obj_set_style_radius(loading_container, 8, 0);
    lv_obj_clear_flag(loading_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(loading_container, LV_OBJ_FLAG_HIDDEN);

    // Spinner at top of loading container
    lv_obj_t* spinner = lv_spinner_create(loading_container, 1000, 60);
    lv_obj_set_size(spinner, 40, 40);
    lv_obj_align(spinner, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_arc_color(spinner, LV_COLOR_ACCENT_CYAN, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, getThemeColors()->bg_deep, LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 4, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner, 4, LV_PART_MAIN);

    // Loading text below spinner
    pota_loading_text = lv_label_create(loading_container);
    lv_label_set_text(pota_loading_text, "Loading POTA spots...");
    lv_obj_set_style_text_font(pota_loading_text, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(pota_loading_text, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(pota_loading_text, LV_ALIGN_BOTTOM_MID, 0, -12);

    // Store container as loading label for show/hide
    pota_loading_label = loading_container;

    // Move loading container to front so it shows above table
    lv_obj_move_foreground(loading_container);

    // Loading bar not used anymore
    pota_loading_bar = NULL;

    // Timestamp label
    pota_updated_label = lv_label_create(screen);
    lv_obj_set_style_text_font(pota_updated_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(pota_updated_label, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_set_pos(pota_updated_label, 15, SCREEN_HEIGHT - FOOTER_HEIGHT - 18);
    updatePOTATimestampLabel();

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_color(footer, getThemeColors()->bg_deep, 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* footer_text = lv_label_create(footer);
    lv_label_set_text(footer_text, LV_SYMBOL_UP LV_SYMBOL_DOWN " Scroll   ENTER Details   F Filter   R Refresh   ESC Back");
    lv_obj_set_style_text_font(footer_text, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(footer_text, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_align(footer_text, LV_ALIGN_CENTER, 0, 0);

    Serial.println("[POTA] UI complete, initializing data display...");

    // Initialize table with 1 empty row (LVGL tables need at least 1 row)
    if (pota_spots_table) {
        lv_table_set_row_cnt(pota_spots_table, 1);
        lv_table_set_cell_value(pota_spots_table, 0, 0, "");
        lv_table_set_cell_value(pota_spots_table, 0, 1, "");
        lv_table_set_cell_value(pota_spots_table, 0, 2, "");
        lv_table_set_cell_value(pota_spots_table, 0, 3, "");
    }

    Serial.println("[POTA] Active Spots screen ready");
    return screen;
}

static void pota_spots_key_handler(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // R or r - Refresh
    if (key == 'R' || key == 'r') {
        Serial.println("[POTA] R key pressed - refresh requested");

        if (!pota_is_loading) {
            Serial.println("[POTA] Starting fetch...");
            showSpotsLoadingState(true, "Loading POTA spots...", LV_COLOR_TEXT_PRIMARY);
            lv_timer_handler();  // Force UI update

            Serial.printf("[POTA] Free heap before fetch: %d\n", ESP.getFreeHeap());
            int result = fetchActiveSpots(potaSpotsCache);
            Serial.printf("[POTA] Fetch returned: %d, free heap: %d\n", result, ESP.getFreeHeap());

            showSpotsLoadingState(false);

            if (result >= 0) {
                Serial.println("[POTA] Fetch success, refreshing display...");
                refreshPOTASpotsDisplay();
                Serial.println("[POTA] Display refresh complete");
                beep(1000, 100);  // Success
            } else {
                Serial.println("[POTA] Fetch failed, showing error");
                if (WiFi.status() != WL_CONNECTED) {
                    showSpotsLoadingState(true, "WiFi not connected!", LV_COLOR_ERROR);
                } else {
                    showSpotsLoadingState(true, "Failed to load. Press R to retry.", LV_COLOR_ERROR);
                }
                beep(400, 200);  // Error
            }
        } else {
            Serial.println("[POTA] Already loading, ignoring");
        }
        lv_event_stop_processing(e);
        return;
    }

    // F or f - Filter
    if (key == 'F' || key == 'f') {
        onLVGLMenuSelect(POTA_MODE_FILTERS);
        lv_event_stop_processing(e);
        return;
    }

    // UP/DOWN - Navigate rows
    if (key == LV_KEY_UP || key == LV_KEY_PREV) {
        if (pota_spots_selected_row > 0) {
            pota_spots_selected_row--;
            lv_coord_t row_h = 30;  // Approximate row height
            lv_obj_scroll_to_y(pota_spots_table, pota_spots_selected_row * row_h, LV_ANIM_ON);
        }
        lv_event_stop_processing(e);
        return;
    }

    if (key == LV_KEY_DOWN || key == LV_KEY_NEXT) {
        if (pota_spots_selected_row < filteredSpotCount - 1) {
            pota_spots_selected_row++;
            lv_coord_t row_h = 30;
            lv_obj_scroll_to_y(pota_spots_table, pota_spots_selected_row * row_h, LV_ANIM_ON);
        }
        lv_event_stop_processing(e);
        return;
    }

    // ENTER - View detail
    if (key == LV_KEY_ENTER) {
        if (filteredSpotCount > 0 && pota_spots_selected_row < filteredSpotCount) {
            selectedSpotIndex = filteredSpotIndices[pota_spots_selected_row];
            onLVGLMenuSelect(POTA_MODE_SPOT_DETAIL);
        }
        lv_event_stop_processing(e);
        return;
    }

    // ESC - Go back
    if (key == LV_KEY_ESC) {
        onLVGLBackNavigation();
        lv_event_stop_processing(e);
        return;
    }
}

// ============================================
// Spot Detail Screen
// ============================================

static void pota_detail_key_handler(lv_event_t* e);

void updateDetailTabStyles() {
    for (int i = 0; i < 4; i++) {
        if (!pota_detail_tabs[i]) continue;

        if (i == pota_detail_selected_tab) {
            lv_obj_set_style_bg_color(pota_detail_tabs[i], LV_COLOR_ACCENT_CYAN, 0);
            lv_obj_t* label = lv_obj_get_child(pota_detail_tabs[i], 0);
            if (label) lv_obj_set_style_text_color(label, getThemeColors()->text_on_accent, 0);
        } else {
            lv_obj_set_style_bg_color(pota_detail_tabs[i], getThemeColors()->bg_layer2, 0);
            lv_obj_t* label = lv_obj_get_child(pota_detail_tabs[i], 0);
            if (label) lv_obj_set_style_text_color(label, LV_COLOR_TEXT_SECONDARY, 0);
        }
    }
}

void updateDetailContent() {
    if (!pota_detail_content) return;
    if (selectedSpotIndex < 0 || selectedSpotIndex >= potaSpotsCache.count) return;

    POTASpot& spot = potaSpotsCache.spots[selectedSpotIndex];

    // Clear content
    lv_obj_clean(pota_detail_content);

    char buf[128];

    switch (pota_detail_selected_tab) {
        case 0:  // Overview
        {
            lv_obj_t* park_lbl = lv_label_create(pota_detail_content);
            snprintf(buf, sizeof(buf), "PARK: %s", spot.parkName);
            lv_label_set_text(park_lbl, buf);
            lv_obj_set_style_text_font(park_lbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(park_lbl, LV_COLOR_TEXT_PRIMARY, 0);
            lv_label_set_long_mode(park_lbl, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(park_lbl, 420);

            lv_obj_t* loc_lbl = lv_label_create(pota_detail_content);
            snprintf(buf, sizeof(buf), "LOCATION: %s", spot.locationDesc);
            lv_label_set_text(loc_lbl, buf);
            lv_obj_set_style_text_font(loc_lbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(loc_lbl, LV_COLOR_TEXT_SECONDARY, 0);

            lv_obj_t* grid_lbl = lv_label_create(pota_detail_content);
            snprintf(buf, sizeof(buf), "GRID: %s", strlen(spot.grid6) > 0 ? spot.grid6 : spot.grid4);
            lv_label_set_text(grid_lbl, buf);
            lv_obj_set_style_text_font(grid_lbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(grid_lbl, LV_COLOR_TEXT_SECONDARY, 0);

            float freq = parseFrequency(spot.frequency);
            lv_obj_t* freq_lbl = lv_label_create(pota_detail_content);
            snprintf(buf, sizeof(buf), "FREQUENCY: %.3f MHz    MODE: %s", freq, spot.mode);
            lv_label_set_text(freq_lbl, buf);
            lv_obj_set_style_text_font(freq_lbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(freq_lbl, LV_COLOR_ACCENT_CYAN, 0);

            lv_obj_t* spotter_lbl = lv_label_create(pota_detail_content);
            char age[16];
            getSpotAge(spot.spotTime, age, sizeof(age));
            snprintf(buf, sizeof(buf), "SPOTTED BY: %s    TIME: %s", spot.spotter, age);
            lv_label_set_text(spotter_lbl, buf);
            lv_obj_set_style_text_font(spotter_lbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(spotter_lbl, LV_COLOR_TEXT_SECONDARY, 0);

            if (strlen(spot.comments) > 0) {
                lv_obj_t* comments_lbl = lv_label_create(pota_detail_content);
                snprintf(buf, sizeof(buf), "COMMENTS: %s", spot.comments);
                lv_label_set_text(comments_lbl, buf);
                lv_obj_set_style_text_font(comments_lbl, &lv_font_montserrat_12, 0);
                lv_obj_set_style_text_color(comments_lbl, LV_COLOR_TEXT_TERTIARY, 0);
                lv_label_set_long_mode(comments_lbl, LV_LABEL_LONG_WRAP);
                lv_obj_set_width(comments_lbl, 420);
            }
            break;
        }

        case 1:  // Activator Info
        {
            lv_obj_t* call_lbl = lv_label_create(pota_detail_content);
            snprintf(buf, sizeof(buf), "CALLSIGN: %s", spot.activator);
            lv_label_set_text(call_lbl, buf);
            lv_obj_set_style_text_font(call_lbl, &lv_font_montserrat_18, 0);
            lv_obj_set_style_text_color(call_lbl, LV_COLOR_ACCENT_CYAN, 0);

            lv_obj_t* ref_lbl = lv_label_create(pota_detail_content);
            snprintf(buf, sizeof(buf), "PARK: %s", spot.reference);
            lv_label_set_text(ref_lbl, buf);
            lv_obj_set_style_text_font(ref_lbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(ref_lbl, LV_COLOR_TEXT_PRIMARY, 0);

            if (spot.qsoCount > 0) {
                lv_obj_t* qso_lbl = lv_label_create(pota_detail_content);
                snprintf(buf, sizeof(buf), "QSO COUNT: %d", spot.qsoCount);
                lv_label_set_text(qso_lbl, buf);
                lv_obj_set_style_text_font(qso_lbl, &lv_font_montserrat_14, 0);
                lv_obj_set_style_text_color(qso_lbl, LV_COLOR_SUCCESS, 0);
            }

            lv_obj_t* grid_lbl = lv_label_create(pota_detail_content);
            snprintf(buf, sizeof(buf), "GRID: %s", strlen(spot.grid6) > 0 ? spot.grid6 : spot.grid4);
            lv_label_set_text(grid_lbl, buf);
            lv_obj_set_style_text_font(grid_lbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(grid_lbl, LV_COLOR_TEXT_SECONDARY, 0);

            lv_obj_t* loc_lbl = lv_label_create(pota_detail_content);
            snprintf(buf, sizeof(buf), "LOCATION: %s", spot.locationDesc);
            lv_label_set_text(loc_lbl, buf);
            lv_obj_set_style_text_font(loc_lbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(loc_lbl, LV_COLOR_TEXT_SECONDARY, 0);
            break;
        }

        case 2:  // Recent Spots
        {
            lv_obj_t* title_lbl = lv_label_create(pota_detail_content);
            lv_label_set_text(title_lbl, "Recent Spots");
            lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(title_lbl, LV_COLOR_ACCENT_CYAN, 0);

            lv_obj_t* info_lbl = lv_label_create(pota_detail_content);
            lv_label_set_text(info_lbl, "Recent hunter spots for this\nactivator will be shown here.\n\n(Coming soon)");
            lv_obj_set_style_text_font(info_lbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(info_lbl, LV_COLOR_TEXT_SECONDARY, 0);
            break;
        }

        case 3:  // QRZ Info
        {
            lv_obj_t* title_lbl = lv_label_create(pota_detail_content);
            lv_label_set_text(title_lbl, "QRZ Lookup");
            lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(title_lbl, LV_COLOR_ACCENT_CYAN, 0);

            lv_obj_t* info_lbl = lv_label_create(pota_detail_content);
            snprintf(buf, sizeof(buf), "QRZ lookup for %s\n\nConfigure QRZ API key in Settings\nto enable operator information.", spot.activator);
            lv_label_set_text(info_lbl, buf);
            lv_obj_set_style_text_font(info_lbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(info_lbl, LV_COLOR_TEXT_SECONDARY, 0);
            break;
        }
    }
}

lv_obj_t* createPOTASpotDetailScreen() {
    clearNavigationGroup();

    if (selectedSpotIndex < 0 || selectedSpotIndex >= potaSpotsCache.count) {
        // No spot selected, go back
        return createPOTAActiveSpotsScreen();
    }

    POTASpot& spot = potaSpotsCache.spots[selectedSpotIndex];

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    pota_screen = screen;

    // Reset tab selection
    pota_detail_selected_tab = 0;

    // Header
    lv_obj_t* header = lv_obj_create(screen);
    lv_obj_set_size(header, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_add_style(header, getStyleStatusBar(), 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    // Title: Callsign @ Reference
    char title_buf[32];
    snprintf(title_buf, sizeof(title_buf), "%s @ %s", spot.activator, spot.reference);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, title_buf);
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    createCompactStatusBar(screen);

    // Tab bar
    const char* tabNames[] = {"Overview", "Activator", "Recent", "QRZ"};
    int tabWidths[] = {90, 90, 70, 50};
    int tabX = 15;

    for (int i = 0; i < 4; i++) {
        lv_obj_t* tab = lv_obj_create(screen);
        lv_obj_set_size(tab, tabWidths[i], 28);
        lv_obj_set_pos(tab, tabX, HEADER_HEIGHT + 5);
        lv_obj_set_style_radius(tab, 6, 0);
        lv_obj_set_style_border_width(tab, 1, 0);
        lv_obj_set_style_border_color(tab, LV_COLOR_ACCENT_CYAN, 0);
        lv_obj_set_style_pad_all(tab, 0, 0);
        lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* tab_lbl = lv_label_create(tab);
        lv_label_set_text(tab_lbl, tabNames[i]);
        lv_obj_set_style_text_font(tab_lbl, &lv_font_montserrat_12, 0);
        lv_obj_center(tab_lbl);

        pota_detail_tabs[i] = tab;
        tabX += tabWidths[i] + 8;
    }
    updateDetailTabStyles();

    // Content area
    pota_detail_content = lv_obj_create(screen);
    lv_obj_set_size(pota_detail_content, SCREEN_WIDTH - 30, 150);
    lv_obj_set_pos(pota_detail_content, 15, HEADER_HEIGHT + 40);
    applyCardStyle(pota_detail_content);
    lv_obj_set_layout(pota_detail_content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(pota_detail_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(pota_detail_content, 10, 0);
    lv_obj_set_style_pad_row(pota_detail_content, 6, 0);
    lv_obj_clear_flag(pota_detail_content, LV_OBJ_FLAG_SCROLLABLE);

    updateDetailContent();

    // Make content area navigable and add key handler
    lv_obj_add_flag(pota_detail_content, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(pota_detail_content, pota_detail_key_handler, LV_EVENT_KEY, NULL);
    addNavigableWidget(pota_detail_content);

    // LOG QSO button
    lv_obj_t* log_btn = lv_btn_create(screen);
    lv_obj_set_size(log_btn, 150, 40);
    lv_obj_align(log_btn, LV_ALIGN_BOTTOM_MID, 0, -FOOTER_HEIGHT - 10);
    lv_obj_set_style_bg_color(log_btn, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_bg_color(log_btn, lv_color_hex(0x00AAAA), LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(log_btn, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(log_btn, lv_color_hex(0xFFFFFF), LV_STATE_FOCUSED);

    lv_obj_t* btn_lbl = lv_label_create(log_btn);
    lv_label_set_text(btn_lbl, "LOG QSO");
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(btn_lbl, getThemeColors()->text_on_accent, 0);
    lv_obj_center(btn_lbl);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_color(footer, getThemeColors()->bg_deep, 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* footer_text = lv_label_create(footer);
    lv_label_set_text(footer_text, LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " Switch Tab   ENTER Log QSO   ESC Back");
    lv_obj_set_style_text_font(footer_text, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(footer_text, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_align(footer_text, LV_ALIGN_CENTER, 0, 0);

    return screen;
}

static void pota_detail_key_handler(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // LEFT/RIGHT - Switch tabs
    if (key == LV_KEY_LEFT) {
        if (pota_detail_selected_tab > 0) {
            pota_detail_selected_tab--;
            updateDetailTabStyles();
            updateDetailContent();
        }
        lv_event_stop_processing(e);
        return;
    }

    if (key == LV_KEY_RIGHT) {
        if (pota_detail_selected_tab < 3) {
            pota_detail_selected_tab++;
            updateDetailTabStyles();
            updateDetailContent();
        }
        lv_event_stop_processing(e);
        return;
    }

    // ENTER - Log QSO
    if (key == LV_KEY_ENTER) {
        if (selectedSpotIndex >= 0 && selectedSpotIndex < potaSpotsCache.count) {
            POTASpot& spot = potaSpotsCache.spots[selectedSpotIndex];

            // Initialize log entry with spot data
            initLogEntry();

            // Pre-fill from spot
            strncpy(logEntryState.callsign, spot.activator, sizeof(logEntryState.callsign) - 1);
            strncpy(logEntryState.frequency, spot.frequency, sizeof(logEntryState.frequency) - 1);

            // Map mode string to mode index
            for (int i = 0; i < NUM_MODES; i++) {
                if (strcasecmp(QSO_MODES[i], spot.mode) == 0) {
                    logEntryState.modeIndex = i;
                    break;
                }
            }

            // Set their POTA reference
            strncpy(logEntryState.theirPOTA, spot.reference, sizeof(logEntryState.theirPOTA) - 1);

            // Set grid if available
            if (strlen(spot.grid6) > 0) {
                strncpy(logEntryState.theirGrid, spot.grid6, sizeof(logEntryState.theirGrid) - 1);
            } else if (strlen(spot.grid4) > 0) {
                strncpy(logEntryState.theirGrid, spot.grid4, sizeof(logEntryState.theirGrid) - 1);
            }

            // Navigate to QSO Logger
            onLVGLMenuSelect(37);  // LVGL_MODE_QSO_LOG_ENTRY
        }
        lv_event_stop_processing(e);
        return;
    }

    // ESC - Go back
    if (key == LV_KEY_ESC) {
        onLVGLBackNavigation();
        lv_event_stop_processing(e);
        return;
    }
}

// ============================================
// Filter Screen
// ============================================

static lv_obj_t* filter_band_label = NULL;
static lv_obj_t* filter_mode_label = NULL;
static lv_obj_t* filter_region_label = NULL;
static lv_obj_t* filter_rows[5] = {NULL};

static void pota_filter_key_handler(lv_event_t* e);

void updateFilterRowStyles() {
    for (int i = 0; i < 5; i++) {
        if (!filter_rows[i]) continue;

        if (i == pota_filter_focus_row) {
            lv_obj_set_style_bg_color(filter_rows[i], getThemeColors()->card_secondary, 0);
            lv_obj_set_style_border_color(filter_rows[i], LV_COLOR_ACCENT_CYAN, 0);
            lv_obj_set_style_border_width(filter_rows[i], 2, 0);
        } else {
            lv_obj_set_style_bg_color(filter_rows[i], getThemeColors()->bg_deep, 0);
            lv_obj_set_style_border_width(filter_rows[i], 0, 0);
        }
    }
}

void updateFilterValues() {
    if (filter_band_label) {
        char buf[16];
        snprintf(buf, sizeof(buf), "< %s >", bandFilterOptions[pota_filter_band_idx]);
        lv_label_set_text(filter_band_label, buf);
    }

    if (filter_mode_label) {
        char buf[16];
        snprintf(buf, sizeof(buf), "< %s >", modeFilterOptions[pota_filter_mode_idx]);
        lv_label_set_text(filter_mode_label, buf);
    }

    if (filter_region_label) {
        char buf[16];
        snprintf(buf, sizeof(buf), "< %s >", regionFilterOptions[pota_filter_region_idx]);
        lv_label_set_text(filter_region_label, buf);
    }
}

lv_obj_t* createPOTAFilterScreen() {
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    pota_screen = screen;

    // Reset focus
    pota_filter_focus_row = 0;

    // Find current filter indices
    pota_filter_band_idx = 0;
    for (int i = 0; i < NUM_BAND_FILTERS; i++) {
        if (strcmp(potaSpotFilter.band, bandFilterOptions[i]) == 0) {
            pota_filter_band_idx = i;
            break;
        }
    }

    pota_filter_mode_idx = 0;
    for (int i = 0; i < NUM_MODE_FILTERS; i++) {
        if (strcmp(potaSpotFilter.mode, modeFilterOptions[i]) == 0) {
            pota_filter_mode_idx = i;
            break;
        }
    }

    pota_filter_region_idx = 0;
    for (int i = 0; i < NUM_REGION_FILTERS; i++) {
        if (strcmp(potaSpotFilter.region, regionFilterOptions[i]) == 0) {
            pota_filter_region_idx = i;
            break;
        }
    }

    // Header
    lv_obj_t* header = lv_obj_create(screen);
    lv_obj_set_size(header, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_add_style(header, getStyleStatusBar(), 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "FILTER SPOTS");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Content card
    lv_obj_t* card = lv_obj_create(screen);
    lv_obj_set_size(card, SCREEN_WIDTH - 40, 180);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    applyCardStyle(card);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(card, 15, 0);
    lv_obj_set_style_pad_row(card, 10, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // Band row
    lv_obj_t* band_row = lv_obj_create(card);
    lv_obj_set_size(band_row, LV_PCT(100), 30);
    lv_obj_set_style_bg_opa(band_row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(band_row, 6, 0);
    lv_obj_set_style_pad_all(band_row, 5, 0);
    lv_obj_clear_flag(band_row, LV_OBJ_FLAG_SCROLLABLE);
    filter_rows[0] = band_row;

    lv_obj_t* band_title = lv_label_create(band_row);
    lv_label_set_text(band_title, "BAND:");
    lv_obj_set_style_text_font(band_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(band_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(band_title, LV_ALIGN_LEFT_MID, 0, 0);

    filter_band_label = lv_label_create(band_row);
    lv_obj_set_style_text_font(filter_band_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(filter_band_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(filter_band_label, LV_ALIGN_RIGHT_MID, 0, 0);

    // Mode row
    lv_obj_t* mode_row = lv_obj_create(card);
    lv_obj_set_size(mode_row, LV_PCT(100), 30);
    lv_obj_set_style_bg_opa(mode_row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(mode_row, 6, 0);
    lv_obj_set_style_pad_all(mode_row, 5, 0);
    lv_obj_clear_flag(mode_row, LV_OBJ_FLAG_SCROLLABLE);
    filter_rows[1] = mode_row;

    lv_obj_t* mode_title = lv_label_create(mode_row);
    lv_label_set_text(mode_title, "MODE:");
    lv_obj_set_style_text_font(mode_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(mode_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(mode_title, LV_ALIGN_LEFT_MID, 0, 0);

    filter_mode_label = lv_label_create(mode_row);
    lv_obj_set_style_text_font(filter_mode_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(filter_mode_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(filter_mode_label, LV_ALIGN_RIGHT_MID, 0, 0);

    // Region row
    lv_obj_t* region_row = lv_obj_create(card);
    lv_obj_set_size(region_row, LV_PCT(100), 30);
    lv_obj_set_style_bg_opa(region_row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(region_row, 6, 0);
    lv_obj_set_style_pad_all(region_row, 5, 0);
    lv_obj_clear_flag(region_row, LV_OBJ_FLAG_SCROLLABLE);
    filter_rows[2] = region_row;

    lv_obj_t* region_title = lv_label_create(region_row);
    lv_label_set_text(region_title, "REGION:");
    lv_obj_set_style_text_font(region_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(region_title, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(region_title, LV_ALIGN_LEFT_MID, 0, 0);

    filter_region_label = lv_label_create(region_row);
    lv_obj_set_style_text_font(filter_region_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(filter_region_label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(filter_region_label, LV_ALIGN_RIGHT_MID, 0, 0);

    // Button row
    lv_obj_t* btn_row = lv_obj_create(card);
    lv_obj_set_size(btn_row, LV_PCT(100), 40);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);

    // Apply button
    lv_obj_t* apply_btn = lv_obj_create(btn_row);
    lv_obj_set_size(apply_btn, 150, 35);
    lv_obj_align(apply_btn, LV_ALIGN_LEFT_MID, 20, 0);
    lv_obj_set_style_bg_color(apply_btn, LV_COLOR_SUCCESS, 0);
    lv_obj_set_style_radius(apply_btn, 6, 0);
    lv_obj_set_style_border_width(apply_btn, 0, 0);
    lv_obj_clear_flag(apply_btn, LV_OBJ_FLAG_SCROLLABLE);
    filter_rows[3] = apply_btn;

    lv_obj_t* apply_lbl = lv_label_create(apply_btn);
    lv_label_set_text(apply_lbl, "APPLY");
    lv_obj_set_style_text_font(apply_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(apply_lbl, getThemeColors()->text_on_accent, 0);
    lv_obj_center(apply_lbl);

    // Clear button
    lv_obj_t* clear_btn = lv_obj_create(btn_row);
    lv_obj_set_size(clear_btn, 150, 35);
    lv_obj_align(clear_btn, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_set_style_bg_color(clear_btn, LV_COLOR_ERROR, 0);
    lv_obj_set_style_radius(clear_btn, 6, 0);
    lv_obj_set_style_border_width(clear_btn, 0, 0);
    lv_obj_clear_flag(clear_btn, LV_OBJ_FLAG_SCROLLABLE);
    filter_rows[4] = clear_btn;

    lv_obj_t* clear_lbl = lv_label_create(clear_btn);
    lv_label_set_text(clear_lbl, "CLEAR");
    lv_obj_set_style_text_font(clear_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(clear_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(clear_lbl);

    // Make card navigable and add key handler
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card, pota_filter_key_handler, LV_EVENT_KEY, NULL);
    addNavigableWidget(card);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_color(footer, getThemeColors()->bg_deep, 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* footer_text = lv_label_create(footer);
    lv_label_set_text(footer_text, LV_SYMBOL_UP LV_SYMBOL_DOWN " Navigate   " LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " Adjust   ENTER Apply   ESC Cancel");
    lv_obj_set_style_text_font(footer_text, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(footer_text, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_align(footer_text, LV_ALIGN_CENTER, 0, 0);

    // Initialize display
    updateFilterRowStyles();
    updateFilterValues();

    return screen;
}

static void pota_filter_key_handler(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // UP/DOWN - Navigate rows
    if (key == LV_KEY_UP || key == LV_KEY_PREV) {
        if (pota_filter_focus_row > 0) {
            pota_filter_focus_row--;
            updateFilterRowStyles();
        }
        lv_event_stop_processing(e);
        return;
    }

    if (key == LV_KEY_DOWN || key == LV_KEY_NEXT) {
        if (pota_filter_focus_row < 4) {
            pota_filter_focus_row++;
            updateFilterRowStyles();
        }
        lv_event_stop_processing(e);
        return;
    }

    // LEFT/RIGHT - Adjust values
    if (key == LV_KEY_LEFT) {
        switch (pota_filter_focus_row) {
            case 0:  // Band
                if (pota_filter_band_idx > 0) {
                    pota_filter_band_idx--;
                    updateFilterValues();
                }
                break;
            case 1:  // Mode
                if (pota_filter_mode_idx > 0) {
                    pota_filter_mode_idx--;
                    updateFilterValues();
                }
                break;
            case 2:  // Region
                if (pota_filter_region_idx > 0) {
                    pota_filter_region_idx--;
                    updateFilterValues();
                }
                break;
            case 3:  // Apply (do nothing)
            case 4:  // Clear (do nothing)
                break;
        }
        lv_event_stop_processing(e);
        return;
    }

    if (key == LV_KEY_RIGHT) {
        switch (pota_filter_focus_row) {
            case 0:  // Band
                if (pota_filter_band_idx < NUM_BAND_FILTERS - 1) {
                    pota_filter_band_idx++;
                    updateFilterValues();
                }
                break;
            case 1:  // Mode
                if (pota_filter_mode_idx < NUM_MODE_FILTERS - 1) {
                    pota_filter_mode_idx++;
                    updateFilterValues();
                }
                break;
            case 2:  // Region
                if (pota_filter_region_idx < NUM_REGION_FILTERS - 1) {
                    pota_filter_region_idx++;
                    updateFilterValues();
                }
                break;
            case 3:  // Apply (do nothing)
            case 4:  // Clear (do nothing)
                break;
        }
        lv_event_stop_processing(e);
        return;
    }

    // ENTER - Apply or Clear
    if (key == LV_KEY_ENTER) {
        if (pota_filter_focus_row == 3) {
            // Apply filter
            strcpy(potaSpotFilter.band, bandFilterOptions[pota_filter_band_idx]);
            strcpy(potaSpotFilter.mode, modeFilterOptions[pota_filter_mode_idx]);
            strcpy(potaSpotFilter.region, regionFilterOptions[pota_filter_region_idx]);
            updateFilterActiveStatus();
            beep(1000, 100);
            onLVGLMenuSelect(POTA_MODE_ACTIVE_SPOTS);
        } else if (pota_filter_focus_row == 4) {
            // Clear filter
            resetSpotFilter();
            pota_filter_band_idx = 0;
            pota_filter_mode_idx = 0;
            pota_filter_region_idx = 0;
            beep(800, 100);
            onLVGLMenuSelect(POTA_MODE_ACTIVE_SPOTS);
        }
        lv_event_stop_processing(e);
        return;
    }

    // ESC - Go back
    if (key == LV_KEY_ESC) {
        onLVGLBackNavigation();
        lv_event_stop_processing(e);
        return;
    }
}

// ============================================
// Screen Selector
// ============================================

lv_obj_t* createPOTAScreenForMode(int mode) {
    switch (mode) {
        case POTA_MODE_MENU:
            return createPOTAMenuScreen();
        case POTA_MODE_ACTIVE_SPOTS:
            return createPOTAActiveSpotsScreen();
        case POTA_MODE_SPOT_DETAIL:
            return createPOTASpotDetailScreen();
        case POTA_MODE_FILTERS:
            return createPOTAFilterScreen();
        case POTA_MODE_ACTIVATE:
            return createComingSoonScreen("ACTIVATE A PARK");
        default:
            return NULL;
    }
}

// ============================================
// Start POTA Mode
// ============================================

static void pota_autoload_cb(lv_timer_t* timer) {
    // Delete the one-shot timer
    if (pota_autoload_timer) {
        lv_timer_del(pota_autoload_timer);
        pota_autoload_timer = NULL;
    }

    // Only proceed if we're still on the active spots screen
    if (getCurrentModeAsInt() != POTA_MODE_ACTIVE_SPOTS) {
        return;
    }

    // Check if we already have valid cached data
    if (potaSpotsCache.valid && potaSpotsCache.count > 0) {
        Serial.println("[POTA] Using cached spots data");
        refreshPOTASpotsDisplay();
        return;
    }

    // No valid cache - fetch new data
    if (!pota_is_loading && WiFi.status() == WL_CONNECTED) {
        Serial.println("[POTA] Auto-loading spots...");
        showSpotsLoadingState(true, "Loading POTA spots...", LV_COLOR_TEXT_PRIMARY);
        lv_timer_handler();  // Force UI update to show loading

        int result = fetchActiveSpots(potaSpotsCache);
        showSpotsLoadingState(false);

        if (result >= 0) {
            Serial.printf("[POTA] Auto-load success: %d spots\n", potaSpotsCache.count);
            refreshPOTASpotsDisplay();
        } else {
            Serial.println("[POTA] Auto-load failed");
            showSpotsLoadingState(true, "Failed to load. Press R to retry.", LV_COLOR_ERROR);
        }
    } else if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[POTA] No WiFi - skipping auto-load");
        showSpotsLoadingState(true, "WiFi not connected. Press R when connected.", LV_COLOR_WARNING);
    }
}

void startPOTAActiveSpots(LGFX& display) {
    Serial.println("[POTA] startPOTAActiveSpots called");

    // Start refresh/timestamp timers
    startPOTATimers();

    // Schedule auto-load after a brief delay to let screen render first
    if (pota_autoload_timer) {
        lv_timer_del(pota_autoload_timer);
    }
    pota_autoload_timer = lv_timer_create(pota_autoload_cb, 200, NULL);  // 200ms delay
    lv_timer_set_repeat_count(pota_autoload_timer, 1);  // One-shot

    Serial.println("[POTA] Auto-load scheduled");
}

#endif // LV_POTA_SCREENS_H
