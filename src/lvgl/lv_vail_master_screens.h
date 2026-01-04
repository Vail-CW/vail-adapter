/*
 * VAIL SUMMIT - LVGL Vail Master Screens
 * Provides LVGL UI for the Vail Master CW sending trainer
 */

#ifndef LV_VAIL_MASTER_SCREENS_H
#define LV_VAIL_MASTER_SCREENS_H

#include <lvgl.h>
#include "lv_theme_summit.h"
#include "lv_widgets_summit.h"
#include "lv_screen_manager.h"
#include "../core/config.h"
#include "../training/training_vail_master.h"

// Forward declarations from mode integration
extern void onLVGLMenuSelect(int target_mode);
extern void onLVGLBackNavigation();

// Mode constants
#define LVGL_MODE_VAIL_MASTER            70
#define LVGL_MODE_VAIL_MASTER_PRACTICE   71
#define LVGL_MODE_VAIL_MASTER_SETTINGS   72
#define LVGL_MODE_VAIL_MASTER_HISTORY    73
#define LVGL_MODE_VAIL_MASTER_CHARSET    74

// ============================================
// Static Screen Pointers
// ============================================

static lv_obj_t* vm_menu_screen = NULL;
static lv_obj_t* vm_practice_screen = NULL;
static lv_obj_t* vm_settings_screen = NULL;
static lv_obj_t* vm_history_screen = NULL;
static lv_obj_t* vm_charset_screen = NULL;

// Practice screen widgets
static lv_obj_t* vm_target_label = NULL;
static lv_obj_t* vm_echo_label = NULL;
static lv_obj_t* vm_score_label = NULL;
static lv_obj_t* vm_trial_label = NULL;
static lv_obj_t* vm_streak_label = NULL;
static lv_obj_t* vm_efficiency_label = NULL;
static lv_obj_t* vm_hint_label = NULL;

// Settings screen widgets
static lv_obj_t* vm_wpm_value = NULL;
static lv_obj_t* vm_runlen_value = NULL;
static lv_obj_t* vm_groupcnt_value = NULL;
static lv_obj_t* vm_grouplen_value = NULL;

// Menu cards
static lv_obj_t* vm_mode_cards[5] = {NULL};

// Character set editor state
static bool vm_charset_selected[50] = {false};
static lv_obj_t* vm_charset_btns[50] = {NULL};

// Timer for UI updates during practice
static lv_timer_t* vm_update_timer = NULL;

// ============================================
// Helper Functions
// ============================================

static void cleanupVailMasterScreenPointers() {
    vm_menu_screen = NULL;
    vm_practice_screen = NULL;
    vm_settings_screen = NULL;
    vm_history_screen = NULL;
    vm_charset_screen = NULL;
    vm_target_label = NULL;
    vm_echo_label = NULL;
    vm_score_label = NULL;
    vm_trial_label = NULL;
    vm_streak_label = NULL;
    vm_efficiency_label = NULL;
    vm_hint_label = NULL;
    vm_wpm_value = NULL;
    vm_runlen_value = NULL;
    vm_groupcnt_value = NULL;
    vm_grouplen_value = NULL;
    for (int i = 0; i < 5; i++) vm_mode_cards[i] = NULL;
    for (int i = 0; i < 50; i++) vm_charset_btns[i] = NULL;

    if (vm_update_timer) {
        lv_timer_del(vm_update_timer);
        vm_update_timer = NULL;
    }
}

// ============================================
// Menu Screen
// ============================================

// Grid navigation constants for Vail Master menu
static const int VM_MENU_COLUMNS = 3;  // 3 columns for 5 mode cards

static void vm_mode_card_click_handler(lv_event_t* e) {
    lv_obj_t* card = lv_event_get_target(e);
    int mode_idx = (int)(intptr_t)lv_event_get_user_data(e);

    VailMasterMode mode = (VailMasterMode)mode_idx;
    Serial.printf("[VailMaster] Mode selected: %d (%s)\n", mode_idx, vmGetModeName(mode));

    // Note: onLVGLMenuSelect already plays TONE_SELECT beep

    // Start session and go to practice screen
    vmStartSession(mode);
    onLVGLMenuSelect(LVGL_MODE_VAIL_MASTER_PRACTICE);
}

/*
 * Custom 2D grid navigation handler for Vail Master menu
 * Intercepts arrow keys to navigate between mode cards in a grid layout
 */
static void vm_menu_grid_nav_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // Handle arrow keys for grid navigation
    if (key != LV_KEY_LEFT && key != LV_KEY_RIGHT &&
        key != LV_KEY_PREV && key != LV_KEY_NEXT &&
        key != LV_KEY_UP && key != LV_KEY_DOWN) {
        // Handle S and H keys
        if (key == 's' || key == 'S') {
            lv_event_stop_processing(e);
            onLVGLMenuSelect(LVGL_MODE_VAIL_MASTER_SETTINGS);
            return;
        } else if (key == 'h' || key == 'H') {
            lv_event_stop_processing(e);
            onLVGLMenuSelect(LVGL_MODE_VAIL_MASTER_HISTORY);
            return;
        } else if (key == LV_KEY_ESC) {
            lv_event_stop_processing(e);
            onLVGLBackNavigation();
            return;
        }
        return;
    }

    // Stop propagation to prevent LVGL's default linear navigation
    lv_event_stop_processing(e);

    // Get current focused object
    lv_obj_t* focused = lv_event_get_target(e);
    if (!focused) return;

    // Find index of focused object in our card array
    int focused_idx = -1;
    for (int i = 0; i < 5; i++) {
        if (vm_mode_cards[i] == focused) {
            focused_idx = i;
            break;
        }
    }
    if (focused_idx < 0) return;

    // Calculate grid position (3 columns: 0,1,2 on row 0; 3,4 on row 1)
    int row = focused_idx / VM_MENU_COLUMNS;
    int col = focused_idx % VM_MENU_COLUMNS;
    int total_rows = (5 + VM_MENU_COLUMNS - 1) / VM_MENU_COLUMNS;

    int target_idx = -1;

    if (key == LV_KEY_RIGHT) {
        // Move right: only if not at rightmost column AND target exists
        if (col < VM_MENU_COLUMNS - 1) {
            int potential = focused_idx + 1;
            if (potential < 5) {
                target_idx = potential;
            }
        }
    } else if (key == LV_KEY_LEFT) {
        // Move left: only if not at leftmost column
        if (col > 0) {
            target_idx = focused_idx - 1;
        }
    } else if (key == LV_KEY_NEXT || key == LV_KEY_DOWN) {
        // Move down: go to same column in next row
        if (row < total_rows - 1) {
            int potential = focused_idx + VM_MENU_COLUMNS;
            if (potential < 5) {
                target_idx = potential;
            }
        }
    } else if (key == LV_KEY_PREV || key == LV_KEY_UP) {
        // Move up: go to same column in previous row
        if (row > 0) {
            target_idx = focused_idx - VM_MENU_COLUMNS;
        }
    }

    // Focus target if valid
    if (target_idx >= 0 && target_idx < 5) {
        lv_obj_t* target = vm_mode_cards[target_idx];
        if (target) {
            lv_group_focus_obj(target);
            lv_obj_scroll_to_view(target, LV_ANIM_ON);
        }
    }
}

