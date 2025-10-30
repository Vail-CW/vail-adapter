/*
 * Mirrored ST7789 Display Class
 *
 * Extends Adafruit_ST7789 to automatically maintain a full-resolution
 * framebuffer in PSRAM for screen mirroring purposes.
 *
 * Every draw operation writes to both the physical display AND the
 * PSRAM framebuffer, enabling instant readback for web streaming.
 */

#ifndef MIRROR_DISPLAY_H
#define MIRROR_DISPLAY_H

#include <Adafruit_ST7789.h>

// External references from screen_mirror.h
extern bool mirrorEnabled;
extern bool mirrorDirty;
extern uint16_t* fullFramebuffer;
extern int framebufferWidth;   // Runtime-determined dimensions
extern int framebufferHeight;

/*
 * MirroredST7789 Class
 *
 * This class overrides key Adafruit_GFX drawing primitives to maintain
 * a shadow framebuffer in PSRAM. All high-level drawing functions
 * (circles, rectangles, text, etc.) eventually call these primitives,
 * so overriding them captures everything.
 */
class MirroredST7789 : public Adafruit_ST7789 {
public:
  // Constructor - pass through to parent
  MirroredST7789(int8_t cs, int8_t dc, int8_t rst)
    : Adafruit_ST7789(cs, dc, rst) {}

  MirroredST7789(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst)
    : Adafruit_ST7789(cs, dc, mosi, sclk, rst) {}

  /*
   * Override: drawPixel
   *
   * This is the most fundamental drawing operation. All other GFX
   * functions eventually call this.
   */
  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    // Draw to physical display
    Adafruit_ST7789::drawPixel(x, y, color);

