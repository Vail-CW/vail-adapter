/*
 * Boot Splash Screen
 * Displays mountain logo with "VAIL SUMMIT" text and progress bar
 * Shown immediately after display initialization for fast visual feedback
 */

#ifndef SPLASH_SCREEN_H
#define SPLASH_SCREEN_H

#include "config.h"
#include "hardware_init.h"
#include "mountain_bitmap.h"  // Logo bitmap data

// Splash screen layout constants
#define SPLASH_MOUNTAIN_HEIGHT  160
#define SPLASH_MOUNTAIN_WIDTH   220
#define SPLASH_MOUNTAIN_Y       25   // Top of mountain
#define SPLASH_TITLE_Y          210  // "VAIL SUMMIT" text position (adjusted for taller logo)
#define SPLASH_PROGRESS_Y       265  // Progress bar Y position (adjusted for taller logo)
#define SPLASH_PROGRESS_WIDTH   300
#define SPLASH_PROGRESS_HEIGHT  14

// Progress bar state
static int splashProgressPercent = 0;
static int16_t progressBarX = 0;
static int16_t progressBarInnerWidth = 0;

/*
 * Draw the mountain logo using 1-bit bitmap
 * Converts monochrome bitmap to RGB565 on-the-fly and renders
 * White pixels in bitmap = logo visible (white), Black pixels = transparent (skip drawing)
 */
void drawMountain(LGFX& display) {
    int16_t centerX = SCREEN_WIDTH / 2;
    int16_t logoX = centerX - (MOUNTAIN_LOGO_WIDTH / 2);
    int16_t logoY = SPLASH_MOUNTAIN_Y;

    // Color for logo outline (white to match theme)
    uint16_t logoColor = COLOR_TEXT_PRIMARY;  // White

    // Draw logo pixel-by-pixel, skipping background pixels for transparency
    int byteWidth = (MOUNTAIN_LOGO_WIDTH + 7) / 8;  // Bytes per row

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
            // Only draw the logo pixels, skip background pixels for transparency
            if (!(byte & (1 << bitIndex))) {
                display.drawPixel(logoX + x, logoY + y, logoColor);
            }
            // Background pixels: don't draw anything (transparent)
        }
    }
}

/*
 * Draw the "VAIL SUMMIT" title text using a nice font
 */
void drawSplashTitle(LGFX& display) {
    // Use FreeSansBold font for a cleaner, modern look
    display.setFont(&FreeSansBold18pt7b);
    display.setTextSize(1);
    display.setTextColor(COLOR_TITLE);

    const char* title = "VAIL SUMMIT";
    int16_t textWidth = display.textWidth(title);
    int16_t textX = (SCREEN_WIDTH - textWidth) / 2;

    display.setCursor(textX, SPLASH_TITLE_Y);
    display.print(title);

    // Reset to default font for other UI elements
    display.setFont(nullptr);
}

/*
 * Draw the progress bar outline (empty)
 */
void drawProgressBarOutline(LGFX& display) {
    progressBarX = (SCREEN_WIDTH - SPLASH_PROGRESS_WIDTH) / 2;
    progressBarInnerWidth = SPLASH_PROGRESS_WIDTH - 4;  // Account for border

    // Draw outer border (white)
    display.drawRect(
        progressBarX,
        SPLASH_PROGRESS_Y,
        SPLASH_PROGRESS_WIDTH,
        SPLASH_PROGRESS_HEIGHT,
        COLOR_TEXT
    );

    // Inner black fill (will be filled with cyan as progress increases)
    display.fillRect(
        progressBarX + 2,
        SPLASH_PROGRESS_Y + 2,
        progressBarInnerWidth,
        SPLASH_PROGRESS_HEIGHT - 4,
        COLOR_BACKGROUND
    );
}

/*
 * Update the progress bar to show current progress
 * percent: 0-100
 */
void updateSplashProgress(LGFX& display, int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    splashProgressPercent = percent;

    // Calculate fill width
    int16_t fillWidth = (progressBarInnerWidth * percent) / 100;

    // Fill the progress portion with cyan
    if (fillWidth > 0) {
        display.fillRect(
            progressBarX + 2,
            SPLASH_PROGRESS_Y + 2,
            fillWidth,
            SPLASH_PROGRESS_HEIGHT - 4,
            COLOR_TITLE  // Cyan
        );
    }
}

/*
 * Draw the complete boot splash screen
 * Call this immediately after initDisplay()
 */
void drawBootSplashScreen(LGFX& display) {
    // Clear screen to black
    display.fillScreen(COLOR_BACKGROUND);

    // Draw the mountain graphic
    drawMountain(display);

    // Draw the title
    drawSplashTitle(display);

    // Draw empty progress bar
    drawProgressBarOutline(display);

    // Set initial progress
    updateSplashProgress(display, 10);
}

#endif // SPLASH_SCREEN_H
