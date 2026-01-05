/*
 * VAIL SUMMIT - LVGL Spark Watch Screens
 * Provides LVGL UI for the Spark Watch maritime morse training game
 */

#ifndef LV_SPARK_WATCH_SCREENS_H
#define LV_SPARK_WATCH_SCREENS_H

#include <lvgl.h>
#include "lv_theme_summit.h"
#include "lv_widgets_summit.h"
#include "lv_screen_manager.h"
#include "../core/config.h"
#include "../games/game_spark_watch.h"
#include "../games/game_spark_watch_data.h"

// Forward declaration for back navigation and mode changes
extern void onLVGLBackNavigation();
extern void onLVGLMenuSelect(int targetMode);

// Mode constants
#define SPARK_MODE_MENU        78
#define SPARK_MODE_DIFFICULTY  79
#define SPARK_MODE_CAMPAIGN    80
#define SPARK_MODE_MISSION     81
#define SPARK_MODE_CHALLENGE   82
#define SPARK_MODE_BRIEFING    83
#define SPARK_MODE_GAMEPLAY    84
#define SPARK_MODE_RESULTS     85
#define SPARK_MODE_DEBRIEFING  86
#define SPARK_MODE_SETTINGS    87
#define SPARK_MODE_STATS       88

// ============================================
// Static Screen Pointers
// ============================================

static lv_obj_t* spark_screen = NULL;

// Gameplay screen elements
static lv_obj_t* spark_briefing_label = NULL;
static lv_obj_t* spark_speed_buttons[6] = {NULL};
static lv_obj_t* spark_score_label = NULL;
static lv_obj_t* spark_play_btn = NULL;
static lv_obj_t* spark_pause_btn = NULL;
static lv_obj_t* spark_replay_btn = NULL;

// Form input elements
static lv_obj_t* spark_signal_input = NULL;
static lv_obj_t* spark_ship_input = NULL;
static lv_obj_t* spark_distress_input = NULL;
static lv_obj_t* spark_lat_deg_input = NULL;
static lv_obj_t* spark_lat_min_input = NULL;
static lv_obj_t* spark_lat_dir_btn = NULL;
static lv_obj_t* spark_lon_deg_input = NULL;
static lv_obj_t* spark_lon_min_input = NULL;
static lv_obj_t* spark_lon_dir_btn = NULL;

// Additional gameplay screen elements
static lv_obj_t* spark_focus_container = NULL;
static lv_obj_t* spark_submit_btn = NULL;
static lv_obj_t* spark_ref_btn = NULL;
static lv_obj_t* spark_hint_btn = NULL;
static lv_obj_t* spark_hint_modal = NULL;
static lv_obj_t* spark_ref_modal = NULL;
static lv_obj_t* spark_position_row = NULL;
static lv_obj_t* spark_lat_deg_label = NULL;
static lv_obj_t* spark_lat_min_label = NULL;
static lv_obj_t* spark_lon_deg_label = NULL;
static lv_obj_t* spark_lon_min_label = NULL;

// Results screen elements
static lv_obj_t* spark_retry_btn = NULL;
static lv_obj_t* spark_continue_btn = NULL;

// Selected difficulty and challenge for navigation
static SparkWatchDifficulty spark_selected_difficulty = SPARK_EASY;
static int spark_selected_challenge_index = 0;
static int spark_selected_campaign_id = 0;
static int spark_selected_mission = 0;

// ============================================
// Cleanup Functions
// ============================================

static void cleanupSparkScreenPointers() {
    spark_screen = NULL;
    spark_briefing_label = NULL;
    spark_score_label = NULL;
    spark_play_btn = NULL;
    spark_pause_btn = NULL;
    spark_replay_btn = NULL;
    spark_signal_input = NULL;
    spark_ship_input = NULL;
    spark_distress_input = NULL;
    spark_lat_deg_input = NULL;
    spark_lat_min_input = NULL;
    spark_lat_dir_btn = NULL;
    spark_lon_deg_input = NULL;
    spark_lon_min_input = NULL;
    spark_lon_dir_btn = NULL;
    for (int i = 0; i < 6; i++) {
        spark_speed_buttons[i] = NULL;
    }

    // Gameplay screen pointers
    spark_focus_container = NULL;
    spark_submit_btn = NULL;
    spark_ref_btn = NULL;
    spark_hint_btn = NULL;
    spark_hint_modal = NULL;
    spark_ref_modal = NULL;
    spark_position_row = NULL;
    spark_lat_deg_label = NULL;
    spark_lat_min_label = NULL;
    spark_lon_deg_label = NULL;
    spark_lon_min_label = NULL;

    // Results screen pointers
    spark_retry_btn = NULL;
    spark_continue_btn = NULL;
}

// ============================================
// Event Callbacks
// ============================================

static void spark_menu_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int target = (int)(intptr_t)lv_event_get_user_data(e);
        onLVGLMenuSelect(target);
    }
}

static void spark_difficulty_select_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        SparkWatchDifficulty diff = (SparkWatchDifficulty)(intptr_t)lv_event_get_user_data(e);
        spark_selected_difficulty = diff;
        onLVGLMenuSelect(SPARK_MODE_CHALLENGE);
    }
}

static void spark_challenge_select_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int index = (int)(intptr_t)lv_event_get_user_data(e);
        spark_selected_challenge_index = index;

        // Set up the session
        const SparkWatchChallenge* challenge = getChallengeByIndex(index);
        if (challenge) {
            resetSparkWatchSession();
            sparkSession.currentChallenge = challenge;
            sparkSession.challengeIndex = index;

            // Set minimum speed for difficulty
            int minSpeedIdx = SPARK_MIN_SPEED_INDEX[challenge->difficulty];
            if (sparkSession.speedIndex < minSpeedIdx) {
                setSparkSpeed(minSpeedIdx);
            }

            onLVGLMenuSelect(SPARK_MODE_BRIEFING);
        }
    }
}

static void spark_campaign_select_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int campaignId = (int)(intptr_t)lv_event_get_user_data(e);
        spark_selected_campaign_id = campaignId;
        onLVGLMenuSelect(SPARK_MODE_MISSION);
    }
}

static void spark_key_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ESC) {
        onLVGLBackNavigation();
        lv_event_stop_processing(e);
    }
}

/*
 * Linear navigation handler for Spark Watch menus
 * Converts UP/DOWN arrow keys to PREV/NEXT for LVGL's linear group navigation
 * This is needed because CardKB sends LV_KEY_UP/DOWN but LVGL group navigation
 * uses LV_KEY_PREV/NEXT by default.
 */
static void spark_linear_nav_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // Only handle UP/DOWN - let other keys pass through
    if (key != LV_KEY_UP && key != LV_KEY_DOWN) return;

    // Stop default handling
    lv_event_stop_processing(e);

    // Get the input group and navigate
    lv_group_t* group = getLVGLInputGroup();
    if (!group) return;

    if (key == LV_KEY_UP) {
        lv_group_focus_prev(group);
    } else if (key == LV_KEY_DOWN) {
        lv_group_focus_next(group);
    }

    // Scroll focused object into view
    lv_obj_t* focused = lv_group_get_focused(group);
    if (focused) {
        lv_obj_scroll_to_view(focused, LV_ANIM_ON);
    }
}

/*
 * Helper to add navigation handler to a widget
 * Adds both ESC handling and UP/DOWN linear navigation
 */
static void addSparkNavHandler(lv_obj_t* widget) {
    lv_obj_add_event_cb(widget, spark_linear_nav_handler, LV_EVENT_KEY, NULL);
}

// ============================================
// Main Menu Screen
// ============================================

