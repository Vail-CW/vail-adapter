# Screen Mirroring Developer Guide

This document explains how to write display code that correctly works with the VAIL SUMMIT screen mirroring system.

## Overview

VAIL SUMMIT includes a web-based screen mirroring feature that captures the display in real-time and streams it to a browser. To make this work reliably, all drawing code must follow specific patterns that allow the `MirroredST7789` class to intercept and record display operations.

## Architecture

The mirroring system works by:

1. **Custom Display Class**: `MirroredST7789` extends `Adafruit_ST7789` and overrides key drawing methods
2. **PSRAM Framebuffer**: A full copy of the display is maintained in PSRAM (240×180 resolution)
3. **Dual Writes**: Every drawing operation writes to both the physical display AND the framebuffer
4. **JPEG Encoding**: The framebuffer is periodically encoded to JPEG and served via web interface

## Critical Rendering Rules

### ✅ DO: Use These Drawing Functions

These functions are **guaranteed to be captured** by the mirroring system:

#### Basic Primitives
```cpp
tft.drawPixel(x, y, color);           // Single pixel
tft.fillRect(x, y, w, h, color);      // Filled rectangle (CRITICAL - we override this)
tft.drawRect(x, y, w, h, color);      // Rectangle outline
tft.fillScreen(color);                // Fill entire screen
```

#### Lines
```cpp
tft.drawFastHLine(x, y, w, color);    // Horizontal line
tft.drawFastVLine(x, y, h, color);    // Vertical line
tft.drawLine(x0, y0, x1, y1, color);  // Arbitrary line
```

#### Shapes
```cpp
tft.drawCircle(x, y, r, color);       // Circle outline
tft.fillCircle(x, y, r, color);       // Filled circle
tft.drawTriangle(...);                // Triangle outline
tft.fillTriangle(...);                // Filled triangle
tft.drawRoundRect(...);               // Rounded rectangle outline
tft.fillRoundRect(...);               // Filled rounded rectangle
```

#### Text
```cpp
tft.print("text");                    // Print text at cursor
tft.println("text");                  // Print with newline
tft.setCursor(x, y);                  // Set text position
tft.setTextColor(color);              // Set text color
tft.setTextColor(fg, bg);             // Set text and background color
tft.setFont(&font);                   // Set custom font
```

### ❌ DON'T: Avoid These Patterns

These patterns will **NOT be captured** and will cause missing elements in the mirror:

#### Direct Hardware Access
```cpp
// ❌ NEVER do this - bypasses mirroring completely
digitalWrite(TFT_CS, LOW);
SPI.transfer(...);
digitalWrite(TFT_CS, HIGH);
```

#### Low-Level Pixel Functions (if calling directly)
```cpp
// ❌ Avoid if possible - these are internal functions
tft.writePixels(buffer, len);         // Direct pixel buffer write
tft.writeColor(color, len);           // Bulk color write
tft.writeFillRectPreclipped(...);     // Internal fill function
```

**Note**: These functions ARE overridden in our class, but you should use the higher-level `fillRect()` instead for clarity.

### 🔍 Why fillRect() Needed Special Handling

The Adafruit_SPITFT library uses an optimization where `fillRect()` calls non-virtual internal functions:

```
fillRect() → writeFillRectPreclipped() [NOT VIRTUAL]
           → writeColor() [NOT VIRTUAL]
           → writePixels() [NOT VIRTUAL]
           → Hardware SPI
```

**Solution**: We override `fillRect()` itself at the public API level to capture the operation before it reaches the non-virtual fast path.

## Complete Drawing Call Chain

Here's how different drawing operations flow through the system:

### Text Rendering
```
tft.print("Hello")
  ↓
Adafruit_GFX::write()
  ↓
Adafruit_GFX::drawChar()
  ↓
tft.drawPixel() [VIRTUAL - captured by MirroredST7789]
  ↓
Physical display + Framebuffer
```

### Rectangle Fill
```
tft.fillRect(x, y, w, h, color)
  ↓
MirroredST7789::fillRect() [OUR OVERRIDE - captures to framebuffer]
  ↓
Adafruit_ST7789::fillRect()
  ↓
Physical display
```

### Rectangle Outline
```
tft.drawRect(x, y, w, h, color)
  ↓
Adafruit_GFX::drawRect()
  ↓
writeFastHLine() × 2 [VIRTUAL - captured]
writeFastVLine() × 2 [VIRTUAL - captured]
  ↓
Physical display + Framebuffer
```

### Circle
```
tft.fillCircle(x, y, r, color)
  ↓
Adafruit_GFX::fillCircle()
  ↓
Multiple calls to drawFastHLine() [VIRTUAL - captured]
  ↓
Physical display + Framebuffer
```

## Overridden Functions in MirroredST7789

Our custom display class overrides these virtual functions:

1. **drawPixel()** - Most fundamental drawing operation
2. **fillRect()** - CRITICAL: Captures fills before they hit non-virtual fast path
3. **writeFillRect()** - Internal fill optimization (backup)
4. **drawFastHLine()** - Horizontal line optimization
5. **drawFastVLine()** - Vertical line optimization
6. **fillScreen()** - Full screen clear optimization

All of these functions:
- Write to the PSRAM framebuffer with coordinate scaling (320×240 → 240×180)
- Call the parent class to update the physical display
- Set `mirrorDirty = true` to trigger encoding

## Resolution and Scaling

- **Physical Display**: 320×240 pixels (ST7789 LCD)
- **Framebuffer**: 240×180 pixels (75% scale to save memory)
- **Coordinate Mapping**: `fbX = (x * 240) / 320`, `fbY = (y * 180) / 240`
- **Pixel Thickening**: Each display pixel may write a 2×2 block in framebuffer for visibility

