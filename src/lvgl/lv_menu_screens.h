/*
 * VAIL SUMMIT - LVGL Menu Screens
 * Replaces LovyanGFX menu rendering with LVGL
 */

#ifndef LV_MENU_SCREENS_H
#define LV_MENU_SCREENS_H

#include <lvgl.h>
#include <WiFi.h>
#include "lv_theme_summit.h"
#include "lv_widgets_summit.h"
#include "lv_screen_manager.h"
#include "../core/config.h"

// Forward declarations from main file
extern int currentSelection;

// ============================================
// Menu Data Structures
// ============================================

// Menu item structure for LVGL menus
struct LVMenuItem {
    const char* icon;
    const char* title;
    int target_mode;  // MenuMode enum value to switch to
};

// ============================================
// Menu Data (mirrors menu_ui.h data)
// Mode values MUST match MenuMode enum in menu_ui.h
// ============================================

// Main menu items - using LVGL symbols for modern look
static const LVMenuItem mainMenuItems[] = {
    {LV_SYMBOL_AUDIO, "CW", 44},           // MODE_CW_MENU
    {LV_SYMBOL_PLAY, "Games", 15},         // MODE_GAMES_MENU
    {LV_SYMBOL_DIRECTORY, "Ham Tools", 45}, // MODE_HAM_TOOLS_MENU
    {LV_SYMBOL_SETTINGS, "Settings", 21}   // MODE_SETTINGS_MENU
};
#define MAIN_MENU_COUNT 4

// CW submenu items
static const LVMenuItem cwMenuItems[] = {
    {LV_SYMBOL_EDIT, "Training", 1},           // MODE_TRAINING_MENU
    {LV_SYMBOL_REFRESH, "Practice", 6},        // MODE_PRACTICE
    {LV_SYMBOL_UPLOAD, "Vail Repeater", 31},   // MODE_VAIL_REPEATER
    {LV_SYMBOL_BLUETOOTH, "Bluetooth", 32},    // MODE_BLUETOOTH_MENU
    {LV_SYMBOL_POWER, "Radio Output", 19},     // MODE_RADIO_OUTPUT
    {LV_SYMBOL_SAVE, "CW Memories", 20}        // MODE_CW_MEMORIES
};
#define CW_MENU_COUNT 6

// Training submenu items
static const LVMenuItem trainingMenuItems[] = {
    {LV_SYMBOL_AUDIO, "Hear It Type It", 2},   // MODE_HEAR_IT_MENU
    {LV_SYMBOL_LOOP, "Koch Method", 7},        // MODE_KOCH_METHOD
    {LV_SYMBOL_FILE, "CW Academy", 8}          // MODE_CW_ACADEMY_TRACK_SELECT
};
#define TRAINING_MENU_COUNT 3

// Games submenu items
static const LVMenuItem gamesMenuItems[] = {
    {LV_SYMBOL_PLAY, "Morse Shooter", 16},     // MODE_MORSE_SHOOTER
    {LV_SYMBOL_LOOP, "Memory Chain", 17}       // MODE_MORSE_MEMORY
};
#define GAMES_MENU_COUNT 2

// Settings submenu items
static const LVMenuItem settingsMenuItems[] = {
    {LV_SYMBOL_HOME, "Device Settings", 22},   // MODE_DEVICE_SETTINGS_MENU
    {LV_SYMBOL_AUDIO, "CW Settings", 26}       // MODE_CW_SETTINGS
};
#define SETTINGS_MENU_COUNT 2

// Device settings submenu items
static const LVMenuItem deviceSettingsMenuItems[] = {
    {LV_SYMBOL_WIFI, "WiFi", 23},              // MODE_WIFI_SUBMENU
    {LV_SYMBOL_SETTINGS, "General", 24},       // MODE_GENERAL_SUBMENU
    {LV_SYMBOL_BLUETOOTH, "Bluetooth", 54},    // MODE_DEVICE_BT_SUBMENU
    {LV_SYMBOL_HOME, "System Info", 61}        // MODE_SYSTEM_INFO
};
#define DEVICE_SETTINGS_COUNT 4