lv_obj_t* createVailMasterMenuScreen() {
    cleanupVailMasterScreenPointers();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    vm_menu_screen = screen;

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "VAIL MASTER");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Status bar
    createCompactStatusBar(screen);

    // Mode selection cards - use absolute positioning for precise 2-row layout
    // Row 1: Sprint, Sweepstakes, Mixed (3 cards)
    // Row 2: Uniform, Free Practice (2 cards centered)
    // Screen: 480x320, Header: 40px, Footer: 30px, Available: ~250px

    const int cardW = 140;
    const int cardH = 95;
    const int gapX = 10;
    const int gapY = 10;
    const int startY = HEADER_HEIGHT + 10;

    // Row 1: 3 cards evenly spaced
    // Total width for 3 cards: 3*140 + 2*10 = 440px, centered in 480 = start at 20
    const int row1StartX = (SCREEN_WIDTH - (3 * cardW + 2 * gapX)) / 2;
    // Row 2: 2 cards centered
    const int row2StartX = (SCREEN_WIDTH - (2 * cardW + 1 * gapX)) / 2;
    const int row2Y = startY + cardH + gapY;

    const char* mode_names[] = {"Sprint", "Sweepstakes", "Mixed", "Uniform", "Free Practice"};
    const char* mode_descs[] = {
        "ARRL contest",
        "SS exchange",
        "Random groups",
        "Single char",
        "Unscored"
    };

    // Card positions: [x, y] for each of 5 cards
    int card_positions[5][2] = {
        {row1StartX, startY},                          // Sprint
        {row1StartX + cardW + gapX, startY},           // Sweepstakes
        {row1StartX + 2*(cardW + gapX), startY},       // Mixed
        {row2StartX, row2Y},                           // Uniform
        {row2StartX + cardW + gapX, row2Y}             // Free Practice
    };

    for (int i = 0; i < 5; i++) {
        lv_obj_t* card = lv_btn_create(screen);
        lv_obj_set_size(card, cardW, cardH);
        lv_obj_set_pos(card, card_positions[i][0], card_positions[i][1]);
        lv_obj_add_style(card, getStyleMenuCard(), 0);
        lv_obj_add_style(card, getStyleMenuCardFocused(), LV_STATE_FOCUSED);

        // Create column layout inside card for centered text
        lv_obj_t* col = lv_obj_create(card);
        lv_obj_set_size(col, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(col, 0, 0);
        lv_obj_set_style_pad_all(col, 2, 0);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(col, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);

        // Mode name - centered
        lv_obj_t* name = lv_label_create(col);
        lv_label_set_text(name, mode_names[i]);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(name, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, 0);

        // Description - smaller font, gray, centered
        lv_obj_t* desc = lv_label_create(col);
        lv_label_set_text(desc, mode_descs[i]);
        lv_obj_set_style_text_font(desc, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(desc, lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, 0);

        lv_obj_add_event_cb(card, vm_mode_card_click_handler, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_obj_add_event_cb(card, vm_menu_grid_nav_handler, LV_EVENT_KEY, NULL);
        addNavigableWidget(card);

        vm_mode_cards[i] = card;
    }

    // Footer with hints
    lv_obj_t* footer = lv_label_create(screen);
    lv_label_set_text(footer, "S Settings   H History   ESC Back");
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(footer, lv_color_hex(0xFD9800), 0);  // Orange
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -8);

    return screen;
}

// ============================================
// Practice Screen
// ============================================

// Track last displayed trial to detect trial changes
static int vm_last_displayed_trial = -1;

static void vm_practice_update_timer_cb(lv_timer_t* timer) {
    // Update echo text if changed
    if (vmNeedsUIUpdate && vm_echo_label != NULL) {
        String echo = vmEchoText;
        if (echo.length() == 0) echo = "_";
        lv_label_set_text(vm_echo_label, echo.c_str());
    }

    // Check for trial change - update target when new trial starts
    if (vmState == VM_STATE_READY || vmState == VM_STATE_LISTENING) {
        if (vm_last_displayed_trial != vmSession.currentTrial) {
            vm_last_displayed_trial = vmSession.currentTrial;
            // Update target label with new trial target
            if (vm_target_label != NULL) {
                lv_label_set_text(vm_target_label, vmSession.trials[vmSession.currentTrial].target);
            }
            // Clear echo display
            if (vm_echo_label != NULL) {
                lv_label_set_text(vm_echo_label, "_");
            }
        }
    }

    // Always clear the UI update flag after processing
    vmNeedsUIUpdate = false;

    // Update score display
    if (vm_score_label != NULL) {
        lv_label_set_text_fmt(vm_score_label, "Score: %d", vmSession.totalScore);
    }

    // Update trial counter
    if (vm_trial_label != NULL) {
        if (vmSession.mode == VM_MODE_FREE_PRACTICE) {
            lv_label_set_text(vm_trial_label, "Free Practice");
        } else {
            lv_label_set_text_fmt(vm_trial_label, "Trial %d / %d",
                                  vmSession.currentTrial + 1, vmSession.runLength);
        }
    }

    // Update streak
    if (vm_streak_label != NULL) {
        lv_label_set_text_fmt(vm_streak_label, "Streak: %d", vmSession.currentStreak);
    }

    // Update efficiency
    if (vm_efficiency_label != NULL && vmSession.maxPossibleScore > 0) {
        float eff = (float)vmSession.totalScore / vmSession.maxPossibleScore * 100.0f;
        lv_label_set_text_fmt(vm_efficiency_label, "Eff: %.0f%%", eff);
    }

    // Check for state changes
    if (vmState == VM_STATE_RUN_COMPLETE) {
        // Show run complete summary
        if (vm_target_label != NULL) {
            float eff = 0;
            if (vmSession.maxPossibleScore > 0) {
                eff = (float)vmSession.totalScore / vmSession.maxPossibleScore * 100.0f;
            }
            int perfPct = vmSession.perfectCount * 100 / vmSession.runLength;

            char summary[128];
            snprintf(summary, sizeof(summary),
                     "RUN COMPLETE!\nScore: %d  Efficiency: %.0f%%\nPerfect: %d/%d (%d%%)  Best Streak: %d",
                     vmSession.totalScore, eff,
                     vmSession.perfectCount, vmSession.runLength, perfPct,
                     vmSession.bestStreak);
            lv_label_set_text(vm_target_label, summary);
        }
        if (vm_hint_label != NULL) {
            lv_label_set_text(vm_hint_label, "Press SPACE to restart or ESC to exit");
        }
        // Mark as displayed to avoid re-triggering target update
        vm_last_displayed_trial = -1;
    }
}

static void vm_practice_key_handler(lv_event_t* e) {
    uint32_t key = lv_event_get_key(e);

    switch (key) {
        case LV_KEY_ESC:
            lv_event_stop_processing(e);  // Prevent double ESC
            vmHandleEsc();
            onLVGLBackNavigation();
            break;
        case ' ':  // Space
            if (vmState == VM_STATE_RUN_COMPLETE) {
                // Restart run
                vmStartSession(vmSession.mode);
                vm_last_displayed_trial = -1;  // Reset to trigger UI update
                if (vm_target_label != NULL) {
                    lv_label_set_text(vm_target_label, vmSession.trials[0].target);
                }
                if (vm_echo_label != NULL) {
                    lv_label_set_text(vm_echo_label, "_");
                }
                if (vm_hint_label != NULL) {
                    lv_label_set_text(vm_hint_label, "SPACE Skip   C Clear   R Restart   S Settings   ESC Exit");
                }
            } else {
                vmHandleSpace();  // Skip trial
            }
            break;
        case 'c':
        case 'C':
            vmHandleClear();
            if (vm_echo_label != NULL) {
                lv_label_set_text(vm_echo_label, "_");
            }
            break;
        case 'r':
        case 'R':
            vmHandleRestart();
            vm_last_displayed_trial = -1;  // Reset to trigger UI update
            if (vm_target_label != NULL) {
                lv_label_set_text(vm_target_label, vmSession.trials[0].target);
            }
            if (vm_echo_label != NULL) {
                lv_label_set_text(vm_echo_label, "_");
            }
            if (vm_hint_label != NULL) {
                lv_label_set_text(vm_hint_label, "SPACE Skip   C Clear   R Restart   S Settings   ESC Exit");
            }
            break;
        case 's':
        case 'S':
            // Pause and go to settings - full cleanup to prevent delay
            vmActive = false;
            stopTone();
            vmDecoder.flush();
            onLVGLMenuSelect(LVGL_MODE_VAIL_MASTER_SETTINGS);
            break;
    }
}

lv_obj_t* createVailMasterPracticeScreen() {
    // Clear navigation group first (required before creating widgets)
    clearNavigationGroup();
    cleanupVailMasterScreenPointers();

    // Reset trial tracking for fresh start
    vm_last_displayed_trial = 0;

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    vm_practice_screen = screen;

    // Title bar with mode and score
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text_fmt(title, "VAIL MASTER - %s", vmGetModeName(vmSession.mode));
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Score on right side of title bar
    vm_score_label = lv_label_create(title_bar);
    lv_label_set_text_fmt(vm_score_label, "Score: %d", vmSession.totalScore);
    lv_obj_set_style_text_font(vm_score_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(vm_score_label, lv_color_hex(0x07E0), 0);  // Green
    lv_obj_align(vm_score_label, LV_ALIGN_RIGHT_MID, -15, 0);

    // Stats row
    lv_obj_t* stats_row = lv_obj_create(screen);
    lv_obj_set_size(stats_row, SCREEN_WIDTH - 20, 35);
    lv_obj_set_pos(stats_row, 10, HEADER_HEIGHT + 5);
    lv_obj_set_layout(stats_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(stats_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(stats_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(stats_row, LV_OBJ_FLAG_SCROLLABLE);
    applyCardStyle(stats_row);

    // Trial counter
    vm_trial_label = lv_label_create(stats_row);
    lv_label_set_text_fmt(vm_trial_label, "Trial 1 / %d", vmSession.runLength);
    lv_obj_set_style_text_font(vm_trial_label, &lv_font_montserrat_14, 0);

    // Streak
    vm_streak_label = lv_label_create(stats_row);
    lv_label_set_text(vm_streak_label, "Streak: 0");
    lv_obj_set_style_text_font(vm_streak_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(vm_streak_label, lv_color_hex(0x07FF), 0);  // Cyan

    // Efficiency
    vm_efficiency_label = lv_label_create(stats_row);
    lv_label_set_text(vm_efficiency_label, "Eff: --");
    lv_obj_set_style_text_font(vm_efficiency_label, &lv_font_montserrat_14, 0);

    // Target display (large, centered)
    lv_obj_t* target_container = lv_obj_create(screen);
    lv_obj_set_size(target_container, SCREEN_WIDTH - 20, 80);
    lv_obj_set_pos(target_container, 10, HEADER_HEIGHT + 50);
    applyCardStyle(target_container);
    lv_obj_clear_flag(target_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* target_title = lv_label_create(target_container);
    lv_label_set_text(target_title, "TARGET:");
    lv_obj_set_style_text_font(target_title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(target_title, lv_color_hex(0x888888), 0);
    lv_obj_align(target_title, LV_ALIGN_TOP_LEFT, 5, 2);

    vm_target_label = lv_label_create(target_container);
    lv_label_set_text(vm_target_label, vmSession.trials[0].target);
    lv_obj_set_style_text_font(vm_target_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(vm_target_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_width(vm_target_label, SCREEN_WIDTH - 40);
    lv_label_set_long_mode(vm_target_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(vm_target_label, LV_ALIGN_CENTER, 0, 8);

    // Echo display
    lv_obj_t* echo_container = lv_obj_create(screen);
    lv_obj_set_size(echo_container, SCREEN_WIDTH - 20, 70);
    lv_obj_set_pos(echo_container, 10, HEADER_HEIGHT + 140);
    applyCardStyle(echo_container);
    lv_obj_clear_flag(echo_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* echo_title = lv_label_create(echo_container);
    lv_label_set_text(echo_title, "ECHO:");
    lv_obj_set_style_text_font(echo_title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(echo_title, lv_color_hex(0x888888), 0);
    lv_obj_align(echo_title, LV_ALIGN_TOP_LEFT, 5, 2);

    vm_echo_label = lv_label_create(echo_container);
    lv_label_set_text(vm_echo_label, "_");
    lv_obj_set_style_text_font(vm_echo_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(vm_echo_label, lv_color_hex(0x07FF), 0);  // Cyan
    lv_obj_set_width(vm_echo_label, SCREEN_WIDTH - 40);
    lv_label_set_long_mode(vm_echo_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(vm_echo_label, LV_ALIGN_CENTER, 0, 8);

    // Hints/controls footer
    vm_hint_label = lv_label_create(screen);
    lv_label_set_text(vm_hint_label, "SPACE Skip   C Clear   R Restart   S Settings   ESC Exit");
    lv_obj_set_style_text_font(vm_hint_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(vm_hint_label, lv_color_hex(0xFD9800), 0);
    lv_obj_align(vm_hint_label, LV_ALIGN_BOTTOM_MID, 0, -8);

    // Invisible focus container for keyboard input
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);
    lv_obj_set_pos(focus_container, 0, 0);
    lv_obj_set_style_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(focus_container, vm_practice_key_handler, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus_container);

    // Start update timer (50ms interval)
    vm_update_timer = lv_timer_create(vm_practice_update_timer_cb, 50, NULL);

    return screen;
}

// ============================================
// Settings Screen
// ============================================

// Settings focus tracking (0=WPM, 1=RunLen, 2=Groups, 3=GroupLen, 4=Charset)
static int vm_settings_focus = 0;
static const int VM_SETTINGS_COUNT = 5;

// Settings screen widget pointers for focus management
static lv_obj_t* vm_wpm_slider = NULL;
static lv_obj_t* vm_runlen_btns[3] = {NULL};
static lv_obj_t* vm_grpcnt_slider = NULL;
static lv_obj_t* vm_grplen_slider = NULL;
static lv_obj_t* vm_charset_btn = NULL;
static lv_obj_t* vm_settings_rows[5] = {NULL};

static void vm_settings_update_focus() {
    // Update visual focus indicator on rows
    // Build local array of row pointers (safer than using static array)
    lv_obj_t* rows[VM_SETTINGS_COUNT] = {NULL};
    for (int i = 0; i < VM_SETTINGS_COUNT; i++) {
        rows[i] = vm_settings_rows[i];
    }

    for (int i = 0; i < VM_SETTINGS_COUNT; i++) {
        if (rows[i] == NULL) continue;

        if (i == vm_settings_focus) {
            lv_obj_set_style_border_color(rows[i], lv_color_hex(0x07FF), 0);
            lv_obj_set_style_border_width(rows[i], 2, 0);
            // Scroll the focused row into view
            lv_obj_scroll_to_view(rows[i], LV_ANIM_ON);
        } else {
            lv_obj_set_style_border_color(rows[i], lv_color_hex(0x333333), 0);
            lv_obj_set_style_border_width(rows[i], 1, 0);
        }
    }
}

static void vm_settings_key_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // Block LVGL's default navigation
    if (key == LV_KEY_NEXT || key == LV_KEY_PREV) {
        lv_event_stop_processing(e);
        return;
    }

    // Handle ESC
    if (key == LV_KEY_ESC) {
        lv_event_stop_processing(e);
        vmSaveSettings();
        onLVGLBackNavigation();
        return;
    }

    // Handle UP/DOWN for navigation between settings
    if (key == LV_KEY_UP) {
        lv_event_stop_processing(e);
        if (vm_settings_focus > 0) {
            vm_settings_focus--;
            vm_settings_update_focus();
            beep(TONE_MENU_NAV, BEEP_SHORT);
        }
        return;
    }
    if (key == LV_KEY_DOWN) {
        lv_event_stop_processing(e);
        if (vm_settings_focus < VM_SETTINGS_COUNT - 1) {
            vm_settings_focus++;
            vm_settings_update_focus();
            beep(TONE_MENU_NAV, BEEP_SHORT);
        }
        return;
    }

    // Handle LEFT/RIGHT for value adjustment
    if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT) {
        lv_event_stop_processing(e);
        int delta = (key == LV_KEY_RIGHT) ? 1 : -1;

        switch (vm_settings_focus) {
            case 0:  // WPM slider
                if (vm_wpm_slider) {
                    int val = lv_slider_get_value(vm_wpm_slider) + delta;
                    if (val < WPM_MIN) val = WPM_MIN;
                    if (val > WPM_MAX) val = WPM_MAX;
                    lv_slider_set_value(vm_wpm_slider, val, LV_ANIM_OFF);
                    vmWPM = val;
                    if (vm_wpm_value) lv_label_set_text_fmt(vm_wpm_value, "%d WPM", vmWPM);
                }
                break;
            case 1:  // Run length buttons (cycle through 10, 25, 50)
                {
                    int lengths[] = {10, 25, 50};
                    int idx = 0;
                    for (int i = 0; i < 3; i++) if (vmRunLength == lengths[i]) idx = i;
                    idx += delta;
                    if (idx < 0) idx = 0;
                    if (idx > 2) idx = 2;
                    vmRunLength = lengths[idx];
                    if (vm_runlen_value) lv_label_set_text_fmt(vm_runlen_value, "%d trials", vmRunLength);
                }
                break;
            case 2:  // Group count slider
                if (vm_grpcnt_slider) {
                    int val = lv_slider_get_value(vm_grpcnt_slider) + delta;
                    if (val < 1) val = 1;
                    if (val > 5) val = 5;
                    lv_slider_set_value(vm_grpcnt_slider, val, LV_ANIM_OFF);
                    vmMixedSettings.groupCount = val;
                    if (vm_groupcnt_value) lv_label_set_text_fmt(vm_groupcnt_value, "%d groups", val);
                }
                break;
            case 3:  // Group length slider
                if (vm_grplen_slider) {
                    int val = lv_slider_get_value(vm_grplen_slider) + delta;
                    if (val < 3) val = 3;
                    if (val > 10) val = 10;
                    lv_slider_set_value(vm_grplen_slider, val, LV_ANIM_OFF);
                    vmMixedSettings.groupLength = val;
                    if (vm_grouplen_value) lv_label_set_text_fmt(vm_grouplen_value, "%d chars", val);
                }
                break;
            case 4:  // Charset button - does nothing on left/right
                break;
        }
        return;
    }

    // Handle ENTER for charset edit button
    if (key == LV_KEY_ENTER && vm_settings_focus == 4) {
        lv_event_stop_processing(e);
        vmSaveSettings();
        onLVGLMenuSelect(LVGL_MODE_VAIL_MASTER_CHARSET);
        return;
    }

    // Block all other keys to prevent LVGL default handling
    lv_event_stop_processing(e);
}

static void vm_wpm_slider_cb(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    vmWPM = lv_slider_get_value(slider);
    if (vm_wpm_value) {
        lv_label_set_text_fmt(vm_wpm_value, "%d WPM", vmWPM);
    }
}

static void vm_runlen_btn_cb(lv_event_t* e) {
    int len = (int)(intptr_t)lv_event_get_user_data(e);
    vmRunLength = len;
    beep(TONE_MENU_NAV, BEEP_SHORT);

    if (vm_runlen_value) {
        lv_label_set_text_fmt(vm_runlen_value, "%d trials", vmRunLength);
    }
}

static void vm_groupcnt_slider_cb(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    vmMixedSettings.groupCount = lv_slider_get_value(slider);
    if (vm_groupcnt_value) {
        lv_label_set_text_fmt(vm_groupcnt_value, "%d groups", vmMixedSettings.groupCount);
    }
}

static void vm_grouplen_slider_cb(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    vmMixedSettings.groupLength = lv_slider_get_value(slider);
    if (vm_grouplen_value) {
        lv_label_set_text_fmt(vm_grouplen_value, "%d chars", vmMixedSettings.groupLength);
    }
}

static void vm_charset_edit_btn_cb(lv_event_t* e) {
    vmSaveSettings();
    onLVGLMenuSelect(LVGL_MODE_VAIL_MASTER_CHARSET);
}

lv_obj_t* createVailMasterSettingsScreen() {
    // Clear navigation group first (required before creating widgets)
    clearNavigationGroup();

    // Stop practice update timer if running (prevents crash when coming from practice screen)
    if (vm_update_timer) {
        lv_timer_del(vm_update_timer);
        vm_update_timer = NULL;
    }

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    vm_settings_screen = screen;

    // Reset focus state
    vm_settings_focus = 0;

    // Clear widget pointers
    vm_wpm_slider = NULL;
    vm_grpcnt_slider = NULL;
    vm_grplen_slider = NULL;
    vm_charset_btn = NULL;
    for (int i = 0; i < 5; i++) vm_settings_rows[i] = NULL;

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "VAIL MASTER SETTINGS");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Settings container (scrollable)
    lv_obj_t* container = lv_obj_create(screen);
    lv_obj_set_size(container, SCREEN_WIDTH - 20, SCREEN_HEIGHT - HEADER_HEIGHT - 50);
    lv_obj_set_pos(container, 10, HEADER_HEIGHT + 5);
    lv_obj_set_layout(container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(container, 8, 0);
    lv_obj_set_style_pad_all(container, 8, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);

    // WPM setting (row 0)
    lv_obj_t* wpm_row = lv_obj_create(container);
    lv_obj_set_size(wpm_row, SCREEN_WIDTH - 50, 45);
    applyCardStyle(wpm_row);
    lv_obj_clear_flag(wpm_row, LV_OBJ_FLAG_SCROLLABLE);
    vm_settings_rows[0] = wpm_row;

    lv_obj_t* wpm_label = lv_label_create(wpm_row);
    lv_label_set_text(wpm_label, "Speed:");
    lv_obj_align(wpm_label, LV_ALIGN_LEFT_MID, 10, 0);

    vm_wpm_value = lv_label_create(wpm_row);
    lv_label_set_text_fmt(vm_wpm_value, "%d WPM", vmWPM);
    lv_obj_set_style_text_color(vm_wpm_value, lv_color_hex(0x07FF), 0);
    lv_obj_align(vm_wpm_value, LV_ALIGN_RIGHT_MID, -10, 0);

    vm_wpm_slider = lv_slider_create(wpm_row);
    lv_obj_set_width(vm_wpm_slider, 200);
    lv_slider_set_range(vm_wpm_slider, WPM_MIN, WPM_MAX);
    lv_slider_set_value(vm_wpm_slider, vmWPM, LV_ANIM_OFF);
    lv_obj_align(vm_wpm_slider, LV_ALIGN_CENTER, 0, 0);

    // Run Length setting (row 1)
    lv_obj_t* runlen_row = lv_obj_create(container);
    lv_obj_set_size(runlen_row, SCREEN_WIDTH - 50, 45);
    applyCardStyle(runlen_row);
    lv_obj_clear_flag(runlen_row, LV_OBJ_FLAG_SCROLLABLE);
    vm_settings_rows[1] = runlen_row;

    lv_obj_t* runlen_label = lv_label_create(runlen_row);
    lv_label_set_text(runlen_label, "Run Length:");
    lv_obj_align(runlen_label, LV_ALIGN_LEFT_MID, 10, 0);

    vm_runlen_value = lv_label_create(runlen_row);
    lv_label_set_text_fmt(vm_runlen_value, "%d trials", vmRunLength);
    lv_obj_set_style_text_color(vm_runlen_value, lv_color_hex(0x07FF), 0);
    lv_obj_align(vm_runlen_value, LV_ALIGN_RIGHT_MID, -10, 0);

    // Group Count (row 2)
    lv_obj_t* grpcnt_row = lv_obj_create(container);
    lv_obj_set_size(grpcnt_row, SCREEN_WIDTH - 50, 45);
    applyCardStyle(grpcnt_row);
    lv_obj_clear_flag(grpcnt_row, LV_OBJ_FLAG_SCROLLABLE);
    vm_settings_rows[2] = grpcnt_row;

    lv_obj_t* grpcnt_label = lv_label_create(grpcnt_row);
    lv_label_set_text(grpcnt_label, "Groups:");
    lv_obj_align(grpcnt_label, LV_ALIGN_LEFT_MID, 10, 0);

    vm_groupcnt_value = lv_label_create(grpcnt_row);
    lv_label_set_text_fmt(vm_groupcnt_value, "%d groups", vmMixedSettings.groupCount);
    lv_obj_set_style_text_color(vm_groupcnt_value, lv_color_hex(0x07FF), 0);
    lv_obj_align(vm_groupcnt_value, LV_ALIGN_RIGHT_MID, -10, 0);

    vm_grpcnt_slider = lv_slider_create(grpcnt_row);
    lv_obj_set_width(vm_grpcnt_slider, 150);
    lv_slider_set_range(vm_grpcnt_slider, 1, 5);
    lv_slider_set_value(vm_grpcnt_slider, vmMixedSettings.groupCount, LV_ANIM_OFF);
    lv_obj_align(vm_grpcnt_slider, LV_ALIGN_CENTER, 0, 0);

    // Group Length (row 3)
    lv_obj_t* grplen_row = lv_obj_create(container);
    lv_obj_set_size(grplen_row, SCREEN_WIDTH - 50, 45);
    applyCardStyle(grplen_row);
    lv_obj_clear_flag(grplen_row, LV_OBJ_FLAG_SCROLLABLE);
    vm_settings_rows[3] = grplen_row;

    lv_obj_t* grplen_label = lv_label_create(grplen_row);
    lv_label_set_text(grplen_label, "Group Length:");
    lv_obj_align(grplen_label, LV_ALIGN_LEFT_MID, 10, 0);

    vm_grouplen_value = lv_label_create(grplen_row);
    lv_label_set_text_fmt(vm_grouplen_value, "%d chars", vmMixedSettings.groupLength);
    lv_obj_set_style_text_color(vm_grouplen_value, lv_color_hex(0x07FF), 0);
    lv_obj_align(vm_grouplen_value, LV_ALIGN_RIGHT_MID, -10, 0);

    vm_grplen_slider = lv_slider_create(grplen_row);
    lv_obj_set_width(vm_grplen_slider, 150);
    lv_slider_set_range(vm_grplen_slider, 3, 10);
    lv_slider_set_value(vm_grplen_slider, vmMixedSettings.groupLength, LV_ANIM_OFF);
    lv_obj_align(vm_grplen_slider, LV_ALIGN_CENTER, 0, 0);

    // Character Set Edit button (row 4)
    lv_obj_t* charset_row = lv_obj_create(container);
    lv_obj_set_size(charset_row, SCREEN_WIDTH - 50, 45);
    applyCardStyle(charset_row);
    lv_obj_clear_flag(charset_row, LV_OBJ_FLAG_SCROLLABLE);
    vm_settings_rows[4] = charset_row;

    lv_obj_t* charset_label = lv_label_create(charset_row);
    lv_label_set_text(charset_label, "Character Set:");
    lv_obj_align(charset_label, LV_ALIGN_LEFT_MID, 10, 0);

    vm_charset_btn = lv_btn_create(charset_row);
    lv_obj_set_size(vm_charset_btn, 80, 30);
    lv_obj_align(vm_charset_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_add_style(vm_charset_btn, getStyleMenuCard(), 0);
    lv_obj_add_style(vm_charset_btn, getStyleMenuCardFocused(), LV_STATE_FOCUSED);

    lv_obj_t* charset_btn_label = lv_label_create(vm_charset_btn);
    lv_label_set_text(charset_btn_label, "Edit >");
    lv_obj_center(charset_btn_label);

    lv_obj_add_event_cb(vm_charset_btn, vm_charset_edit_btn_cb, LV_EVENT_CLICKED, NULL);

    // Focus container for keyboard input (positioned off-screen)
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);
    lv_obj_set_pos(focus_container, -10, -10);
    lv_obj_set_style_bg_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, LV_STATE_FOCUSED);
    lv_obj_clear_flag(focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(focus_container, vm_settings_key_handler, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus_container);

    // Set editing mode and focus
    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }
    lv_group_focus_obj(focus_container);

    // Initialize focus visual
    vm_settings_update_focus();

    // Footer
    lv_obj_t* footer = lv_label_create(screen);
    lv_label_set_text(footer, "UP/DOWN Navigate   LEFT/RIGHT Adjust   ESC Save");
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(footer, lv_color_hex(0xFD9800), 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -8);

    return screen;
}

// ============================================
// Score History Screen
// ============================================

static void vm_history_key_handler(lv_event_t* e) {
    uint32_t key = lv_event_get_key(e);

    if (key == LV_KEY_ESC) {
        lv_event_stop_processing(e);  // Prevent double ESC
        onLVGLBackNavigation();
    }
}

lv_obj_t* createVailMasterHistoryScreen() {
    // Clear navigation group first (required before creating widgets)
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    vm_history_screen = screen;

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "SCORE HISTORY");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Load scores for all modes (load each mode's scores and combine)
    // We'll aggregate scores from all modes into one view
    VailMasterScoreRecord allScores[100];
    int totalScores = 0;

    // Load scores for each mode (skip Free Practice which doesn't save)
    for (int m = 0; m < 4; m++) {  // Sprint, Sweepstakes, Mixed, Uniform
        vmLoadScoreHistory((VailMasterMode)m, vmRunLength);
        for (int i = 0; i < vmScoreHistoryCount && totalScores < 100; i++) {
            allScores[totalScores++] = vmScoreHistory[i];
        }
    }

    // Sort by timestamp (newest first)
    for (int i = 0; i < totalScores - 1; i++) {
        for (int j = i + 1; j < totalScores; j++) {
            if (allScores[j].timestamp > allScores[i].timestamp) {
                VailMasterScoreRecord temp = allScores[i];
                allScores[i] = allScores[j];
                allScores[j] = temp;
            }
        }
    }

    // Score table
    lv_obj_t* table = lv_table_create(screen);
    lv_obj_set_size(table, SCREEN_WIDTH - 20, SCREEN_HEIGHT - HEADER_HEIGHT - 50);
    lv_obj_set_pos(table, 10, HEADER_HEIGHT + 5);
    lv_table_set_col_cnt(table, 5);
    lv_table_set_col_width(table, 0, 80);   // Mode
    lv_table_set_col_width(table, 1, 80);   // Score
    lv_table_set_col_width(table, 2, 60);   // Eff%
    lv_table_set_col_width(table, 3, 100);  // Perfect
    lv_table_set_col_width(table, 4, 60);   // Streak

    // Header row
    lv_table_set_cell_value(table, 0, 0, "Mode");
    lv_table_set_cell_value(table, 0, 1, "Score");
    lv_table_set_cell_value(table, 0, 2, "Eff%");
    lv_table_set_cell_value(table, 0, 3, "Perfect");
    lv_table_set_cell_value(table, 0, 4, "Streak");

    // Data rows
    int row = 1;
    for (int i = 0; i < totalScores && row < 15; i++) {
        char buf[32];

        snprintf(buf, sizeof(buf), "%s", vmGetModeShortName(allScores[i].mode));
        lv_table_set_cell_value(table, row, 0, buf);

        snprintf(buf, sizeof(buf), "%d", allScores[i].totalScore);
        lv_table_set_cell_value(table, row, 1, buf);

        snprintf(buf, sizeof(buf), "%.0f%%", allScores[i].efficiency);
        lv_table_set_cell_value(table, row, 2, buf);

        snprintf(buf, sizeof(buf), "%d (%d%%)", allScores[i].perfectCount, allScores[i].perfectPercent);
        lv_table_set_cell_value(table, row, 3, buf);

        snprintf(buf, sizeof(buf), "%d", allScores[i].bestStreak);
        lv_table_set_cell_value(table, row, 4, buf);

        row++;
    }

    if (totalScores == 0) {
        lv_table_set_cell_value(table, 1, 0, "No");
        lv_table_set_cell_value(table, 1, 1, "scores");
        lv_table_set_cell_value(table, 1, 2, "yet");
    }

    // Invisible focus container for keyboard input
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);
    lv_obj_set_pos(focus_container, 0, 0);
    lv_obj_set_style_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(focus_container, vm_history_key_handler, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus_container);

    // Footer
    lv_obj_t* footer = lv_label_create(screen);
    lv_label_set_text(footer, "ESC Back");
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(footer, lv_color_hex(0xFD9800), 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -8);

    return screen;
}

// ============================================
// Character Set Editor Screen
// ============================================

// Grid navigation constants for charset screen
// With 35px buttons + 5px gap = 40px per cell, in ~440px container = ~11 columns
static const int VM_CHARSET_COLUMNS = 11;
static const int VM_CHARSET_TOTAL = 41;  // 26 letters + 10 digits + 5 punctuation

static void vm_charset_grid_nav_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // Handle ESC for save and exit
    if (key == LV_KEY_ESC) {
        lv_event_stop_processing(e);

        // Build charset string from selected characters
        const char* all_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/=";
        int all_len = strlen(all_chars);

        vmMixedSettings.charsetLength = 0;
        for (int i = 0; i < all_len && i < 50; i++) {
            if (vm_charset_selected[i]) {
                vmMixedSettings.charset[vmMixedSettings.charsetLength++] = all_chars[i];
            }
        }
        vmMixedSettings.charset[vmMixedSettings.charsetLength] = '\0';

        // Ensure at least some characters selected
        if (vmMixedSettings.charsetLength == 0) {
            strcpy(vmMixedSettings.charset, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
            vmMixedSettings.charsetLength = 26;
        }

        vmSaveSettings();
        Serial.printf("[VailMaster] Charset saved: %s (%d chars)\n",
                      vmMixedSettings.charset, vmMixedSettings.charsetLength);

        onLVGLBackNavigation();
        return;
    }

    // Handle ENTER for toggle
    if (key == LV_KEY_ENTER) {
        // Let the click handler deal with it
        return;
    }

    // Handle arrow keys for grid navigation
    if (key != LV_KEY_LEFT && key != LV_KEY_RIGHT &&
        key != LV_KEY_PREV && key != LV_KEY_NEXT &&
        key != LV_KEY_UP && key != LV_KEY_DOWN) {
        return;
    }

    // Stop propagation to prevent LVGL's default linear navigation
    lv_event_stop_processing(e);

    // Get current focused object
    lv_obj_t* focused = lv_event_get_target(e);
    if (!focused) return;

    // Find index of focused object in our button array
    int focused_idx = -1;
    for (int i = 0; i < VM_CHARSET_TOTAL; i++) {
        if (vm_charset_btns[i] == focused) {
            focused_idx = i;
            break;
        }
    }
    if (focused_idx < 0) return;

    // Calculate grid position
    int row = focused_idx / VM_CHARSET_COLUMNS;
    int col = focused_idx % VM_CHARSET_COLUMNS;
    int total_rows = (VM_CHARSET_TOTAL + VM_CHARSET_COLUMNS - 1) / VM_CHARSET_COLUMNS;

    int target_idx = -1;

    if (key == LV_KEY_RIGHT) {
        // Move right
        int potential = focused_idx + 1;
        if (potential < VM_CHARSET_TOTAL) {
            target_idx = potential;
        }
    } else if (key == LV_KEY_LEFT) {
        // Move left
        if (focused_idx > 0) {
            target_idx = focused_idx - 1;
        }
    } else if (key == LV_KEY_NEXT || key == LV_KEY_DOWN) {
        // Move down
        int potential = focused_idx + VM_CHARSET_COLUMNS;
        if (potential < VM_CHARSET_TOTAL) {
            target_idx = potential;
        } else if (row < total_rows - 1) {
            // Go to last item if target is past end
            target_idx = VM_CHARSET_TOTAL - 1;
        }
    } else if (key == LV_KEY_PREV || key == LV_KEY_UP) {
        // Move up
        int potential = focused_idx - VM_CHARSET_COLUMNS;
        if (potential >= 0) {
            target_idx = potential;
        }
    }

    // Focus target if valid
    if (target_idx >= 0 && target_idx < VM_CHARSET_TOTAL) {
        lv_obj_t* target = vm_charset_btns[target_idx];
        if (target) {
            lv_group_focus_obj(target);
            lv_obj_scroll_to_view(target, LV_ANIM_ON);
            beep(TONE_MENU_NAV, BEEP_SHORT);
        }
    }
}

static void vm_charset_btn_toggle_cb(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    int idx = (int)(intptr_t)lv_event_get_user_data(e);

    vm_charset_selected[idx] = !vm_charset_selected[idx];

    if (vm_charset_selected[idx]) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x07FF), 0);  // Cyan when selected
        lv_obj_set_style_text_color(lv_obj_get_child(btn, 0), lv_color_hex(0x000000), 0);
    } else {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1A1A2E), 0);  // Dark when deselected
        lv_obj_set_style_text_color(lv_obj_get_child(btn, 0), lv_color_hex(0xFFFFFF), 0);
    }

    beep(TONE_MENU_NAV, BEEP_SHORT);
}

lv_obj_t* createVailMasterCharsetScreen() {
    // Clear navigation group first (required before creating widgets)
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    vm_charset_screen = screen;

    // Title bar
    lv_obj_t* title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_add_style(title_bar, getStyleStatusBar(), 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, "CHARACTER SET");
    lv_obj_add_style(title, getStyleLabelTitle(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    // Initialize selection state from current charset
    const char* all_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/=";
    int all_len = strlen(all_chars);

    for (int i = 0; i < all_len; i++) {
        vm_charset_selected[i] = (strchr(vmMixedSettings.charset, all_chars[i]) != NULL);
    }

    // Character grid container
    lv_obj_t* grid_container = lv_obj_create(screen);
    lv_obj_set_size(grid_container, SCREEN_WIDTH - 20, SCREEN_HEIGHT - HEADER_HEIGHT - 50);
    lv_obj_set_pos(grid_container, 10, HEADER_HEIGHT + 5);
    lv_obj_set_layout(grid_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(grid_container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(grid_container, 5, 0);
    lv_obj_set_style_pad_all(grid_container, 10, 0);
    lv_obj_set_style_bg_opa(grid_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid_container, 0, 0);

    // Create button for each character
    for (int i = 0; i < all_len; i++) {
        lv_obj_t* btn = lv_btn_create(grid_container);
        lv_obj_set_size(btn, 35, 35);

        // Style based on selection state
        if (vm_charset_selected[i]) {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x07FF), 0);
        } else {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x1A1A2E), 0);
        }
        lv_obj_set_style_border_color(btn, lv_color_hex(0x444444), 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_radius(btn, 5, 0);

        // Focused style
        lv_obj_set_style_outline_color(btn, lv_color_hex(0x07FF), LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(btn, 2, LV_STATE_FOCUSED);

        lv_obj_t* label = lv_label_create(btn);
        char ch_str[2] = {all_chars[i], '\0'};
        lv_label_set_text(label, ch_str);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
        if (vm_charset_selected[i]) {
            lv_obj_set_style_text_color(label, lv_color_hex(0x000000), 0);
        } else {
            lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
        }
        lv_obj_center(label);

        lv_obj_add_event_cb(btn, vm_charset_btn_toggle_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_obj_add_event_cb(btn, vm_charset_grid_nav_handler, LV_EVENT_KEY, NULL);
        addNavigableWidget(btn);

        vm_charset_btns[i] = btn;
    }

    // Footer
    lv_obj_t* footer = lv_label_create(screen);
    lv_label_set_text(footer, "ENTER Toggle   ESC Save & Back");
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(footer, lv_color_hex(0xFD9800), 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -8);

    return screen;
}

// ============================================
// Screen Selector
// ============================================

lv_obj_t* createVailMasterScreenForMode(int mode) {
    switch (mode) {
        case LVGL_MODE_VAIL_MASTER:
            return createVailMasterMenuScreen();
        case LVGL_MODE_VAIL_MASTER_PRACTICE:
            return createVailMasterPracticeScreen();
        case LVGL_MODE_VAIL_MASTER_SETTINGS:
            return createVailMasterSettingsScreen();
        case LVGL_MODE_VAIL_MASTER_HISTORY:
            return createVailMasterHistoryScreen();
        case LVGL_MODE_VAIL_MASTER_CHARSET:
            return createVailMasterCharsetScreen();
        default:
            return NULL;
    }
}

#endif // LV_VAIL_MASTER_SCREENS_H
