/*
 * Screen Mirroring Module
 * Captures display output and streams to web interface
 *
 * Approach: Capture screen as RGB565 data and encode as JPEG
 * Target: 10+ FPS for remote monitoring
 */

#ifndef SCREEN_MIRROR_H
#define SCREEN_MIRROR_H

#include <Adafruit_ST7789.h>
#include <esp_task_wdt.h>

// Try to include ESP32 camera image converters (may not be available)
#if __has_include("img_converters.h")
  #include "img_converters.h"
  #define HAS_JPEG_ENCODER 1
#else
  #define HAS_JPEG_ENCODER 0
  #warning "img_converters.h not found - using BMP format (larger files)"
#endif

// Screen mirror configuration
#define MIRROR_WIDTH 320
#define MIRROR_HEIGHT 240
#define MIRROR_SCALE 1          // No downsampling - use framebuffer as-is
#define MIRROR_QUALITY 60       // JPEG quality (1-100, higher = better but slower. 60 = good balance)

// Framebuffer dimensions (adjusted based on available memory)
int framebufferWidth = MIRROR_WIDTH;
int framebufferHeight = MIRROR_HEIGHT;

// Screen mirror state
bool mirrorEnabled = false;
bool mirrorDirty = false;             // True when screen has changed and needs encoding
unsigned long lastCaptureTime = 0;
unsigned long captureInterval = 500;  // ms (2 FPS - prioritize quality over speed)

// Full-resolution framebuffer (size determined at runtime)
uint16_t* fullFramebuffer = nullptr;

// No separate mirror buffer needed - we convert directly from framebuffer to RGB888

// JPEG output buffer
uint8_t* jpegBuffer = nullptr;
size_t jpegBufferLen = 0;
size_t jpegBufferSize = 0;

// Forward declarations
void initScreenMirror();
void enableScreenMirror(bool enable, Adafruit_ST7789* tft = nullptr);
void setMirrorFPS(int fps);
bool captureScreen(Adafruit_ST7789& tft);
uint8_t* getJpegBuffer();
size_t getJpegBufferSize();
bool encodeToJpeg();
void rgb565ToRgb888(uint8_t* input, uint8_t* output, size_t pixelCount);

/*
 * Initialize screen mirroring system
 */
void initScreenMirror() {
  // Check PSRAM availability
  Serial.printf("PSRAM available: %d bytes\n", ESP.getPsramSize());
  Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());

  // Allocate framebuffer (try PSRAM first, then heap with reduced size)
  if (fullFramebuffer == nullptr) {
    // Use 75% resolution (240x180) - JPEG encoder more stable at this resolution
    // 320x240 causes JPEG encoder to hang on 2nd encode (known ESP32 issue)
    // 240x180 = 86,400 bytes
    framebufferWidth = (MIRROR_WIDTH * 3) / 4;    // 240
    framebufferHeight = (MIRROR_HEIGHT * 3) / 4;  // 180
    size_t bufferSize = framebufferWidth * framebufferHeight * 2;
    Serial.printf("Attempting to allocate %d bytes for framebuffer (%dx%d @ 75%%)...\n",
                  bufferSize, framebufferWidth, framebufferHeight);

    // Try PSRAM first
    fullFramebuffer = (uint16_t*)ps_malloc(bufferSize);
    if (fullFramebuffer != nullptr) {
      Serial.printf("Framebuffer allocated in PSRAM: %d bytes (%dx%d)\n",
                    bufferSize, framebufferWidth, framebufferHeight);
    } else {
      // PSRAM not available - try heap with reduced resolution as fallback
      Serial.println("PSRAM allocation failed, trying reduced size in heap...");
      // Use 50% resolution to conserve heap: 160x120 = 38,400 bytes
      framebufferWidth = MIRROR_WIDTH / 2;    // 160
      framebufferHeight = MIRROR_HEIGHT / 2;  // 120
      size_t reducedBufferSize = framebufferWidth * framebufferHeight * 2;

      fullFramebuffer = (uint16_t*)malloc(reducedBufferSize);
      if (fullFramebuffer == nullptr) {
        Serial.println("ERROR: Failed to allocate framebuffer!");
        Serial.println("Screen mirroring will not be available.");
        return;
      }
      Serial.printf("Reduced framebuffer allocated in HEAP: %d bytes (%dx%d)\n",
                    reducedBufferSize, framebufferWidth, framebufferHeight);
      Serial.println("NOTE: Using 50% resolution to conserve heap memory for WiFi");
    }
    // Clear framebuffer
    memset(fullFramebuffer, 0, framebufferWidth * framebufferHeight * 2);
  }

  // No separate mirror buffer needed - we convert directly from framebuffer

  // Allocate buffer for output image (BMP or JPEG)
  // BMP format: 54 byte header + RGB888 data
  // For 160x120: 54 + (160*120*3) = 57,654 bytes
  jpegBufferSize = framebufferWidth * framebufferHeight * 3 + 100;  // RGB888 + header
  if (jpegBuffer == nullptr) {
    jpegBuffer = (uint8_t*)malloc(jpegBufferSize);
    if (jpegBuffer == nullptr) {
      Serial.println("ERROR: Failed to allocate output buffer!");
      return;
    }
    Serial.printf("Screen mirror output buffer allocated: %zu bytes\n", jpegBufferSize);
  }

  mirrorEnabled = false;
  lastCaptureTime = 0;
  jpegBufferLen = 0;

  Serial.println("Screen mirroring initialized successfully!");
  Serial.printf("Total memory used: framebuffer=%zu bytes, output=%zu bytes\n",
                framebufferWidth * framebufferHeight * 2, jpegBufferSize);
  Serial.printf("Free heap after init: %d bytes\n", ESP.getFreeHeap());
}