// WiFi submenu items
static const LVMenuItem wifiSubmenuItems[] = {
    {LV_SYMBOL_WIFI, "WiFi Setup", 25},        // MODE_WIFI_SETTINGS
    {LV_SYMBOL_EYE_CLOSE, "Web Password", 30}  // MODE_WEB_PASSWORD_SETTINGS
};
#define WIFI_SUBMENU_COUNT 2

// General submenu items
static const LVMenuItem generalSubmenuItems[] = {
    {LV_SYMBOL_CALL, "Callsign", 29},          // MODE_CALLSIGN_SETTINGS
    {LV_SYMBOL_VOLUME_MAX, "Volume", 27},      // MODE_VOLUME_SETTINGS
    {LV_SYMBOL_IMAGE, "Brightness", 28},       // MODE_BRIGHTNESS_SETTINGS
    {LV_SYMBOL_EYE_OPEN, "UI Theme", 59}       // MODE_THEME_SETTINGS
};
#define GENERAL_SUBMENU_COUNT 4

// Ham Tools submenu items
static const LVMenuItem hamToolsMenuItems[] = {
    {LV_SYMBOL_SAVE, "QSO Logger", 36},        // MODE_QSO_LOGGER_MENU
    {LV_SYMBOL_GPS, "POTA", 62},               // MODE_POTA_MENU
    {LV_SYMBOL_LIST, "Band Plans", 46},        // MODE_BAND_PLANS
    {LV_SYMBOL_REFRESH, "Band Conditions", 47}, // MODE_PROPAGATION (Band Conditions)
    {LV_SYMBOL_CHARGE, "Antennas", 48},        // MODE_ANTENNAS
    {LV_SYMBOL_FILE, "License Study", 50},     // MODE_LICENSE_SELECT
    {LV_SYMBOL_ENVELOPE, "Summit Chat", 53}    // MODE_SUMMIT_CHAT
};
#define HAM_TOOLS_COUNT 7

// Bluetooth submenu items
static const LVMenuItem bluetoothMenuItems[] = {
    {LV_SYMBOL_KEYBOARD, "HID (Keyboard)", 33},  // MODE_BT_HID
    {LV_SYMBOL_AUDIO, "MIDI", 34}                // MODE_BT_MIDI
};
#define BLUETOOTH_MENU_COUNT 2

// QSO Logger submenu items
static const LVMenuItem qsoLoggerMenuItems[] = {
    {LV_SYMBOL_PLUS, "New Log Entry", 37},     // MODE_QSO_LOG_ENTRY
    {LV_SYMBOL_LIST, "View Logs", 38},         // MODE_QSO_VIEW_LOGS
    {LV_SYMBOL_IMAGE, "Statistics", 39},       // MODE_QSO_STATISTICS
    {LV_SYMBOL_SETTINGS, "Logger Settings", 40} // MODE_QSO_LOGGER_SETTINGS
};
#define QSO_LOGGER_COUNT 4

// ============================================
// Screen Objects
// ============================================

static lv_obj_t* current_menu_screen = NULL;
static lv_obj_t* menu_list = NULL;
static lv_obj_t* status_bar = NULL;
static int current_menu_item_count = 0;

// Callback for menu item selection (to be set by main app)
typedef void (*MenuSelectCallback)(int target_mode);
static MenuSelectCallback menu_select_callback = NULL;

// ============================================
// Menu Item Click Handler
// ============================================

static void menu_item_click_handler(lv_event_t* e) {
    lv_obj_t* target = lv_event_get_target(e);
    int target_mode = (int)(intptr_t)lv_obj_get_user_data(target);

    if (menu_select_callback != NULL) {
        menu_select_callback(target_mode);
    }
}