    // Update framebuffer if mirroring enabled
    if (mirrorEnabled && fullFramebuffer != nullptr) {
      // Scale coordinates based on framebuffer resolution
      int fbX = (x * framebufferWidth) / 320;
      int fbY = (y * framebufferHeight) / 240;

      if (fbX >= 0 && fbX < framebufferWidth && fbY >= 0 && fbY < framebufferHeight) {
        fullFramebuffer[fbY * framebufferWidth + fbX] = color;

        // Draw thicker pixels to make text/thin lines more visible
        // This is necessary because 240/320 = 0.75 scaling causes aliasing
        if (fbX + 1 < framebufferWidth) {
          fullFramebuffer[fbY * framebufferWidth + fbX + 1] = color;
        }
        if (fbY + 1 < framebufferHeight) {
          fullFramebuffer[(fbY + 1) * framebufferWidth + fbX] = color;
        }

        mirrorDirty = true;
      }
    }
  }

  /*
   * Override: fillRect
   *
   * CRITICAL: fillRect() in Adafruit_SPITFT uses non-virtual internal functions
   * (writeFillRectPreclipped, writeColor, writePixels) that bypass our overrides.
   * We MUST override fillRect() itself to capture fill operations before delegating.
   */
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
    // Update framebuffer FIRST if mirroring enabled
    if (mirrorEnabled && fullFramebuffer != nullptr) {
      // Scale coordinates
      int fbX = (x * framebufferWidth) / 320;
      int fbY = (y * framebufferHeight) / 240;
      int fbW = (w * framebufferWidth) / 320;
      int fbH = (h * framebufferHeight) / 240;

      // Ensure minimum 1px if original had size
      if (fbW == 0 && w > 0) fbW = 1;
      if (fbH == 0 && h > 0) fbH = 1;

      // Clip to framebuffer bounds
      if (fbX < 0) { fbW += fbX; fbX = 0; }
      if (fbY < 0) { fbH += fbY; fbY = 0; }
      if (fbX + fbW > framebufferWidth) fbW = framebufferWidth - fbX;
      if (fbY + fbH > framebufferHeight) fbH = framebufferHeight - fbY;

      // Fill framebuffer
      if (fbW > 0 && fbH > 0) {
        for (int16_t j = 0; j < fbH; j++) {
          uint16_t* row = &fullFramebuffer[(fbY + j) * framebufferWidth + fbX];
          for (int16_t i = 0; i < fbW; i++) {
            row[i] = color;
          }
        }
        mirrorDirty = true;
      }
    }

    // Then draw to physical display
    Adafruit_ST7789::fillRect(x, y, w, h, color);
  }

  /*
   * Override: writeFillRect
   *
   * Optimized filled rectangle used internally by fillRect, fillScreen, etc.
   * This is called instead of drawPixel for bulk operations.
   */
  void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
    // Draw to physical display
    Adafruit_ST7789::writeFillRect(x, y, w, h, color);

    // Update framebuffer if mirroring enabled
    if (mirrorEnabled && fullFramebuffer != nullptr) {
      // Scale coordinates
      int fbX = (x * framebufferWidth) / 320;
      int fbY = (y * framebufferHeight) / 240;
      int fbW = (w * framebufferWidth) / 320;
      int fbH = (h * framebufferHeight) / 240;

      if (fbW == 0 && w > 0) fbW = 1;
      if (fbH == 0 && h > 0) fbH = 1;

      if (fbX < 0) { fbW += fbX; fbX = 0; }
      if (fbY < 0) { fbH += fbY; fbY = 0; }
      if (fbX + fbW > framebufferWidth) fbW = framebufferWidth - fbX;
      if (fbY + fbH > framebufferHeight) fbH = framebufferHeight - fbY;

      if (fbW > 0 && fbH > 0) {
        for (int16_t j = 0; j < fbH; j++) {
          uint16_t* row = &fullFramebuffer[(fbY + j) * framebufferWidth + fbX];
          for (int16_t i = 0; i < fbW; i++) {
            row[i] = color;
          }
        }
        mirrorDirty = true;
      }
    }
  }

  /*
   * Override: drawFastHLine
   *
   * Optimized horizontal line drawing
   */
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override {
    // Draw to physical display
    Adafruit_ST7789::drawFastHLine(x, y, w, color);

    // Update framebuffer if mirroring enabled
    if (mirrorEnabled && fullFramebuffer != nullptr) {
      int fbX = (x * framebufferWidth) / 320;
      int fbY = (y * framebufferHeight) / 240;
      int fbW = (w * framebufferWidth) / 320;
      if (fbW == 0 && w > 0) fbW = 1;

      if (fbY >= 0 && fbY < framebufferHeight) {
        if (fbX < 0) { fbW += fbX; fbX = 0; }
        if (fbX + fbW > framebufferWidth) fbW = framebufferWidth - fbX;

        if (fbW > 0) {
          uint16_t* row = &fullFramebuffer[fbY * framebufferWidth + fbX];
          for (int16_t i = 0; i < fbW; i++) {
            row[i] = color;
          }
          mirrorDirty = true;
        }
      }
    }
  }

  /*
   * Override: drawFastVLine
   *
   * Optimized vertical line drawing
   */
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override {
    // Draw to physical display
    Adafruit_ST7789::drawFastVLine(x, y, h, color);

    // Update framebuffer if mirroring enabled
    if (mirrorEnabled && fullFramebuffer != nullptr) {
      int fbX = (x * framebufferWidth) / 320;
      int fbY = (y * framebufferHeight) / 240;
      int fbH = (h * framebufferHeight) / 240;
      if (fbH == 0 && h > 0) fbH = 1;

      if (fbX >= 0 && fbX < framebufferWidth) {
        if (fbY < 0) { fbH += fbY; fbY = 0; }
        if (fbY + fbH > framebufferHeight) fbH = framebufferHeight - fbY;

        if (fbH > 0) {
          for (int16_t j = 0; j < fbH; j++) {
            fullFramebuffer[(fbY + j) * framebufferWidth + fbX] = color;
          }
          mirrorDirty = true;
        }
      }
    }
  }

  /*
   * Override: fillScreen
   *
   * Special case for full screen clear - very common operation
   */
  void fillScreen(uint16_t color) override {
    // Draw to physical display
    Adafruit_ST7789::fillScreen(color);

    // Update framebuffer if mirroring enabled
    if (mirrorEnabled && fullFramebuffer != nullptr) {
      // Fast fill entire framebuffer
      for (int i = 0; i < framebufferWidth * framebufferHeight; i++) {
        fullFramebuffer[i] = color;
      }
      mirrorDirty = true;
    }
  }

  // Note: Circle and rounded rect functions aren't virtual in Adafruit_GFX
  // They already call our overridden drawPixel/drawFastHLine/drawFastVLine
  // So we don't need to override them - they'll automatically use our framebuffer
};

#endif // MIRROR_DISPLAY_H
