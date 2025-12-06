/*
 * Mountain Bitmap for Boot Splash Screen
 * 150x150 pixel image in RGB565 format
 * Snow-capped mountain with shading for depth
 *
 * Image is stored in PROGMEM (flash) to save RAM
 * Total size: 150 * 150 * 2 = 45,000 bytes
 */

#ifndef MOUNTAIN_BITMAP_H
#define MOUNTAIN_BITMAP_H

#include <pgmspace.h>

#define MOUNTAIN_WIDTH  150
#define MOUNTAIN_HEIGHT 150

// RGB565 color definitions for the mountain
#define MTN_SNOW_BRIGHT   0xFFFF  // Pure white snow highlight
#define MTN_SNOW_MID      0xE71C  // Slightly shaded snow
#define MTN_SNOW_SHADOW   0xC618  // Snow in shadow
#define MTN_ROCK_LIGHT    0x8410  // Light gray rock (lit side)
#define MTN_ROCK_MID      0x6B4D  // Medium gray rock
#define MTN_ROCK_DARK     0x4A49  // Dark gray rock (shadow side)
#define MTN_ROCK_DEEP     0x3186  // Deep shadow
#define MTN_SKY           0x0000  // Transparent (black background)

// Mountain bitmap data - 150x150 RGB565
// Generated procedurally to create a realistic snow-capped mountain
// with lighting from upper-left, creating shadow on the right face

const uint16_t mountainBitmap[MOUNTAIN_HEIGHT * MOUNTAIN_WIDTH] PROGMEM = {
// Row 0-9: Sky (transparent/background)
// The mountain peak starts around row 10

// This is a procedurally-defined mountain shape
// Due to the size, we define it programmatically in the drawing function
// rather than storing 45KB of raw pixel data

// Placeholder - actual rendering done in splash_screen.h using drawMountain()
0x0000
};

// Instead of a massive bitmap array, we'll draw the mountain procedurally
// This saves significant flash space while still achieving a polished look

/*
 * Mountain shape definition using vertex points
 * The mountain is drawn as filled polygons with gradient shading
 */

// Main peak outline (from left base to peak to right base)
const int16_t mountainOutlineX[] = {0, 25, 45, 60, 75, 85, 95, 110, 125, 150};
const int16_t mountainOutlineY[] = {150, 120, 90, 50, 0, 50, 90, 110, 130, 150};
const int mountainOutlinePoints = 10;

// Snow line (defines where snow meets rock)
const int16_t snowLineX[] = {45, 60, 75, 85, 95};
const int16_t snowLineY[] = {60, 30, 0, 30, 55};
const int snowLinePoints = 5;

// Ridge line (divides lit side from shadow side)
const int16_t ridgeX[] = {75, 75};
const int16_t ridgeY[] = {0, 150};

#endif // MOUNTAIN_BITMAP_H