/*
 * Enable or disable screen mirroring
 * If tft parameter is provided, force an immediate first capture
 */
void enableScreenMirror(bool enable, Adafruit_ST7789* tft) {
  mirrorEnabled = enable;
  if (enable) {
    Serial.println("Screen mirroring ENABLED");
    // Reset capture timer to force immediate first capture
    lastCaptureTime = 0;

    // Mark as dirty to trigger initial encode
    mirrorDirty = true;

    // Note: Framebuffer needs to be populated by redrawing the screen
    // Call drawMenu() or similar after enabling mirroring to populate buffer
    Serial.println("NOTE: Framebuffer will be black until next screen redraw");
    Serial.println("Navigate menus or trigger a screen update to see content");

    // Force an immediate capture if display is provided
    if (tft != nullptr) {
      Serial.println("Performing initial screen capture...");
      if (captureScreen(*tft)) {
        encodeToJpeg();
        Serial.println("Initial capture complete!");
      } else {
        Serial.println("Waiting for screen changes to capture...");
      }
    }
  } else {
    Serial.println("Screen mirroring DISABLED");
    mirrorDirty = false;
  }
}

/*
 * Set mirror frame rate
 */
void setMirrorFPS(int fps) {
  if (fps < 1) fps = 1;
  if (fps > 30) fps = 30;
  captureInterval = 1000 / fps;
  Serial.printf("Mirror FPS set to: %d (%lu ms interval)\n", fps, captureInterval);
}

/*
 * Capture current screen contents
 * Returns true if screen has changed and needs encoding
 *
 * Reads from PSRAM framebuffer and downsamples to mirror buffer.
 * Uses dirty flag system: only captures when screen has been modified.
 */
bool captureScreen(Adafruit_ST7789& tft) {
  if (!mirrorEnabled || fullFramebuffer == nullptr) {
    return false;
  }

  // Check if screen has changed (dirty flag set by MirroredST7789 draw operations)
  if (!mirrorDirty) {
    return false;  // No changes, don't encode
  }

  // Throttle updates to max FPS (prevent encoding too frequently)
  unsigned long currentTime = millis();
  if (currentTime - lastCaptureTime < captureInterval) {
    return false;  // Too soon, wait for next interval
  }
  lastCaptureTime = currentTime;

  // Clear dirty flag (will be set again by next draw operation)
  mirrorDirty = false;

  // Framebuffer is ready - no copying needed, we'll convert directly to RGB888 in encodeToJpeg()
  Serial.printf("Screen changed, ready to encode %dx%d framebuffer\n",
                framebufferWidth, framebufferHeight);

  // Signal that encoding should happen
  return true;
}

