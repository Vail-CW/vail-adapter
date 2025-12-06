/*
 * Boot Splash Screen
 * Displays mountain logo with "VAIL SUMMIT" text and progress bar
 * Shown immediately after display initialization for fast visual feedback
 */

#ifndef SPLASH_SCREEN_H
#define SPLASH_SCREEN_H

#include "config.h"
#include "hardware_init.h"

// Splash screen layout constants
#define SPLASH_MOUNTAIN_HEIGHT  140
#define SPLASH_MOUNTAIN_WIDTH   200
#define SPLASH_MOUNTAIN_Y       25   // Top of mountain
#define SPLASH_TITLE_Y          200  // "VAIL SUMMIT" text position
#define SPLASH_PROGRESS_Y       255  // Progress bar Y position
#define SPLASH_PROGRESS_WIDTH   300
#define SPLASH_PROGRESS_HEIGHT  14

// Mountain colors - shaded for realistic appearance (RGB565)
// Snow colors
#define MTN_SNOW_BRIGHT   0xFFFF  // Pure white - snow highlight
#define MTN_SNOW_LIGHT    0xF7BE  // Very light gray - lit snow
#define MTN_SNOW_MID      0xE71C  // Light gray - snow mid-tone
#define MTN_SNOW_SHADOW   0xCE59  // Medium gray - snow in shadow
#define MTN_SNOW_DARK     0xB5B6  // Darker gray - deep snow shadow

// Rock colors
#define MTN_ROCK_BRIGHT   0x8C71  // Light brown/gray - brightest rock
#define MTN_ROCK_LIGHT    0x7BCF  // Medium light - lit rock face
#define MTN_ROCK_MID      0x6B4D  // Medium gray - rock mid-tone
#define MTN_ROCK_DARK     0x52AA  // Dark gray - shadow rock face
#define MTN_ROCK_DEEP     0x4208  // Deep shadow - darkest rock

// Progress bar state
static int splashProgressPercent = 0;
static int16_t progressBarX = 0;
static int16_t progressBarInnerWidth = 0;

/*
 * Draw the mountain graphic - realistic shaded snow-capped peak
 * Draws as solid filled shapes to avoid gaps
 * Lighting from upper-left creates natural shadow on right face
 */
void drawMountain(LGFX& display) {
    int16_t centerX = SCREEN_WIDTH / 2;
    int16_t baseY = SPLASH_MOUNTAIN_Y + SPLASH_MOUNTAIN_HEIGHT;
    int16_t peakY = SPLASH_MOUNTAIN_Y;
    int16_t halfWidth = SPLASH_MOUNTAIN_WIDTH / 2;

    // Key coordinates
    int16_t leftBase = centerX - halfWidth;
    int16_t rightBase = centerX + halfWidth;
    int16_t peakX = centerX;

    // Snow line position (where snow meets rock)
    int16_t snowLineY = SPLASH_MOUNTAIN_Y + 45;

    // Ridge position (divides lit/shadow sides) - slightly right of peak for depth
    int16_t ridgeBottomX = centerX + 8;

    // === STEP 1: Draw the entire mountain base shape first (darkest color) ===
    // This ensures no gaps - we'll overlay lighter colors on top
    display.fillTriangle(
        leftBase, baseY,
        peakX, peakY,
        rightBase, baseY,
        MTN_ROCK_DEEP
    );

    // === STEP 2: Left rock face (lit side - lighter) ===
    // Calculate snow line intersection points
    int16_t leftSnowX = leftBase + (int16_t)((float)(peakX - leftBase) * (snowLineY - peakY) / (baseY - peakY));
    int16_t ridgeSnowX = ridgeBottomX;  // Ridge runs from peak down

    // Left face - from left base, up to peak, down ridge to base
    display.fillTriangle(
        leftBase, baseY,
        peakX, peakY,
        ridgeBottomX, baseY,
        MTN_ROCK_LIGHT
    );

    // Add gradient shading on left rock face
    // Darker strip near the ridge
    int16_t midLeftX = centerX - 20;
    display.fillTriangle(
        midLeftX, baseY,
        peakX, peakY + 30,
        ridgeBottomX, baseY,
        MTN_ROCK_MID
    );

    // === STEP 3: Right rock face (shadow side - stays dark from base fill) ===
    // Already filled with MTN_ROCK_DEEP, add slight variation
    int16_t rightSnowX = rightBase - (int16_t)((float)(rightBase - peakX) * (snowLineY - peakY) / (baseY - peakY));

    // Slight mid-tone strip on right face for depth
    display.fillTriangle(
        ridgeBottomX + 15, baseY,
        peakX + 10, peakY + 40,
        centerX + 40, baseY,
        MTN_ROCK_DARK
    );

    // === STEP 4: Snow cap - left side (bright, lit) ===
    display.fillTriangle(
        leftSnowX, snowLineY,
        peakX, peakY,
        ridgeBottomX, snowLineY,
        MTN_SNOW_BRIGHT
    );

    // Snow gradient - slightly less bright band
    display.fillTriangle(
        leftSnowX + 10, snowLineY,
        peakX, peakY + 15,
        peakX - 15, snowLineY,
        MTN_SNOW_LIGHT
    );

    // === STEP 5: Snow cap - right side (in shadow) ===
    display.fillTriangle(
        ridgeBottomX, snowLineY,
        peakX, peakY,
        rightSnowX, snowLineY,
        MTN_SNOW_SHADOW
    );

    // Darker shadow band on right snow
    display.fillTriangle(
        peakX + 5, snowLineY - 5,
        peakX, peakY + 10,
        rightSnowX - 10, snowLineY,
        MTN_SNOW_DARK
    );

    // === STEP 6: Peak highlight (very top - brightest) ===
    display.fillTriangle(
        peakX - 12, peakY + 18,
        peakX, peakY,
        peakX + 5, peakY + 15,
        MTN_SNOW_BRIGHT
    );

    // === STEP 7: Ridge line (subtle definition) ===
    display.drawLine(peakX, peakY, ridgeBottomX, baseY, MTN_ROCK_MID);

    // === STEP 8: Edge highlights for depth ===
    // Left edge highlight
    display.drawLine(leftBase, baseY, leftSnowX - 5, snowLineY + 5, MTN_ROCK_BRIGHT);

    // Right edge (darker)
    display.drawLine(rightBase, baseY, rightSnowX + 5, snowLineY + 5, MTN_ROCK_DEEP);
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