This scaling is **automatic and transparent** - just draw normally to the `tft` object.

## Example: Drawing a UI Element

### ✅ Correct Pattern
```cpp
void drawStatusBar() {
  // Fill background - uses fillRect() which is captured
  tft.fillRect(0, 0, SCREEN_WIDTH, 40, COLOR_DARK_BLUE);

  // Draw text - uses drawPixel() internally, captured
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 28);
  tft.print("VAIL SUMMIT");

  // Draw battery icon - uses drawRect/fillRect, captured
  tft.drawRect(280, 10, 24, 14, ST77XX_WHITE);
  tft.fillRect(282, 12, 20, 10, ST77XX_GREEN);
}
```

### ❌ Incorrect Pattern
```cpp
void drawStatusBarWrong() {
  // ❌ Direct SPI access - NOT captured
  digitalWrite(TFT_CS, LOW);
  // ... manual SPI commands ...
  digitalWrite(TFT_CS, HIGH);

  // ❌ Using low-level functions directly (though these ARE overridden)
  tft.writeColor(COLOR_DARK_BLUE, SCREEN_WIDTH * 40);
}
```

## Testing Your Code

To verify your drawing code works with mirroring:

1. **Enable mirroring**: Connect to WiFi and navigate to `http://vail-summit.local/`
2. **Start mirror**: Click "Start Mirroring" button
3. **Navigate to your screen**: Go to the menu/mode you're testing
4. **Check the web view**: Verify ALL elements appear in the browser
5. **Look for missing elements**:
   - Top status bars
   - Icons (battery, WiFi)
   - Small text
   - Outlines and borders

### Common Issues

**Top bar is black/missing**:
- You're probably using `fillRect()` which needed special override (now fixed)
- Solution: Already handled by our `fillRect()` override

**Text is missing**:
- Check that you're using `tft.print()` not manual pixel manipulation
- Verify text color isn't same as background
- Small text may need pixel thickening (automatic at 240×180)

**Icons don't appear**:
- Icons should use `drawRect`, `fillRect`, `drawTriangle`, `fillTriangle`
- These all eventually call our overridden primitives

**Some elements work, others don't**:
- Working elements use virtual functions we override
- Missing elements might use non-virtual internal functions
- File an issue with specific function names

## Memory Constraints

The screen mirroring system uses:

- **PSRAM Framebuffer**: 86,400 bytes (240 × 180 × 2 bytes per pixel)
- **RGB888 Conversion Buffer**: 129,600 bytes (240 × 180 × 3 bytes per pixel)
- **JPEG Output Buffer**: ~10-30KB (varies by quality/complexity)

**Total**: ~225KB of PSRAM + ~20KB heap

This leaves sufficient memory for WiFi, web server, and application code.

## Enabling/Disabling Mirroring

Mirroring can be toggled at runtime:

```cpp
extern bool mirrorEnabled;  // Defined in screen_mirror.h

// Disable mirroring temporarily (e.g., during animations)
mirrorEnabled = false;
// ... fast drawing operations ...
mirrorEnabled = true;
```

Disabling mirroring:
- Saves CPU time (no framebuffer writes)
- Reduces memory bandwidth
- Useful during intensive operations like games or audio playback

## Performance Considerations

### Drawing Performance
- Mirroring adds ~5-10% overhead to drawing operations
- Most overhead is from framebuffer writes, not encoding
- Encoding happens at 2 FPS (500ms interval) by default

### Optimization Tips
1. **Batch updates**: Minimize number of drawing calls
2. **Disable during intensive operations**: Turn off mirroring during games/audio
3. **Use bulk operations**: `fillRect()` is faster than pixel-by-pixel
4. **Avoid redundant draws**: Don't redraw unchanged elements

## Advanced: Adding New Overrides

If you find a drawing function that isn't captured, you can add an override:

1. **Check if it's virtual**: Look in `Adafruit_GFX.h` or `Adafruit_SPITFT.h`
2. **Add override to MirroredST7789**:
   ```cpp
   void newFunction(...) override {
     // Update framebuffer first
     if (mirrorEnabled && fullFramebuffer != nullptr) {
       // ... framebuffer update logic ...
       mirrorDirty = true;
     }

     // Call parent to update physical display
     Adafruit_ST7789::newFunction(...);
   }
   ```
3. **Test thoroughly**: Verify both physical display and mirror work

## Files Modified for Mirroring

If you're working on mirroring internals:

- **mirror_display.h** - `MirroredST7789` class with overrides
- **screen_mirror.h** - PSRAM allocation, JPEG encoding, web endpoints
- **web_server.h** - Web interface for viewing mirror
- **vail-summit.ino** - Changed `Adafruit_ST7789 tft` to `MirroredST7789 tft`
- **All UI headers** - Changed `extern Adafruit_ST7789 tft` to `extern MirroredST7789 tft`

## Summary

**Golden Rule**: Use standard Adafruit_GFX drawing functions (`fillRect`, `drawPixel`, `print`, etc.) and mirroring will work automatically. Avoid direct hardware access or undocumented low-level functions.

The `MirroredST7789` class is designed to be a **drop-in replacement** for `Adafruit_ST7789` with no code changes required. Just draw normally and mirroring happens transparently.

## Questions?

If you encounter drawing code that doesn't mirror correctly:

1. Check this guide to ensure you're using supported functions
2. Test with the web mirror interface
3. File an issue with the specific drawing function that doesn't work
4. Check `mirror_display.h` to see if that function is overridden

For more details, see:
- `mirror_display.h` - Override implementations
- `screen_mirror.h` - Encoding and memory management
- `CLAUDE.md` - Complete project architecture