lv_obj_t* createSparkWatchMenuScreen() {
    cleanupSparkScreenPointers();
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    createCompactStatusBar(screen);

    // Title
    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "Spark Watch");
    lv_obj_set_style_text_font(title, getThemeFonts()->font_title, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Subtitle
    lv_obj_t* subtitle = lv_label_create(screen);
    lv_label_set_text(subtitle, "Maritime Morse Training");
    lv_obj_set_style_text_font(subtitle, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(subtitle, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 40);

    // Menu container
    lv_obj_t* menu_cont = lv_obj_create(screen);
    lv_obj_set_size(menu_cont, SCREEN_WIDTH - 40, 200);
    lv_obj_align(menu_cont, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_layout(menu_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(menu_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(menu_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(menu_cont, 10, 0);
    lv_obj_set_style_pad_all(menu_cont, 15, 0);
    lv_obj_set_style_bg_opa(menu_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(menu_cont, 0, 0);

    // Menu items
    const char* menu_items[] = {"Quick Play", "Campaigns", "Settings", "Statistics"};
    const char* menu_icons[] = {LV_SYMBOL_PLAY, LV_SYMBOL_LIST, LV_SYMBOL_SETTINGS, LV_SYMBOL_CHARGE};
    int menu_modes[] = {SPARK_MODE_DIFFICULTY, SPARK_MODE_CAMPAIGN, SPARK_MODE_SETTINGS, SPARK_MODE_STATS};

    for (int i = 0; i < 4; i++) {
        lv_obj_t* btn = createMenuCard(menu_cont, menu_icons[i], menu_items[i],
                                       spark_menu_event_cb, (void*)(intptr_t)menu_modes[i]);
        lv_obj_set_width(btn, SCREEN_WIDTH - 80);
        // Add UP/DOWN navigation handler (createMenuCard already adds to nav group)
        addSparkNavHandler(btn);
    }

    // Stats display at bottom
    lv_obj_t* stats = lv_label_create(screen);
    char stats_text[64];
    snprintf(stats_text, sizeof(stats_text), "Total Score: %d  |  Completed: %d",
             sparkProgress.totalScore, sparkProgress.challengesCompleted);
    lv_label_set_text(stats, stats_text);
    lv_obj_set_style_text_font(stats, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(stats, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(stats, LV_ALIGN_BOTTOM_MID, 0, -10);

    spark_screen = screen;
    return screen;
}

// ============================================
// Difficulty Select Screen
// ============================================

lv_obj_t* createSparkWatchDifficultyScreen() {
    cleanupSparkScreenPointers();
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    createCompactStatusBar(screen);

    // Title
    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "Select Difficulty");
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Difficulty buttons container
    lv_obj_t* diff_cont = lv_obj_create(screen);
    lv_obj_set_size(diff_cont, SCREEN_WIDTH - 40, 240);
    lv_obj_align(diff_cont, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_layout(diff_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(diff_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(diff_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(diff_cont, 8, 0);
    lv_obj_set_style_pad_all(diff_cont, 10, 0);
    lv_obj_set_style_bg_opa(diff_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(diff_cont, 0, 0);

    // Difficulty options with descriptions
    const char* diff_labels[] = {
        "Easy - Signal + Ship (25 pts)",
        "Medium - + Nature (50 pts)",
        "Hard - + Position (100 pts)",
        "Expert - Complex (150 pts)",
        "Master - Full Speed (250 pts)"
    };
    const char* diff_icons[] = {LV_SYMBOL_OK, LV_SYMBOL_PLUS, LV_SYMBOL_GPS, LV_SYMBOL_WARNING, LV_SYMBOL_CHARGE};

    for (int i = 0; i < 5; i++) {
        lv_obj_t* btn = createMenuCard(diff_cont, diff_icons[i], diff_labels[i],
                                       spark_difficulty_select_cb, (void*)(intptr_t)i);
        lv_obj_set_width(btn, SCREEN_WIDTH - 80);
        // Add UP/DOWN navigation handler (createMenuCard already adds to nav group)
        addSparkNavHandler(btn);

        // Show completed count
        char count_text[32];
        snprintf(count_text, sizeof(count_text), "(%d completed)",
                 sparkProgress.completedByDifficulty[i]);
        lv_obj_t* count_label = lv_label_create(btn);
        lv_label_set_text(count_label, count_text);
        lv_obj_set_style_text_font(count_label, getThemeFonts()->font_small, 0);
        lv_obj_set_style_text_color(count_label, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_align(count_label, LV_ALIGN_RIGHT_MID, -10, 0);
    }

    spark_screen = screen;
    return screen;
}

// ============================================
// Challenge Select Screen
// ============================================

lv_obj_t* createSparkWatchChallengeScreen() {
    cleanupSparkScreenPointers();
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    createCompactStatusBar(screen);

    // Title with difficulty name
    lv_obj_t* title = lv_label_create(screen);
    char title_text[64];
    snprintf(title_text, sizeof(title_text), "%s Challenges",
             SPARK_DIFFICULTY_NAMES[spark_selected_difficulty]);
    lv_label_set_text(title, title_text);
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Scrollable challenge list
    lv_obj_t* list = lv_obj_create(screen);
    lv_obj_set_size(list, SCREEN_WIDTH - 20, 250);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(list, 5, 0);
    lv_obj_set_style_pad_all(list, 10, 0);
    lv_obj_set_style_bg_color(list, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_radius(list, 8, 0);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);

    // Get challenges for this difficulty
    const SparkWatchChallenge* challenges[20];
    int count = getChallengesByDifficulty(spark_selected_difficulty, challenges, 20);

    for (int i = 0; i < count; i++) {
        // Find actual index in main array
        int actualIndex = -1;
        for (int j = 0; j < SPARK_CHALLENGE_COUNT; j++) {
            if (&sparkChallenges[j] == challenges[i]) {
                actualIndex = j;
                break;
            }
        }

        lv_obj_t* btn = lv_btn_create(list);
        lv_obj_set_size(btn, SCREEN_WIDTH - 60, 45);
        lv_obj_add_style(btn, getStyleMenuCard(), 0);
        lv_obj_add_style(btn, getStyleMenuCardFocused(), LV_STATE_FOCUSED);
        lv_obj_add_event_cb(btn, spark_challenge_select_cb, LV_EVENT_CLICKED, (void*)(intptr_t)actualIndex);
        addNavigableWidget(btn);
        addSparkNavHandler(btn);

        // Challenge title
        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text(label, challenges[i]->title);
        lv_obj_set_style_text_font(label, getThemeFonts()->font_body, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 0);
    }

    if (count == 0) {
        lv_obj_t* empty = lv_label_create(list);
        lv_label_set_text(empty, "No challenges available");
        lv_obj_set_style_text_color(empty, LV_COLOR_TEXT_SECONDARY, 0);
    }

    spark_screen = screen;
    return screen;
}

// ============================================
// Campaign Select Screen
// ============================================

lv_obj_t* createSparkWatchCampaignScreen() {
    cleanupSparkScreenPointers();
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    createCompactStatusBar(screen);

    // Title
    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "Campaigns");
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Campaign list
    lv_obj_t* list = lv_obj_create(screen);
    lv_obj_set_size(list, SCREEN_WIDTH - 20, 250);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(list, 8, 0);
    lv_obj_set_style_pad_all(list, 10, 0);
    lv_obj_set_style_bg_color(list, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_radius(list, 8, 0);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < SPARK_CAMPAIGN_COUNT; i++) {
        const SparkWatchCampaign* camp = &sparkCampaigns[i];
        bool unlocked = sparkProgress.campaignUnlocked[camp->id];
        int progress = sparkProgress.campaignProgress[camp->id];

        lv_obj_t* btn = lv_btn_create(list);
        lv_obj_set_size(btn, SCREEN_WIDTH - 60, 55);
        lv_obj_add_style(btn, getStyleMenuCard(), 0);
        lv_obj_add_style(btn, getStyleMenuCardFocused(), LV_STATE_FOCUSED);

        if (unlocked) {
            lv_obj_add_event_cb(btn, spark_campaign_select_cb, LV_EVENT_CLICKED,
                               (void*)(intptr_t)camp->id);
            addNavigableWidget(btn);
            addSparkNavHandler(btn);
        } else {
            lv_obj_set_style_bg_opa(btn, LV_OPA_50, 0);
        }

        // Campaign name
        lv_obj_t* name = lv_label_create(btn);
        lv_label_set_text(name, camp->name);
        lv_obj_set_style_text_font(name, getThemeFonts()->font_body, 0);
        lv_obj_align(name, LV_ALIGN_TOP_LEFT, 10, 5);

        // Ship name and progress
        lv_obj_t* info = lv_label_create(btn);
        char info_text[64];
        if (unlocked) {
            snprintf(info_text, sizeof(info_text), "%s  |  %d/%d missions",
                     camp->ship, progress, camp->totalMissions);
        } else {
            snprintf(info_text, sizeof(info_text), "Locked - Complete previous campaign");
        }
        lv_label_set_text(info, info_text);
        lv_obj_set_style_text_font(info, getThemeFonts()->font_small, 0);
        lv_obj_set_style_text_color(info, unlocked ? LV_COLOR_TEXT_SECONDARY : LV_COLOR_WARNING, 0);
        lv_obj_align(info, LV_ALIGN_BOTTOM_LEFT, 10, -5);
    }

    spark_screen = screen;
    return screen;
}

// ============================================
// Briefing Screen
// ============================================

lv_obj_t* createSparkWatchBriefingScreen() {
    cleanupSparkScreenPointers();
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    createCompactStatusBar(screen);

    const SparkWatchChallenge* ch = sparkSession.currentChallenge;
    if (!ch) {
        lv_obj_t* error = lv_label_create(screen);
        lv_label_set_text(error, "No challenge selected");
        lv_obj_center(error);
        return screen;
    }

    // Title
    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, ch->title);
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Difficulty badge
    lv_obj_t* diff_badge = lv_label_create(screen);
    char badge_text[32];
    snprintf(badge_text, sizeof(badge_text), "[%s - %d pts]",
             SPARK_DIFFICULTY_NAMES[ch->difficulty],
             (int)(SPARK_BASE_POINTS[ch->difficulty] * sparkSession.currentSpeedMult));
    lv_label_set_text(diff_badge, badge_text);
    lv_obj_set_style_text_font(diff_badge, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(diff_badge, LV_COLOR_SUCCESS, 0);
    lv_obj_align(diff_badge, LV_ALIGN_TOP_MID, 0, 35);

    // Briefing text
    lv_obj_t* briefing_cont = lv_obj_create(screen);
    lv_obj_set_size(briefing_cont, SCREEN_WIDTH - 40, 120);
    lv_obj_align(briefing_cont, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(briefing_cont, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_radius(briefing_cont, 8, 0);
    lv_obj_set_style_pad_all(briefing_cont, 10, 0);

    lv_obj_t* briefing = lv_label_create(briefing_cont);
    lv_label_set_text(briefing, ch->briefing);
    lv_label_set_long_mode(briefing, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(briefing, SCREEN_WIDTH - 70);
    lv_obj_set_style_text_font(briefing, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(briefing, LV_COLOR_TEXT_PRIMARY, 0);

    // Speed selection - positioned higher to avoid overlap with start button
    lv_obj_t* speed_label = lv_label_create(screen);
    lv_label_set_text(speed_label, "Playback Speed:");
    lv_obj_set_style_text_font(speed_label, getThemeFonts()->font_body, 0);
    lv_obj_align(speed_label, LV_ALIGN_TOP_LEFT, 20, 185);

    lv_obj_t* speed_cont = lv_obj_create(screen);
    lv_obj_set_size(speed_cont, SCREEN_WIDTH - 40, 36);
    lv_obj_align(speed_cont, LV_ALIGN_TOP_MID, 0, 205);
    lv_obj_set_layout(speed_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(speed_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(speed_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(speed_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(speed_cont, 0, 0);
    lv_obj_set_style_pad_all(speed_cont, 0, 0);

    int minSpeed = SPARK_MIN_SPEED_INDEX[ch->difficulty];
    const char* speed_labels[] = {"0.5x", "0.75x", "1.0x", "1.25x", "1.5x", "2.0x"};
    int firstNavigableSpeed = -1;

    for (int i = 0; i < 6; i++) {
        lv_obj_t* btn = lv_btn_create(speed_cont);
        lv_obj_set_size(btn, 58, 28);

        if (i < minSpeed) {
            // Disabled - below minimum for difficulty
            lv_obj_set_style_bg_opa(btn, LV_OPA_30, 0);
            lv_obj_set_style_bg_color(btn, LV_COLOR_BG_LAYER2, 0);
        } else {
            // All enabled buttons are navigable
            if (firstNavigableSpeed < 0) firstNavigableSpeed = i;

            if (i == sparkSession.speedIndex) {
                // Currently selected - show with cyan background
                lv_obj_set_style_bg_color(btn, LV_COLOR_ACCENT_CYAN, 0);
                lv_obj_set_style_text_color(btn, lv_color_black(), 0);
            } else {
                // Not selected - normal card style
                lv_obj_add_style(btn, getStyleMenuCard(), 0);
            }
            // Focused style when navigated to (glow effect)
            lv_obj_add_style(btn, getStyleMenuCardFocused(), LV_STATE_FOCUSED);
            addNavigableWidget(btn);
            addSparkNavHandler(btn);
        }

        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text(label, speed_labels[i]);
        lv_obj_set_style_text_font(label, getThemeFonts()->font_small, 0);
        if (i == sparkSession.speedIndex && i >= minSpeed) {
            lv_obj_set_style_text_color(label, lv_color_black(), 0);
        }
        lv_obj_center(label);

        spark_speed_buttons[i] = btn;
    }

    // Start button - smaller to fit below speed buttons with good spacing
    lv_obj_t* start_btn = lv_btn_create(screen);
    lv_obj_set_size(start_btn, 160, 40);
    lv_obj_align(start_btn, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_style_bg_color(start_btn, LV_COLOR_SUCCESS, 0);
    lv_obj_add_style(start_btn, getStyleMenuCardFocused(), LV_STATE_FOCUSED);
    lv_obj_add_event_cb(start_btn, spark_menu_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SPARK_MODE_GAMEPLAY);
    addNavigableWidget(start_btn);
    addSparkNavHandler(start_btn);

    lv_obj_t* start_label = lv_label_create(start_btn);
    lv_label_set_text(start_label, "Begin");
    lv_obj_set_style_text_font(start_label, getThemeFonts()->font_body, 0);
    lv_obj_center(start_label);

    spark_screen = screen;
    return screen;
}

// ============================================
// Gameplay Screen - Full Implementation
// ============================================

// Gameplay state tracking (pointer declarations moved to top of file)
static int spark_gameplay_focus = 0;  // 0=Play, 1=Pause, 2=Replay, 3=Signal, 4=Ship, 5=Distress, 6+=Position, N=Submit, N+1=Ref, N+2=Hint

// Get total number of focusable items for current challenge
static int getSparkFocusCount() {
    if (!sparkSession.currentChallenge) return 6;  // Play, Pause, Replay, Signal, Ship, Submit

    int count = 6;  // Base: 3 playback + Signal + Ship + Submit
    if (sparkSession.currentChallenge->difficulty >= SPARK_MEDIUM) count++;  // Distress
    if (sparkSession.currentChallenge->difficulty >= SPARK_HARD) count += 6; // Position (6 fields)
    count++;  // Morse Ref button
    if (sparkSession.currentChallenge->hint) count++;  // Hint button
    return count;
}

// Get field index offsets based on difficulty
static int getSparkFieldOffset() {
    return 3;  // Play, Pause, Replay take indices 0-2
}

static int getSparkSubmitIndex() {
    int idx = 3 + 2;  // Base: Signal, Ship
    if (sparkSession.currentChallenge && sparkSession.currentChallenge->difficulty >= SPARK_MEDIUM) idx++;
    if (sparkSession.currentChallenge && sparkSession.currentChallenge->difficulty >= SPARK_HARD) idx += 6;
    return idx;
}

// Forward declarations
static void spark_gameplay_update_focus();
static void spark_copy_inputs_to_session();
static void spark_show_hint_modal();
static void spark_show_ref_modal();
static void spark_close_modal();

// Update visual focus indicator on gameplay screen
static void spark_gameplay_update_focus() {
    // Update button highlighting
    if (spark_play_btn) {
        lv_obj_set_style_bg_color(spark_play_btn,
            (spark_gameplay_focus == 0) ? LV_COLOR_ACCENT_CYAN : LV_COLOR_SUCCESS, 0);
    }
    if (spark_pause_btn) {
        lv_obj_set_style_bg_color(spark_pause_btn,
            (spark_gameplay_focus == 1) ? LV_COLOR_ACCENT_CYAN : LV_COLOR_BG_LAYER2, 0);
    }
    if (spark_replay_btn) {
        lv_obj_set_style_bg_color(spark_replay_btn,
            (spark_gameplay_focus == 2) ? LV_COLOR_ACCENT_CYAN : LV_COLOR_BG_LAYER2, 0);
    }

    // Update textarea borders based on focus
    int fieldStart = getSparkFieldOffset();
    if (spark_signal_input) {
        lv_obj_set_style_border_color(spark_signal_input,
            (spark_gameplay_focus == fieldStart) ? LV_COLOR_ACCENT_CYAN : LV_COLOR_BORDER_SUBTLE, 0);
        lv_obj_set_style_border_width(spark_signal_input, (spark_gameplay_focus == fieldStart) ? 2 : 1, 0);
    }
    if (spark_ship_input) {
        lv_obj_set_style_border_color(spark_ship_input,
            (spark_gameplay_focus == fieldStart + 1) ? LV_COLOR_ACCENT_CYAN : LV_COLOR_BORDER_SUBTLE, 0);
        lv_obj_set_style_border_width(spark_ship_input, (spark_gameplay_focus == fieldStart + 1) ? 2 : 1, 0);
    }
    if (spark_distress_input) {
        lv_obj_set_style_border_color(spark_distress_input,
            (spark_gameplay_focus == fieldStart + 2) ? LV_COLOR_ACCENT_CYAN : LV_COLOR_BORDER_SUBTLE, 0);
        lv_obj_set_style_border_width(spark_distress_input, (spark_gameplay_focus == fieldStart + 2) ? 2 : 1, 0);
    }

    // Position field highlights (Hard+)
    if (sparkSession.currentChallenge && sparkSession.currentChallenge->difficulty >= SPARK_HARD) {
        int posStart = fieldStart + 3;
        if (spark_lat_deg_input) {
            lv_obj_set_style_border_color(spark_lat_deg_input,
                (spark_gameplay_focus == posStart) ? LV_COLOR_ACCENT_CYAN : LV_COLOR_BORDER_SUBTLE, 0);
            lv_obj_set_style_border_width(spark_lat_deg_input, (spark_gameplay_focus == posStart) ? 2 : 1, 0);
        }
        if (spark_lat_min_input) {
            lv_obj_set_style_border_color(spark_lat_min_input,
                (spark_gameplay_focus == posStart + 1) ? LV_COLOR_ACCENT_CYAN : LV_COLOR_BORDER_SUBTLE, 0);
            lv_obj_set_style_border_width(spark_lat_min_input, (spark_gameplay_focus == posStart + 1) ? 2 : 1, 0);
        }
        if (spark_lat_dir_btn) {
            lv_obj_set_style_bg_color(spark_lat_dir_btn,
                (spark_gameplay_focus == posStart + 2) ? LV_COLOR_ACCENT_CYAN : LV_COLOR_BG_LAYER2, 0);
        }
        if (spark_lon_deg_input) {
            lv_obj_set_style_border_color(spark_lon_deg_input,
                (spark_gameplay_focus == posStart + 3) ? LV_COLOR_ACCENT_CYAN : LV_COLOR_BORDER_SUBTLE, 0);
            lv_obj_set_style_border_width(spark_lon_deg_input, (spark_gameplay_focus == posStart + 3) ? 2 : 1, 0);
        }
        if (spark_lon_min_input) {
            lv_obj_set_style_border_color(spark_lon_min_input,
                (spark_gameplay_focus == posStart + 4) ? LV_COLOR_ACCENT_CYAN : LV_COLOR_BORDER_SUBTLE, 0);
            lv_obj_set_style_border_width(spark_lon_min_input, (spark_gameplay_focus == posStart + 4) ? 2 : 1, 0);
        }
        if (spark_lon_dir_btn) {
            lv_obj_set_style_bg_color(spark_lon_dir_btn,
                (spark_gameplay_focus == posStart + 5) ? LV_COLOR_ACCENT_CYAN : LV_COLOR_BG_LAYER2, 0);
        }
    }

    // Bottom buttons
    int submitIdx = getSparkSubmitIndex();
    if (spark_submit_btn) {
        lv_obj_set_style_bg_color(spark_submit_btn,
            (spark_gameplay_focus == submitIdx) ? LV_COLOR_ACCENT_CYAN : LV_COLOR_SUCCESS, 0);
    }
    if (spark_ref_btn) {
        lv_obj_set_style_bg_color(spark_ref_btn,
            (spark_gameplay_focus == submitIdx + 1) ? LV_COLOR_ACCENT_CYAN : LV_COLOR_BG_LAYER2, 0);
    }
    if (spark_hint_btn) {
        lv_obj_set_style_bg_color(spark_hint_btn,
            (spark_gameplay_focus == submitIdx + 2) ? LV_COLOR_ACCENT_CYAN : LV_COLOR_BG_LAYER2, 0);
    }
}

// Copy textarea values to session struct
static void spark_copy_inputs_to_session() {
    if (spark_signal_input) {
        strncpy(sparkSession.inputSignalType, lv_textarea_get_text(spark_signal_input), SPARK_MAX_SIGNAL_TYPE - 1);
    }
    if (spark_ship_input) {
        strncpy(sparkSession.inputShipName, lv_textarea_get_text(spark_ship_input), SPARK_MAX_SHIP_NAME - 1);
    }
    if (spark_distress_input) {
        strncpy(sparkSession.inputDistressNature, lv_textarea_get_text(spark_distress_input), SPARK_MAX_DISTRESS - 1);
    }
    if (spark_lat_deg_input) {
        strncpy(sparkSession.inputLatDegrees, lv_textarea_get_text(spark_lat_deg_input), SPARK_MAX_POSITION - 1);
    }
    if (spark_lat_min_input) {
        strncpy(sparkSession.inputLatMinutes, lv_textarea_get_text(spark_lat_min_input), SPARK_MAX_POSITION - 1);
    }
    if (spark_lon_deg_input) {
        strncpy(sparkSession.inputLonDegrees, lv_textarea_get_text(spark_lon_deg_input), SPARK_MAX_POSITION - 1);
    }
    if (spark_lon_min_input) {
        strncpy(sparkSession.inputLonMinutes, lv_textarea_get_text(spark_lon_min_input), SPARK_MAX_POSITION - 1);
    }
}

// Update score label
static void spark_update_score_display() {
    if (spark_score_label) {
        int potential = getPotentialScore();
        int penalties = sparkSession.penaltyPoints;
        lv_label_set_text_fmt(spark_score_label, "Score: %d (-%d)", potential - penalties, penalties);
    }
}

// Show hint modal
static void spark_show_hint_modal() {
    if (!sparkSession.currentChallenge || !sparkSession.currentChallenge->hint) return;

    applyHintPenalty();
    spark_update_score_display();

    // Create modal overlay
    spark_hint_modal = lv_obj_create(lv_scr_act());
    lv_obj_set_size(spark_hint_modal, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(spark_hint_modal, 0, 0);
    lv_obj_set_style_bg_color(spark_hint_modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(spark_hint_modal, LV_OPA_70, 0);
    lv_obj_clear_flag(spark_hint_modal, LV_OBJ_FLAG_SCROLLABLE);

    // Card
    lv_obj_t* card = lv_obj_create(spark_hint_modal);
    lv_obj_set_size(card, SCREEN_WIDTH - 60, 140);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_pad_all(card, 15, 0);

    lv_obj_t* title = lv_label_create(card);
    lv_label_set_text(title, "Hint (-2 pts)");
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_WARNING, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* hint = lv_label_create(card);
    lv_label_set_text(hint, sparkSession.currentChallenge->hint);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(hint, SCREEN_WIDTH - 100);
    lv_obj_set_style_text_font(hint, getThemeFonts()->font_body, 0);
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* close_hint = lv_label_create(card);
    lv_label_set_text(close_hint, "Press any key to close");
    lv_obj_set_style_text_font(close_hint, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(close_hint, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(close_hint, LV_ALIGN_BOTTOM_MID, 0, 0);
}

// Show morse reference modal
static void spark_show_ref_modal() {
    applyReferencePenalty();
    spark_update_score_display();

    spark_ref_modal = lv_obj_create(lv_scr_act());
    lv_obj_set_size(spark_ref_modal, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(spark_ref_modal, 0, 0);
    lv_obj_set_style_bg_color(spark_ref_modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(spark_ref_modal, LV_OPA_70, 0);
    lv_obj_clear_flag(spark_ref_modal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* card = lv_obj_create(spark_ref_modal);
    lv_obj_set_size(card, SCREEN_WIDTH - 40, 220);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_pad_all(card, 10, 0);

    lv_obj_t* title = lv_label_create(card);
    lv_label_set_text(title, "Morse Reference (-5 pts)");
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_WARNING, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // Common distress signals
    lv_obj_t* ref = lv_label_create(card);
    lv_label_set_text(ref,
        "CQD = -.-. --.- -..  (Old distress)\n"
        "SOS = ... --- ...  (Distress)\n"
        "DE = -.. .  (From)\n"
        "N = -.  S = ...  E = .  W = .--\n"
        "Numbers: 0-9 spelled out");
    lv_obj_set_style_text_font(ref, getThemeFonts()->font_body, 0);
    lv_obj_align(ref, LV_ALIGN_CENTER, 0, 5);

    lv_obj_t* close_hint = lv_label_create(card);
    lv_label_set_text(close_hint, "Press any key to close");
    lv_obj_set_style_text_font(close_hint, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(close_hint, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(close_hint, LV_ALIGN_BOTTOM_MID, 0, 0);
}

// Close any open modal
static void spark_close_modal() {
    if (spark_hint_modal) {
        lv_obj_del(spark_hint_modal);
        spark_hint_modal = NULL;
    }
    if (spark_ref_modal) {
        lv_obj_del(spark_ref_modal);
        spark_ref_modal = NULL;
    }
}

// Key event callback for gameplay screen
static void spark_gameplay_key_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // If modal is open, close it on any key
    if (spark_hint_modal || spark_ref_modal) {
        spark_close_modal();
        lv_event_stop_processing(e);
        return;
    }

    int maxFocus = getSparkFocusCount() - 1;
    int fieldStart = getSparkFieldOffset();
    int submitIdx = getSparkSubmitIndex();

    switch(key) {
        case LV_KEY_ESC:
            stopSparkTransmission();
            onLVGLBackNavigation();
            lv_event_stop_processing(e);
            break;

        case LV_KEY_UP:
            spark_gameplay_focus--;
            if (spark_gameplay_focus < 0) spark_gameplay_focus = maxFocus;
            spark_gameplay_update_focus();
            beep(TONE_MENU_NAV, BEEP_SHORT);
            break;

        case LV_KEY_DOWN:
            spark_gameplay_focus++;
            if (spark_gameplay_focus > maxFocus) spark_gameplay_focus = 0;
            spark_gameplay_update_focus();
            beep(TONE_MENU_NAV, BEEP_SHORT);
            break;

        case LV_KEY_LEFT:
        case LV_KEY_RIGHT:
            // For direction buttons, toggle N/S or E/W
            if (sparkSession.currentChallenge && sparkSession.currentChallenge->difficulty >= SPARK_HARD) {
                int posStart = fieldStart + 3;
                if (spark_gameplay_focus == posStart + 2 && spark_lat_dir_btn) {
                    // Toggle lat direction
                    sparkSession.inputLatDirection = (sparkSession.inputLatDirection == 'N') ? 'S' : 'N';
                    char dir[2] = {sparkSession.inputLatDirection, '\0'};
                    lv_obj_t* lbl = lv_obj_get_child(spark_lat_dir_btn, 0);
                    if (lbl) lv_label_set_text(lbl, dir);
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                } else if (spark_gameplay_focus == posStart + 5 && spark_lon_dir_btn) {
                    // Toggle lon direction
                    sparkSession.inputLonDirection = (sparkSession.inputLonDirection == 'E') ? 'W' : 'E';
                    char dir[2] = {sparkSession.inputLonDirection, '\0'};
                    lv_obj_t* lbl = lv_obj_get_child(spark_lon_dir_btn, 0);
                    if (lbl) lv_label_set_text(lbl, dir);
                    beep(TONE_MENU_NAV, BEEP_SHORT);
                }
            }
            break;

        case LV_KEY_ENTER:
            // Handle button presses
            if (spark_gameplay_focus == 0) {
                // Play
                playSparkTransmission();
                beep(TONE_SELECT, BEEP_SHORT);
            } else if (spark_gameplay_focus == 1) {
                // Pause (stop)
                stopSparkTransmission();
                beep(TONE_SELECT, BEEP_SHORT);
            } else if (spark_gameplay_focus == 2) {
                // Replay
                stopSparkTransmission();
                sparkSession.playbackCharIndex = 0;
                playSparkTransmission();
                beep(TONE_SELECT, BEEP_SHORT);
            } else if (spark_gameplay_focus == submitIdx) {
                // Submit
                spark_copy_inputs_to_session();
                beep(TONE_SELECT, BEEP_MEDIUM);
                onLVGLMenuSelect(SPARK_MODE_RESULTS);
                lv_event_stop_processing(e);
            } else if (spark_gameplay_focus == submitIdx + 1) {
                // Morse Reference
                spark_show_ref_modal();
                beep(TONE_SELECT, BEEP_SHORT);
            } else if (spark_gameplay_focus == submitIdx + 2 && sparkSession.currentChallenge && sparkSession.currentChallenge->hint) {
                // Hint
                spark_show_hint_modal();
                beep(TONE_SELECT, BEEP_SHORT);
            }
            break;

        default:
            // For text input fields, handle typing
            if (spark_gameplay_focus >= fieldStart && spark_gameplay_focus < submitIdx) {
                // Determine which textarea should receive input
                lv_obj_t* active_ta = NULL;
                int posStart = fieldStart + 3;

                if (spark_gameplay_focus == fieldStart && spark_signal_input) {
                    active_ta = spark_signal_input;
                } else if (spark_gameplay_focus == fieldStart + 1 && spark_ship_input) {
                    active_ta = spark_ship_input;
                } else if (spark_gameplay_focus == fieldStart + 2 && spark_distress_input) {
                    active_ta = spark_distress_input;
                } else if (sparkSession.currentChallenge && sparkSession.currentChallenge->difficulty >= SPARK_HARD) {
                    if (spark_gameplay_focus == posStart && spark_lat_deg_input) {
                        active_ta = spark_lat_deg_input;
                    } else if (spark_gameplay_focus == posStart + 1 && spark_lat_min_input) {
                        active_ta = spark_lat_min_input;
                    } else if (spark_gameplay_focus == posStart + 3 && spark_lon_deg_input) {
                        active_ta = spark_lon_deg_input;
                    } else if (spark_gameplay_focus == posStart + 4 && spark_lon_min_input) {
                        active_ta = spark_lon_min_input;
                    }
                }

                if (active_ta) {
                    if (key == LV_KEY_BACKSPACE) {
                        lv_textarea_del_char(active_ta);
                    } else if (key >= 32 && key <= 126) {
                        // Convert to uppercase for text fields
                        char ch = toupper((char)key);
                        char str[2] = {ch, '\0'};
                        lv_textarea_add_text(active_ta, str);
                    }
                }
            }
            break;
    }
}

lv_obj_t* createSparkWatchGameplayScreen() {
    cleanupSparkScreenPointers();
    clearNavigationGroup();
    spark_gameplay_focus = 0;  // Start on Play button

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    createCompactStatusBar(screen);

    const SparkWatchChallenge* ch = sparkSession.currentChallenge;
    if (!ch) {
        lv_obj_t* error = lv_label_create(screen);
        lv_label_set_text(error, "No challenge loaded");
        lv_obj_center(error);
        return screen;
    }

    // Title bar
    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, ch->title);
    lv_obj_set_style_text_font(title, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 5);

    // Score display
    spark_score_label = lv_label_create(screen);
    spark_update_score_display();
    lv_obj_set_style_text_font(spark_score_label, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(spark_score_label, LV_COLOR_SUCCESS, 0);
    lv_obj_align(spark_score_label, LV_ALIGN_TOP_RIGHT, -80, 5);

    // Playback controls row
    lv_obj_t* controls = lv_obj_create(screen);
    lv_obj_set_size(controls, SCREEN_WIDTH - 20, 40);
    lv_obj_align(controls, LV_ALIGN_TOP_MID, 0, 28);
    lv_obj_set_layout(controls, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(controls, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(controls, 10, 0);
    lv_obj_set_style_bg_opa(controls, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(controls, 0, 0);
    lv_obj_clear_flag(controls, LV_OBJ_FLAG_SCROLLABLE);

    // Play button
    spark_play_btn = lv_btn_create(controls);
    lv_obj_set_size(spark_play_btn, 75, 32);
    lv_obj_set_style_bg_color(spark_play_btn, LV_COLOR_SUCCESS, 0);
    lv_obj_set_style_radius(spark_play_btn, 6, 0);
    lv_obj_t* play_label = lv_label_create(spark_play_btn);
    lv_label_set_text(play_label, LV_SYMBOL_PLAY " Play");
    lv_obj_set_style_text_font(play_label, getThemeFonts()->font_small, 0);
    lv_obj_center(play_label);

    // Pause button
    spark_pause_btn = lv_btn_create(controls);
    lv_obj_set_size(spark_pause_btn, 75, 32);
    lv_obj_set_style_bg_color(spark_pause_btn, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_radius(spark_pause_btn, 6, 0);
    lv_obj_t* pause_label = lv_label_create(spark_pause_btn);
    lv_label_set_text(pause_label, LV_SYMBOL_PAUSE " Stop");
    lv_obj_set_style_text_font(pause_label, getThemeFonts()->font_small, 0);
    lv_obj_center(pause_label);

    // Replay button
    spark_replay_btn = lv_btn_create(controls);
    lv_obj_set_size(spark_replay_btn, 85, 32);
    lv_obj_set_style_bg_color(spark_replay_btn, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_radius(spark_replay_btn, 6, 0);
    lv_obj_t* replay_label = lv_label_create(spark_replay_btn);
    lv_label_set_text(replay_label, LV_SYMBOL_REFRESH " Replay");
    lv_obj_set_style_text_font(replay_label, getThemeFonts()->font_small, 0);
    lv_obj_center(replay_label);

    // Speed indicator
    lv_obj_t* speed_lbl = lv_label_create(controls);
    char speed_str[16];
    snprintf(speed_str, sizeof(speed_str), "%.2fx", sparkSession.currentSpeedMult);
    lv_label_set_text(speed_lbl, speed_str);
    lv_obj_set_style_text_font(speed_lbl, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(speed_lbl, LV_COLOR_WARNING, 0);

    // Calculate form height based on difficulty
    int formHeight = 85;  // Base: Signal + Ship
    if (ch->difficulty >= SPARK_MEDIUM) formHeight += 35;  // Distress
    if (ch->difficulty >= SPARK_HARD) formHeight += 45;    // Position row

    // Form container - scrollable if needed
    lv_obj_t* form = lv_obj_create(screen);
    lv_obj_set_size(form, SCREEN_WIDTH - 20, formHeight);
    lv_obj_align(form, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_layout(form, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(form, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(form, 5, 0);
    lv_obj_set_style_pad_all(form, 8, 0);
    lv_obj_set_style_bg_color(form, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_radius(form, 8, 0);
    lv_obj_clear_flag(form, LV_OBJ_FLAG_SCROLLABLE);

    // Helper to create form row
    auto createFormRow = [&](lv_obj_t* parent, const char* label_text, int ta_width) -> lv_obj_t* {
        lv_obj_t* row = lv_obj_create(parent);
        lv_obj_set_size(row, LV_PCT(100), 32);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* lbl = lv_label_create(row);
        lv_label_set_text(lbl, label_text);
        lv_obj_set_style_text_font(lbl, getThemeFonts()->font_small, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t* ta = lv_textarea_create(row);
        lv_obj_set_size(ta, ta_width, 28);
        lv_obj_align(ta, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_textarea_set_one_line(ta, true);
        lv_obj_set_style_text_font(ta, getThemeFonts()->font_body, 0);
        lv_obj_set_style_border_width(ta, 1, 0);
        lv_obj_set_style_border_color(ta, LV_COLOR_BORDER_SUBTLE, 0);
        lv_obj_set_style_radius(ta, 4, 0);

        return ta;
    };

    // Signal Type (always)
    spark_signal_input = createFormRow(form, "Signal:", 140);
    lv_textarea_set_max_length(spark_signal_input, SPARK_MAX_SIGNAL_TYPE - 1);
    lv_textarea_set_placeholder_text(spark_signal_input, "CQD or SOS");

    // Ship Name (always)
    spark_ship_input = createFormRow(form, "Ship:", 180);
    lv_textarea_set_max_length(spark_ship_input, SPARK_MAX_SHIP_NAME - 1);
    lv_textarea_set_placeholder_text(spark_ship_input, "Ship name");

    // Nature of Distress (Medium+)
    if (ch->difficulty >= SPARK_MEDIUM) {
        spark_distress_input = createFormRow(form, "Distress:", 180);
        lv_textarea_set_max_length(spark_distress_input, SPARK_MAX_DISTRESS - 1);
        lv_textarea_set_placeholder_text(spark_distress_input, "Type of emergency");
    }

    // Position (Hard+)
    if (ch->difficulty >= SPARK_HARD) {
        spark_position_row = lv_obj_create(form);
        lv_obj_set_size(spark_position_row, LV_PCT(100), 40);
        lv_obj_set_style_bg_opa(spark_position_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(spark_position_row, 0, 0);
        lv_obj_set_style_pad_all(spark_position_row, 0, 0);
        lv_obj_clear_flag(spark_position_row, LV_OBJ_FLAG_SCROLLABLE);

        // Position label
        lv_obj_t* pos_lbl = lv_label_create(spark_position_row);
        lv_label_set_text(pos_lbl, "Pos:");
        lv_obj_set_style_text_font(pos_lbl, getThemeFonts()->font_small, 0);
        lv_obj_align(pos_lbl, LV_ALIGN_LEFT_MID, 0, 0);

        // Lat degrees
        spark_lat_deg_input = lv_textarea_create(spark_position_row);
        lv_obj_set_size(spark_lat_deg_input, 40, 28);
        lv_obj_set_pos(spark_lat_deg_input, 35, 6);
        lv_textarea_set_one_line(spark_lat_deg_input, true);
        lv_textarea_set_max_length(spark_lat_deg_input, 3);
        lv_obj_set_style_text_font(spark_lat_deg_input, getThemeFonts()->font_small, 0);
        lv_obj_set_style_border_width(spark_lat_deg_input, 1, 0);
        lv_obj_set_style_radius(spark_lat_deg_input, 4, 0);

        spark_lat_deg_label = lv_label_create(spark_position_row);
        lv_label_set_text(spark_lat_deg_label, "°");
        lv_obj_set_pos(spark_lat_deg_label, 78, 12);

        // Lat minutes
        spark_lat_min_input = lv_textarea_create(spark_position_row);
        lv_obj_set_size(spark_lat_min_input, 40, 28);
        lv_obj_set_pos(spark_lat_min_input, 88, 6);
        lv_textarea_set_one_line(spark_lat_min_input, true);
        lv_textarea_set_max_length(spark_lat_min_input, 3);
        lv_obj_set_style_text_font(spark_lat_min_input, getThemeFonts()->font_small, 0);
        lv_obj_set_style_border_width(spark_lat_min_input, 1, 0);
        lv_obj_set_style_radius(spark_lat_min_input, 4, 0);

        spark_lat_min_label = lv_label_create(spark_position_row);
        lv_label_set_text(spark_lat_min_label, "'");
        lv_obj_set_pos(spark_lat_min_label, 131, 12);

        // Lat direction button
        spark_lat_dir_btn = lv_btn_create(spark_position_row);
        lv_obj_set_size(spark_lat_dir_btn, 28, 28);
        lv_obj_set_pos(spark_lat_dir_btn, 140, 6);
        lv_obj_set_style_bg_color(spark_lat_dir_btn, LV_COLOR_BG_LAYER2, 0);
        lv_obj_set_style_radius(spark_lat_dir_btn, 4, 0);
        char lat_dir[2] = {sparkSession.inputLatDirection, '\0'};
        lv_obj_t* lat_dir_lbl = lv_label_create(spark_lat_dir_btn);
        lv_label_set_text(lat_dir_lbl, lat_dir);
        lv_obj_center(lat_dir_lbl);

        // Separator
        lv_obj_t* sep = lv_label_create(spark_position_row);
        lv_label_set_text(sep, "/");
        lv_obj_set_pos(sep, 175, 12);

        // Lon degrees
        spark_lon_deg_input = lv_textarea_create(spark_position_row);
        lv_obj_set_size(spark_lon_deg_input, 45, 28);
        lv_obj_set_pos(spark_lon_deg_input, 190, 6);
        lv_textarea_set_one_line(spark_lon_deg_input, true);
        lv_textarea_set_max_length(spark_lon_deg_input, 3);
        lv_obj_set_style_text_font(spark_lon_deg_input, getThemeFonts()->font_small, 0);
        lv_obj_set_style_border_width(spark_lon_deg_input, 1, 0);
        lv_obj_set_style_radius(spark_lon_deg_input, 4, 0);

        spark_lon_deg_label = lv_label_create(spark_position_row);
        lv_label_set_text(spark_lon_deg_label, "°");
        lv_obj_set_pos(spark_lon_deg_label, 238, 12);

        // Lon minutes
        spark_lon_min_input = lv_textarea_create(spark_position_row);
        lv_obj_set_size(spark_lon_min_input, 40, 28);
        lv_obj_set_pos(spark_lon_min_input, 248, 6);
        lv_textarea_set_one_line(spark_lon_min_input, true);
        lv_textarea_set_max_length(spark_lon_min_input, 3);
        lv_obj_set_style_text_font(spark_lon_min_input, getThemeFonts()->font_small, 0);
        lv_obj_set_style_border_width(spark_lon_min_input, 1, 0);
        lv_obj_set_style_radius(spark_lon_min_input, 4, 0);

        spark_lon_min_label = lv_label_create(spark_position_row);
        lv_label_set_text(spark_lon_min_label, "'");
        lv_obj_set_pos(spark_lon_min_label, 291, 12);

        // Lon direction button
        spark_lon_dir_btn = lv_btn_create(spark_position_row);
        lv_obj_set_size(spark_lon_dir_btn, 28, 28);
        lv_obj_set_pos(spark_lon_dir_btn, 300, 6);
        lv_obj_set_style_bg_color(spark_lon_dir_btn, LV_COLOR_BG_LAYER2, 0);
        lv_obj_set_style_radius(spark_lon_dir_btn, 4, 0);
        char lon_dir[2] = {sparkSession.inputLonDirection, '\0'};
        lv_obj_t* lon_dir_lbl = lv_label_create(spark_lon_dir_btn);
        lv_label_set_text(lon_dir_lbl, lon_dir);
        lv_obj_center(lon_dir_lbl);
    }

    // Bottom buttons
    lv_obj_t* btn_cont = lv_obj_create(screen);
    lv_obj_set_size(btn_cont, SCREEN_WIDTH - 20, 45);
    lv_obj_align(btn_cont, LV_ALIGN_BOTTOM_MID, 0, -25);
    lv_obj_set_layout(btn_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(btn_cont, 10, 0);
    lv_obj_set_style_bg_opa(btn_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_cont, 0, 0);
    lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_SCROLLABLE);

    // Submit button
    spark_submit_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(spark_submit_btn, 100, 38);
    lv_obj_set_style_bg_color(spark_submit_btn, LV_COLOR_SUCCESS, 0);
    lv_obj_set_style_radius(spark_submit_btn, 6, 0);
    lv_obj_t* submit_label = lv_label_create(spark_submit_btn);
    lv_label_set_text(submit_label, "Submit");
    lv_obj_set_style_text_font(submit_label, getThemeFonts()->font_body, 0);
    lv_obj_center(submit_label);

    // Morse Reference button
    spark_ref_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(spark_ref_btn, 110, 38);
    lv_obj_set_style_bg_color(spark_ref_btn, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_radius(spark_ref_btn, 6, 0);
    lv_obj_t* ref_label = lv_label_create(spark_ref_btn);
    lv_label_set_text(ref_label, "Ref (-5)");
    lv_obj_set_style_text_font(ref_label, getThemeFonts()->font_small, 0);
    lv_obj_center(ref_label);

    // Hint button (if available)
    if (ch->hint) {
        spark_hint_btn = lv_btn_create(btn_cont);
        lv_obj_set_size(spark_hint_btn, 100, 38);
        lv_obj_set_style_bg_color(spark_hint_btn, LV_COLOR_BG_LAYER2, 0);
        lv_obj_set_style_radius(spark_hint_btn, 6, 0);
        lv_obj_t* hint_label = lv_label_create(spark_hint_btn);
        lv_label_set_text(hint_label, "Hint (-2)");
        lv_obj_set_style_text_font(hint_label, getThemeFonts()->font_small, 0);
        lv_obj_center(hint_label);
    }

    // Footer with keyboard hints
    lv_obj_t* footer = lv_label_create(screen);
    lv_label_set_text(footer, LV_SYMBOL_UP LV_SYMBOL_DOWN " Navigate  " LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " Toggle Dir  Type in fields  ESC Back");
    lv_obj_set_style_text_font(footer, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(footer, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -5);

    // Invisible focus container for keyboard input
    spark_focus_container = lv_obj_create(screen);
    lv_obj_set_size(spark_focus_container, 1, 1);
    lv_obj_set_pos(spark_focus_container, -10, -10);
    lv_obj_set_style_bg_opa(spark_focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spark_focus_container, 0, 0);
    lv_obj_set_style_outline_width(spark_focus_container, 0, 0);
    lv_obj_set_style_outline_width(spark_focus_container, 0, LV_STATE_FOCUSED);
    lv_obj_clear_flag(spark_focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(spark_focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(spark_focus_container, spark_gameplay_key_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(spark_focus_container);

    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }
    lv_group_focus_obj(spark_focus_container);

    // Apply initial focus styling
    spark_gameplay_update_focus();

    spark_screen = screen;
    return screen;
}

// ============================================
// Results Screen - Full Implementation
// ============================================

// Results screen focus tracking (pointer declarations moved to top of file)
static int spark_results_focus = 0;

static void spark_results_update_focus(bool hasRetry) {
    if (hasRetry && spark_retry_btn) {
        lv_obj_set_style_bg_color(spark_retry_btn,
            (spark_results_focus == 0) ? LV_COLOR_ACCENT_CYAN : LV_COLOR_WARNING, 0);
    }
    if (spark_continue_btn) {
        int contIdx = hasRetry ? 1 : 0;
        bool isFocused = (spark_results_focus == contIdx);
        // Keep green if success, otherwise layer2
        if (sparkSession.challengeCompleted) {
            lv_obj_set_style_bg_color(spark_continue_btn,
                isFocused ? LV_COLOR_ACCENT_CYAN : LV_COLOR_SUCCESS, 0);
        } else {
            lv_obj_set_style_bg_color(spark_continue_btn,
                isFocused ? LV_COLOR_ACCENT_CYAN : LV_COLOR_BG_LAYER2, 0);
        }
    }
}

// Key event callback for results screen
static void spark_results_key_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    bool hasRetry = (spark_retry_btn != NULL);
    int maxFocus = hasRetry ? 1 : 0;

    switch(key) {
        case LV_KEY_ESC:
            onLVGLBackNavigation();
            lv_event_stop_processing(e);
            break;

        case LV_KEY_UP:
        case LV_KEY_LEFT:
            if (hasRetry) {
                spark_results_focus = (spark_results_focus == 0) ? maxFocus : spark_results_focus - 1;
                spark_results_update_focus(hasRetry);
                beep(TONE_MENU_NAV, BEEP_SHORT);
            }
            break;

        case LV_KEY_DOWN:
        case LV_KEY_RIGHT:
            if (hasRetry) {
                spark_results_focus = (spark_results_focus == maxFocus) ? 0 : spark_results_focus + 1;
                spark_results_update_focus(hasRetry);
                beep(TONE_MENU_NAV, BEEP_SHORT);
            }
            break;

        case LV_KEY_ENTER:
            beep(TONE_SELECT, BEEP_MEDIUM);
            if (hasRetry && spark_results_focus == 0) {
                // Retry - go back to gameplay, keep inputs
                onLVGLMenuSelect(SPARK_MODE_GAMEPLAY);
            } else {
                // Continue
                if (sparkSession.challengeCompleted) {
                    onLVGLMenuSelect(SPARK_MODE_DEBRIEFING);
                } else {
                    onLVGLMenuSelect(SPARK_MODE_MENU);
                }
            }
            lv_event_stop_processing(e);
            break;
    }
}

lv_obj_t* createSparkWatchResultsScreen() {
    cleanupSparkScreenPointers();
    clearNavigationGroup();
    spark_results_focus = 0;
    spark_retry_btn = NULL;
    spark_continue_btn = NULL;

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // Calculate score and validate
    SparkValidationResult result = validateSparkAnswers();
    int score = result.allCorrect ? calculateSparkScore() : 0;

    // Record completion if correct (before displaying)
    if (result.allCorrect && !sparkSession.challengeCompleted) {
        recordChallengeCompletion(score);
    }

    const SparkWatchChallenge* ch = sparkSession.currentChallenge;

    // Title
    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, result.allCorrect ? "Challenge Complete!" : "Not Quite Right...");
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(title, result.allCorrect ? LV_COLOR_SUCCESS : LV_COLOR_WARNING, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Score (if correct)
    if (result.allCorrect) {
        lv_obj_t* score_label = lv_label_create(screen);
        lv_label_set_text_fmt(score_label, "+%d points", score);
        lv_obj_set_style_text_font(score_label, getThemeFonts()->font_title, 0);
        lv_obj_set_style_text_color(score_label, LV_COLOR_ACCENT_CYAN, 0);
        lv_obj_align(score_label, LV_ALIGN_TOP_MID, 0, 35);
    }

    // Results container - scrollable for many fields
    lv_obj_t* results_cont = lv_obj_create(screen);
    lv_obj_set_size(results_cont, SCREEN_WIDTH - 20, result.allCorrect ? 170 : 200);
    lv_obj_align(results_cont, LV_ALIGN_TOP_MID, 0, result.allCorrect ? 70 : 45);
    lv_obj_set_style_bg_color(results_cont, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_radius(results_cont, 8, 0);
    lv_obj_set_style_pad_all(results_cont, 10, 0);
    lv_obj_set_layout(results_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(results_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(results_cont, 4, 0);
    lv_obj_set_scrollbar_mode(results_cont, LV_SCROLLBAR_MODE_AUTO);

    // Helper to create result row
    auto createResultRow = [&](const char* label, const char* userAnswer, const char* correct, bool isCorrect) {
        lv_obj_t* row = lv_obj_create(results_cont);
        lv_obj_set_size(row, LV_PCT(100), 28);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        // Field label
        lv_obj_t* field_lbl = lv_label_create(row);
        lv_label_set_text(field_lbl, label);
        lv_obj_set_style_text_font(field_lbl, getThemeFonts()->font_small, 0);
        lv_obj_set_style_text_color(field_lbl, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_align(field_lbl, LV_ALIGN_LEFT_MID, 0, 0);

        // User answer
        lv_obj_t* answer_lbl = lv_label_create(row);
        if (strlen(userAnswer) == 0) {
            lv_label_set_text(answer_lbl, "(empty)");
            lv_obj_set_style_text_color(answer_lbl, LV_COLOR_TEXT_SECONDARY, 0);
        } else {
            lv_label_set_text(answer_lbl, userAnswer);
            lv_obj_set_style_text_color(answer_lbl,
                isCorrect ? LV_COLOR_SUCCESS : LV_COLOR_ERROR, 0);
        }
        lv_obj_set_style_text_font(answer_lbl, getThemeFonts()->font_body, 0);
        lv_obj_align(answer_lbl, LV_ALIGN_LEFT_MID, 70, 0);

        // Correct answer (if wrong)
        if (!isCorrect) {
            lv_obj_t* correct_lbl = lv_label_create(row);
            lv_label_set_text_fmt(correct_lbl, LV_SYMBOL_RIGHT " %s", correct);
            lv_obj_set_style_text_font(correct_lbl, getThemeFonts()->font_small, 0);
            lv_obj_set_style_text_color(correct_lbl, LV_COLOR_SUCCESS, 0);
            lv_obj_align(correct_lbl, LV_ALIGN_RIGHT_MID, 0, 0);
        } else {
            // Checkmark
            lv_obj_t* check_lbl = lv_label_create(row);
            lv_label_set_text(check_lbl, LV_SYMBOL_OK);
            lv_obj_set_style_text_color(check_lbl, LV_COLOR_SUCCESS, 0);
            lv_obj_align(check_lbl, LV_ALIGN_RIGHT_MID, 0, 0);
        }
    };

    // Display each field result
    if (ch) {
        createResultRow("Signal:", sparkSession.inputSignalType, ch->signalType, result.signalTypeCorrect);
        createResultRow("Ship:", sparkSession.inputShipName, ch->shipName, result.shipNameCorrect);

        if (ch->difficulty >= SPARK_MEDIUM) {
            createResultRow("Distress:", sparkSession.inputDistressNature,
                           ch->distressNature ? ch->distressNature : "N/A", result.distressCorrect);
        }

        if (ch->difficulty >= SPARK_HARD) {
            // Format position strings
            char userPos[32], correctPos[32];
            snprintf(userPos, sizeof(userPos), "%s°%s'%c / %s°%s'%c",
                     sparkSession.inputLatDegrees, sparkSession.inputLatMinutes, sparkSession.inputLatDirection,
                     sparkSession.inputLonDegrees, sparkSession.inputLonMinutes, sparkSession.inputLonDirection);
            snprintf(correctPos, sizeof(correctPos), "%s°%s'%c / %s°%s'%c",
                     ch->latDegrees, ch->latMinutes, ch->latDirection,
                     ch->lonDegrees, ch->lonMinutes, ch->lonDirection);
            createResultRow("Position:", userPos, correctPos, result.positionCorrect);
        }
    }

    // Summary line
    lv_obj_t* summary = lv_label_create(results_cont);
    lv_label_set_text_fmt(summary, "%d of %d fields correct", result.correctFieldCount, result.totalFieldCount);
    lv_obj_set_style_text_font(summary, getThemeFonts()->font_small, 0);
    lv_obj_set_style_text_color(summary,
        result.allCorrect ? LV_COLOR_SUCCESS : LV_COLOR_TEXT_SECONDARY, 0);

    // Buttons
    lv_obj_t* btn_cont = lv_obj_create(screen);
    lv_obj_set_size(btn_cont, SCREEN_WIDTH - 40, 50);
    lv_obj_align(btn_cont, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_layout(btn_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(btn_cont, 15, 0);
    lv_obj_set_style_bg_opa(btn_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_cont, 0, 0);
    lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_SCROLLABLE);

    if (!result.allCorrect) {
        // Retry button
        spark_retry_btn = lv_btn_create(btn_cont);
        lv_obj_set_size(spark_retry_btn, 120, 40);
        lv_obj_set_style_bg_color(spark_retry_btn, LV_COLOR_WARNING, 0);
        lv_obj_set_style_radius(spark_retry_btn, 6, 0);
        lv_obj_t* retry_label = lv_label_create(spark_retry_btn);
        lv_label_set_text(retry_label, "Try Again");
        lv_obj_set_style_text_font(retry_label, getThemeFonts()->font_body, 0);
        lv_obj_center(retry_label);
    }

    // Continue button
    spark_continue_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(spark_continue_btn, 130, 40);
    lv_obj_set_style_bg_color(spark_continue_btn, result.allCorrect ? LV_COLOR_SUCCESS : LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_radius(spark_continue_btn, 6, 0);
    lv_obj_t* cont_label = lv_label_create(spark_continue_btn);
    lv_label_set_text(cont_label, result.allCorrect ? "Continue" : "Back");
    lv_obj_set_style_text_font(cont_label, getThemeFonts()->font_body, 0);
    lv_obj_center(cont_label);

    // Invisible focus container for keyboard input
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);
    lv_obj_set_pos(focus_container, -10, -10);
    lv_obj_set_style_bg_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, LV_STATE_FOCUSED);
    lv_obj_clear_flag(focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(focus_container, spark_results_key_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus_container);

    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }
    lv_group_focus_obj(focus_container);

    // Apply initial focus styling
    spark_results_update_focus(spark_retry_btn != NULL);

    spark_screen = screen;
    return screen;
}

// ============================================
// Debriefing Screen - Full Implementation
// ============================================

// Key event callback for debriefing screen
static void spark_debriefing_key_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    switch(key) {
        case LV_KEY_ESC:
            onLVGLMenuSelect(SPARK_MODE_MENU);
            lv_event_stop_processing(e);
            break;

        case LV_KEY_ENTER:
            beep(TONE_SELECT, BEEP_MEDIUM);
            onLVGLMenuSelect(SPARK_MODE_MENU);
            lv_event_stop_processing(e);
            break;
    }
}

lv_obj_t* createSparkWatchDebriefingScreen() {
    cleanupSparkScreenPointers();
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    const SparkWatchChallenge* ch = sparkSession.currentChallenge;

    // Title
    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "Historical Context");
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    if (ch) {
        // Challenge title
        lv_obj_t* ch_title = lv_label_create(screen);
        lv_label_set_text(ch_title, ch->title);
        lv_obj_set_style_text_font(ch_title, getThemeFonts()->font_body, 0);
        lv_obj_set_style_text_color(ch_title, LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_align(ch_title, LV_ALIGN_TOP_MID, 0, 45);

        // Debriefing container - scrollable
        lv_obj_t* debrief_cont = lv_obj_create(screen);
        lv_obj_set_size(debrief_cont, SCREEN_WIDTH - 20, 170);
        lv_obj_align(debrief_cont, LV_ALIGN_CENTER, 0, 10);
        lv_obj_set_style_bg_color(debrief_cont, LV_COLOR_BG_LAYER2, 0);
        lv_obj_set_style_radius(debrief_cont, 8, 0);
        lv_obj_set_style_pad_all(debrief_cont, 12, 0);
        lv_obj_set_scrollbar_mode(debrief_cont, LV_SCROLLBAR_MODE_AUTO);

        lv_obj_t* debrief = lv_label_create(debrief_cont);
        lv_label_set_text(debrief, ch->debriefing ? ch->debriefing : "No additional historical information available for this challenge.");
        lv_label_set_long_mode(debrief, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(debrief, SCREEN_WIDTH - 50);
        lv_obj_set_style_text_font(debrief, getThemeFonts()->font_body, 0);

        // Correct transmission display
        if (ch->morseTransmission) {
            lv_obj_t* trans_title = lv_label_create(debrief_cont);
            lv_label_set_text(trans_title, "\nTransmission:");
            lv_obj_set_style_text_font(trans_title, getThemeFonts()->font_small, 0);
            lv_obj_set_style_text_color(trans_title, LV_COLOR_WARNING, 0);

            lv_obj_t* trans_text = lv_label_create(debrief_cont);
            lv_label_set_text(trans_text, ch->morseTransmission);
            lv_label_set_long_mode(trans_text, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(trans_text, SCREEN_WIDTH - 50);
            lv_obj_set_style_text_font(trans_text, getThemeFonts()->font_small, 0);
            lv_obj_set_style_text_color(trans_text, LV_COLOR_ACCENT_CYAN, 0);
        }
    }

    // Continue button
    lv_obj_t* cont_btn = lv_btn_create(screen);
    lv_obj_set_size(cont_btn, 160, 40);
    lv_obj_align(cont_btn, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_style_bg_color(cont_btn, LV_COLOR_SUCCESS, 0);
    lv_obj_set_style_radius(cont_btn, 6, 0);
    lv_obj_t* cont_label = lv_label_create(cont_btn);
    lv_label_set_text(cont_label, "Back to Menu");
    lv_obj_set_style_text_font(cont_label, getThemeFonts()->font_body, 0);
    lv_obj_center(cont_label);

    // Invisible focus container for keyboard input
    lv_obj_t* focus_container = lv_obj_create(screen);
    lv_obj_set_size(focus_container, 1, 1);
    lv_obj_set_pos(focus_container, -10, -10);
    lv_obj_set_style_bg_opa(focus_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, 0);
    lv_obj_set_style_outline_width(focus_container, 0, LV_STATE_FOCUSED);
    lv_obj_clear_flag(focus_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(focus_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(focus_container, spark_debriefing_key_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus_container);

    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_set_editing(group, true);
    }
    lv_group_focus_obj(focus_container);

    spark_screen = screen;
    return screen;
}

// ============================================
// Mission Select Screen (Campaign Missions)
// ============================================

// Mission select callback
static void spark_mission_select_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int missionNum = (int)(intptr_t)lv_event_get_user_data(e);
        // Find the challenge for this campaign/mission
        for (int i = 0; i < SPARK_CHALLENGE_COUNT; i++) {
            const SparkWatchChallenge* ch = &sparkChallenges[i];
            if (ch->campaignId == spark_selected_campaign_id && ch->missionNumber == missionNum) {
                initSparkSession(ch);
                beep(TONE_SELECT, BEEP_SHORT);
                onLVGLMenuSelect(SPARK_MODE_BRIEFING);
                return;
            }
        }
    }
}

lv_obj_t* createSparkWatchMissionScreen() {
    cleanupSparkScreenPointers();
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    createCompactStatusBar(screen);

    // Find the campaign
    const SparkWatchCampaign* camp = NULL;
    for (int i = 0; i < SPARK_CAMPAIGN_COUNT; i++) {
        if (sparkCampaigns[i].id == spark_selected_campaign_id) {
            camp = &sparkCampaigns[i];
            break;
        }
    }

    if (!camp) {
        lv_obj_t* error = lv_label_create(screen);
        lv_label_set_text(error, "Campaign not found");
        lv_obj_center(error);
        return screen;
    }

    // Title
    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, camp->name);
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Campaign description
    lv_obj_t* desc = lv_label_create(screen);
    lv_label_set_text(desc, camp->ship);
    lv_obj_set_style_text_font(desc, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(desc, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(desc, LV_ALIGN_TOP_MID, 0, 35);

    // Mission list
    lv_obj_t* list = lv_obj_create(screen);
    lv_obj_set_size(list, SCREEN_WIDTH - 20, 210);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(list, 6, 0);
    lv_obj_set_style_pad_all(list, 8, 0);
    lv_obj_set_style_bg_color(list, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_radius(list, 8, 0);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);

    int completedMissions = sparkProgress.campaignProgress[camp->id];

    // Find all missions for this campaign
    for (int m = 1; m <= camp->totalMissions; m++) {
        // Find the challenge for this mission
        const SparkWatchChallenge* mission_ch = NULL;
        for (int i = 0; i < SPARK_CHALLENGE_COUNT; i++) {
            if (sparkChallenges[i].campaignId == camp->id &&
                sparkChallenges[i].missionNumber == m) {
                mission_ch = &sparkChallenges[i];
                break;
            }
        }

        lv_obj_t* btn = lv_btn_create(list);
        lv_obj_set_size(btn, SCREEN_WIDTH - 50, 45);
        lv_obj_add_style(btn, getStyleMenuCard(), 0);
        lv_obj_add_style(btn, getStyleMenuCardFocused(), LV_STATE_FOCUSED);

        // Mission is unlocked if it's #1 or previous mission is completed
        bool unlocked = (m == 1) || (m <= completedMissions + 1);
        bool completed = (m <= completedMissions);

        if (unlocked && mission_ch) {
            lv_obj_add_event_cb(btn, spark_mission_select_cb, LV_EVENT_CLICKED,
                               (void*)(intptr_t)m);
            addNavigableWidget(btn);
            addSparkNavHandler(btn);
        } else {
            lv_obj_set_style_bg_opa(btn, LV_OPA_50, 0);
        }

        // Mission number and status
        lv_obj_t* num_label = lv_label_create(btn);
        char num_text[16];
        if (completed) {
            snprintf(num_text, sizeof(num_text), LV_SYMBOL_OK " %d", m);
            lv_obj_set_style_text_color(num_label, LV_COLOR_SUCCESS, 0);
        } else if (unlocked) {
            snprintf(num_text, sizeof(num_text), "%d", m);
            lv_obj_set_style_text_color(num_label, LV_COLOR_ACCENT_CYAN, 0);
        } else {
            snprintf(num_text, sizeof(num_text), LV_SYMBOL_CLOSE " %d", m);
            lv_obj_set_style_text_color(num_label, LV_COLOR_TEXT_SECONDARY, 0);
        }
        lv_label_set_text(num_label, num_text);
        lv_obj_set_style_text_font(num_label, getThemeFonts()->font_body, 0);
        lv_obj_align(num_label, LV_ALIGN_LEFT_MID, 10, 0);

        // Mission title
        lv_obj_t* title_label = lv_label_create(btn);
        if (mission_ch && unlocked) {
            lv_label_set_text(title_label, mission_ch->title);
        } else {
            lv_label_set_text(title_label, "Locked");
        }
        lv_obj_set_style_text_font(title_label, getThemeFonts()->font_body, 0);
        lv_obj_set_style_text_color(title_label, unlocked ? LV_COLOR_TEXT_PRIMARY : LV_COLOR_TEXT_SECONDARY, 0);
        lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 45, 0);

        // Difficulty
        if (mission_ch && unlocked) {
            lv_obj_t* diff_label = lv_label_create(btn);
            lv_label_set_text(diff_label, SPARK_DIFFICULTY_NAMES[mission_ch->difficulty]);
            lv_obj_set_style_text_font(diff_label, getThemeFonts()->font_small, 0);
            lv_obj_set_style_text_color(diff_label, LV_COLOR_WARNING, 0);
            lv_obj_align(diff_label, LV_ALIGN_RIGHT_MID, -10, 0);
        }
    }

    spark_screen = screen;
    return screen;
}

// ============================================
// Settings Screen (Placeholder)
// ============================================

lv_obj_t* createSparkWatchSettingsScreen() {
    cleanupSparkScreenPointers();
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    createCompactStatusBar(screen);

    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "Spark Watch Settings");
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t* placeholder = lv_label_create(screen);
    lv_label_set_text(placeholder, "Settings coming soon...\n\n- Default playback speed\n- Show morse reference\n- Audio settings");
    lv_obj_set_style_text_font(placeholder, getThemeFonts()->font_body, 0);
    lv_obj_center(placeholder);

    // Add invisible focus target for ESC handling (not hidden, just off-screen and tiny)
    lv_obj_t* focus = lv_obj_create(screen);
    lv_obj_set_size(focus, 1, 1);
    lv_obj_set_pos(focus, -10, -10);
    lv_obj_set_style_bg_opa(focus, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus, 0, 0);
    lv_obj_set_style_outline_width(focus, 0, 0);
    lv_obj_set_style_outline_width(focus, 0, LV_STATE_FOCUSED);
    lv_obj_add_flag(focus, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(focus, spark_key_event_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus);

    spark_screen = screen;
    return screen;
}

// ============================================
// Statistics Screen (Placeholder)
// ============================================

lv_obj_t* createSparkWatchStatsScreen() {
    cleanupSparkScreenPointers();
    clearNavigationGroup();

    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);
    createCompactStatusBar(screen);

    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "Statistics");
    lv_obj_set_style_text_font(title, getThemeFonts()->font_subtitle, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Stats container
    lv_obj_t* stats = lv_obj_create(screen);
    lv_obj_set_size(stats, SCREEN_WIDTH - 40, 220);
    lv_obj_align(stats, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_color(stats, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_radius(stats, 8, 0);
    lv_obj_set_style_pad_all(stats, 15, 0);

    char stats_text[512];
    snprintf(stats_text, sizeof(stats_text),
        "Total Score: %d\n"
        "Challenges Completed: %d\n"
        "Perfect Challenges: %d\n\n"
        "By Difficulty:\n"
        "  Easy: %d   Medium: %d\n"
        "  Hard: %d   Expert: %d\n"
        "  Master: %d",
        sparkProgress.totalScore,
        sparkProgress.challengesCompleted,
        sparkProgress.perfectChallenges,
        sparkProgress.completedByDifficulty[0],
        sparkProgress.completedByDifficulty[1],
        sparkProgress.completedByDifficulty[2],
        sparkProgress.completedByDifficulty[3],
        sparkProgress.completedByDifficulty[4]
    );

    lv_obj_t* stats_label = lv_label_create(stats);
    lv_label_set_text(stats_label, stats_text);
    lv_obj_set_style_text_font(stats_label, getThemeFonts()->font_body, 0);

    // Add invisible focus target for ESC handling (not hidden, just off-screen and tiny)
    lv_obj_t* focus = lv_obj_create(screen);
    lv_obj_set_size(focus, 1, 1);
    lv_obj_set_pos(focus, -10, -10);
    lv_obj_set_style_bg_opa(focus, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus, 0, 0);
    lv_obj_set_style_outline_width(focus, 0, 0);
    lv_obj_set_style_outline_width(focus, 0, LV_STATE_FOCUSED);
    lv_obj_add_flag(focus, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(focus, spark_key_event_cb, LV_EVENT_KEY, NULL);
    addNavigableWidget(focus);

    spark_screen = screen;
    return screen;
}

// ============================================
// Screen Factory Function
// ============================================

lv_obj_t* createSparkWatchScreenForMode(int mode) {
    switch (mode) {
        case SPARK_MODE_MENU:
            return createSparkWatchMenuScreen();
        case SPARK_MODE_DIFFICULTY:
            return createSparkWatchDifficultyScreen();
        case SPARK_MODE_CAMPAIGN:
            return createSparkWatchCampaignScreen();
        case SPARK_MODE_MISSION:
            return createSparkWatchMissionScreen();
        case SPARK_MODE_CHALLENGE:
            return createSparkWatchChallengeScreen();
        case SPARK_MODE_BRIEFING:
            return createSparkWatchBriefingScreen();
        case SPARK_MODE_GAMEPLAY:
            return createSparkWatchGameplayScreen();
        case SPARK_MODE_RESULTS:
            return createSparkWatchResultsScreen();
        case SPARK_MODE_DEBRIEFING:
            return createSparkWatchDebriefingScreen();
        case SPARK_MODE_SETTINGS:
            return createSparkWatchSettingsScreen();
        case SPARK_MODE_STATS:
            return createSparkWatchStatsScreen();
        default:
            return createSparkWatchMenuScreen();
    }
}

#endif // LV_SPARK_WATCH_SCREENS_H
