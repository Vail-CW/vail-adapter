/*
 * VAIL SUMMIT - LVGL WiFi Configuration Screen
 * Full on-device WiFi setup with network scanning, password entry, and AP mode
 */

#ifndef LV_WIFI_SCREEN_H
#define LV_WIFI_SCREEN_H

#include <lvgl.h>
#include <WiFi.h>
#include "lv_theme_summit.h"
#include "lv_widgets_summit.h"
#include "lv_screen_manager.h"
#include "../core/config.h"
#include "../settings/settings_wifi.h"

// ============================================
// WiFi Screen State Machine
// ============================================

enum WiFiLVGLState {
    LVGL_WIFI_CURRENT_CONNECTION,  // Show current WiFi connection details
    LVGL_WIFI_SCANNING,            // Scanning for networks (spinner)
    LVGL_WIFI_NETWORK_LIST,        // Scrollable list of networks
    LVGL_WIFI_PASSWORD_INPUT,      // Password entry for encrypted networks
    LVGL_WIFI_CONNECTING,          // Connection in progress
    LVGL_WIFI_CONNECTED,           // Connection successful
    LVGL_WIFI_ERROR,               // Connection failed
    LVGL_WIFI_AP_MODE,             // AP mode active
    LVGL_WIFI_RESET_CONFIRM        // Reset credentials confirmation
};

// ============================================
// Static Variables
// ============================================

static WiFiLVGLState wifi_lvgl_state = LVGL_WIFI_SCANNING;
static WiFiLVGLState wifi_lvgl_prev_state = LVGL_WIFI_SCANNING;  // For ESC navigation
static lv_obj_t* wifi_setup_screen = NULL;
static lv_obj_t* wifi_content = NULL;
static lv_obj_t* wifi_footer_label = NULL;
static lv_obj_t* wifi_password_textarea = NULL;
static lv_obj_t* wifi_loading_spinner = NULL;
static lv_obj_t* wifi_loading_label = NULL;
static lv_obj_t* wifi_network_list = NULL;  // Container for network items

static int wifi_selected_network = 0;
static bool wifi_password_visible = false;
static String wifi_error_message = "";
static String wifi_failed_ssid = "";
static bool wifi_scan_pending = false;  // Flag to trigger scan after screen loads
static lv_obj_t* wifi_password_hint = NULL;  // For partial updates of hint text

// ============================================
// Forward Declarations
// ============================================

lv_obj_t* createWiFiSetupScreen();
void updateWiFiContent();
void updateWiFiFooter();
void createCurrentConnectionView(lv_obj_t* parent);
void createScanningView(lv_obj_t* parent);
void createNetworkListView(lv_obj_t* parent);
void createPasswordInputView(lv_obj_t* parent);
void createConnectingView(lv_obj_t* parent);
void createConnectedView(lv_obj_t* parent);
void createErrorView(lv_obj_t* parent);
void createAPModeView(lv_obj_t* parent);
void createResetConfirmView(lv_obj_t* parent);
void createSignalBars(lv_obj_t* parent, int rssi, int x_offset);
void performWiFiScan();
void attemptWiFiConnection(const String& ssid, const String& password);
void updateWiFiScreen();  // Called from main loop for non-blocking connection
void startWiFiSetupLVGL();
void triggerWiFiScan();  // Deferred scan trigger
void cleanupWiFiScreen();  // Cleanup when leaving screen

// External back navigation (only used when we actually want to exit)
extern void onLVGLBackNavigation();

// ============================================
// Signal Strength Bars Helper
// ============================================

