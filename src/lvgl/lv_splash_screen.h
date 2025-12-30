/*
 * VAIL SUMMIT - LVGL Splash Screen
 * Boot splash with mountain logo, title, and progress bar
 * Uses lv_canvas to render the 1-bit bitmap logo
 */

#ifndef LV_SPLASH_SCREEN_H
#define LV_SPLASH_SCREEN_H

#include <lvgl.h>
#include "lv_theme_summit.h"
#include "../core/config.h"
#include "../core/mountain_bitmap.h"

// ============================================
// Splash Screen Layout
// ============================================

#define SPLASH_LOGO_Y       30      // Top of mountain logo
#define SPLASH_TITLE_Y      200     // "VAIL SUMMIT" text position
#define SPLASH_BAR_Y        260     // Progress bar Y position
#define SPLASH_BAR_WIDTH    300
#define SPLASH_BAR_HEIGHT   16

// ============================================
// Splash Screen Objects
// ============================================

static lv_obj_t* splash_screen = NULL;
static lv_obj_t* splash_canvas = NULL;
static lv_obj_t* splash_title = NULL;
static lv_obj_t* splash_bar = NULL;
static lv_obj_t* splash_status = NULL;

// Canvas buffer for mountain logo (must be static/persistent)
static lv_color_t* canvas_buf = NULL;

// ============================================
// Mountain Logo Rendering
// ============================================

/*
 * Draw the mountain logo on an LVGL canvas
 * Converts 1-bit PROGMEM bitmap to LVGL canvas pixels
 */
void drawMountainOnCanvas(lv_obj_t* canvas) {
    if (canvas == NULL) return;

    // Logo color (white/cyan for visibility)
    lv_color_t logo_color = LV_COLOR_TEXT_PRIMARY;
    lv_color_t bg_color = LV_COLOR_BG_DEEP;

    int byteWidth = (MOUNTAIN_LOGO_WIDTH + 7) / 8;

    // Clear canvas with background color first
    lv_canvas_fill_bg(canvas, bg_color, LV_OPA_COVER);

    // Draw logo pixel-by-pixel
    for (int y = 0; y < MOUNTAIN_LOGO_HEIGHT; y++) {
        for (int x = 0; x < MOUNTAIN_LOGO_WIDTH; x++) {
            int byteIndex = y * byteWidth + (x / 8);
            int bitIndex = 7 - (x % 8);

            // Read byte from PROGMEM
            uint8_t byte = pgm_read_byte(&mountain_logoBitmap[byteIndex]);

            // Check if bit is NOT set (black pixel in original = logo outline)
            // Original PNG: white background, black outline
            // After inversion: white=1, black=0
            // We want outline (originally black, now 0) to show as white logo
            if (!(byte & (1 << bitIndex))) {
                lv_canvas_set_px_color(canvas, x, y, logo_color);
            }
        }
    }
}

// ============================================
// Splash Screen Creation
// ============================================

/*
 * Create the LVGL splash screen
 * Call this early in boot, after LVGL is initialized
 */