// ============================================
// 2D Grid Navigation Handler
// ============================================

// Number of columns in menu grid layout
static const int MENU_COLUMNS = 2;

// Store menu button references for grid navigation
#define MAX_MENU_BUTTONS 16
static lv_obj_t* menu_buttons[MAX_MENU_BUTTONS] = {NULL};
static int menu_button_count = 0;

/*
 * Custom event handler for FULL 2D grid navigation
 * Intercepts ALL arrow keys to navigate as a proper 2D grid:
 * - UP (LV_KEY_UP or LV_KEY_PREV): Move to same column in previous row
 * - DOWN (LV_KEY_DOWN or LV_KEY_NEXT): Move to same column in next row
 * - LEFT: Move to left column in same row
 * - RIGHT: Move to right column in same row
 * At any edge: does nothing (no wrap)
 */
static void menu_grid_nav_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // Handle all four arrow keys (support both UP/DOWN and PREV/NEXT mappings)
    if (key != LV_KEY_LEFT && key != LV_KEY_RIGHT &&
        key != LV_KEY_PREV && key != LV_KEY_NEXT &&
        key != LV_KEY_UP && key != LV_KEY_DOWN) return;

    // Always stop propagation to prevent LVGL's default linear navigation
    lv_event_stop_processing(e);

    if (menu_button_count <= 1) return;

    // Get current focused object
    lv_obj_t* focused = lv_event_get_target(e);
    if (!focused) return;

    // Find index of focused object in our button array
    int focused_idx = -1;
    for (int i = 0; i < menu_button_count; i++) {
        if (menu_buttons[i] == focused) {
            focused_idx = i;
            break;
        }
    }
    if (focused_idx < 0) return;

    // Calculate grid position
    int row = focused_idx / MENU_COLUMNS;
    int col = focused_idx % MENU_COLUMNS;
    int total_rows = (menu_button_count + MENU_COLUMNS - 1) / MENU_COLUMNS;

    int target_idx = -1;

    if (key == LV_KEY_RIGHT) {
        // Move right: only if not at rightmost column AND target exists
        if (col < MENU_COLUMNS - 1) {
            int potential = focused_idx + 1;
            if (potential < menu_button_count) {
                target_idx = potential;
            }
        }
    } else if (key == LV_KEY_LEFT) {
        // Move left: only if not at leftmost column
        if (col > 0) {
            target_idx = focused_idx - 1;
        }
    } else if (key == LV_KEY_NEXT || key == LV_KEY_DOWN) {  // DOWN arrow
        // Move down: go to same column in next row
        if (row < total_rows - 1) {
            int potential = focused_idx + MENU_COLUMNS;
            if (potential < menu_button_count) {
                target_idx = potential;
            }
        }
    } else if (key == LV_KEY_PREV || key == LV_KEY_UP) {  // UP arrow
        // Move up: go to same column in previous row
        if (row > 0) {
            target_idx = focused_idx - MENU_COLUMNS;
        }
    }

    // Focus target if valid and scroll into view
    if (target_idx >= 0 && target_idx < menu_button_count) {
        lv_obj_t* target = menu_buttons[target_idx];
        if (target) {
            lv_group_focus_obj(target);
            lv_obj_scroll_to_view(target, LV_ANIM_ON);
        }
    }
}

// ============================================
// Create Menu Screen
// ============================================

// Menu header height constant (used for content positioning)
static const int MENU_HEADER_HEIGHT = 50;

// Status bar globals from status_bar.h
extern int batteryPercent;
extern bool wifiConnected;

/*
 * Create a modern header bar with title and status icons (battery, WiFi)
 */
