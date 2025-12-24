/*
 * VAIL SUMMIT - LVGL Initialization
 * Display and input driver integration with LovyanGFX
 */

#ifndef LV_INIT_H
#define LV_INIT_H

#include <lvgl.h>
#include <LovyanGFX.hpp>
#include "../core/config.h"

// ============================================
// Display Buffer Configuration
// ============================================

// Buffer size: Width x 40 lines for double buffering
// ~38KB per buffer, 76KB total for double buffering
#define LV_BUF_LINES 40
#define LV_BUF_SIZE (SCREEN_WIDTH * LV_BUF_LINES)

// Display buffers (allocated in PSRAM if available)
static lv_color_t* lvgl_buf1 = NULL;
static lv_color_t* lvgl_buf2 = NULL;

// LVGL display and input driver structures
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;
static lv_indev_t* indev_keypad = NULL;
static lv_group_t* input_group = NULL;

// Reference to LovyanGFX display (set during init)
static LGFX* lvgl_tft = NULL;

// ============================================
// Key Acceleration System
// ============================================
// Provides accelerating value changes when arrow keys are held
// Thresholds and multipliers are conservative for precise control

#define ACCEL_DELAY_THRESHOLD_1 500   // ms before medium acceleration (2x)
#define ACCEL_DELAY_THRESHOLD_2 1500  // ms before fast acceleration (4x)
#define ACCEL_STEP_NORMAL 1
#define ACCEL_STEP_MEDIUM 2
#define ACCEL_STEP_FAST 4

static uint32_t key_hold_start_time = 0;
static uint32_t last_accel_key = 0;
static int accel_level = 0;

// Get the current acceleration step multiplier
// Call this from slider/value adjustment handlers
int getKeyAccelerationStep() {
    switch (accel_level) {
        case 2: return ACCEL_STEP_FAST;
        case 1: return ACCEL_STEP_MEDIUM;
        default: return ACCEL_STEP_NORMAL;
    }
}

// Reset acceleration state (call when key is released)
void resetKeyAcceleration() {
    last_accel_key = 0;
    accel_level = 0;
    key_hold_start_time = 0;
}

// Update acceleration state based on key hold duration
void updateKeyAcceleration(uint32_t key, uint32_t now) {
    // Only track arrow keys for acceleration
    if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT ||
        key == LV_KEY_UP || key == LV_KEY_DOWN) {
        if (key != last_accel_key) {
            // New key - reset acceleration
            key_hold_start_time = now;
            last_accel_key = key;
            accel_level = 0;
        } else {
            // Same key held - calculate acceleration level based on duration
            uint32_t hold_duration = now - key_hold_start_time;
            if (hold_duration > ACCEL_DELAY_THRESHOLD_2) {
                accel_level = 2;  // Fast
            } else if (hold_duration > ACCEL_DELAY_THRESHOLD_1) {
                accel_level = 1;  // Medium
            } else {
                accel_level = 0;  // Normal
            }
        }
    }
}

// ============================================
// Display Flush Callback
// ============================================

/*
 * Flush display buffer to screen via LovyanGFX
 * Called by LVGL when a portion of the screen needs updating
 *
 * Uses swap565 = true to handle byte swapping for SPI displays.
 * This works with LV_COLOR_16_SWAP = 0 in lv_conf.h.
 */
void lvgl_disp_flush(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
    if (lvgl_tft == NULL) {
        lv_disp_flush_ready(drv);
        return;
    }

    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    // Push pixels with byte swap enabled for correct color display
    // The 'true' parameter tells LovyanGFX to swap bytes for SPI displays
    lvgl_tft->startWrite();
    lvgl_tft->setAddrWindow(area->x1, area->y1, w, h);
    lvgl_tft->pushPixels((uint16_t*)color_p, w * h, true);  // swap565 = true
    lvgl_tft->endWrite();

    lv_disp_flush_ready(drv);
}

// ============================================
// CardKB Input Driver
// ============================================

/*
 * Read CardKB keyboard via I2C
 * Returns the key code or 0 if no key pressed
 */
char readCardKBForLVGL() {
    Wire.requestFrom(CARDKB_ADDR, 1);
    if (Wire.available()) {
        char c = Wire.read();
        if (c != 0) {
            return c;
        }
    }
    return 0;
}

/*
 * Map CardKB key codes to LVGL key codes
 * NOTE: Using LV_KEY_UP/DOWN for arrow keys
 * Menu screens handle these with custom navigation logic for 2D grid movement
 * Screens needing linear list navigation must handle UP/DOWN in their key handlers
 */
uint32_t mapCardKBtoLVGL(char key) {
    switch (key) {
        case KEY_UP:      return LV_KEY_UP;      // Up arrow
        case KEY_DOWN:    return LV_KEY_DOWN;    // Down arrow
        case KEY_LEFT:    return LV_KEY_LEFT;    // Adjust value left
        case KEY_RIGHT:   return LV_KEY_RIGHT;   // Adjust value right
        case KEY_ENTER:   return LV_KEY_ENTER;   // Select/activate
        case KEY_ENTER_ALT: return LV_KEY_ENTER; // Alternate enter
        case KEY_ESC:     return LV_KEY_ESC;     // Back/cancel
        case KEY_BACKSPACE: return LV_KEY_BACKSPACE; // Delete
        // Tab key passes through as raw '\t' for mode-specific handling (e.g., stats toggle)
        case KEY_TAB:     return '\t';
        default:
            // For printable characters, return as-is
            if (key >= 32 && key <= 126) {
                return key;
            }
            return 0;
    }
}