lv_obj_t* createSplashScreen() {
    // Create screen with dark background
    splash_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(splash_screen, LV_COLOR_BG_DEEP, 0);
    lv_obj_clear_flag(splash_screen, LV_OBJ_FLAG_SCROLLABLE);

    // Allocate canvas buffer (in PSRAM if available)
    size_t buf_size = LV_CANVAS_BUF_SIZE_TRUE_COLOR(MOUNTAIN_LOGO_WIDTH, MOUNTAIN_LOGO_HEIGHT);
    if (psramFound()) {
        canvas_buf = (lv_color_t*)ps_malloc(buf_size);
    } else {
        canvas_buf = (lv_color_t*)malloc(buf_size);
    }

    if (canvas_buf != NULL) {
        // Create canvas for mountain logo
        splash_canvas = lv_canvas_create(splash_screen);
        lv_canvas_set_buffer(splash_canvas, canvas_buf, MOUNTAIN_LOGO_WIDTH, MOUNTAIN_LOGO_HEIGHT, LV_IMG_CF_TRUE_COLOR);
        lv_obj_align(splash_canvas, LV_ALIGN_TOP_MID, 0, SPLASH_LOGO_Y);

        // Draw the mountain logo
        drawMountainOnCanvas(splash_canvas);
    } else {
        Serial.println("[Splash] WARNING: Failed to allocate canvas buffer for logo");
    }

    // Title: "VAIL SUMMIT"
    splash_title = lv_label_create(splash_screen);
    lv_label_set_text(splash_title, "VAIL SUMMIT");
    lv_obj_set_style_text_font(splash_title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(splash_title, LV_COLOR_ACCENT_CYAN, 0);
    lv_obj_align(splash_title, LV_ALIGN_TOP_MID, 0, SPLASH_TITLE_Y);

    // Subtitle
    lv_obj_t* subtitle = lv_label_create(splash_screen);
    lv_label_set_text(subtitle, "Morse Code Training Device");
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(subtitle, LV_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, SPLASH_TITLE_Y + 35);

    // Version info
    lv_obj_t* version = lv_label_create(splash_screen);
    lv_label_set_text_fmt(version, "v%s", FIRMWARE_VERSION);
    lv_obj_set_style_text_font(version, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(version, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_align(version, LV_ALIGN_TOP_MID, 0, SPLASH_TITLE_Y + 55);

    // Progress bar
    splash_bar = lv_bar_create(splash_screen);
    lv_obj_set_size(splash_bar, SPLASH_BAR_WIDTH, SPLASH_BAR_HEIGHT);
    lv_obj_align(splash_bar, LV_ALIGN_TOP_MID, 0, SPLASH_BAR_Y);
    lv_bar_set_range(splash_bar, 0, 100);
    lv_bar_set_value(splash_bar, 0, LV_ANIM_OFF);

    // Style the progress bar
    lv_obj_set_style_bg_color(splash_bar, LV_COLOR_BG_LAYER2, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(splash_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(splash_bar, 4, LV_PART_MAIN);
    lv_obj_set_style_border_color(splash_bar, LV_COLOR_BORDER_SUBTLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(splash_bar, 1, LV_PART_MAIN);

    lv_obj_set_style_bg_color(splash_bar, LV_COLOR_ACCENT_CYAN, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(splash_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(splash_bar, 4, LV_PART_INDICATOR);

    // Status text below progress bar
    splash_status = lv_label_create(splash_screen);
    lv_label_set_text(splash_status, "Initializing...");
    lv_obj_set_style_text_font(splash_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(splash_status, LV_COLOR_TEXT_TERTIARY, 0);
    lv_obj_align(splash_status, LV_ALIGN_TOP_MID, 0, SPLASH_BAR_Y + SPLASH_BAR_HEIGHT + 10);

    return splash_screen;
}

// ============================================
// Progress Updates
// ============================================

/*
 * Update splash screen progress
 * percent: 0-100
 * status: Optional status text (NULL to keep current)
 */
void updateSplashProgressLVGL(int percent, const char* status) {
    if (splash_bar == NULL) return;

    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    // Update progress bar with animation
    lv_bar_set_value(splash_bar, percent, LV_ANIM_ON);

    // Update status text if provided
    if (status != NULL && splash_status != NULL) {
        lv_label_set_text(splash_status, status);
    }

    // Force refresh to show update immediately
    lv_refr_now(NULL);
}

/*
 * Show the splash screen and set initial progress
 */
void showSplashScreen() {
    if (splash_screen == NULL) {
        createSplashScreen();
    }

    if (splash_screen != NULL) {
        lv_scr_load(splash_screen);
        updateSplashProgressLVGL(5, "Starting...");
    }
}

/*
 * Clean up splash screen resources
 * Call this after boot is complete and main menu is shown
 *
 * NOTE: We do NOT delete the splash screen here because loadScreen()
 * will handle the screen deletion via lv_scr_load_anim(..., true).
 * We only need to free the canvas buffer which is separately allocated.
 */
void cleanupSplashScreen() {
    // Clear object pointers (screen will be deleted by loadScreen)
    splash_screen = NULL;
    splash_canvas = NULL;
    splash_title = NULL;
    splash_bar = NULL;
    splash_status = NULL;

    // Free the canvas buffer (this is our own allocation, not LVGL's)
    if (canvas_buf != NULL) {
        free(canvas_buf);
        canvas_buf = NULL;
    }

    Serial.println("[Splash] Splash screen resources cleaned up");
}

// ============================================
// Boot Sequence Helpers
// ============================================

/*
 * Helper to show progress during boot sequence
 * Maps boot stages to percentages and status messages
 */
void setSplashStage(int stage) {
    switch (stage) {
        case 0:
            updateSplashProgressLVGL(5, "Starting...");
            break;
        case 1:
            updateSplashProgressLVGL(15, "Initializing I2C...");
            break;
        case 2:
            updateSplashProgressLVGL(25, "Starting audio...");
            break;
        case 3:
            updateSplashProgressLVGL(35, "Loading settings...");
            break;
        case 4:
            updateSplashProgressLVGL(50, "Configuring WiFi...");
            break;
        case 5:
            updateSplashProgressLVGL(65, "Starting web server...");
            break;
        case 6:
            updateSplashProgressLVGL(80, "Initializing UI...");
            break;
        case 7:
            updateSplashProgressLVGL(95, "Almost ready...");
            break;
        case 8:
            updateSplashProgressLVGL(100, "Ready!");
            break;
        default:
            break;
    }
}

#endif // LV_SPLASH_SCREEN_H