lv_obj_t* createHeader(lv_obj_t* parent, const char* title) {
    lv_obj_t* header = lv_obj_create(parent);
    lv_obj_set_size(header, LV_PCT(100), MENU_HEADER_HEIGHT);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_all(header, 10, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    // Title - use theme font
    lv_obj_t* lbl_title = lv_label_create(header);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_font(lbl_title, getThemeFonts()->font_input, 0);  // Theme font
    lv_obj_set_style_text_color(lbl_title, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 5, 0);

    // WiFi icon - use Montserrat for LVGL symbols
    lv_obj_t* wifi_icon = lv_label_create(header);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_20, 0);
    if (WiFi.status() == WL_CONNECTED) {
        lv_obj_set_style_text_color(wifi_icon, LV_COLOR_SUCCESS, 0);
    } else {
        lv_obj_set_style_text_color(wifi_icon, LV_COLOR_ERROR, 0);
    }
    lv_obj_align(wifi_icon, LV_ALIGN_RIGHT_MID, -50, 0);

    // Battery icon - use Montserrat for LVGL symbols
    lv_obj_t* batt_icon = lv_label_create(header);
    lv_obj_set_style_text_font(batt_icon, &lv_font_montserrat_20, 0);
    lv_obj_align(batt_icon, LV_ALIGN_RIGHT_MID, -10, 0);

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

    return header;
}

/*
 * Create a generic menu screen with modern LVGL layout
 * Uses lv_btn for menu items with proper focus handling
 */
lv_obj_t* createMenuScreen(const char* title, const LVMenuItem* items, int item_count) {
    // Clear menu button tracking array for 2D navigation
    for (int i = 0; i < MAX_MENU_BUTTONS; i++) {
        menu_buttons[i] = NULL;
    }
    menu_button_count = 0;

    // Create screen with dark background
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, LV_COLOR_BG_DEEP, 0);

    // Header
    createHeader(screen, title);

    // Content area - positioned below header with proper spacing
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, LV_PCT(100), SCREEN_HEIGHT - MENU_HEADER_HEIGHT - FOOTER_HEIGHT - 10);
    lv_obj_set_pos(content, 0, MENU_HEADER_HEIGHT + 5);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 10, 0);
    lv_obj_set_style_pad_row(content, 10, 0);  // Gap between rows
    lv_obj_set_style_pad_column(content, 20, 0);  // Gap between columns
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW_WRAP);
    // Use START for main axis to prevent items being pushed above container
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Enable vertical scrolling for menus with many items
    lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);

    // Create menu buttons
    for (int i = 0; i < item_count && i < MAX_MENU_BUTTONS; i++) {
        // Create button with proper styling
        // Size: 200x80 to fit more items on screen (3 rows visible at once)
        lv_obj_t* btn = lv_btn_create(content);
        lv_obj_set_size(btn, 200, 80);

        // Apply styles
        lv_obj_add_style(btn, getStyleMenuCard(), 0);
        lv_obj_add_style(btn, getStyleMenuCardFocused(), LV_STATE_FOCUSED);

        // Container for icon and text
        lv_obj_t* col = lv_obj_create(btn);
        lv_obj_set_size(col, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(col, 0, 0);
        lv_obj_set_style_pad_all(col, 0, 0);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(col, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);

        // Icon (use symbol) - always use Montserrat for LVGL symbols
        lv_obj_t* icon = lv_label_create(col);
        lv_label_set_text(icon, items[i].icon);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0);  // Montserrat has LVGL symbols
        lv_obj_set_style_text_color(icon, LV_COLOR_TEXT_PRIMARY, 0);

        // Text - use theme font for menu labels
        lv_obj_t* lbl = lv_label_create(col);
        lv_label_set_text(lbl, items[i].title);
        lv_obj_set_style_text_font(lbl, getThemeFonts()->font_input, 0);  // Theme font
        lv_obj_set_style_text_color(lbl, LV_COLOR_TEXT_PRIMARY, 0);

        // Store target mode and add click handler
        lv_obj_set_user_data(btn, (void*)(intptr_t)items[i].target_mode);
        lv_obj_add_event_cb(btn, menu_item_click_handler, LV_EVENT_CLICKED, (void*)(intptr_t)items[i].target_mode);

        // Add 2D grid navigation handler for all arrow keys
        lv_obj_add_event_cb(btn, menu_grid_nav_handler, LV_EVENT_KEY, NULL);

        // Store button reference for 2D navigation
        menu_buttons[i] = btn;
        menu_button_count++;

        // Add to navigation group
        addNavigableWidget(btn);
    }

    // Ensure content scroll starts at top (fixes items appearing above screen)
    lv_obj_scroll_to_y(content, 0, LV_ANIM_OFF);

    // Footer hint - use standardized footer text
    lv_obj_t* footer = lv_label_create(screen);
    lv_label_set_text(footer, FOOTER_NAV_ENTER_ESC);
    lv_obj_set_style_text_font(footer, getThemeFonts()->font_body, 0);  // Theme font
    lv_obj_set_style_text_color(footer, LV_COLOR_WARNING, 0);  // Orange for visibility
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -5);

    current_menu_item_count = item_count;

    return screen;
}

