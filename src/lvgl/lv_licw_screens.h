/*
 * VAIL SUMMIT - LVGL LICW Training Screens
 *
 * LVGL screens for the Long Island CW Club (LICW) training implementation.
 * Implements carousel selection, lesson selection, and practice type screens.
 *
 * IMPORTANT: NO morse pattern visuals (.-) - characters are SOUNDS
 *
 * Copyright (c) 2025 VAIL SUMMIT Contributors
 */

#ifndef LV_LICW_SCREENS_H
#define LV_LICW_SCREENS_H

#include <lvgl.h>
#include "lv_theme_summit.h"
#include "lv_widgets_summit.h"
#include "lv_screen_manager.h"
#include "lv_menu_screens.h"
#include "../training/training_licw_core.h"
#include "../training/training_licw_data.h"
#include "../core/config.h"
#include "../core/morse_code.h"
#include "../audio/i2s_audio.h"
#include "../audio/morse_decoder_adaptive.h"
#include "../settings/settings_cw.h"

// Forward declarations
extern void onLVGLMenuSelect(int target_mode);
extern int cwTone;     // From settings
extern int cwWPM;      // From settings

// KeyType is defined in settings_cw.h and imported via other headers
// Just reference the extern variable (it's already included via training files)

// ============================================
// Screen State Variables
// ============================================

// Current selections
static LICWCarousel licw_ui_carousel = LICW_BC1;
static int licw_ui_lesson = 1;
static LICWPracticeType licw_ui_practice_type = LICW_PRACTICE_COPY;

// UI objects for updates
static lv_obj_t* licw_carousel_btns[9] = {NULL};
static lv_obj_t* licw_lesson_btns[10] = {NULL};
static lv_obj_t* licw_practice_btns[8] = {NULL};

// ============================================
// Carousel Selection Screen
// ============================================

// Carousel button click handler
static void licw_carousel_click_handler(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    int carousel_idx = (int)(intptr_t)lv_obj_get_user_data(btn);

    licw_ui_carousel = (LICWCarousel)carousel_idx;
    licwSelectedCarousel = licw_ui_carousel;

    Serial.printf("[LICW] Carousel selected: %d (%s)\n",
                  carousel_idx, licwCarouselShortNames[carousel_idx]);

    // Navigate to lesson selection
    onLVGLMenuSelect(121);  // LVGL_MODE_LICW_LESSON_SELECT
}

/*
 * Create carousel selection screen
 * Shows all 9 carousels (BC1-3, INT1-3, ADV1-3) in a 3x3 grid
 */