void createSignalBars(lv_obj_t* parent, int rssi, int x_offset) {
    int bars = map(rssi, -100, -40, 1, 4);
    bars = constrain(bars, 1, 4);

    for (int i = 0; i < 4; i++) {
        int barHeight = (i + 1) * 4;
        lv_obj_t* bar = lv_obj_create(parent);
        lv_obj_set_size(bar, 4, barHeight);
        lv_obj_set_pos(bar, x_offset + i * 6, 20 - barHeight);
        lv_obj_set_style_radius(bar, 1, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

        if (i < bars) {
            lv_obj_set_style_bg_color(bar, LV_COLOR_SUCCESS, 0);
        } else {
            lv_obj_set_style_bg_color(bar, lv_color_hex(0x404040), 0);
        }
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    }
}

// ============================================
// Network List Item Creation
// ============================================

static void network_item_click_handler(lv_event_t* e) {
    lv_obj_t* item = lv_event_get_target(e);
    int index = (int)(intptr_t)lv_obj_get_user_data(item);

    wifi_selected_network = index;
    wifi_lvgl_prev_state = LVGL_WIFI_NETWORK_LIST;  // Remember we came from network list

    // Load saved networks to check if this one is saved
    String savedSSIDs[3];
    String savedPasswords[3];
    int savedCount = loadAllWiFiCredentials(savedSSIDs, savedPasswords);

    bool isSaved = false;
    String savedPassword = "";
    for (int j = 0; j < savedCount; j++) {
        if (networks[index].ssid == savedSSIDs[j]) {
            isSaved = true;
            savedPassword = savedPasswords[j];
            break;
        }
    }

    if (isSaved) {
        // Connect using saved password
        wifi_lvgl_state = LVGL_WIFI_CONNECTING;
        updateWiFiContent();
        beep(TONE_SELECT, BEEP_MEDIUM);
        attemptWiFiConnection(networks[index].ssid, savedPassword);
    } else if (networks[index].encrypted) {
        // Need password
        wifi_lvgl_state = LVGL_WIFI_PASSWORD_INPUT;
        wifi_password_visible = false;
        updateWiFiContent();
        beep(TONE_SELECT, BEEP_MEDIUM);
    } else {
        // Open network - connect immediately
        wifi_lvgl_state = LVGL_WIFI_CONNECTING;
        updateWiFiContent();
        beep(TONE_SELECT, BEEP_MEDIUM);
        attemptWiFiConnection(networks[index].ssid, "");
    }
}

// Key handler for network list items (handles ENTER, ESC, UP/DOWN, and special keys)
static void network_item_key_handler(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    uint32_t key = lv_event_get_key(e);
    lv_obj_t* item = lv_event_get_target(e);

    if (key == LV_KEY_ENTER) {
        // Trigger the click handler
        lv_event_send(item, LV_EVENT_CLICKED, NULL);
    } else if (key == LV_KEY_UP || key == LV_KEY_PREV) {
        // Move to previous network item
        int index = (int)(intptr_t)lv_obj_get_user_data(item);
        if (index > 0 && wifi_network_list) {
            lv_obj_t* prev_item = lv_obj_get_child(wifi_network_list, index - 1);
            if (prev_item) {
                lv_group_focus_obj(prev_item);
                lv_obj_scroll_to_view(prev_item, LV_ANIM_ON);
            }
        }
    } else if (key == LV_KEY_DOWN || key == LV_KEY_NEXT) {
        // Move to next network item
        int index = (int)(intptr_t)lv_obj_get_user_data(item);
        if (index < networkCount - 1 && wifi_network_list) {
            lv_obj_t* next_item = lv_obj_get_child(wifi_network_list, index + 1);
            if (next_item) {
                lv_group_focus_obj(next_item);
                lv_obj_scroll_to_view(next_item, LV_ANIM_ON);
            }
        }
    } else if (key == LV_KEY_ESC) {
        // Exit WiFi setup
        onLVGLBackNavigation();
        lv_event_stop_processing(e);  // Prevent global ESC handler from also firing
    } else if (key == 'a' || key == 'A') {
        // Start AP mode
        startAPMode();
        wifi_lvgl_state = LVGL_WIFI_AP_MODE;
        updateWiFiContent();
        beep(TONE_SELECT, BEEP_MEDIUM);
    } else if (key == 'r' || key == 'R') {
        // Show reset confirmation
        wifi_lvgl_state = LVGL_WIFI_RESET_CONFIRM;
        updateWiFiContent();
        beep(TONE_MENU_NAV, BEEP_SHORT);
    } else if (key == 's' || key == 'S') {
        // Rescan
        wifi_lvgl_state = LVGL_WIFI_SCANNING;
        updateWiFiContent();
        beep(TONE_SELECT, BEEP_MEDIUM);
        triggerWiFiScan();
    }
}

lv_obj_t* createNetworkListItem(lv_obj_t* parent, int index, bool isSaved) {
    lv_obj_t* item = lv_obj_create(parent);
    lv_obj_set_size(item, lv_pct(100), 42);
    lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);

    // Style
    lv_obj_set_style_bg_color(item, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_bg_color(item, LV_COLOR_CARD_TEAL, LV_STATE_FOCUSED);
    lv_obj_set_style_bg_opa(item, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(item, 6, 0);
    lv_obj_set_style_border_width(item, 1, 0);
    lv_obj_set_style_border_color(item, LV_COLOR_BORDER_SUBTLE, 0);
    lv_obj_set_style_border_color(item, LV_COLOR_ACCENT_CYAN, LV_STATE_FOCUSED);
    lv_obj_set_style_pad_all(item, 8, 0);

    // Signal bars
    createSignalBars(item, networks[index].rssi, 5);

    // Lock icon if encrypted (positioned after signal bars)
    int textStartX = 35;
    if (networks[index].encrypted) {
        lv_obj_t* lock = lv_label_create(item);
        lv_label_set_text(lock, LV_SYMBOL_EYE_CLOSE);
        lv_obj_set_style_text_color(lock, LV_COLOR_WARNING, 0);
        lv_obj_set_style_text_color(lock, lv_color_hex(0x1A1A2E), LV_STATE_FOCUSED);
        lv_obj_set_style_text_font(lock, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(lock, 35, 10);
        textStartX = 55;
    }

    // Star for saved network
    if (isSaved) {
        lv_obj_t* star = lv_label_create(item);
        lv_label_set_text(star, "*");
        lv_obj_set_style_text_color(star, LV_COLOR_WARNING, 0);
        lv_obj_set_style_text_color(star, lv_color_hex(0x1A1A2E), LV_STATE_FOCUSED);
        lv_obj_set_style_text_font(star, &lv_font_montserrat_18, 0);
        lv_obj_set_pos(star, textStartX, 8);
        textStartX += 15;
    }

    // SSID label
    lv_obj_t* ssid_label = lv_label_create(item);
    String ssid = networks[index].ssid;
    if (ssid.length() > 28) {
        ssid = ssid.substring(0, 25) + "...";
    }
    lv_label_set_text(ssid_label, ssid.c_str());
    lv_obj_set_style_text_color(ssid_label, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_color(ssid_label, lv_color_hex(0x1A1A2E), LV_STATE_FOCUSED);
    lv_obj_set_style_text_font(ssid_label, getThemeFonts()->font_body, 0);
    lv_obj_set_pos(ssid_label, textStartX, 10);

    // Store index in user data
    lv_obj_set_user_data(item, (void*)(intptr_t)index);
    lv_obj_add_event_cb(item, network_item_click_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(item, network_item_key_handler, LV_EVENT_KEY, NULL);

    return item;
}

// ============================================
// Global Key Event Handler (for special keys like A, R, S, ESC)
// ============================================

static void wifi_global_key_handler(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    uint32_t key = lv_event_get_key(e);

    // Handle ESC key for internal navigation
    if (key == LV_KEY_ESC) {
        switch (wifi_lvgl_state) {
            case LVGL_WIFI_PASSWORD_INPUT:
                // Go back to network list
                wifi_lvgl_state = LVGL_WIFI_NETWORK_LIST;
                updateWiFiContent();
                beep(TONE_MENU_NAV, BEEP_SHORT);
                return;  // Don't let ESC bubble up

            case LVGL_WIFI_CONNECTED:
                // Go to current connection view
                wifi_lvgl_state = LVGL_WIFI_CURRENT_CONNECTION;
                updateWiFiContent();
                beep(TONE_MENU_NAV, BEEP_SHORT);
                return;

            case LVGL_WIFI_ERROR:
                // Go back to network list
                wifi_lvgl_state = LVGL_WIFI_NETWORK_LIST;
                wifi_failed_ssid = "";
                updateWiFiContent();
                beep(TONE_MENU_NAV, BEEP_SHORT);
                return;

            case LVGL_WIFI_RESET_CONFIRM:
                // Cancel - go back to network list
                wifi_lvgl_state = LVGL_WIFI_NETWORK_LIST;
                updateWiFiContent();
                beep(TONE_MENU_NAV, BEEP_SHORT);
                return;

            case LVGL_WIFI_AP_MODE:
                // ESC from AP mode goes back to network list (AP stays active)
                wifi_lvgl_state = LVGL_WIFI_NETWORK_LIST;
                updateWiFiContent();
                beep(TONE_MENU_NAV, BEEP_SHORT);
                return;

            case LVGL_WIFI_CURRENT_CONNECTION:
            case LVGL_WIFI_NETWORK_LIST:
                // These states allow ESC to exit the WiFi setup entirely
                onLVGLBackNavigation();
                return;

            default:
                // For scanning/connecting states, ignore ESC
                return;
        }
    }

    // Handle other special keys based on state
    switch (wifi_lvgl_state) {
        case LVGL_WIFI_CURRENT_CONNECTION:
            if (key == 'c' || key == 'C') {
                // Change networks - show scanning then scan
                wifi_lvgl_state = LVGL_WIFI_SCANNING;
                updateWiFiContent();
                beep(TONE_SELECT, BEEP_MEDIUM);
                triggerWiFiScan();
            } else if (key == 'a' || key == 'A') {
                // Start AP mode
                startAPMode();
                wifi_lvgl_state = LVGL_WIFI_AP_MODE;
                updateWiFiContent();
                beep(TONE_SELECT, BEEP_MEDIUM);
            }
            break;

        case LVGL_WIFI_NETWORK_LIST:
            if (key == 'a' || key == 'A') {
                // Start AP mode
                startAPMode();
                wifi_lvgl_state = LVGL_WIFI_AP_MODE;
                updateWiFiContent();
                beep(TONE_SELECT, BEEP_MEDIUM);
            } else if (key == 'r' || key == 'R') {
                // Show reset confirmation
                wifi_lvgl_state = LVGL_WIFI_RESET_CONFIRM;
                updateWiFiContent();
                beep(TONE_MENU_NAV, BEEP_SHORT);
            } else if (key == 's' || key == 'S') {
                // Rescan
                wifi_lvgl_state = LVGL_WIFI_SCANNING;
                updateWiFiContent();
                beep(TONE_SELECT, BEEP_MEDIUM);
                triggerWiFiScan();
            }
            break;

        case LVGL_WIFI_PASSWORD_INPUT:
            if (key == '\t' || key == 9) {
                // Toggle password visibility
                wifi_password_visible = !wifi_password_visible;
                if (wifi_password_textarea) {
                    lv_textarea_set_password_mode(wifi_password_textarea, !wifi_password_visible);
                }
                // Update hint text
                updateWiFiContent();
                beep(TONE_MENU_NAV, BEEP_SHORT);
            } else if (key == LV_KEY_ENTER) {
                // Attempt connection
                if (wifi_password_textarea) {
                    String password = lv_textarea_get_text(wifi_password_textarea);
                    wifi_lvgl_state = LVGL_WIFI_CONNECTING;
                    updateWiFiContent();
                    beep(TONE_SELECT, BEEP_MEDIUM);
                    attemptWiFiConnection(networks[wifi_selected_network].ssid, password);
                }
            }
            break;

        case LVGL_WIFI_CONNECTED:
            if (key == LV_KEY_ENTER) {
                // Return to show current connection
                wifi_lvgl_state = LVGL_WIFI_CURRENT_CONNECTION;
                updateWiFiContent();
            }
            break;

        case LVGL_WIFI_ERROR:
            if (key == 'p' || key == 'P') {
                // Retry password if this was a password error
                if (wifi_failed_ssid.length() > 0) {
                    for (int i = 0; i < networkCount; i++) {
                        if (networks[i].ssid == wifi_failed_ssid) {
                            wifi_selected_network = i;
                            wifi_lvgl_state = LVGL_WIFI_PASSWORD_INPUT;
                            wifi_password_visible = false;
                            wifi_failed_ssid = "";
                            updateWiFiContent();
                            beep(TONE_SELECT, BEEP_MEDIUM);
                            break;
                        }
                    }
                }
            } else if (key == LV_KEY_ENTER) {
                // Rescan
                wifi_failed_ssid = "";
                wifi_lvgl_state = LVGL_WIFI_SCANNING;
                updateWiFiContent();
                beep(TONE_SELECT, BEEP_MEDIUM);
                triggerWiFiScan();
            }
            break;

        case LVGL_WIFI_AP_MODE:
            if (key == 'a' || key == 'A') {
                // Disable AP mode and rescan
                stopAPMode();
                wifi_lvgl_state = LVGL_WIFI_SCANNING;
                updateWiFiContent();
                beep(TONE_MENU_NAV, BEEP_SHORT);
                triggerWiFiScan();
            }
            break;

        case LVGL_WIFI_RESET_CONFIRM:
            if (key == 'y' || key == 'Y') {
                // Confirm reset
                resetWiFiSettings();
                beep(TONE_ERROR, BEEP_LONG);
                wifi_error_message = "WiFi settings erased";
                wifi_lvgl_state = LVGL_WIFI_ERROR;
                updateWiFiContent();
            } else if (key == 'n' || key == 'N') {
                // Cancel - back to network list
                wifi_lvgl_state = LVGL_WIFI_NETWORK_LIST;
                updateWiFiContent();
                beep(TONE_MENU_NAV, BEEP_SHORT);
            }
            break;

        default:
            break;
    }
}

// ============================================
// Key handler for views without list items (current connection, AP mode, etc.)
// ============================================

static void wifi_view_key_handler(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    uint32_t key = lv_event_get_key(e);

    switch (wifi_lvgl_state) {
        case LVGL_WIFI_CURRENT_CONNECTION:
            if (key == 'c' || key == 'C') {
                // Change networks - show scanning then scan
                wifi_lvgl_state = LVGL_WIFI_SCANNING;
                updateWiFiContent();
                beep(TONE_SELECT, BEEP_MEDIUM);
                triggerWiFiScan();
            } else if (key == 'a' || key == 'A') {
                // Start AP mode
                startAPMode();
                wifi_lvgl_state = LVGL_WIFI_AP_MODE;
                updateWiFiContent();
                beep(TONE_SELECT, BEEP_MEDIUM);
            } else if (key == LV_KEY_ESC) {
                onLVGLBackNavigation();
            }
            break;

        case LVGL_WIFI_AP_MODE:
            if (key == 'a' || key == 'A') {
                // Disable AP mode and rescan
                stopAPMode();
                wifi_lvgl_state = LVGL_WIFI_SCANNING;
                updateWiFiContent();
                beep(TONE_MENU_NAV, BEEP_SHORT);
                triggerWiFiScan();
            } else if (key == LV_KEY_ESC) {
                // ESC from AP mode goes back to network list (AP stays active)
                wifi_lvgl_state = LVGL_WIFI_NETWORK_LIST;
                updateWiFiContent();
                beep(TONE_MENU_NAV, BEEP_SHORT);
            }
            break;

        case LVGL_WIFI_CONNECTED:
            if (key == LV_KEY_ENTER || key == LV_KEY_ESC) {
                wifi_lvgl_state = LVGL_WIFI_CURRENT_CONNECTION;
                updateWiFiContent();
                beep(TONE_MENU_NAV, BEEP_SHORT);
            }
            break;

        case LVGL_WIFI_ERROR:
            if (key == 'p' || key == 'P') {
                if (wifi_failed_ssid.length() > 0) {
                    for (int i = 0; i < networkCount; i++) {
                        if (networks[i].ssid == wifi_failed_ssid) {
                            wifi_selected_network = i;
                            wifi_lvgl_state = LVGL_WIFI_PASSWORD_INPUT;
                            wifi_password_visible = false;
                            wifi_failed_ssid = "";
                            updateWiFiContent();
                            beep(TONE_SELECT, BEEP_MEDIUM);
                            break;
                        }
                    }
                }
            } else if (key == LV_KEY_ENTER) {
                wifi_failed_ssid = "";
                wifi_lvgl_state = LVGL_WIFI_SCANNING;
                updateWiFiContent();
                beep(TONE_SELECT, BEEP_MEDIUM);
                triggerWiFiScan();
            } else if (key == LV_KEY_ESC) {
                wifi_lvgl_state = LVGL_WIFI_NETWORK_LIST;
                wifi_failed_ssid = "";
                updateWiFiContent();
                beep(TONE_MENU_NAV, BEEP_SHORT);
            }
            break;

        case LVGL_WIFI_RESET_CONFIRM:
            if (key == 'y' || key == 'Y') {
                resetWiFiSettings();
                beep(TONE_ERROR, BEEP_LONG);
                wifi_error_message = "WiFi settings erased";
                wifi_lvgl_state = LVGL_WIFI_ERROR;
                updateWiFiContent();
            } else if (key == 'n' || key == 'N' || key == LV_KEY_ESC) {
                wifi_lvgl_state = LVGL_WIFI_NETWORK_LIST;
                updateWiFiContent();
                beep(TONE_MENU_NAV, BEEP_SHORT);
            }
            break;

        default:
            break;
    }
}

// Create an invisible key receiver widget for views without focusable items
static lv_obj_t* createKeyReceiver(lv_obj_t* parent) {
    lv_obj_t* receiver = lv_obj_create(parent);
    lv_obj_set_size(receiver, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(receiver, 0, 0);
    lv_obj_set_style_bg_opa(receiver, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(receiver, 0, 0);
    lv_obj_clear_flag(receiver, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(receiver, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(receiver, wifi_view_key_handler, LV_EVENT_KEY, NULL);
    addNavigableWidget(receiver);
    return receiver;
}

// ============================================
// Content View Implementations
// ============================================

void createCurrentConnectionView(lv_obj_t* parent) {
    // Key receiver for this view
    createKeyReceiver(parent);

    // Connected status
    lv_obj_t* status = lv_label_create(parent);
    lv_label_set_text(status, "WiFi Connected");
    lv_obj_set_style_text_color(status, LV_COLOR_SUCCESS, 0);
    lv_obj_set_style_text_font(status, getThemeFonts()->font_subtitle, 0);
    lv_obj_align(status, LV_ALIGN_TOP_MID, 0, 10);

    // Info card
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, lv_pct(90), 130);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 45);
    applyCardStyle(card);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(card, 15, 0);
    lv_obj_set_style_pad_row(card, 8, 0);

    // Network name row
    lv_obj_t* ssid_row = lv_obj_create(card);
    lv_obj_set_size(ssid_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(ssid_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ssid_row, 0, 0);
    lv_obj_set_style_pad_all(ssid_row, 0, 0);
    lv_obj_clear_flag(ssid_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* ssid_lbl = lv_label_create(ssid_row);
    lv_label_set_text(ssid_lbl, "Network:");
    lv_obj_add_style(ssid_lbl, getStyleLabelBody(), 0);
    lv_obj_align(ssid_lbl, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t* ssid_val = lv_label_create(ssid_row);
    String ssid = WiFi.SSID();
    if (ssid.length() > 20) ssid = ssid.substring(0, 17) + "...";
    lv_label_set_text(ssid_val, ssid.c_str());
    lv_obj_set_style_text_color(ssid_val, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(ssid_val, getThemeFonts()->font_input, 0);
    lv_obj_align(ssid_val, LV_ALIGN_RIGHT_MID, 0, 0);

    // IP address row
    lv_obj_t* ip_row = lv_obj_create(card);
    lv_obj_set_size(ip_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(ip_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ip_row, 0, 0);
    lv_obj_set_style_pad_all(ip_row, 0, 0);
    lv_obj_clear_flag(ip_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* ip_lbl = lv_label_create(ip_row);
    lv_label_set_text(ip_lbl, "IP Address:");
    lv_obj_add_style(ip_lbl, getStyleLabelBody(), 0);
    lv_obj_align(ip_lbl, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t* ip_val = lv_label_create(ip_row);
    lv_label_set_text(ip_val, WiFi.localIP().toString().c_str());
    lv_obj_set_style_text_color(ip_val, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(ip_val, getThemeFonts()->font_input, 0);
    lv_obj_align(ip_val, LV_ALIGN_RIGHT_MID, 0, 0);

    // Signal strength row
    lv_obj_t* sig_row = lv_obj_create(card);
    lv_obj_set_size(sig_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(sig_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sig_row, 0, 0);
    lv_obj_set_style_pad_all(sig_row, 0, 0);
    lv_obj_clear_flag(sig_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* sig_lbl = lv_label_create(sig_row);
    lv_label_set_text(sig_lbl, "Signal:");
    lv_obj_add_style(sig_lbl, getStyleLabelBody(), 0);
    lv_obj_align(sig_lbl, LV_ALIGN_LEFT_MID, 0, 0);

    // Signal bars container
    lv_obj_t* sig_bars = lv_obj_create(sig_row);
    lv_obj_set_size(sig_bars, 50, 25);
    lv_obj_set_style_bg_opa(sig_bars, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sig_bars, 0, 0);
    lv_obj_clear_flag(sig_bars, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(sig_bars, LV_ALIGN_RIGHT_MID, 0, 0);
    createSignalBars(sig_bars, WiFi.RSSI(), 5);

    lv_obj_t* rssi_val = lv_label_create(sig_row);
    char rssi_str[16];
    snprintf(rssi_str, sizeof(rssi_str), "%d dBm", WiFi.RSSI());
    lv_label_set_text(rssi_val, rssi_str);
    lv_obj_set_style_text_color(rssi_val, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(rssi_val, getThemeFonts()->font_small, 0);
    lv_obj_align(rssi_val, LV_ALIGN_RIGHT_MID, -55, 0);
}

void createScanningView(lv_obj_t* parent) {
    // Scanning message
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, "Scanning for networks...");
    lv_obj_set_style_text_color(label, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(label, getThemeFonts()->font_subtitle, 0);
    lv_obj_center(label);

    // Spinner
    wifi_loading_spinner = lv_spinner_create(parent, 1000, 60);
    lv_obj_set_size(wifi_loading_spinner, 50, 50);
    lv_obj_align(wifi_loading_spinner, LV_ALIGN_CENTER, 0, 50);
}

void createNetworkListView(lv_obj_t* parent) {
    // Load saved networks for star indicator
    String savedSSIDs[3];
    String savedPasswords[3];
    int savedCount = loadAllWiFiCredentials(savedSSIDs, savedPasswords);

    // Title
    lv_obj_t* title = lv_label_create(parent);
    char title_text[32];
    snprintf(title_text, sizeof(title_text), "Available Networks (%d)", networkCount);
    lv_label_set_text(title, title_text);
    lv_obj_add_style(title, getStyleLabelSubtitle(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 5, 5);

    // Scrollable list container
    wifi_network_list = lv_obj_create(parent);
    lv_obj_set_size(wifi_network_list, lv_pct(100), 160);
    lv_obj_align(wifi_network_list, LV_ALIGN_TOP_LEFT, 0, 35);
    lv_obj_set_layout(wifi_network_list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(wifi_network_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(wifi_network_list, 5, 0);
    lv_obj_set_style_pad_all(wifi_network_list, 5, 0);
    lv_obj_set_style_bg_opa(wifi_network_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wifi_network_list, 0, 0);
    lv_obj_add_flag(wifi_network_list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(wifi_network_list, LV_SCROLLBAR_MODE_AUTO);

    // Create network items
    for (int i = 0; i < networkCount; i++) {
        // Check if saved
        bool isSaved = false;
        for (int j = 0; j < savedCount; j++) {
            if (networks[i].ssid == savedSSIDs[j]) {
                isSaved = true;
                break;
            }
        }

        lv_obj_t* item = createNetworkListItem(wifi_network_list, i, isSaved);
        addNavigableWidget(item);
    }

    // If no networks found, show empty message
    if (networkCount == 0) {
        lv_obj_t* empty = lv_label_create(wifi_network_list);
        lv_label_set_text(empty, "No networks found.\nPress 'S' to scan again.");
        lv_obj_add_style(empty, getStyleLabelBody(), 0);
        lv_obj_set_style_text_align(empty, LV_TEXT_ALIGN_CENTER, 0);
    }
}

// Password textarea key handler - intercepts Enter, Tab, and ESC before default handling
static void password_textarea_key_handler(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    uint32_t key = lv_event_get_key(e);

    if (key == LV_KEY_ENTER) {
        // Attempt connection
        if (wifi_password_textarea) {
            String password = lv_textarea_get_text(wifi_password_textarea);
            wifi_lvgl_state = LVGL_WIFI_CONNECTING;
            updateWiFiContent();
            beep(TONE_SELECT, BEEP_MEDIUM);
            attemptWiFiConnection(networks[wifi_selected_network].ssid, password);
        }
        lv_event_stop_bubbling(e);  // Prevent default textarea handling
    } else if (key == '\t' || key == 9) {
        // Toggle password visibility
        wifi_password_visible = !wifi_password_visible;
        if (wifi_password_textarea) {
            lv_textarea_set_password_mode(wifi_password_textarea, !wifi_password_visible);
        }
        // Update hint label directly instead of full rebuild
        if (wifi_password_hint) {
            lv_label_set_text(wifi_password_hint, wifi_password_visible ? "TAB: Hide password" : "TAB: Show password");
        }
        beep(TONE_MENU_NAV, BEEP_SHORT);
        lv_event_stop_bubbling(e);  // Prevent default textarea handling
    } else if (key == LV_KEY_ESC) {
        // Go back to network list
        wifi_lvgl_state = LVGL_WIFI_NETWORK_LIST;
        updateWiFiContent();
        beep(TONE_MENU_NAV, BEEP_SHORT);
        lv_event_stop_bubbling(e);  // Prevent default handling
    }
}

void createPasswordInputView(lv_obj_t* parent) {
    // Network name
    lv_obj_t* ssid_label = lv_label_create(parent);
    char label_text[64];
    String ssid = networks[wifi_selected_network].ssid;
    if (ssid.length() > 25) ssid = ssid.substring(0, 22) + "...";
    snprintf(label_text, sizeof(label_text), "Connect to: %s", ssid.c_str());
    lv_label_set_text(ssid_label, label_text);
    lv_obj_add_style(ssid_label, getStyleLabelSubtitle(), 0);
    lv_obj_align(ssid_label, LV_ALIGN_TOP_MID, 0, 15);

    // Password label
    lv_obj_t* pw_label = lv_label_create(parent);
    lv_label_set_text(pw_label, "Password:");
    lv_obj_add_style(pw_label, getStyleLabelBody(), 0);
    lv_obj_align(pw_label, LV_ALIGN_TOP_LEFT, 20, 55);

    // Password input
    wifi_password_textarea = lv_textarea_create(parent);
    lv_obj_set_size(wifi_password_textarea, lv_pct(85), 50);
    lv_obj_align(wifi_password_textarea, LV_ALIGN_TOP_MID, 0, 80);
    lv_textarea_set_one_line(wifi_password_textarea, true);
    lv_textarea_set_password_mode(wifi_password_textarea, !wifi_password_visible);
    lv_textarea_set_max_length(wifi_password_textarea, 63);
    lv_textarea_set_placeholder_text(wifi_password_textarea, "Enter WiFi password");
    lv_obj_add_style(wifi_password_textarea, getStyleTextarea(), 0);
    lv_obj_set_style_text_font(wifi_password_textarea, getThemeFonts()->font_input, 0);

    // Add key handler BEFORE adding to navigation group so it processes keys first
    lv_obj_add_event_cb(wifi_password_textarea, password_textarea_key_handler, LV_EVENT_KEY, NULL);
    addNavigableWidget(wifi_password_textarea);

    // Auto-focus the password textarea for immediate input
    focusWidget(wifi_password_textarea);

    // Visibility toggle hint (store reference for partial updates)
    wifi_password_hint = lv_label_create(parent);
    lv_label_set_text(wifi_password_hint, wifi_password_visible ? "TAB: Hide password" : "TAB: Show password");
    lv_obj_set_style_text_color(wifi_password_hint, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(wifi_password_hint, getThemeFonts()->font_small, 0);
    lv_obj_align(wifi_password_hint, LV_ALIGN_TOP_MID, 0, 140);
}

void createConnectingView(lv_obj_t* parent) {
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, "Connecting...");
    lv_obj_set_style_text_color(label, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(label, getThemeFonts()->font_subtitle, 0);
    lv_obj_center(label);

    // Spinner
    lv_obj_t* spinner = lv_spinner_create(parent, 1000, 60);
    lv_obj_set_size(spinner, 50, 50);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 50);
}

void createConnectedView(lv_obj_t* parent) {
    // Key receiver for this view
    createKeyReceiver(parent);

    lv_obj_t* icon = lv_label_create(parent);
    lv_label_set_text(icon, LV_SYMBOL_OK);
    lv_obj_set_style_text_color(icon, LV_COLOR_SUCCESS, 0);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, "Connected!");
    lv_obj_set_style_text_color(label, LV_COLOR_SUCCESS, 0);
    lv_obj_set_style_text_font(label, getThemeFonts()->font_subtitle, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 10);

    lv_obj_t* ip = lv_label_create(parent);
    char ip_text[32];
    snprintf(ip_text, sizeof(ip_text), "IP: %s", WiFi.localIP().toString().c_str());
    lv_label_set_text(ip, ip_text);
    lv_obj_set_style_text_color(ip, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(ip, getThemeFonts()->font_body, 0);
    lv_obj_align(ip, LV_ALIGN_CENTER, 0, 50);
}

void createErrorView(lv_obj_t* parent) {
    // Key receiver for this view
    createKeyReceiver(parent);

    lv_obj_t* icon = lv_label_create(parent);
    lv_label_set_text(icon, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(icon, LV_COLOR_ERROR, 0);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, "Connection Failed");
    lv_obj_set_style_text_color(label, LV_COLOR_ERROR, 0);
    lv_obj_set_style_text_font(label, getThemeFonts()->font_subtitle, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* msg = lv_label_create(parent);
    lv_label_set_text(msg, wifi_error_message.c_str());
    lv_obj_add_style(msg, getStyleLabelBody(), 0);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, 40);
}

void createAPModeView(lv_obj_t* parent) {
    // Key receiver for this view
    createKeyReceiver(parent);

    // Title
    lv_obj_t* title = lv_label_create(parent);
    lv_label_set_text(title, "AP Mode Active");
    lv_obj_set_style_text_color(title, LV_COLOR_SUCCESS, 0);
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Info card - increased height from 140 to 180 to fit all content
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, lv_pct(90), 180);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 45);
    applyCardStyle(card);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_set_style_pad_row(card, 6, 0);

    // SSID
    lv_obj_t* ssid_lbl = lv_label_create(card);
    lv_label_set_text(ssid_lbl, "Network Name (SSID):");
    lv_obj_add_style(ssid_lbl, getStyleLabelBody(), 0);

    lv_obj_t* ssid_val = lv_label_create(card);
    lv_label_set_text(ssid_val, WiFi.softAPSSID().c_str());
    lv_obj_set_style_text_color(ssid_val, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(ssid_val, getThemeFonts()->font_input, 0);

    // Password
    lv_obj_t* pw_lbl = lv_label_create(card);
    lv_label_set_text(pw_lbl, "Password:");
    lv_obj_add_style(pw_lbl, getStyleLabelBody(), 0);

    lv_obj_t* pw_val = lv_label_create(card);
    lv_label_set_text(pw_val, apPassword.c_str());
    lv_obj_set_style_text_color(pw_val, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_text_font(pw_val, getThemeFonts()->font_input, 0);

    // Instructions
    lv_obj_t* instr = lv_label_create(card);
    lv_label_set_text(instr, "Connect and browse to:\nhttp://192.168.4.1");
    lv_obj_add_style(instr, getStyleLabelBody(), 0);
    lv_obj_set_style_text_align(instr, LV_TEXT_ALIGN_CENTER, 0);
}

void createResetConfirmView(lv_obj_t* parent) {
    // Key receiver for this view
    createKeyReceiver(parent);

    lv_obj_t* icon = lv_label_create(parent);
    lv_label_set_text(icon, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(icon, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -50);

    lv_obj_t* title = lv_label_create(parent);
    lv_label_set_text(title, "Reset WiFi Settings?");
    lv_obj_set_style_text_color(title, LV_COLOR_ERROR, 0);
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t* msg = lv_label_create(parent);
    lv_label_set_text(msg, "This will erase ALL saved\nWiFi network credentials.\nThis cannot be undone.");
    lv_obj_add_style(msg, getStyleLabelBody(), 0);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, 50);
}

// ============================================
// WiFi Operations
// ============================================

// Timer callback for deferred scan
static void wifi_scan_timer_cb(lv_timer_t* timer) {
    lv_timer_del(timer);  // One-shot timer

    // Check if screen was destroyed (navigated away)
    if (!wifi_content) {
        Serial.println("[WiFi LVGL] Timer fired but screen destroyed, aborting");
        return;
    }

    if (wifi_lvgl_state != LVGL_WIFI_SCANNING) return;  // State changed, abort

    Serial.println("[WiFi LVGL] Starting network scan...");

    // Perform the blocking scan
    scanNetworks();

    Serial.printf("[WiFi LVGL] Scan complete, found %d networks\n", networkCount);

    // Update state based on results
    if (networkCount > 0) {
        wifi_lvgl_state = LVGL_WIFI_NETWORK_LIST;
        wifi_selected_network = 0;
    } else {
        wifi_lvgl_state = LVGL_WIFI_ERROR;
        wifi_error_message = "No networks found.\nTry scanning again.";
    }

    updateWiFiContent();
}

void triggerWiFiScan() {
    // Schedule scan for next LVGL tick so UI can update first
    lv_timer_create(wifi_scan_timer_cb, 50, NULL);  // 50ms delay
}

void performWiFiScan() {
    // Show scanning UI first
    wifi_lvgl_state = LVGL_WIFI_SCANNING;
    updateWiFiContent();

    // Trigger deferred scan
    triggerWiFiScan();
}

void attemptWiFiConnection(const String& ssid, const String& password) {
    // Non-blocking: just request the connection
    // UI is already set to LVGL_WIFI_CONNECTING by caller
    requestWiFiConnection(ssid, password);
    // Main loop will poll updateWiFiConnection() and call updateWiFiScreen()
}

// Called from main loop to check connection progress and update UI
void updateWiFiScreen() {
    // Only process if we're in connecting state
    if (wifi_lvgl_state != LVGL_WIFI_CONNECTING) {
        return;
    }

    // Check for connection state changes
    WiFiConnectionState connState = getWiFiConnectionState();

    if (connState == WIFI_CONN_SUCCESS) {
        wifi_lvgl_state = LVGL_WIFI_CONNECTED;
        wifi_failed_ssid = "";
        clearWiFiConnectionState();
        beep(TONE_SUCCESS, BEEP_LONG);
        updateWiFiContent();
    } else if (connState == WIFI_CONN_FAILED) {
        wifi_lvgl_state = LVGL_WIFI_ERROR;
        wifi_error_message = "Connection failed.\nCheck password and try again.";
        wifi_failed_ssid = wifiConnRequest.ssid;  // Get SSID before clearing
        clearWiFiConnectionState();
        beep(TONE_ERROR, BEEP_LONG);
        updateWiFiContent();
    }
    // If still WIFI_CONN_STARTING or WIFI_CONN_REQUESTED, spinner continues
}

// ============================================
// Footer Update
// ============================================

void updateWiFiFooter() {
    if (!wifi_footer_label) return;

    const char* text = "";
    switch (wifi_lvgl_state) {
        case LVGL_WIFI_CURRENT_CONNECTION:
            text = "C: Change    A: AP Mode    ESC: Back";
            break;
        case LVGL_WIFI_SCANNING:
            text = "Scanning...";
            break;
        case LVGL_WIFI_NETWORK_LIST:
            text = "ENTER: Connect   A: AP   R: Reset   S: Scan   ESC: Back";
            break;
        case LVGL_WIFI_PASSWORD_INPUT:
            text = "ENTER: Connect    TAB: Show/Hide    ESC: Back";
            break;
        case LVGL_WIFI_CONNECTING:
            text = "Connecting...";
            break;
        case LVGL_WIFI_CONNECTED:
            text = "ENTER or ESC to continue";
            break;
        case LVGL_WIFI_ERROR:
            if (wifi_failed_ssid.length() > 0) {
                text = "P: Retry Password    ENTER: Rescan    ESC: Back";
            } else {
                text = "ENTER: Rescan    ESC: Back";
            }
            break;
        case LVGL_WIFI_AP_MODE:
            text = "A: Disable AP Mode    ESC: Back";
            break;
        case LVGL_WIFI_RESET_CONFIRM:
            text = "Y: Yes, erase all    N/ESC: Cancel";
            break;
    }

    lv_label_set_text(wifi_footer_label, text);
}

// ============================================
// Content Update
// ============================================

void updateWiFiContent() {
    if (!wifi_content) return;

    // Clear previous content
    lv_obj_clean(wifi_content);
    clearNavigationGroup();

    // Reset pointers
    wifi_network_list = NULL;
    if (wifi_lvgl_state != LVGL_WIFI_PASSWORD_INPUT) {
        wifi_password_textarea = NULL;
        wifi_password_hint = NULL;
    }

    // Rebuild based on state
    switch (wifi_lvgl_state) {
        case LVGL_WIFI_CURRENT_CONNECTION:
            createCurrentConnectionView(wifi_content);
            break;
        case LVGL_WIFI_SCANNING:
            createScanningView(wifi_content);
            break;
        case LVGL_WIFI_NETWORK_LIST:
            createNetworkListView(wifi_content);
            break;
        case LVGL_WIFI_PASSWORD_INPUT:
            createPasswordInputView(wifi_content);
            break;
        case LVGL_WIFI_CONNECTING:
            createConnectingView(wifi_content);
            break;
        case LVGL_WIFI_CONNECTED:
            createConnectedView(wifi_content);
            break;
        case LVGL_WIFI_ERROR:
            createErrorView(wifi_content);
            break;
        case LVGL_WIFI_AP_MODE:
            createAPModeView(wifi_content);
            break;
        case LVGL_WIFI_RESET_CONFIRM:
            createResetConfirmView(wifi_content);
            break;
    }

    // Update footer text
    updateWiFiFooter();
}

// ============================================
// Main Screen Creation
// ============================================

lv_obj_t* createWiFiSetupScreen() {
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "WIFI SETUP");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar (WiFi + battery) on the right side
    createCompactStatusBar(screen);

    // Main content area
    wifi_content = lv_obj_create(screen);
    lv_obj_set_size(wifi_content, SCREEN_WIDTH - 20, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 10);
    lv_obj_set_pos(wifi_content, 10, HEADER_HEIGHT + 5);
    lv_obj_set_style_bg_opa(wifi_content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wifi_content, 0, 0);
    lv_obj_clear_flag(wifi_content, LV_OBJ_FLAG_SCROLLABLE);

    // Footer
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    wifi_footer_label = lv_label_create(footer);
    lv_obj_set_style_text_color(wifi_footer_label, LV_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(wifi_footer_label, getThemeFonts()->font_small, 0);
    lv_obj_center(wifi_footer_label);

    // Register global key handler on the screen itself
    // Note: Do NOT add screen to navigation group - only actual UI widgets should be navigable
    // The screen's event handler will receive events that bubble up from focused widgets
    lv_obj_add_event_cb(screen, wifi_global_key_handler, LV_EVENT_KEY, NULL);

    // Determine initial state
    if (WiFi.status() == WL_CONNECTED) {
        wifi_lvgl_state = LVGL_WIFI_CURRENT_CONNECTION;
        updateWiFiContent();
    } else {
        // Show scanning UI immediately, then trigger scan
        wifi_lvgl_state = LVGL_WIFI_SCANNING;
        updateWiFiContent();
        triggerWiFiScan();
    }

    wifi_setup_screen = screen;
    return screen;
}

// ============================================
// Start Function (called from mode integration)
// ============================================

void startWiFiSetupLVGL() {
    Serial.println("[WiFi LVGL] Starting WiFi Setup screen");

    // Reset state
    wifi_selected_network = 0;
    wifi_password_visible = false;
    wifi_error_message = "";
    wifi_failed_ssid = "";
    wifi_scan_pending = false;

    // Screen will be created by the mode integration system
}

/*
 * Cleanup WiFi screen when navigating away
 * Clears all static pointers to prevent dangling pointer crashes
 */
void cleanupWiFiScreen() {
    Serial.println("[WiFi LVGL] Cleaning up WiFi screen");
    wifi_setup_screen = NULL;
    wifi_content = NULL;
    wifi_footer_label = NULL;
    wifi_password_textarea = NULL;
    wifi_loading_spinner = NULL;
    wifi_loading_label = NULL;
    wifi_network_list = NULL;
    wifi_password_hint = NULL;
}

#endif // LV_WIFI_SCREEN_H