// ============================================
// Specific Menu Screen Creators
// ============================================

/*
 * Create main menu screen
 */
lv_obj_t* createMainMenuScreen() {
    return createMenuScreen("VAIL SUMMIT", mainMenuItems, MAIN_MENU_COUNT);
}

/*
 * Create CW menu screen
 */
lv_obj_t* createCWMenuScreen() {
    return createMenuScreen("CW", cwMenuItems, CW_MENU_COUNT);
}

/*
 * Create Training menu screen
 */
lv_obj_t* createTrainingMenuScreen() {
    return createMenuScreen("TRAINING", trainingMenuItems, TRAINING_MENU_COUNT);
}

/*
 * Create Games menu screen
 */
lv_obj_t* createGamesMenuScreen() {
    return createMenuScreen("GAMES", gamesMenuItems, GAMES_MENU_COUNT);
}

/*
 * Create Settings menu screen
 */
lv_obj_t* createSettingsMenuScreen() {
    return createMenuScreen("SETTINGS", settingsMenuItems, SETTINGS_MENU_COUNT);
}

/*
 * Create Device Settings menu screen
 */
lv_obj_t* createDeviceSettingsMenuScreen() {
    return createMenuScreen("DEVICE SETTINGS", deviceSettingsMenuItems, DEVICE_SETTINGS_COUNT);
}

/*
 * Create WiFi submenu screen
 */
lv_obj_t* createWiFiSubmenuScreen() {
    return createMenuScreen("WIFI", wifiSubmenuItems, WIFI_SUBMENU_COUNT);
}

/*
 * Create General submenu screen
 */
lv_obj_t* createGeneralSubmenuScreen() {
    return createMenuScreen("GENERAL", generalSubmenuItems, GENERAL_SUBMENU_COUNT);
}

/*
 * Create Ham Tools menu screen
 */
lv_obj_t* createHamToolsMenuScreen() {
    return createMenuScreen("HAM TOOLS", hamToolsMenuItems, HAM_TOOLS_COUNT);
}

/*
 * Create Bluetooth menu screen
 */
lv_obj_t* createBluetoothMenuScreen() {
    return createMenuScreen("BLUETOOTH", bluetoothMenuItems, BLUETOOTH_MENU_COUNT);
}

/*
 * Create QSO Logger menu screen
 */
lv_obj_t* createQSOLoggerMenuScreen() {
    return createMenuScreen("QSO LOGGER", qsoLoggerMenuItems, QSO_LOGGER_COUNT);
}

// ============================================
// Coming Soon Screen
// ============================================

/*
 * Create a "Coming Soon" placeholder screen
 */