/*
 * LVGL input device read callback for keypad
 * Includes acceleration tracking for arrow keys
 */
void lvgl_keypad_read(lv_indev_drv_t* drv, lv_indev_data_t* data) {
    static uint32_t prev_key = 0;
    static bool key_pressed = false;

    char raw_key = readCardKBForLVGL();
    uint32_t now = millis();

    if (raw_key != 0) {
        // New key pressed
        uint32_t lvgl_key = mapCardKBtoLVGL(raw_key);

        // Debug output
        Serial.printf("[LVGL Input] Raw: 0x%02X -> LVGL: %lu, Group has %d objects\n",
                      (uint8_t)raw_key, lvgl_key,
                      input_group ? lv_group_get_obj_count(input_group) : -1);

        if (lvgl_key != 0) {
            data->key = lvgl_key;
            data->state = LV_INDEV_STATE_PR;

            // Update key acceleration tracking for arrow keys
            updateKeyAcceleration(lvgl_key, now);

            prev_key = lvgl_key;
            key_pressed = true;
        }
    } else if (key_pressed) {
        // Key was pressed, now released
        data->key = prev_key;
        data->state = LV_INDEV_STATE_REL;
        key_pressed = false;

        // Reset acceleration on key release
        resetKeyAcceleration();
    } else {
        // No key activity
        data->state = LV_INDEV_STATE_REL;
    }
}

// ============================================
// Initialization Functions
// ============================================

/*
 * Allocate display buffers
 * Tries PSRAM first, falls back to regular RAM
 */
bool allocateDisplayBuffers() {
    // Try to allocate in PSRAM for better performance
    if (psramFound()) {
        Serial.println("[LVGL] Allocating display buffers in PSRAM");
        lvgl_buf1 = (lv_color_t*)ps_malloc(LV_BUF_SIZE * sizeof(lv_color_t));
        lvgl_buf2 = (lv_color_t*)ps_malloc(LV_BUF_SIZE * sizeof(lv_color_t));
    } else {
        Serial.println("[LVGL] Allocating display buffers in regular RAM");
        lvgl_buf1 = (lv_color_t*)malloc(LV_BUF_SIZE * sizeof(lv_color_t));
        lvgl_buf2 = (lv_color_t*)malloc(LV_BUF_SIZE * sizeof(lv_color_t));
    }

    if (lvgl_buf1 == NULL || lvgl_buf2 == NULL) {
        Serial.println("[LVGL] ERROR: Failed to allocate display buffers!");
        if (lvgl_buf1) free(lvgl_buf1);
        if (lvgl_buf2) free(lvgl_buf2);
        return false;
    }

    Serial.printf("[LVGL] Display buffers allocated: %d bytes each\n",
                  LV_BUF_SIZE * sizeof(lv_color_t));
    return true;
}

/*
 * Initialize LVGL display driver
 */
void initLVGLDisplay(LGFX& tft) {
    lvgl_tft = &tft;

    // Initialize draw buffer with double buffering
    lv_disp_draw_buf_init(&draw_buf, lvgl_buf1, lvgl_buf2, LV_BUF_SIZE);

    // Initialize display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = lvgl_disp_flush;
    disp_drv.draw_buf = &draw_buf;

    // Register display driver
    lv_disp_drv_register(&disp_drv);

    Serial.printf("[LVGL] Display driver registered: %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
}

/*
 * Initialize LVGL input driver for CardKB
 */
void initLVGLInput() {
    // Create input group for keyboard navigation
    input_group = lv_group_create();
    lv_group_set_default(input_group);

    // Enable wrap-around navigation (arrow keys wrap at list ends)
    lv_group_set_wrap(input_group, true);

    // Initialize keypad input driver
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = lvgl_keypad_read;

    // Register input driver
    indev_keypad = lv_indev_drv_register(&indev_drv);

    // Associate input device with the group
    lv_indev_set_group(indev_keypad, input_group);

    Serial.println("[LVGL] Input driver registered (CardKB keypad)");
}

/*
 * Initialize LVGL library and drivers
 * Call this after display and I2C are initialized
 */
bool initLVGL(LGFX& tft) {
    Serial.println("[LVGL] Initializing...");

    // Initialize LVGL library
    lv_init();
    Serial.println("[LVGL] Library initialized");

    // Allocate display buffers
    if (!allocateDisplayBuffers()) {
        return false;
    }

    // Initialize display driver
    initLVGLDisplay(tft);

    // Initialize input driver
    initLVGLInput();

    Serial.println("[LVGL] Initialization complete!");
    return true;
}

// ============================================
// Helper Functions
// ============================================

/*
 * Get the default input group
 * Add widgets to this group for keyboard navigation
 */
lv_group_t* getLVGLInputGroup() {
    return input_group;
}

/*
 * Get the keypad input device
 */
lv_indev_t* getLVGLKeypad() {
    return indev_keypad;
}

/*
 * Check if LVGL is initialized
 */
bool isLVGLInitialized() {
    return (lvgl_tft != NULL && lvgl_buf1 != NULL);
}

#endif // LV_INIT_H