lv_obj_t* createLICWCarouselSelectScreen() {
    // Reset carousel button tracking
    for (int i = 0; i < 9; i++) {
        licw_carousel_btns[i] = NULL;
    }

    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, LV_COLOR_BG_DEEP, 0);

    // Header
    createHeader(screen, "LICW TRAINING");

    // Content area
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, LV_PCT(100), SCREEN_HEIGHT - 50 - 35);
    lv_obj_set_pos(content, 0, 55);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 10, 0);
    lv_obj_set_style_pad_row(content, 8, 0);
    lv_obj_set_style_pad_column(content, 12, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Create carousel buttons (3 columns x 3 rows)
    for (int i = 0; i < LICW_TOTAL_CAROUSELS; i++) {
        const LICWCarouselDef* carousel = getLICWCarousel((LICWCarousel)i);

        lv_obj_t* btn = lv_btn_create(content);
        lv_obj_set_size(btn, 145, 75);
        lv_obj_add_style(btn, getStyleMenuCard(), 0);
        lv_obj_add_style(btn, getStyleMenuCardFocused(), LV_STATE_FOCUSED);

        // Container for text
        lv_obj_t* col = lv_obj_create(btn);
        lv_obj_set_size(col, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(col, 0, 0);
        lv_obj_set_style_pad_all(col, 2, 0);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(col, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);

        // Short name (e.g., "BC1")
        lv_obj_t* name = lv_label_create(col);
        lv_label_set_text(name, carousel->shortName);
        lv_obj_set_style_text_font(name, getThemeFonts()->font_title, 0);

        // Color code by level
        if (i < 3) {
            lv_obj_set_style_text_color(name, LV_COLOR_SUCCESS, 0);  // Green for BC
        } else if (i < 6) {
            lv_obj_set_style_text_color(name, LV_COLOR_WARNING, 0);  // Orange for INT
        } else {
            lv_obj_set_style_text_color(name, LV_COLOR_ACCENT_CYAN, 0);  // Cyan for ADV
        }

        // Speed info
        char speed_text[24];
        snprintf(speed_text, sizeof(speed_text), "%d/%d WPM",
                 carousel->targetCharWPM, carousel->endingFWPM);
        lv_obj_t* speed = lv_label_create(col);
        lv_label_set_text(speed, speed_text);
        lv_obj_set_style_text_font(speed, getThemeFonts()->font_body, 0);
        lv_obj_set_style_text_color(speed, LV_COLOR_TEXT_SECONDARY, 0);

        // Store carousel index and add click handler
        lv_obj_set_user_data(btn, (void*)(intptr_t)i);
        lv_obj_add_event_cb(btn, licw_carousel_click_handler, LV_EVENT_CLICKED, NULL);

        // Store button reference
        licw_carousel_btns[i] = btn;

        // Add to navigation group
        addNavigableWidget(btn);
    }

    // Footer
    lv_obj_t* footer = lv_label_create(screen);
    lv_label_set_text(footer, "Select level - ESC to go back");
    lv_obj_set_style_text_font(footer, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(footer, LV_COLOR_WARNING, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -5);

    return screen;
}

// ============================================
// Lesson Selection Screen
// ============================================

// Lesson button click handler
static void licw_lesson_click_handler(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    int lesson_num = (int)(intptr_t)lv_obj_get_user_data(btn);

    licw_ui_lesson = lesson_num;
    licwSelectedLesson = lesson_num;

    Serial.printf("[LICW] Lesson selected: %d\n", lesson_num);

    // Navigate to practice type selection
    onLVGLMenuSelect(122);  // LVGL_MODE_LICW_PRACTICE_TYPE
}

/*
 * Create lesson selection screen for current carousel
 */
lv_obj_t* createLICWLessonSelectScreen() {
    const LICWCarouselDef* carousel = getLICWCarousel(licwSelectedCarousel);

    // Reset lesson button tracking
    for (int i = 0; i < 10; i++) {
        licw_lesson_btns[i] = NULL;
    }

    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, LV_COLOR_BG_DEEP, 0);

    // Header with carousel name
    char header_text[32];
    snprintf(header_text, sizeof(header_text), "%s LESSONS", carousel->shortName);
    createHeader(screen, header_text);

    // Content area
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, LV_PCT(100), SCREEN_HEIGHT - 50 - 35);
    lv_obj_set_pos(content, 0, 55);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 10, 0);
    lv_obj_set_style_pad_row(content, 8, 0);
    lv_obj_set_style_pad_column(content, 12, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Create lesson buttons
    for (int i = 1; i <= carousel->totalLessons && i <= 10; i++) {
        const LICWLesson* lesson = getLICWLesson(licwSelectedCarousel, i);

        lv_obj_t* btn = lv_btn_create(content);
        lv_obj_set_size(btn, 145, 80);
        lv_obj_add_style(btn, getStyleMenuCard(), 0);
        lv_obj_add_style(btn, getStyleMenuCardFocused(), LV_STATE_FOCUSED);

        // Container
        lv_obj_t* col = lv_obj_create(btn);
        lv_obj_set_size(col, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(col, 0, 0);
        lv_obj_set_style_pad_all(col, 2, 0);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(col, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);

        // Lesson number
        char num_text[16];
        snprintf(num_text, sizeof(num_text), "Lesson %d", i);
        lv_obj_t* num = lv_label_create(col);
        lv_label_set_text(num, num_text);
        lv_obj_set_style_text_font(num, getThemeFonts()->font_input, 0);
        lv_obj_set_style_text_color(num, LV_COLOR_TEXT_PRIMARY, 0);

        // New characters (if any) - show the characters themselves, NOT morse patterns
        if (lesson->newChars != NULL && strlen(lesson->newChars) > 0) {
            lv_obj_t* chars = lv_label_create(col);
            lv_label_set_text(chars, lesson->newChars);
            lv_obj_set_style_text_font(chars, getThemeFonts()->font_title, 0);
            lv_obj_set_style_text_color(chars, LV_COLOR_ACCENT_CYAN, 0);
        }

        // Speed info
        char speed_text[16];
        snprintf(speed_text, sizeof(speed_text), "%d/%d WPM",
                 lesson->characterWPM, lesson->effectiveWPM);
        lv_obj_t* speed = lv_label_create(col);
        lv_label_set_text(speed, speed_text);
        lv_obj_set_style_text_font(speed, getThemeFonts()->font_body, 0);
        lv_obj_set_style_text_color(speed, LV_COLOR_TEXT_SECONDARY, 0);

        // Store lesson number and add click handler
        lv_obj_set_user_data(btn, (void*)(intptr_t)i);
        lv_obj_add_event_cb(btn, licw_lesson_click_handler, LV_EVENT_CLICKED, NULL);

        // Store button reference
        licw_lesson_btns[i - 1] = btn;

        // Add to navigation group
        addNavigableWidget(btn);
    }

    // Footer
    lv_obj_t* footer = lv_label_create(screen);
    lv_label_set_text(footer, "Select lesson - ESC to go back");
    lv_obj_set_style_text_font(footer, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(footer, LV_COLOR_WARNING, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -5);

    return screen;
}

// ============================================
// Practice Type Selection Screen
// ============================================

// Practice type icons (no morse patterns!)
static const char* licw_practice_icons[] = {
    LV_SYMBOL_AUDIO,      // CSF - New Character
    LV_SYMBOL_EDIT,       // Copy Practice
    LV_SYMBOL_KEYBOARD,   // Sending Practice
    LV_SYMBOL_LOOP,       // IFR Training
    LV_SYMBOL_REFRESH,    // CFP (Character Flow)
    LV_SYMBOL_LIST,       // Word Discovery
    LV_SYMBOL_CALL,       // QSO Practice
    LV_SYMBOL_WARNING     // Adverse Copy
};

// Practice type click handler
static void licw_practice_click_handler(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    int practice_idx = (int)(intptr_t)lv_obj_get_user_data(btn);

    licw_ui_practice_type = (LICWPracticeType)practice_idx;
    licwSelectedPracticeType = licw_ui_practice_type;

    Serial.printf("[LICW] Practice type selected: %d (%s)\n",
                  practice_idx, licwPracticeTypeNames[practice_idx]);

    // Navigate to appropriate practice mode
    int target_mode;
    switch (licw_ui_practice_type) {
        case LICW_PRACTICE_CSF:
            target_mode = 127;  // LVGL_MODE_LICW_CSF_INTRO
            break;
        case LICW_PRACTICE_COPY:
            target_mode = 123;  // LVGL_MODE_LICW_COPY_PRACTICE
            break;
        case LICW_PRACTICE_SENDING:
            target_mode = 124;  // LVGL_MODE_LICW_SEND_PRACTICE
            break;
        case LICW_PRACTICE_IFR:
            target_mode = 126;  // LVGL_MODE_LICW_IFR_PRACTICE
            break;
        case LICW_PRACTICE_CFP:
            target_mode = 132;  // LVGL_MODE_LICW_CFP_PRACTICE
            break;
        case LICW_PRACTICE_WORD_DISCOVERY:
            target_mode = 128;  // LVGL_MODE_LICW_WORD_DISCOVERY
            break;
        case LICW_PRACTICE_QSO:
            target_mode = 129;  // LVGL_MODE_LICW_QSO_PRACTICE
            break;
        case LICW_PRACTICE_ADVERSE:
            target_mode = 133;  // LVGL_MODE_LICW_ADVERSE_COPY
            break;
        default:
            target_mode = 123;  // Default to copy practice
            break;
    }

    onLVGLMenuSelect(target_mode);
}

/*
 * Determine which practice types are available for current carousel/lesson
 */
bool isLICWPracticeAvailable(LICWCarousel carousel, int lesson, LICWPracticeType practice) {
    const LICWLesson* lessonData = getLICWLesson(carousel, lesson);

    switch (practice) {
        case LICW_PRACTICE_CSF:
            // CSF only for lessons with new characters
            return (lessonData->newChars != NULL && strlen(lessonData->newChars) > 0);

        case LICW_PRACTICE_COPY:
        case LICW_PRACTICE_SENDING:
        case LICW_PRACTICE_IFR:
        case LICW_PRACTICE_CFP:
            // Always available
            return true;

        case LICW_PRACTICE_WORD_DISCOVERY:
            // Available from INT1 onwards
            return (carousel >= LICW_INT1);

        case LICW_PRACTICE_QSO:
            // Available from BC3 onwards
            return (carousel >= LICW_BC3);

        case LICW_PRACTICE_ADVERSE:
            // Available from INT2 onwards
            return (carousel >= LICW_INT2);

        default:
            return false;
    }
}

/*
 * Create practice type selection screen
 */
lv_obj_t* createLICWPracticeTypeScreen() {
    const LICWCarouselDef* carousel = getLICWCarousel(licwSelectedCarousel);
    const LICWLesson* lesson = getLICWLesson(licwSelectedCarousel, licwSelectedLesson);

    // Reset practice button tracking
    for (int i = 0; i < 8; i++) {
        licw_practice_btns[i] = NULL;
    }

    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, LV_COLOR_BG_DEEP, 0);

    // Header
    char header_text[48];
    snprintf(header_text, sizeof(header_text), "%s L%d PRACTICE",
             carousel->shortName, licwSelectedLesson);
    createHeader(screen, header_text);

    // Info panel showing current lesson details
    lv_obj_t* info = lv_obj_create(screen);
    lv_obj_set_size(info, LV_PCT(95), 50);
    lv_obj_set_pos(info, 10, 55);
    lv_obj_set_style_bg_color(info, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_border_width(info, 1, 0);
    lv_obj_set_style_border_color(info, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_radius(info, 8, 0);
    lv_obj_set_style_pad_all(info, 8, 0);
    lv_obj_clear_flag(info, LV_OBJ_FLAG_SCROLLABLE);

    // Info text
    char info_text[128];
    if (lesson->newChars && strlen(lesson->newChars) > 0) {
        snprintf(info_text, sizeof(info_text),
                 "Characters: %s   Speed: %d/%d WPM",
                 lesson->cumulativeChars, lesson->characterWPM, lesson->effectiveWPM);
    } else {
        snprintf(info_text, sizeof(info_text),
                 "All characters   Speed: %d/%d WPM",
                 lesson->characterWPM, lesson->effectiveWPM);
    }
    lv_obj_t* info_lbl = lv_label_create(info);
    lv_label_set_text(info_lbl, info_text);
    lv_obj_set_style_text_font(info_lbl, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(info_lbl, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_center(info_lbl);

    // Content area for practice buttons
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, LV_PCT(100), SCREEN_HEIGHT - 110 - 35);
    lv_obj_set_pos(content, 0, 110);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 10, 0);
    lv_obj_set_style_pad_row(content, 8, 0);
    lv_obj_set_style_pad_column(content, 12, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Create practice type buttons
    int btn_count = 0;
    for (int i = 0; i < LICW_TOTAL_PRACTICE_TYPES; i++) {
        bool available = isLICWPracticeAvailable(licwSelectedCarousel, licwSelectedLesson, (LICWPracticeType)i);

        lv_obj_t* btn = lv_btn_create(content);
        lv_obj_set_size(btn, 145, 70);
        lv_obj_add_style(btn, getStyleMenuCard(), 0);
        lv_obj_add_style(btn, getStyleMenuCardFocused(), LV_STATE_FOCUSED);

        // Gray out unavailable options
        if (!available) {
            lv_obj_set_style_bg_opa(btn, LV_OPA_50, 0);
        }

        // Container
        lv_obj_t* col = lv_obj_create(btn);
        lv_obj_set_size(col, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(col, 0, 0);
        lv_obj_set_style_pad_all(col, 2, 0);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(col, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);

        // Icon
        lv_obj_t* icon = lv_label_create(col);
        lv_label_set_text(icon, licw_practice_icons[i]);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(icon, available ? LV_COLOR_ACCENT_CYAN : LV_COLOR_TEXT_SECONDARY, 0);

        // Name
        lv_obj_t* name = lv_label_create(col);
        lv_label_set_text(name, licwPracticeTypeNames[i]);
        lv_obj_set_style_text_font(name, getThemeFonts()->font_body, 0);
        lv_obj_set_style_text_color(name, available ? LV_COLOR_TEXT_PRIMARY : LV_COLOR_TEXT_SECONDARY, 0);

        // Store practice type and add click handler (only if available)
        lv_obj_set_user_data(btn, (void*)(intptr_t)i);
        if (available) {
            lv_obj_add_event_cb(btn, licw_practice_click_handler, LV_EVENT_CLICKED, NULL);
        }

        // Store button reference
        licw_practice_btns[btn_count++] = btn;

        // Add to navigation group
        addNavigableWidget(btn);
    }

    // Footer
    lv_obj_t* footer = lv_label_create(screen);
    lv_label_set_text(footer, "Select practice type - ESC to go back");
    lv_obj_set_style_text_font(footer, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(footer, LV_COLOR_WARNING, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -5);

    return screen;
}

// ============================================
// Copy Practice Screen
// ============================================

// Copy practice state
static lv_obj_t* licw_copy_char_label = NULL;
static lv_obj_t* licw_copy_input_label = NULL;
static lv_obj_t* licw_copy_ttr_label = NULL;
static lv_obj_t* licw_copy_score_label = NULL;
static lv_obj_t* licw_copy_feedback_label = NULL;

static char licw_copy_current_char = 'E';
static char licw_copy_user_input[64] = "";
static int licw_copy_input_pos = 0;
static unsigned long licw_copy_play_end_time = 0;
static bool licw_copy_waiting_for_input = false;

// Forward declaration for morse playback
// Note: Using character WPM for now; full Farnsworth would extend inter-character gaps
extern void playMorseString(const char* str, int wpm, int toneFreq);
extern int cwTone;  // From settings

// Play next character in copy practice
void licwCopyPlayNext() {
    // Get random character from cumulative set
    licw_copy_current_char = getLICWRandomChar(licwSelectedCarousel, licwSelectedLesson);

    // Clear input
    licw_copy_user_input[0] = '\0';
    licw_copy_input_pos = 0;

    // Update UI
    if (licw_copy_input_label) {
        lv_label_set_text(licw_copy_input_label, "_");
    }
    if (licw_copy_char_label) {
        lv_label_set_text(licw_copy_char_label, "?");  // Don't show character until after response
    }
    if (licw_copy_feedback_label) {
        lv_label_set_text(licw_copy_feedback_label, "Listen...");
        lv_obj_set_style_text_color(licw_copy_feedback_label, LV_COLOR_TEXT_SECONDARY, 0);
    }

    // Get lesson speed settings
    const LICWLesson* lesson = getLICWLesson(licwSelectedCarousel, licwSelectedLesson);

    // Play the character using character WPM
    // Note: For full Farnsworth, inter-character gaps should use effective WPM
    char charStr[2] = {licw_copy_current_char, '\0'};
    playMorseString(charStr, lesson->characterWPM, cwTone);

    // Record when playback finishes (approximate - actual would need callback)
    // For now, estimate based on character timing
    int ditMs = 1200 / lesson->characterWPM;
    const char* pattern = getMorseCode(licw_copy_current_char);
    int charDuration = 0;
    if (pattern) {
        for (int i = 0; pattern[i]; i++) {
            charDuration += (pattern[i] == '.') ? ditMs : (ditMs * 3);
            if (pattern[i + 1]) charDuration += ditMs;  // inter-element gap
        }
    }
    licw_copy_play_end_time = millis() + charDuration;

    licw_copy_waiting_for_input = true;

    Serial.printf("[LICW Copy] Playing character: %c\n", licw_copy_current_char);
}

// Handle keypress in copy practice
void licwCopyHandleKey(char key) {
    if (!licw_copy_waiting_for_input) return;

    // Record play end time on first key (if not already set)
    if (licw_copy_play_end_time == 0) {
        // This should be set when audio finishes, but fallback to now
        licw_copy_play_end_time = millis();
    }

    unsigned long ttr = millis() - licw_copy_play_end_time;
    bool correct = (toupper(key) == licw_copy_current_char);

    // Record TTR measurement
    recordLICWTTR(licw_copy_current_char, licw_copy_play_end_time, millis(), correct);

    // Update UI
    char char_str[2] = {licw_copy_current_char, '\0'};
    if (licw_copy_char_label) {
        lv_label_set_text(licw_copy_char_label, char_str);
    }

    char input_str[2] = {(char)toupper(key), '\0'};
    if (licw_copy_input_label) {
        lv_label_set_text(licw_copy_input_label, input_str);
    }

    // TTR display
    char ttr_text[32];
    formatTTR(ttr, ttr_text, sizeof(ttr_text));
    if (licw_copy_ttr_label) {
        char full_ttr[48];
        snprintf(full_ttr, sizeof(full_ttr), "TTR: %s", ttr_text);
        lv_label_set_text(licw_copy_ttr_label, full_ttr);
    }

    // Score update
    if (licw_copy_score_label) {
        char score_text[32];
        snprintf(score_text, sizeof(score_text), "%d/%d (%d%%)",
                 licwProgress.sessionCorrect, licwProgress.sessionTotal,
                 getLICWSessionAccuracy());
        lv_label_set_text(licw_copy_score_label, score_text);
    }

    // Feedback
    if (licw_copy_feedback_label) {
        if (correct) {
            lv_label_set_text(licw_copy_feedback_label, getTTRRating(ttr));
            lv_obj_set_style_text_color(licw_copy_feedback_label, LV_COLOR_SUCCESS, 0);
            beep(TONE_SUCCESS, BEEP_SHORT);
        } else {
            char feedback[32];
            snprintf(feedback, sizeof(feedback), "Was: %c", licw_copy_current_char);
            lv_label_set_text(licw_copy_feedback_label, feedback);
            lv_obj_set_style_text_color(licw_copy_feedback_label, LV_COLOR_ERROR, 0);
            beep(TONE_ERROR, BEEP_MEDIUM);
        }
    }

    licw_copy_waiting_for_input = false;
    licw_copy_play_end_time = 0;

    // Auto-advance after brief delay
    // This would be handled in the update loop
}

// Copy practice keyboard handler
static void licw_copy_key_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    // Handle letter keys (a-z, A-Z)
    if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z')) {
        licwCopyHandleKey((char)key);
    }
    // Handle number keys (0-9)
    else if (key >= '0' && key <= '9') {
        licwCopyHandleKey((char)key);
    }
    // Handle space to play next
    else if (key == ' ') {
        if (!licw_copy_waiting_for_input && !isTonePlaying()) {
            licwCopyPlayNext();
        }
    }
}

/*
 * Create copy practice screen
 */
lv_obj_t* createLICWCopyPracticeScreen() {
    const LICWCarouselDef* carousel = getLICWCarousel(licwSelectedCarousel);
    const LICWLesson* lesson = getLICWLesson(licwSelectedCarousel, licwSelectedLesson);

    // Reset session
    resetLICWSession();

    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, LV_COLOR_BG_DEEP, 0);

    // Header
    char header_text[48];
    snprintf(header_text, sizeof(header_text), "%s L%d COPY",
             carousel->shortName, licwSelectedLesson);
    createHeader(screen, header_text);

    // Main content area
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, LV_PCT(90), 180);
    lv_obj_center(content);
    lv_obj_set_style_bg_color(content, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_border_width(content, 2, 0);
    lv_obj_set_style_border_color(content, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_radius(content, 12, 0);
    lv_obj_set_style_pad_all(content, 20, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Character display (shows "?" until answered, then the character)
    // NO morse patterns - just the character letter itself
    licw_copy_char_label = lv_label_create(content);
    lv_label_set_text(licw_copy_char_label, "?");
    lv_obj_set_style_text_font(licw_copy_char_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(licw_copy_char_label, LV_COLOR_ACCENT_CYAN, 0);

    // User input display
    licw_copy_input_label = lv_label_create(content);
    lv_label_set_text(licw_copy_input_label, "_");
    lv_obj_set_style_text_font(licw_copy_input_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(licw_copy_input_label, LV_COLOR_TEXT_PRIMARY, 0);

    // Feedback label
    licw_copy_feedback_label = lv_label_create(content);
    lv_label_set_text(licw_copy_feedback_label, "Press SPACE to start");
    lv_obj_set_style_text_font(licw_copy_feedback_label, getThemeFonts()->font_input, 0);
    lv_obj_set_style_text_color(licw_copy_feedback_label, LV_COLOR_TEXT_SECONDARY, 0);

    // TTR display
    licw_copy_ttr_label = lv_label_create(content);
    lv_label_set_text(licw_copy_ttr_label, "TTR: --");
    lv_obj_set_style_text_font(licw_copy_ttr_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(licw_copy_ttr_label, LV_COLOR_TEXT_SECONDARY, 0);

    // Score display
    licw_copy_score_label = lv_label_create(screen);
    lv_label_set_text(licw_copy_score_label, "0/0 (0%)");
    lv_obj_set_style_text_font(licw_copy_score_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(licw_copy_score_label, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(licw_copy_score_label, LV_ALIGN_TOP_RIGHT, -20, 55);

    // Speed info
    lv_obj_t* speed_info = lv_label_create(screen);
    char speed_text[32];
    snprintf(speed_text, sizeof(speed_text), "Speed: %d/%d WPM",
             lesson->characterWPM, lesson->effectiveWPM);
    lv_label_set_text(speed_info, speed_text);
    lv_obj_set_style_text_font(speed_info, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(speed_info, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(speed_info, LV_ALIGN_TOP_LEFT, 20, 55);

    // Footer
    lv_obj_t* footer = lv_label_create(screen);
    lv_label_set_text(footer, "Type what you hear - SPACE for next - ESC to exit");
    lv_obj_set_style_text_font(footer, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(footer, LV_COLOR_WARNING, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -5);

    // Add keyboard handler
    lv_obj_add_event_cb(content, licw_copy_key_handler, LV_EVENT_KEY, NULL);
    lv_obj_add_flag(content, LV_OBJ_FLAG_CLICKABLE);  // Make focusable
    addNavigableWidget(content);

    return screen;
}

// ============================================
// Sending Practice Screen
// ============================================

// Sending practice state
static lv_obj_t* licw_send_target_label = NULL;
static lv_obj_t* licw_send_decoded_label = NULL;
static lv_obj_t* licw_send_score_label = NULL;
static lv_obj_t* licw_send_feedback_label = NULL;

static char licw_send_target[32] = "";
static String licw_send_decoded = "";
static int licw_send_correct = 0;
static int licw_send_total = 0;
static int licw_send_round = 0;
static bool licw_send_waiting = true;
static bool licw_send_showing_feedback = false;
static bool licw_send_needs_ui_update = false;

// Decoder for sending practice
static MorseDecoderAdaptive* licwSendDecoder = NULL;

// Paddle/keyer state for sending
static bool licw_send_dit_pressed = false;
static bool licw_send_dah_pressed = false;
static bool licw_send_keyer_active = false;
static bool licw_send_sending_dit = false;
static bool licw_send_sending_dah = false;
static bool licw_send_in_spacing = false;
static bool licw_send_dit_memory = false;
static bool licw_send_dah_memory = false;
static unsigned long licw_send_element_start = 0;
static int licw_send_dit_duration = 80;

// Decoder timing
static unsigned long licw_send_last_change = 0;
static bool licw_send_last_tone_state = false;
static unsigned long licw_send_last_element = 0;

// Start next sending round
void licwSendStartRound() {
    licw_send_round++;
    const LICWLesson* lesson = getLICWLesson(licwSelectedCarousel, licwSelectedLesson);

    // Get content based on lesson
    if (lesson->words && countLICWWords(lesson->words) > 0) {
        // Use a random word
        strncpy(licw_send_target, getLICWRandomWord(licwSelectedCarousel, licwSelectedLesson), sizeof(licw_send_target) - 1);
    } else {
        // Use random character group
        getLICWRandomGroup(licwSelectedCarousel, licwSelectedLesson, licw_send_target, sizeof(licw_send_target));
    }
    licw_send_target[sizeof(licw_send_target) - 1] = '\0';

    licw_send_decoded = "";
    licw_send_waiting = true;
    licw_send_showing_feedback = false;

    // Reset decoder
    if (licwSendDecoder) {
        licwSendDecoder->reset();
        licwSendDecoder->flush();
    }
    licw_send_last_change = 0;
    licw_send_last_tone_state = false;
    licw_send_last_element = 0;

    // Update UI
    if (licw_send_target_label) {
        char display[48];
        snprintf(display, sizeof(display), "Send: %s", licw_send_target);
        lv_label_set_text(licw_send_target_label, display);
    }
    if (licw_send_decoded_label) {
        lv_label_set_text(licw_send_decoded_label, "...");
    }
    if (licw_send_feedback_label) {
        lv_label_set_text(licw_send_feedback_label, "Use paddle to send");
        lv_obj_set_style_text_color(licw_send_feedback_label, LV_COLOR_TEXT_SECONDARY, 0);
    }

    Serial.printf("[LICW Send] Round %d target: %s\n", licw_send_round, licw_send_target);
}

// Handle iambic keyer for sending
void licwSendHandleKeyer() {
    unsigned long currentTime = millis();
    const LICWLesson* lesson = getLICWLesson(licwSelectedCarousel, licwSelectedLesson);
    licw_send_dit_duration = 1200 / lesson->characterWPM;

    // If not sending or spacing, check for new input
    if (!licw_send_keyer_active && !licw_send_in_spacing) {
        if (licw_send_dit_pressed || licw_send_dit_memory) {
            // Start dit
            if (!licw_send_last_tone_state && licw_send_last_change > 0) {
                float silence = currentTime - licw_send_last_change;
                if (silence > 0 && licwSendDecoder) {
                    licwSendDecoder->addTiming(-silence);
                }
            }
            licw_send_last_change = currentTime;
            licw_send_last_tone_state = true;

            licw_send_keyer_active = true;
            licw_send_sending_dit = true;
            licw_send_sending_dah = false;
            licw_send_in_spacing = false;
            licw_send_element_start = currentTime;
            startTone(cwTone);
            licw_send_dit_memory = false;
        }
        else if (licw_send_dah_pressed || licw_send_dah_memory) {
            // Start dah
            if (!licw_send_last_tone_state && licw_send_last_change > 0) {
                float silence = currentTime - licw_send_last_change;
                if (silence > 0 && licwSendDecoder) {
                    licwSendDecoder->addTiming(-silence);
                }
            }
            licw_send_last_change = currentTime;
            licw_send_last_tone_state = true;

            licw_send_keyer_active = true;
            licw_send_sending_dit = false;
            licw_send_sending_dah = true;
            licw_send_in_spacing = false;
            licw_send_element_start = currentTime;
            startTone(cwTone);
            licw_send_dah_memory = false;
        }
    }
    // Sending element
    else if (licw_send_keyer_active && !licw_send_in_spacing) {
        int duration = licw_send_sending_dit ? licw_send_dit_duration : (licw_send_dit_duration * 3);

        // Check for opposite paddle (iambic memory)
        if (licw_send_sending_dit && licw_send_dah_pressed) {
            licw_send_dah_memory = true;
        } else if (licw_send_sending_dah && licw_send_dit_pressed) {
            licw_send_dit_memory = true;
        }

        // Element complete?
        if (currentTime - licw_send_element_start >= (unsigned long)duration) {
            if (licw_send_last_tone_state) {
                float toneDuration = currentTime - licw_send_last_change;
                if (toneDuration > 0 && licwSendDecoder) {
                    licwSendDecoder->addTiming(toneDuration);
                    licw_send_last_element = currentTime;
                }
                licw_send_last_change = currentTime;
                licw_send_last_tone_state = false;
            }
            stopTone();
            licw_send_keyer_active = false;
            licw_send_in_spacing = true;
            licw_send_element_start = currentTime;
        }
    }
    // Inter-element spacing
    else if (licw_send_in_spacing) {
        if (currentTime - licw_send_element_start >= (unsigned long)licw_send_dit_duration) {
            licw_send_in_spacing = false;
        }
    }
}

// Update sending practice (called from main loop integration)
void updateLICWSendingPractice() {
    if (!licw_send_waiting) return;

    const LICWLesson* lesson = getLICWLesson(licwSelectedCarousel, licwSelectedLesson);

    // Check for decoder timeout
    if (licw_send_last_element > 0 && !licw_send_dit_pressed && !licw_send_dah_pressed) {
        unsigned long timeSince = millis() - licw_send_last_element;
        float wordGap = MorseWPM::wordGap(lesson->characterWPM);
        if (timeSince > wordGap && licwSendDecoder) {
            licwSendDecoder->flush();
            licw_send_last_element = 0;
        }
    }

    // Read paddle inputs
    licw_send_dit_pressed = (digitalRead(DIT_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DIT_PIN) > TOUCH_THRESHOLD);
    licw_send_dah_pressed = (digitalRead(DAH_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DAH_PIN) > TOUCH_THRESHOLD);

    // Handle keyer
    if (cwKeyType == KEY_STRAIGHT) {
        // Straight key
        bool toneOn = isTonePlaying();
        if (licw_send_dit_pressed && !toneOn) {
            if (!licw_send_last_tone_state && licw_send_last_change > 0 && licwSendDecoder) {
                float silence = millis() - licw_send_last_change;
                if (silence > 0) licwSendDecoder->addTiming(-silence);
            }
            licw_send_last_change = millis();
            licw_send_last_tone_state = true;
            startTone(cwTone);
        }
        else if (licw_send_dit_pressed && toneOn) {
            continueTone(cwTone);
        }
        else if (!licw_send_dit_pressed && toneOn) {
            if (licw_send_last_tone_state && licwSendDecoder) {
                float toneDuration = millis() - licw_send_last_change;
                if (toneDuration > 0) {
                    licwSendDecoder->addTiming(toneDuration);
                    licw_send_last_element = millis();
                }
            }
            licw_send_last_change = millis();
            licw_send_last_tone_state = false;
            stopTone();
        }
    } else {
        licwSendHandleKeyer();
    }

    // Update UI if decoder produced output
    if (licw_send_needs_ui_update && licw_send_decoded_label) {
        lv_label_set_text(licw_send_decoded_label, licw_send_decoded.length() > 0 ? licw_send_decoded.c_str() : "...");
        licw_send_needs_ui_update = false;
    }
}

// Sending practice keyboard handler
static void licw_send_key_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    if (licw_send_showing_feedback) {
        // Any key advances
        if (licw_send_round >= 10) {
            // Done - go back
            onLVGLMenuSelect(122);  // Practice type select
        } else {
            licwSendStartRound();
        }
        return;
    }

    if (licw_send_waiting) {
        if (key == LV_KEY_ENTER || key == '\r' || key == '\n') {
            // Submit
            if (licwSendDecoder) {
                licwSendDecoder->flush();
            }

            licw_send_total++;
            String target = String(licw_send_target);
            target.toUpperCase();
            String decoded = licw_send_decoded;
            decoded.toUpperCase();
            decoded.trim();

            bool correct = (decoded == target);
            if (correct) {
                licw_send_correct++;
                beep(TONE_SUCCESS, BEEP_SHORT);
            } else {
                beep(TONE_ERROR, BEEP_MEDIUM);
            }

            // Show feedback
            licw_send_showing_feedback = true;
            licw_send_waiting = false;
            stopTone();

            if (licw_send_feedback_label) {
                if (correct) {
                    lv_label_set_text(licw_send_feedback_label, "Correct!");
                    lv_obj_set_style_text_color(licw_send_feedback_label, LV_COLOR_SUCCESS, 0);
                } else {
                    char fb[64];
                    snprintf(fb, sizeof(fb), "Was: %s, You: %s", licw_send_target, decoded.c_str());
                    lv_label_set_text(licw_send_feedback_label, fb);
                    lv_obj_set_style_text_color(licw_send_feedback_label, LV_COLOR_ERROR, 0);
                }
            }

            if (licw_send_score_label) {
                char score[32];
                snprintf(score, sizeof(score), "%d/%d (%d%%)", licw_send_correct, licw_send_total,
                         licw_send_total > 0 ? (licw_send_correct * 100 / licw_send_total) : 0);
                lv_label_set_text(licw_send_score_label, score);
            }
        }
        else if (key == 'P' || key == 'p') {
            // Play target
            const LICWLesson* lesson = getLICWLesson(licwSelectedCarousel, licwSelectedLesson);
            playMorseString(licw_send_target, lesson->characterWPM, cwTone);
        }
    }
}

/*
 * Create sending practice screen
 */
lv_obj_t* createLICWSendPracticeScreen() {
    const LICWCarouselDef* carousel = getLICWCarousel(licwSelectedCarousel);
    const LICWLesson* lesson = getLICWLesson(licwSelectedCarousel, licwSelectedLesson);

    // Initialize decoder
    if (licwSendDecoder == NULL) {
        licwSendDecoder = new MorseDecoderAdaptive(lesson->characterWPM, lesson->characterWPM, 30);
    } else {
        licwSendDecoder->setWPM(lesson->characterWPM);
    }
    licwSendDecoder->messageCallback = [](String morse, String text) {
        for (unsigned int i = 0; i < text.length(); i++) {
            licw_send_decoded += text[i];
        }
        licw_send_needs_ui_update = true;
    };

    // Reset state
    licw_send_correct = 0;
    licw_send_total = 0;
    licw_send_round = 0;
    licw_send_decoded = "";

    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, LV_COLOR_BG_DEEP, 0);

    // Header
    char header_text[48];
    snprintf(header_text, sizeof(header_text), "%s L%d SEND", carousel->shortName, licwSelectedLesson);
    createHeader(screen, header_text);

    // Main content
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, LV_PCT(90), 180);
    lv_obj_center(content);
    lv_obj_set_style_bg_color(content, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_border_width(content, 2, 0);
    lv_obj_set_style_border_color(content, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_radius(content, 12, 0);
    lv_obj_set_style_pad_all(content, 20, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Target display (no morse patterns - just the character/word)
    licw_send_target_label = lv_label_create(content);
    lv_label_set_text(licw_send_target_label, "Send: ---");
    lv_obj_set_style_text_font(licw_send_target_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(licw_send_target_label, LV_COLOR_ACCENT_CYAN, 0);

    // Decoded display
    licw_send_decoded_label = lv_label_create(content);
    lv_label_set_text(licw_send_decoded_label, "...");
    lv_obj_set_style_text_font(licw_send_decoded_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(licw_send_decoded_label, LV_COLOR_TEXT_PRIMARY, 0);

    // Feedback
    licw_send_feedback_label = lv_label_create(content);
    lv_label_set_text(licw_send_feedback_label, "Press SPACE to start");
    lv_obj_set_style_text_font(licw_send_feedback_label, getThemeFonts()->font_input, 0);
    lv_obj_set_style_text_color(licw_send_feedback_label, LV_COLOR_TEXT_SECONDARY, 0);

    // Score
    licw_send_score_label = lv_label_create(screen);
    lv_label_set_text(licw_send_score_label, "0/0 (0%)");
    lv_obj_set_style_text_font(licw_send_score_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(licw_send_score_label, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(licw_send_score_label, LV_ALIGN_TOP_RIGHT, -20, 55);

    // Speed info
    lv_obj_t* speed_info = lv_label_create(screen);
    char speed_text[32];
    snprintf(speed_text, sizeof(speed_text), "Speed: %d/%d WPM", lesson->characterWPM, lesson->effectiveWPM);
    lv_label_set_text(speed_info, speed_text);
    lv_obj_set_style_text_font(speed_info, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(speed_info, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(speed_info, LV_ALIGN_TOP_LEFT, 20, 55);

    // Footer
    lv_obj_t* footer = lv_label_create(screen);
    lv_label_set_text(footer, "Use paddle - P to hear - ENTER when done - ESC exit");
    lv_obj_set_style_text_font(footer, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(footer, LV_COLOR_WARNING, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -5);

    // Event handler
    lv_obj_add_event_cb(content, licw_send_key_handler, LV_EVENT_KEY, NULL);
    lv_obj_add_flag(content, LV_OBJ_FLAG_CLICKABLE);
    addNavigableWidget(content);

    // Start first round
    licwSendStartRound();

    return screen;
}

// ============================================
// IFR (Instant Flow Recovery) Practice Screen
// ============================================

// IFR state - continuous stream, skip misses, keep going
static lv_obj_t* licw_ifr_stream_label = NULL;
static lv_obj_t* licw_ifr_input_label = NULL;
static lv_obj_t* licw_ifr_score_label = NULL;
static lv_obj_t* licw_ifr_feedback_label = NULL;

static char licw_ifr_stream[32] = "";      // Current stream of characters
static char licw_ifr_input[32] = "";       // User's input so far
static int licw_ifr_stream_pos = 0;        // Current position in stream
static int licw_ifr_input_pos = 0;
static int licw_ifr_correct = 0;
static int licw_ifr_total = 0;
static int licw_ifr_round = 0;
static bool licw_ifr_playing = false;
static bool licw_ifr_round_done = false;

// Generate a stream of characters for IFR
void licwIFRGenerateStream() {
    const LICWLesson* lesson = getLICWLesson(licwSelectedCarousel, licwSelectedLesson);
    int streamLen = 5 + (licw_ifr_round / 2);  // 5-10 chars
    if (streamLen > 10) streamLen = 10;

    for (int i = 0; i < streamLen; i++) {
        licw_ifr_stream[i] = getLICWRandomChar(licwSelectedCarousel, licwSelectedLesson);
    }
    licw_ifr_stream[streamLen] = '\0';
    licw_ifr_input[0] = '\0';
    licw_ifr_stream_pos = 0;
    licw_ifr_input_pos = 0;
}

// Start IFR round
void licwIFRStartRound() {
    licw_ifr_round++;
    licwIFRGenerateStream();
    licw_ifr_playing = false;
    licw_ifr_round_done = false;

    if (licw_ifr_stream_label) {
        lv_label_set_text(licw_ifr_stream_label, "?????");  // Hidden until played
    }
    if (licw_ifr_input_label) {
        lv_label_set_text(licw_ifr_input_label, "_");
    }
    if (licw_ifr_feedback_label) {
        lv_label_set_text(licw_ifr_feedback_label, "Press SPACE to play stream");
        lv_obj_set_style_text_color(licw_ifr_feedback_label, LV_COLOR_TEXT_SECONDARY, 0);
    }

    Serial.printf("[LICW IFR] Round %d stream: %s\n", licw_ifr_round, licw_ifr_stream);
}

// Play the IFR stream (continuous, no replay allowed)
void licwIFRPlayStream() {
    if (licw_ifr_playing) return;

    const LICWLesson* lesson = getLICWLesson(licwSelectedCarousel, licwSelectedLesson);
    licw_ifr_playing = true;

    // Play the entire stream
    playMorseString(licw_ifr_stream, lesson->characterWPM, cwTone);

    if (licw_ifr_feedback_label) {
        lv_label_set_text(licw_ifr_feedback_label, "Type as you hear - skip misses!");
        lv_obj_set_style_text_color(licw_ifr_feedback_label, LV_COLOR_WARNING, 0);
    }
}

// Handle IFR input
void licwIFRHandleInput(char key) {
    if (!licw_ifr_playing || licw_ifr_round_done) return;

    key = toupper(key);

    // Add to input buffer
    if (licw_ifr_input_pos < (int)sizeof(licw_ifr_input) - 1) {
        licw_ifr_input[licw_ifr_input_pos++] = key;
        licw_ifr_input[licw_ifr_input_pos] = '\0';
    }

    // Update display
    if (licw_ifr_input_label) {
        lv_label_set_text(licw_ifr_input_label, licw_ifr_input);
    }

    // Check if round complete (user typed enough chars)
    if (licw_ifr_input_pos >= (int)strlen(licw_ifr_stream)) {
        licw_ifr_round_done = true;

        // Score: count matching characters in sequence
        int matches = 0;
        int streamLen = strlen(licw_ifr_stream);
        int inputLen = strlen(licw_ifr_input);

        // Simple scoring: characters that match in position
        for (int i = 0; i < streamLen && i < inputLen; i++) {
            if (licw_ifr_stream[i] == licw_ifr_input[i]) {
                matches++;
            }
        }

        licw_ifr_correct += matches;
        licw_ifr_total += streamLen;

        // Show results
        if (licw_ifr_stream_label) {
            lv_label_set_text(licw_ifr_stream_label, licw_ifr_stream);
        }

        int pct = (matches * 100) / streamLen;
        if (licw_ifr_feedback_label) {
            char fb[48];
            snprintf(fb, sizeof(fb), "%d/%d (%d%%) - press any key", matches, streamLen, pct);
            lv_label_set_text(licw_ifr_feedback_label, fb);
            lv_obj_set_style_text_color(licw_ifr_feedback_label, pct >= 70 ? LV_COLOR_SUCCESS : LV_COLOR_WARNING, 0);
        }

        if (licw_ifr_score_label) {
            char score[32];
            int totalPct = licw_ifr_total > 0 ? (licw_ifr_correct * 100 / licw_ifr_total) : 0;
            snprintf(score, sizeof(score), "%d/%d (%d%%)", licw_ifr_correct, licw_ifr_total, totalPct);
            lv_label_set_text(licw_ifr_score_label, score);
        }

        beep(pct >= 70 ? TONE_SUCCESS : TONE_ERROR, BEEP_SHORT);
    }
}

// IFR key handler
static void licw_ifr_key_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    if (licw_ifr_round_done) {
        // Any key advances
        if (licw_ifr_round >= 10) {
            onLVGLMenuSelect(122);  // Back to practice type
        } else {
            licwIFRStartRound();
        }
        return;
    }

    if (key == ' ' && !licw_ifr_playing) {
        licwIFRPlayStream();
    }
    else if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z') ||
             (key >= '0' && key <= '9')) {
        licwIFRHandleInput((char)key);
    }
}

/*
 * Create IFR practice screen
 */
lv_obj_t* createLICWIFRPracticeScreen() {
    const LICWCarouselDef* carousel = getLICWCarousel(licwSelectedCarousel);
    const LICWLesson* lesson = getLICWLesson(licwSelectedCarousel, licwSelectedLesson);

    // Reset state
    licw_ifr_correct = 0;
    licw_ifr_total = 0;
    licw_ifr_round = 0;

    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, LV_COLOR_BG_DEEP, 0);

    // Header
    char header_text[48];
    snprintf(header_text, sizeof(header_text), "%s L%d IFR", carousel->shortName, licwSelectedLesson);
    createHeader(screen, header_text);

    // Info panel
    lv_obj_t* info = lv_label_create(screen);
    lv_label_set_text(info, "Instant Flow Recovery: Skip misses, keep going!");
    lv_obj_set_style_text_font(info, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(info, LV_COLOR_WARNING, 0);
    lv_obj_align(info, LV_ALIGN_TOP_MID, 0, 55);

    // Main content
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, LV_PCT(90), 150);
    lv_obj_center(content);
    lv_obj_set_style_bg_color(content, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_border_width(content, 2, 0);
    lv_obj_set_style_border_color(content, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_radius(content, 12, 0);
    lv_obj_set_style_pad_all(content, 20, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Stream display (hidden until played)
    licw_ifr_stream_label = lv_label_create(content);
    lv_label_set_text(licw_ifr_stream_label, "?????");
    lv_obj_set_style_text_font(licw_ifr_stream_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(licw_ifr_stream_label, LV_COLOR_TEXT_SECONDARY, 0);

    // User input
    licw_ifr_input_label = lv_label_create(content);
    lv_label_set_text(licw_ifr_input_label, "_");
    lv_obj_set_style_text_font(licw_ifr_input_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(licw_ifr_input_label, LV_COLOR_ACCENT_CYAN, 0);

    // Feedback
    licw_ifr_feedback_label = lv_label_create(content);
    lv_label_set_text(licw_ifr_feedback_label, "Press SPACE to start");
    lv_obj_set_style_text_font(licw_ifr_feedback_label, getThemeFonts()->font_input, 0);
    lv_obj_set_style_text_color(licw_ifr_feedback_label, LV_COLOR_TEXT_SECONDARY, 0);

    // Score
    licw_ifr_score_label = lv_label_create(screen);
    lv_label_set_text(licw_ifr_score_label, "0/0 (0%)");
    lv_obj_set_style_text_font(licw_ifr_score_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(licw_ifr_score_label, LV_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(licw_ifr_score_label, LV_ALIGN_TOP_RIGHT, -20, 55);

    // Footer
    lv_obj_t* footer = lv_label_create(screen);
    lv_label_set_text(footer, "SPACE play - type as you hear - NO replay - ESC exit");
    lv_obj_set_style_text_font(footer, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(footer, LV_COLOR_WARNING, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -5);

    // Event handler
    lv_obj_add_event_cb(content, licw_ifr_key_handler, LV_EVENT_KEY, NULL);
    lv_obj_add_flag(content, LV_OBJ_FLAG_CLICKABLE);
    addNavigableWidget(content);

    // Start first round
    licwIFRStartRound();

    return screen;
}

// ============================================
// CFP (Character Flow Proficiency) Practice Screen
// ============================================

// CFP: Continuous flow, minimal spacing, track flow rate
static lv_obj_t* licw_cfp_char_label = NULL;
static lv_obj_t* licw_cfp_input_label = NULL;
static lv_obj_t* licw_cfp_score_label = NULL;
static lv_obj_t* licw_cfp_rate_label = NULL;

static char licw_cfp_chars[64] = "";
static char licw_cfp_input[64] = "";
static int licw_cfp_pos = 0;
static int licw_cfp_correct = 0;
static int licw_cfp_total = 0;
static unsigned long licw_cfp_start_time = 0;
static bool licw_cfp_active = false;

// Generate CFP character sequence
void licwCFPGenerate() {
    int count = 20;  // 20 characters per round
    for (int i = 0; i < count; i++) {
        licw_cfp_chars[i] = getLICWRandomChar(licwSelectedCarousel, licwSelectedLesson);
    }
    licw_cfp_chars[count] = '\0';
    licw_cfp_input[0] = '\0';
    licw_cfp_pos = 0;
    licw_cfp_active = false;
}

// Start CFP playback
void licwCFPStart() {
    if (licw_cfp_active) return;

    const LICWLesson* lesson = getLICWLesson(licwSelectedCarousel, licwSelectedLesson);
    licw_cfp_active = true;
    licw_cfp_start_time = millis();

    // Play all characters - user types as they hear
    playMorseString(licw_cfp_chars, lesson->characterWPM, cwTone);

    if (licw_cfp_char_label) {
        lv_label_set_text(licw_cfp_char_label, "Listen and type...");
    }
}

// Handle CFP input
void licwCFPHandleInput(char key) {
    if (!licw_cfp_active) return;

    key = toupper(key);

    if (licw_cfp_pos < (int)sizeof(licw_cfp_input) - 1) {
        licw_cfp_input[licw_cfp_pos] = key;
        licw_cfp_pos++;
        licw_cfp_input[licw_cfp_pos] = '\0';
    }

    // Update display
    if (licw_cfp_input_label) {
        lv_label_set_text(licw_cfp_input_label, licw_cfp_input);
    }

    // Check if done
    if (licw_cfp_pos >= (int)strlen(licw_cfp_chars)) {
        licw_cfp_active = false;

        // Calculate score
        int matches = 0;
        for (int i = 0; i < (int)strlen(licw_cfp_chars) && i < (int)strlen(licw_cfp_input); i++) {
            if (licw_cfp_chars[i] == licw_cfp_input[i]) matches++;
        }

        licw_cfp_correct += matches;
        licw_cfp_total += strlen(licw_cfp_chars);

        // Calculate rate (chars per minute)
        unsigned long elapsed = millis() - licw_cfp_start_time;
        int cpm = (elapsed > 0) ? (licw_cfp_pos * 60000 / elapsed) : 0;

        if (licw_cfp_char_label) {
            lv_label_set_text(licw_cfp_char_label, licw_cfp_chars);
        }

        if (licw_cfp_rate_label) {
            char rate[32];
            snprintf(rate, sizeof(rate), "Rate: %d CPM", cpm);
            lv_label_set_text(licw_cfp_rate_label, rate);
        }

        if (licw_cfp_score_label) {
            int pct = licw_cfp_total > 0 ? (licw_cfp_correct * 100 / licw_cfp_total) : 0;
            char score[32];
            snprintf(score, sizeof(score), "%d/%d (%d%%)", licw_cfp_correct, licw_cfp_total, pct);
            lv_label_set_text(licw_cfp_score_label, score);
        }

        beep(matches >= (int)strlen(licw_cfp_chars) * 7 / 10 ? TONE_SUCCESS : TONE_ERROR, BEEP_SHORT);
    }
}

// CFP key handler
static void licw_cfp_key_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    if (!licw_cfp_active && licw_cfp_pos >= (int)strlen(licw_cfp_chars) && licw_cfp_pos > 0) {
        // Round done, any key starts new
        licwCFPGenerate();
        if (licw_cfp_char_label) lv_label_set_text(licw_cfp_char_label, "Press SPACE to start");
        if (licw_cfp_input_label) lv_label_set_text(licw_cfp_input_label, "_");
        return;
    }

    if (key == ' ' && !licw_cfp_active) {
        licwCFPStart();
    }
    else if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z') ||
             (key >= '0' && key <= '9')) {
        licwCFPHandleInput((char)key);
    }
}

/*
 * Create CFP practice screen
 */
lv_obj_t* createLICWCFPPracticeScreen() {
    const LICWCarouselDef* carousel = getLICWCarousel(licwSelectedCarousel);

    // Reset state
    licw_cfp_correct = 0;
    licw_cfp_total = 0;
    licwCFPGenerate();

    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, LV_COLOR_BG_DEEP, 0);

    char header_text[48];
    snprintf(header_text, sizeof(header_text), "%s L%d CFP", carousel->shortName, licwSelectedLesson);
    createHeader(screen, header_text);

    // Info
    lv_obj_t* info = lv_label_create(screen);
    lv_label_set_text(info, "Character Flow: Continuous stream, stay with the flow");
    lv_obj_set_style_text_font(info, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(info, LV_COLOR_WARNING, 0);
    lv_obj_align(info, LV_ALIGN_TOP_MID, 0, 55);

    // Content
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, LV_PCT(90), 150);
    lv_obj_center(content);
    lv_obj_set_style_bg_color(content, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_border_width(content, 2, 0);
    lv_obj_set_style_border_color(content, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_radius(content, 12, 0);
    lv_obj_set_style_pad_all(content, 15, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    licw_cfp_char_label = lv_label_create(content);
    lv_label_set_text(licw_cfp_char_label, "Press SPACE to start");
    lv_obj_set_style_text_font(licw_cfp_char_label, getThemeFonts()->font_input, 0);
    lv_obj_set_style_text_color(licw_cfp_char_label, LV_COLOR_TEXT_SECONDARY, 0);

    licw_cfp_input_label = lv_label_create(content);
    lv_label_set_text(licw_cfp_input_label, "_");
    lv_obj_set_style_text_font(licw_cfp_input_label, getThemeFonts()->font_input, 0);
    lv_obj_set_style_text_color(licw_cfp_input_label, LV_COLOR_ACCENT_CYAN, 0);

    licw_cfp_rate_label = lv_label_create(content);
    lv_label_set_text(licw_cfp_rate_label, "Rate: -- CPM");
    lv_obj_set_style_text_font(licw_cfp_rate_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(licw_cfp_rate_label, LV_COLOR_TEXT_SECONDARY, 0);

    licw_cfp_score_label = lv_label_create(screen);
    lv_label_set_text(licw_cfp_score_label, "0/0 (0%)");
    lv_obj_set_style_text_font(licw_cfp_score_label, getThemeFonts()->font_body, 0);
    lv_obj_align(licw_cfp_score_label, LV_ALIGN_TOP_RIGHT, -20, 55);

    lv_obj_t* footer = lv_label_create(screen);
    lv_label_set_text(footer, "SPACE start - type as you hear - ESC exit");
    lv_obj_set_style_text_font(footer, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(footer, LV_COLOR_WARNING, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -5);

    lv_obj_add_event_cb(content, licw_cfp_key_handler, LV_EVENT_KEY, NULL);
    lv_obj_add_flag(content, LV_OBJ_FLAG_CLICKABLE);
    addNavigableWidget(content);

    return screen;
}

// ============================================
// Word Discovery Practice Screen
// ============================================

static lv_obj_t* licw_word_input_label = NULL;
static lv_obj_t* licw_word_feedback_label = NULL;
static lv_obj_t* licw_word_score_label = NULL;

static char licw_word_current[32] = "";
static char licw_word_input[32] = "";
static int licw_word_input_pos = 0;
static int licw_word_correct = 0;
static int licw_word_total = 0;
static bool licw_word_waiting = true;
static bool licw_word_done = false;

void licwWordStartRound() {
    // Get a random word
    strncpy(licw_word_current, getLICWRandomWord(licwSelectedCarousel, licwSelectedLesson), sizeof(licw_word_current) - 1);
    licw_word_current[sizeof(licw_word_current) - 1] = '\0';

    licw_word_input[0] = '\0';
    licw_word_input_pos = 0;
    licw_word_waiting = true;
    licw_word_done = false;

    if (licw_word_input_label) {
        lv_label_set_text(licw_word_input_label, "_");
    }
    if (licw_word_feedback_label) {
        lv_label_set_text(licw_word_feedback_label, "Press SPACE to hear word");
        lv_obj_set_style_text_color(licw_word_feedback_label, LV_COLOR_TEXT_SECONDARY, 0);
    }

    Serial.printf("[LICW Word] New word: %s\n", licw_word_current);
}

void licwWordPlayCurrent() {
    const LICWLesson* lesson = getLICWLesson(licwSelectedCarousel, licwSelectedLesson);
    playMorseString(licw_word_current, lesson->characterWPM, cwTone);

    if (licw_word_feedback_label) {
        lv_label_set_text(licw_word_feedback_label, "Type the word you heard");
        lv_obj_set_style_text_color(licw_word_feedback_label, LV_COLOR_WARNING, 0);
    }
}

static void licw_word_key_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    if (licw_word_done) {
        licwWordStartRound();
        return;
    }

    if (key == ' ' && licw_word_waiting) {
        licwWordPlayCurrent();
        licw_word_waiting = false;
    }
    else if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z')) {
        if (licw_word_input_pos < (int)sizeof(licw_word_input) - 1) {
            licw_word_input[licw_word_input_pos++] = toupper((char)key);
            licw_word_input[licw_word_input_pos] = '\0';
            if (licw_word_input_label) {
                lv_label_set_text(licw_word_input_label, licw_word_input);
            }
        }
    }
    else if (key == LV_KEY_ENTER || key == '\r') {
        // Submit
        licw_word_total++;
        String target = String(licw_word_current);
        target.toUpperCase();
        String input = String(licw_word_input);
        input.toUpperCase();

        bool correct = (input == target);
        if (correct) {
            licw_word_correct++;
            beep(TONE_SUCCESS, BEEP_SHORT);
        } else {
            beep(TONE_ERROR, BEEP_MEDIUM);
        }

        licw_word_done = true;

        if (licw_word_feedback_label) {
            char fb[64];
            if (correct) {
                snprintf(fb, sizeof(fb), "Correct! '%s'", licw_word_current);
                lv_obj_set_style_text_color(licw_word_feedback_label, LV_COLOR_SUCCESS, 0);
            } else {
                snprintf(fb, sizeof(fb), "Was: %s - any key continues", licw_word_current);
                lv_obj_set_style_text_color(licw_word_feedback_label, LV_COLOR_ERROR, 0);
            }
            lv_label_set_text(licw_word_feedback_label, fb);
        }

        if (licw_word_score_label) {
            char score[32];
            int pct = licw_word_total > 0 ? (licw_word_correct * 100 / licw_word_total) : 0;
            snprintf(score, sizeof(score), "%d/%d (%d%%)", licw_word_correct, licw_word_total, pct);
            lv_label_set_text(licw_word_score_label, score);
        }
    }
    else if (key == LV_KEY_BACKSPACE || key == 0x08) {
        if (licw_word_input_pos > 0) {
            licw_word_input[--licw_word_input_pos] = '\0';
            if (licw_word_input_label) {
                lv_label_set_text(licw_word_input_label, licw_word_input_pos > 0 ? licw_word_input : "_");
            }
        }
    }
}

lv_obj_t* createLICWWordDiscoveryScreen() {
    const LICWCarouselDef* carousel = getLICWCarousel(licwSelectedCarousel);

    licw_word_correct = 0;
    licw_word_total = 0;

    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, LV_COLOR_BG_DEEP, 0);

    char header_text[48];
    snprintf(header_text, sizeof(header_text), "%s L%d WORDS", carousel->shortName, licwSelectedLesson);
    createHeader(screen, header_text);

    lv_obj_t* info = lv_label_create(screen);
    lv_label_set_text(info, "Word Discovery: Hear the word as a whole, not letters");
    lv_obj_set_style_text_font(info, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(info, LV_COLOR_WARNING, 0);
    lv_obj_align(info, LV_ALIGN_TOP_MID, 0, 55);

    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, LV_PCT(90), 150);
    lv_obj_center(content);
    lv_obj_set_style_bg_color(content, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_border_width(content, 2, 0);
    lv_obj_set_style_border_color(content, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_radius(content, 12, 0);
    lv_obj_set_style_pad_all(content, 20, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    licw_word_input_label = lv_label_create(content);
    lv_label_set_text(licw_word_input_label, "_");
    lv_obj_set_style_text_font(licw_word_input_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(licw_word_input_label, LV_COLOR_ACCENT_CYAN, 0);

    licw_word_feedback_label = lv_label_create(content);
    lv_label_set_text(licw_word_feedback_label, "Press SPACE to hear word");
    lv_obj_set_style_text_font(licw_word_feedback_label, getThemeFonts()->font_input, 0);
    lv_obj_set_style_text_color(licw_word_feedback_label, LV_COLOR_TEXT_SECONDARY, 0);

    licw_word_score_label = lv_label_create(screen);
    lv_label_set_text(licw_word_score_label, "0/0 (0%)");
    lv_obj_set_style_text_font(licw_word_score_label, getThemeFonts()->font_body, 0);
    lv_obj_align(licw_word_score_label, LV_ALIGN_TOP_RIGHT, -20, 55);

    lv_obj_t* footer = lv_label_create(screen);
    lv_label_set_text(footer, "SPACE play - type word - ENTER submit - ESC exit");
    lv_obj_set_style_text_font(footer, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(footer, LV_COLOR_WARNING, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -5);

    lv_obj_add_event_cb(content, licw_word_key_handler, LV_EVENT_KEY, NULL);
    lv_obj_add_flag(content, LV_OBJ_FLAG_CLICKABLE);
    addNavigableWidget(content);

    licwWordStartRound();

    return screen;
}

// ============================================
// QSO Practice Screen (BC3+)
// ============================================

static lv_obj_t* licw_qso_exchange_label = NULL;
static lv_obj_t* licw_qso_input_label = NULL;
static lv_obj_t* licw_qso_feedback_label = NULL;

static int licw_qso_step = 0;
static char licw_qso_input[64] = "";
static int licw_qso_input_pos = 0;

void licwQSOStartExchange() {
    licw_qso_step = 0;
    licw_qso_input[0] = '\0';
    licw_qso_input_pos = 0;

    if (licw_qso_exchange_label) {
        lv_label_set_text(licw_qso_exchange_label, "CQ CQ CQ DE ???");
    }
    if (licw_qso_input_label) {
        lv_label_set_text(licw_qso_input_label, "_");
    }
    if (licw_qso_feedback_label) {
        lv_label_set_text(licw_qso_feedback_label, "SPACE to hear CQ - type the callsign");
        lv_obj_set_style_text_color(licw_qso_feedback_label, LV_COLOR_TEXT_SECONDARY, 0);
    }
}

static void licw_qso_key_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    if (key == ' ') {
        // Play sample QSO
        const LICWLesson* lesson = getLICWLesson(licwSelectedCarousel, licwSelectedLesson);
        const char* phrase = getLICWRandomPhrase(licwSelectedCarousel, licwSelectedLesson);
        playMorseString(phrase, lesson->characterWPM, cwTone);

        if (licw_qso_feedback_label) {
            lv_label_set_text(licw_qso_feedback_label, "Listen to the exchange...");
            lv_obj_set_style_text_color(licw_qso_feedback_label, LV_COLOR_WARNING, 0);
        }
    }
    else if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z') ||
             (key >= '0' && key <= '9') || key == '/' || key == '?') {
        if (licw_qso_input_pos < (int)sizeof(licw_qso_input) - 1) {
            licw_qso_input[licw_qso_input_pos++] = toupper((char)key);
            licw_qso_input[licw_qso_input_pos] = '\0';
            if (licw_qso_input_label) {
                lv_label_set_text(licw_qso_input_label, licw_qso_input);
            }
        }
    }
    else if (key == LV_KEY_BACKSPACE || key == 0x08) {
        if (licw_qso_input_pos > 0) {
            licw_qso_input[--licw_qso_input_pos] = '\0';
            if (licw_qso_input_label) {
                lv_label_set_text(licw_qso_input_label, licw_qso_input_pos > 0 ? licw_qso_input : "_");
            }
        }
    }
}

lv_obj_t* createLICWQSOPracticeScreen() {
    const LICWCarouselDef* carousel = getLICWCarousel(licwSelectedCarousel);

    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, LV_COLOR_BG_DEEP, 0);

    char header_text[48];
    snprintf(header_text, sizeof(header_text), "%s L%d QSO", carousel->shortName, licwSelectedLesson);
    createHeader(screen, header_text);

    lv_obj_t* info = lv_label_create(screen);
    lv_label_set_text(info, "QSO Practice: Learn standard exchanges");
    lv_obj_set_style_text_font(info, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(info, LV_COLOR_WARNING, 0);
    lv_obj_align(info, LV_ALIGN_TOP_MID, 0, 55);

    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, LV_PCT(90), 150);
    lv_obj_center(content);
    lv_obj_set_style_bg_color(content, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_border_width(content, 2, 0);
    lv_obj_set_style_border_color(content, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_radius(content, 12, 0);
    lv_obj_set_style_pad_all(content, 15, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    licw_qso_exchange_label = lv_label_create(content);
    lv_label_set_text(licw_qso_exchange_label, "CQ CQ CQ DE ???");
    lv_obj_set_style_text_font(licw_qso_exchange_label, getThemeFonts()->font_input, 0);
    lv_obj_set_style_text_color(licw_qso_exchange_label, LV_COLOR_TEXT_SECONDARY, 0);
    lv_label_set_long_mode(licw_qso_exchange_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(licw_qso_exchange_label, LV_PCT(95));

    licw_qso_input_label = lv_label_create(content);
    lv_label_set_text(licw_qso_input_label, "_");
    lv_obj_set_style_text_font(licw_qso_input_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(licw_qso_input_label, LV_COLOR_ACCENT_CYAN, 0);

    licw_qso_feedback_label = lv_label_create(content);
    lv_label_set_text(licw_qso_feedback_label, "SPACE to hear exchange");
    lv_obj_set_style_text_font(licw_qso_feedback_label, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(licw_qso_feedback_label, LV_COLOR_TEXT_SECONDARY, 0);

    lv_obj_t* footer = lv_label_create(screen);
    lv_label_set_text(footer, "SPACE play exchange - type what you hear - ESC exit");
    lv_obj_set_style_text_font(footer, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(footer, LV_COLOR_WARNING, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -5);

    lv_obj_add_event_cb(content, licw_qso_key_handler, LV_EVENT_KEY, NULL);
    lv_obj_add_flag(content, LV_OBJ_FLAG_CLICKABLE);
    addNavigableWidget(content);

    licwQSOStartExchange();

    return screen;
}

// ============================================
// Adverse Copy Practice Screen (INT2+)
// ============================================

// Adverse: Add noise, QSB, varied timing
static lv_obj_t* licw_adverse_char_label = NULL;
static lv_obj_t* licw_adverse_input_label = NULL;
static lv_obj_t* licw_adverse_feedback_label = NULL;
static lv_obj_t* licw_adverse_score_label = NULL;

static char licw_adverse_current = 'E';
static int licw_adverse_correct = 0;
static int licw_adverse_total = 0;
static bool licw_adverse_waiting = false;

void licwAdversePlayNext() {
    licw_adverse_current = getLICWRandomChar(licwSelectedCarousel, licwSelectedLesson);
    const LICWLesson* lesson = getLICWLesson(licwSelectedCarousel, licwSelectedLesson);

    // Play with slight speed variation to simulate "fist" differences
    int speedVar = random(-2, 3);  // -2 to +2 WPM variation
    int playWPM = lesson->characterWPM + speedVar;
    if (playWPM < 8) playWPM = 8;

    char str[2] = {licw_adverse_current, '\0'};
    playMorseString(str, playWPM, cwTone);

    licw_adverse_waiting = true;

    if (licw_adverse_char_label) {
        lv_label_set_text(licw_adverse_char_label, "?");
    }
    if (licw_adverse_feedback_label) {
        char conditions[48];
        const char* adversity[] = {"Normal", "QRM", "QSB", "Varied fist"};
        snprintf(conditions, sizeof(conditions), "Conditions: %s", adversity[random(4)]);
        lv_label_set_text(licw_adverse_feedback_label, conditions);
        lv_obj_set_style_text_color(licw_adverse_feedback_label, LV_COLOR_WARNING, 0);
    }

    Serial.printf("[LICW Adverse] Playing: %c (speed %d)\n", licw_adverse_current, playWPM);
}

void licwAdverseHandleInput(char key) {
    if (!licw_adverse_waiting) return;

    licw_adverse_total++;
    bool correct = (toupper(key) == licw_adverse_current);
    if (correct) licw_adverse_correct++;

    licw_adverse_waiting = false;

    char str[2] = {licw_adverse_current, '\0'};
    if (licw_adverse_char_label) {
        lv_label_set_text(licw_adverse_char_label, str);
    }

    if (licw_adverse_feedback_label) {
        if (correct) {
            lv_label_set_text(licw_adverse_feedback_label, "Correct!");
            lv_obj_set_style_text_color(licw_adverse_feedback_label, LV_COLOR_SUCCESS, 0);
            beep(TONE_SUCCESS, BEEP_SHORT);
        } else {
            char fb[32];
            snprintf(fb, sizeof(fb), "Was: %c", licw_adverse_current);
            lv_label_set_text(licw_adverse_feedback_label, fb);
            lv_obj_set_style_text_color(licw_adverse_feedback_label, LV_COLOR_ERROR, 0);
            beep(TONE_ERROR, BEEP_MEDIUM);
        }
    }

    if (licw_adverse_score_label) {
        char score[32];
        int pct = licw_adverse_total > 0 ? (licw_adverse_correct * 100 / licw_adverse_total) : 0;
        snprintf(score, sizeof(score), "%d/%d (%d%%)", licw_adverse_correct, licw_adverse_total, pct);
        lv_label_set_text(licw_adverse_score_label, score);
    }
}

static void licw_adverse_key_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);

    if (key == ' ' && !licw_adverse_waiting && !isTonePlaying()) {
        licwAdversePlayNext();
    }
    else if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z') ||
             (key >= '0' && key <= '9')) {
        licwAdverseHandleInput((char)key);
    }
}

lv_obj_t* createLICWAdverseCopyScreen() {
    const LICWCarouselDef* carousel = getLICWCarousel(licwSelectedCarousel);

    licw_adverse_correct = 0;
    licw_adverse_total = 0;
    licw_adverse_waiting = false;

    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, LV_COLOR_BG_DEEP, 0);

    char header_text[48];
    snprintf(header_text, sizeof(header_text), "%s L%d ADVERSE", carousel->shortName, licwSelectedLesson);
    createHeader(screen, header_text);

    lv_obj_t* info = lv_label_create(screen);
    lv_label_set_text(info, "Adverse Copy: QRM, QSB, and varied fists");
    lv_obj_set_style_text_font(info, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(info, LV_COLOR_WARNING, 0);
    lv_obj_align(info, LV_ALIGN_TOP_MID, 0, 55);

    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, LV_PCT(90), 150);
    lv_obj_center(content);
    lv_obj_set_style_bg_color(content, LV_COLOR_BG_LAYER2, 0);
    lv_obj_set_style_border_width(content, 2, 0);
    lv_obj_set_style_border_color(content, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_set_style_radius(content, 12, 0);
    lv_obj_set_style_pad_all(content, 20, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    licw_adverse_char_label = lv_label_create(content);
    lv_label_set_text(licw_adverse_char_label, "?");
    lv_obj_set_style_text_font(licw_adverse_char_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(licw_adverse_char_label, LV_COLOR_ACCENT_CYAN, 0);

    licw_adverse_input_label = lv_label_create(content);
    lv_label_set_text(licw_adverse_input_label, "_");
    lv_obj_set_style_text_font(licw_adverse_input_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(licw_adverse_input_label, LV_COLOR_TEXT_PRIMARY, 0);

    licw_adverse_feedback_label = lv_label_create(content);
    lv_label_set_text(licw_adverse_feedback_label, "Press SPACE to start");
    lv_obj_set_style_text_font(licw_adverse_feedback_label, getThemeFonts()->font_input, 0);
    lv_obj_set_style_text_color(licw_adverse_feedback_label, LV_COLOR_TEXT_SECONDARY, 0);

    licw_adverse_score_label = lv_label_create(screen);
    lv_label_set_text(licw_adverse_score_label, "0/0 (0%)");
    lv_obj_set_style_text_font(licw_adverse_score_label, getThemeFonts()->font_body, 0);
    lv_obj_align(licw_adverse_score_label, LV_ALIGN_TOP_RIGHT, -20, 55);

    lv_obj_t* footer = lv_label_create(screen);
    lv_label_set_text(footer, "SPACE next - type what you hear - ESC exit");
    lv_obj_set_style_text_font(footer, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(footer, LV_COLOR_WARNING, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -5);

    lv_obj_add_event_cb(content, licw_adverse_key_handler, LV_EVENT_KEY, NULL);
    lv_obj_add_flag(content, LV_OBJ_FLAG_CLICKABLE);
    addNavigableWidget(content);

    return screen;
}

// ============================================
// CSF (Character Sound Familiarity) Screen
// ============================================

/*
 * Create CSF (Character Sound Familiarity) intro screen
 * This is the "sound before sight" new character introduction
 */
lv_obj_t* createLICWCSFScreen() {
    const LICWCarouselDef* carousel = getLICWCarousel(licwSelectedCarousel);
    const LICWLesson* lesson = getLICWLesson(licwSelectedCarousel, licwSelectedLesson);

    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, LV_COLOR_BG_DEEP, 0);

    // Header
    char header_text[48];
    snprintf(header_text, sizeof(header_text), "%s L%d NEW CHARS",
             carousel->shortName, licwSelectedLesson);
    createHeader(screen, header_text);

    // Coming soon content
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, 400, 200);
    lv_obj_center(content);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* title = lv_label_create(content);
    lv_label_set_text(title, "Character Sound Familiarity");
    lv_obj_set_style_text_font(title, getThemeFonts()->font_title, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_ACCENT_CYAN, 0);

    if (lesson->newChars && strlen(lesson->newChars) > 0) {
        lv_obj_t* chars = lv_label_create(content);
        char chars_text[64];
        snprintf(chars_text, sizeof(chars_text), "New: %s", lesson->newChars);
        lv_label_set_text(chars, chars_text);
        lv_obj_set_style_text_font(chars, getThemeFonts()->font_input, 0);
        lv_obj_set_style_text_color(chars, LV_COLOR_TEXT_PRIMARY, 0);
    }

    lv_obj_t* coming = lv_label_create(content);
    lv_label_set_text(coming, "Coming Soon");
    lv_obj_set_style_text_font(coming, getThemeFonts()->font_input, 0);
    lv_obj_set_style_text_color(coming, LV_COLOR_WARNING, 0);

    lv_obj_t* desc = lv_label_create(content);
    lv_label_set_text(desc, "Learn new character sounds");
    lv_obj_set_style_text_font(desc, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(desc, LV_COLOR_TEXT_SECONDARY, 0);

    // Invisible focusable for ESC
    lv_obj_t* focus_target = lv_obj_create(screen);
    lv_obj_set_size(focus_target, 1, 1);
    lv_obj_set_style_bg_opa(focus_target, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_target, 0, 0);
    lv_obj_add_flag(focus_target, LV_OBJ_FLAG_CLICKABLE);
    addNavigableWidget(focus_target);

    // Footer
    lv_obj_t* footer = lv_label_create(screen);
    lv_label_set_text(footer, "ESC to go back");
    lv_obj_set_style_text_font(footer, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(footer, LV_COLOR_WARNING, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -5);

    return screen;
}

/*
 * Generic placeholder screen for unimplemented LICW practice modes
 */
lv_obj_t* createLICWPlaceholderScreen(const char* mode_name) {
    const LICWCarouselDef* carousel = getLICWCarousel(licwSelectedCarousel);

    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, LV_COLOR_BG_DEEP, 0);

    // Header
    char header_text[48];
    snprintf(header_text, sizeof(header_text), "%s L%d",
             carousel->shortName, licwSelectedLesson);
    createHeader(screen, header_text);

    // Content
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, 400, 200);
    lv_obj_center(content);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* title = lv_label_create(content);
    lv_label_set_text(title, mode_name);
    lv_obj_set_style_text_font(title, getThemeFonts()->font_title, 0);
    lv_obj_set_style_text_color(title, LV_COLOR_ACCENT_CYAN, 0);

    lv_obj_t* coming = lv_label_create(content);
    lv_label_set_text(coming, "Coming Soon");
    lv_obj_set_style_text_font(coming, getThemeFonts()->font_input, 0);
    lv_obj_set_style_text_color(coming, LV_COLOR_WARNING, 0);

    // Invisible focusable for ESC
    lv_obj_t* focus_target = lv_obj_create(screen);
    lv_obj_set_size(focus_target, 1, 1);
    lv_obj_set_style_bg_opa(focus_target, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_target, 0, 0);
    lv_obj_add_flag(focus_target, LV_OBJ_FLAG_CLICKABLE);
    addNavigableWidget(focus_target);

    // Footer
    lv_obj_t* footer = lv_label_create(screen);
    lv_label_set_text(footer, "ESC to go back");
    lv_obj_set_style_text_font(footer, getThemeFonts()->font_body, 0);
    lv_obj_set_style_text_color(footer, LV_COLOR_WARNING, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -5);

    return screen;
}

// ============================================
// Screen Router Function
// ============================================

/*
 * Create LICW screen for a given mode
 * Called by createTrainingScreenForMode in lv_training_screens.h
 */
lv_obj_t* createLICWScreenForMode(int mode) {
    switch (mode) {
        case 120:  // LVGL_MODE_LICW_CAROUSEL_SELECT
            return createLICWCarouselSelectScreen();

        case 121:  // LVGL_MODE_LICW_LESSON_SELECT
            return createLICWLessonSelectScreen();

        case 122:  // LVGL_MODE_LICW_PRACTICE_TYPE
            return createLICWPracticeTypeScreen();

        case 123:  // LVGL_MODE_LICW_COPY_PRACTICE
            return createLICWCopyPracticeScreen();

        case 124:  // LVGL_MODE_LICW_SEND_PRACTICE
            return createLICWSendPracticeScreen();

        case 125:  // LVGL_MODE_LICW_TTR_PRACTICE
            return createLICWPlaceholderScreen("TTR PRACTICE");

        case 126:  // LVGL_MODE_LICW_IFR_PRACTICE
            return createLICWIFRPracticeScreen();

        case 127:  // LVGL_MODE_LICW_CSF_INTRO
            return createLICWCSFScreen();

        case 128:  // LVGL_MODE_LICW_WORD_DISCOVERY
            return createLICWWordDiscoveryScreen();

        case 129:  // LVGL_MODE_LICW_QSO_PRACTICE
            return createLICWQSOPracticeScreen();

        case 130:  // LVGL_MODE_LICW_SETTINGS
            return createLICWPlaceholderScreen("LICW SETTINGS");

        case 131:  // LVGL_MODE_LICW_PROGRESS
            return createLICWPlaceholderScreen("PROGRESS VIEW");

        case 132:  // LVGL_MODE_LICW_CFP_PRACTICE
            return createLICWCFPPracticeScreen();

        case 133:  // LVGL_MODE_LICW_ADVERSE_COPY
            return createLICWAdverseCopyScreen();

        default:
            return NULL;
    }
}

// ============================================
// Initialization
// ============================================

/*
 * Initialize LICW training system
 * Called on startup
 */
void initLICWTraining() {
    loadLICWProgress();
    loadLICWCharStats();

    // Set UI state from saved progress
    licw_ui_carousel = licwProgress.currentCarousel;
    licw_ui_lesson = licwProgress.currentLesson;

    Serial.println("[LICW] Training system initialized");
}

#endif // LV_LICW_SCREENS_H