lv_obj_t* createComingSoonScreen(const char* feature_name) {
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Centered content
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, 400, 200);
    lv_obj_center(content);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(content, 15, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);

    // Feature name
    lv_obj_t* title = lv_label_create(content);
    lv_label_set_text(title, feature_name);
    lv_obj_add_style(title, getStyleLabelTitle(), 0);

    // Coming Soon text - use theme font
    lv_obj_t* coming = lv_label_create(content);
    lv_label_set_text(coming, "Coming Soon");
    lv_obj_set_style_text_color(coming, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(coming, getThemeFonts()->font_title, 0);  // Theme font

    // Description
    lv_obj_t* desc = lv_label_create(content);
    lv_label_set_text(desc, "This feature is under development");
    lv_obj_add_style(desc, getStyleLabelBody(), 0);

    // ESC instruction - use theme font
    lv_obj_t* esc = lv_label_create(content);
    lv_label_set_text(esc, "Press ESC to go back");
    lv_obj_set_style_text_color(esc, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(esc, getThemeFonts()->font_body, 0);  // Theme font

    // Invisible focusable container for ESC key handling
    // Without a navigable widget, ESC events are never processed
    lv_obj_t* focus_target = lv_obj_create(screen);
    lv_obj_set_size(focus_target, 1, 1);
    lv_obj_set_style_bg_opa(focus_target, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_target, 0, 0);
    lv_obj_add_flag(focus_target, LV_OBJ_FLAG_CLICKABLE);
    addNavigableWidget(focus_target);

    return screen;
}

// ============================================
// Menu Navigation API
// ============================================

/*
 * Set the callback for menu item selection
 */
void setMenuSelectCallback(MenuSelectCallback callback) {
    menu_select_callback = callback;
}

/*
 * Show a menu screen based on mode
 */
void showMenuForMode(int mode) {
    lv_obj_t* screen = NULL;

    switch (mode) {
        case 0:  // MODE_MAIN_MENU
            screen = createMainMenuScreen();
            break;
        case 64: // MODE_CW_MENU
            screen = createCWMenuScreen();
            break;
        case 1:  // MODE_TRAINING_MENU
            screen = createTrainingMenuScreen();
            break;
        case 34: // MODE_GAMES_MENU
            screen = createGamesMenuScreen();
            break;
        case 40: // MODE_SETTINGS_MENU
            screen = createSettingsMenuScreen();
            break;
        case 41: // MODE_DEVICE_SETTINGS_MENU
            screen = createDeviceSettingsMenuScreen();
            break;
        case 42: // MODE_WIFI_SUBMENU
            screen = createWiFiSubmenuScreen();
            break;
        case 43: // MODE_GENERAL_SUBMENU
            screen = createGeneralSubmenuScreen();
            break;
        case 65: // MODE_HAM_TOOLS_MENU
            screen = createHamToolsMenuScreen();
            break;
        case 51: // MODE_BLUETOOTH_MENU
            screen = createBluetoothMenuScreen();
            break;
        case 55: // MODE_QSO_LOGGER_MENU
            screen = createQSOLoggerMenuScreen();
            break;
        // Placeholder screens
        case 67: // MODE_BAND_PLANS
            screen = createComingSoonScreen("BAND PLANS");
            break;
        case 68: // MODE_PROPAGATION
            screen = createComingSoonScreen("PROPAGATION");
            break;
        case 69: // MODE_ANTENNAS
            screen = createComingSoonScreen("ANTENNAS");
            break;
        case 74: // MODE_SUMMIT_CHAT
            screen = createComingSoonScreen("SUMMIT CHAT");
            break;
        default:
            Serial.printf("[MenuScreens] Unknown mode: %d\n", mode);
            return;
    }

    if (screen != NULL) {
        loadScreen(screen, SCREEN_ANIM_FADE);
        current_menu_screen = screen;
    }
}

/*
 * Get the current menu screen
 */
lv_obj_t* getCurrentMenuScreen() {
    return current_menu_screen;
}

#endif // LV_MENU_SCREENS_H