/*
 * Convert RGB565 framebuffer to RGB888 for JPEG encoding
 * input: RGB565 framebuffer (uint16_t array)
 * output: RGB888 buffer (must be pre-allocated, 3x the pixel count)
 */
void framebufferToRgb888(uint16_t* framebuffer, uint8_t* output, size_t pixelCount) {
  for (size_t i = 0; i < pixelCount; i++) {
    uint16_t rgb565 = framebuffer[i];

    // Extract RGB components
    uint8_t r = (rgb565 >> 11) & 0x1F;  // 5 bits
    uint8_t g = (rgb565 >> 5) & 0x3F;   // 6 bits
    uint8_t b = rgb565 & 0x1F;          // 5 bits

    // Scale to 8-bit
    output[i * 3 + 0] = (r << 3) | (r >> 2);  // R
    output[i * 3 + 1] = (g << 2) | (g >> 4);  // G
    output[i * 3 + 2] = (b << 3) | (b >> 2);  // B

    // Feed watchdog every 1000 pixels to prevent reset
    if (i % 1000 == 0) {
      yield();
    }
  }
}

/*
 * Encode RGB565 buffer to JPEG
 * Returns true on success
 */
bool encodeToJpeg() {
  if (fullFramebuffer == nullptr || jpegBuffer == nullptr) {
    Serial.println("Mirror: Encode failed - buffer is null");
    return false;
  }

  // Use actual framebuffer dimensions (160x120 or 320x240)
  int outputWidth = framebufferWidth;
  int outputHeight = framebufferHeight;
  size_t pixelCount = outputWidth * outputHeight;

  Serial.printf("Mirror: Starting encode (%dx%d, %zu pixels)...\n", outputWidth, outputHeight, pixelCount);
  Serial.printf("Free heap before conversion: %d bytes\n", ESP.getFreeHeap());

  // Use output buffer for RGB888 conversion (reuse memory instead of allocating new buffer)
  // BMP header is 54 bytes, so RGB888 data starts at offset 54
  size_t bmpHeaderSize = 54;
  size_t rgb888Size = pixelCount * 3;
  uint8_t* rgb888Buffer = jpegBuffer + bmpHeaderSize;  // Point into output buffer

  if (jpegBufferSize < bmpHeaderSize + rgb888Size) {
    Serial.printf("ERROR: Output buffer too small (%zu < %zu)!\n", jpegBufferSize, bmpHeaderSize + rgb888Size);
    return false;
  }

  Serial.println("Mirror: Converting framebuffer RGB565 to RGB888 (in-place)...");

  // Convert framebuffer directly to RGB888 in output buffer
  unsigned long convStart = millis();
  framebufferToRgb888(fullFramebuffer, rgb888Buffer, pixelCount);
  unsigned long convTime = millis() - convStart;
  Serial.printf("RGB565->RGB888 conversion took %lu ms\n", convTime);

#if HAS_JPEG_ENCODER
  Serial.println("Mirror: JPEG encoder detected, attempting encode...");

  // Convert RGB to BGR for JPEG encoder (it expects BGR format)
  for (size_t i = 0; i < pixelCount; i++) {
    uint8_t r = rgb888Buffer[i * 3 + 0];
    uint8_t b = rgb888Buffer[i * 3 + 2];
    rgb888Buffer[i * 3 + 0] = b;  // Swap R and B
    rgb888Buffer[i * 3 + 2] = r;

    // Aggressive yielding to prevent watchdog timeout
    if (i % 500 == 0) {
      yield();
      vTaskDelay(1);  // Give RTOS time to run other tasks
    }
  }

  // Force task switch before encoding
  vTaskDelay(10);  // 10ms delay to let other tasks run

  // fmt2jpg manages jpegBuffer internally - it will free/reallocate as needed
  // Note: This can take 1-2 seconds on second call - watchdog issue suspected
  unsigned long jpegStart = millis();

  // Try encoding - if it hangs, the issue is in fmt2jpg itself
  Serial.println("Calling fmt2jpg...");
  bool success = fmt2jpg(rgb888Buffer, pixelCount * 3,
                         outputWidth, outputHeight,
                         PIXFORMAT_RGB888, MIRROR_QUALITY,
                         &jpegBuffer, &jpegBufferLen);
  unsigned long jpegTime = millis() - jpegStart;
  Serial.printf("fmt2jpg returned after %lu ms\n", jpegTime);

  if (!success) {
    Serial.printf("ERROR: JPEG encoding failed - falling back to BMP\n");
    // Fall through to BMP encoding below
  } else {
    Serial.printf("JPEG encoded: %zu bytes (quality=%d) in %lu ms\n", jpegBufferLen, MIRROR_QUALITY, jpegTime);
    return true;
  }
#endif

  // BMP encoding (fallback or primary if JPEG not available)
  // RGB888 data is already in place at jpegBuffer + 54, just need to add header
  {
  Serial.println("Mirror: Encoding to BMP...");

  size_t bmpDataSize = pixelCount * 3;  // RGB888
  size_t totalSize = bmpHeaderSize + bmpDataSize;

  // Build BMP header at start of buffer (RGB888 data already at offset 54)
  uint8_t* bmp = jpegBuffer;
  memset(bmp, 0, bmpHeaderSize);

  // BMP File Header (14 bytes)
  bmp[0] = 'B'; bmp[1] = 'M';  // Signature
  *(uint32_t*)&bmp[2] = totalSize;  // File size
  *(uint32_t*)&bmp[10] = bmpHeaderSize;  // Pixel data offset

  // DIB Header (40 bytes - BITMAPINFOHEADER)
  *(uint32_t*)&bmp[14] = 40;  // DIB header size
  *(int32_t*)&bmp[18] = outputWidth;  // Width
  *(int32_t*)&bmp[22] = -outputHeight;  // Height (negative = top-down)
  *(uint16_t*)&bmp[26] = 1;  // Planes
  *(uint16_t*)&bmp[28] = 24;  // Bits per pixel (RGB888)
  *(uint32_t*)&bmp[30] = 0;  // Compression (0 = uncompressed)
  *(uint32_t*)&bmp[34] = bmpDataSize;  // Image size

  // Convert RGB to BGR in-place (BMP format requires BGR order)
  // rgb888Buffer already points to jpegBuffer + bmpHeaderSize
  for (size_t i = 0; i < pixelCount; i++) {
    uint8_t r = rgb888Buffer[i * 3 + 0];
    uint8_t b = rgb888Buffer[i * 3 + 2];

    // Swap R and B channels
    rgb888Buffer[i * 3 + 0] = b;
    rgb888Buffer[i * 3 + 2] = r;
    // G stays in place

    // Feed watchdog every 1000 pixels
    if (i % 1000 == 0) {
      yield();
    }
  }

  jpegBufferLen = totalSize;

  Serial.printf("Mirror: Created BMP image %zu bytes (%dx%d)\n", jpegBufferLen, outputWidth, outputHeight);
  Serial.printf("Free heap after encode: %d bytes\n", ESP.getFreeHeap());
  #if HAS_JPEG_ENCODER
    Serial.println("NOTE: JPEG encoding failed, using BMP format");
  #else
    Serial.println("NOTE: Using BMP format (JPEG encoder not available)");
  #endif
  return true;
  }
}

/*
 * Get pointer to JPEG buffer for web server
 */
uint8_t* getJpegBuffer() {
  return jpegBuffer;
}

/*
 * Get current JPEG buffer size
 */
size_t getJpegBufferSize() {
  return jpegBufferLen;
}

#endif // SCREEN_MIRROR_H