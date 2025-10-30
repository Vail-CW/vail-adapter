# Adafruit GFX Library Call Chain Analysis

## Critical Finding: fillRect() Bypass Issue

Your `MirroredST7789` class overrides are being bypassed for `fillRect()` calls due to **optimization in the SPITFT class hierarchy that uses inline functions and direct virtual function calls that skip your overrides**.

### The Problem

When you call `tft.fillRect(0, 0, 320, 40, 0x1082)`, the call chain is:

```
User Code: tft.fillRect(0, 0, 320, 40, 0x1082)
    ↓
Adafruit_SPITFT::fillRect() [SPITFT.cpp:1730]
    ↓
Adafruit_SPITFT::writeFillRectPreclipped() [SPITFT.cpp:1676] ← INLINE FUNCTION
    ↓
Adafruit_SPITFT::setAddrWindow() [ST77xx subclass]
    ↓
Adafruit_SPITFT::writeColor() [SPITFT.cpp:1211]
    ↓
Adafruit_SPITFT::writePixels() [NOT virtual - direct SPITFT implementation]
```

### Key Issue: writeFillRectPreclipped() is Inline

The function `writeFillRectPreclipped()` is declared **inline**:

```cpp
// Adafruit_SPITFT.cpp, line 1676
inline void Adafruit_SPITFT::writeFillRectPreclipped(int16_t x, int16_t y,
                                                     int16_t w, int16_t h,
                                                     uint16_t color) {
  setAddrWindow(x, y, w, h);
  writeColor(color, (uint32_t)w * h);
}
```

### Why This Matters

1. **writeFillRectPreclipped() is NOT virtual** - It's a regular method in SPITFT
2. **It's marked inline** - Compiler may inline it directly
3. **writeColor() is NOT virtual** - It's an SPITFT-specific implementation
4. **writePixels() is NOT virtual** - It's the actual low-level pixel writer

The inheritance hierarchy looks like:

```
Adafruit_GFX (base class)
  ↓
Adafruit_SPITFT : public Adafruit_GFX
  ├─ fillRect() [virtual override of GFX]
  ├─ writeFillRectPreclipped() [NOT virtual - regular method]
  ├─ writeColor() [NOT virtual]
  └─ writePixels() [NOT virtual]
     ↓
Adafruit_ST77xx : public Adafruit_SPITFT
  ├─ setAddrWindow() [virtual, overridden]
     ↓
Adafruit_ST7789 : public Adafruit_ST77xx
     ↓
MirroredST7789 : public Adafruit_ST7789 ← YOUR CLASS
  ├─ fillRect() [YOUR OVERRIDE]
  ├─ writePixel() [YOUR OVERRIDE]
  └─ [missing: writeFillRectPreclipped, writeColor, writePixels]
```

## Call Paths Comparison

### Path 1: Your Overridden Functions (CAPTURED)
```
User: tft.drawPixel(x, y, color)
    ↓
MirroredST7789::drawPixel() [YOUR OVERRIDE] ✓ CAPTURED
    ↓
Calls parent or your custom logic
```

```
User: tft.fillRect() [in some contexts]
    ↓
MirroredST7789::fillRect() [YOUR OVERRIDE] ✓ CAPTURED
    ↓
Custom logic
```

### Path 2: SPITFT Optimized Path (BYPASSES YOUR CLASS)
```
User: tft.fillRect(0, 0, 320, 40, 0x1082)
    ↓
Adafruit_SPITFT::fillRect() [line 1730] - virtual, so calls your override
    BUT if NOT overridden, falls through to SPITFT::fillRect()
    ↓ [inside SPITFT::fillRect()]
    startWrite()
    writeFillRectPreclipped(x, y, w, h, color) ← CALLS SPITFT VERSION DIRECTLY
    ↓ [NOT virtual - static dispatch to SPITFT::writeFillRectPreclipped]
    setAddrWindow(x, y, w, h) ← This IS virtual, so reaches your parent
    writeColor(color, (uint32_t)w * h) ← NOT virtual - SPITFT::writeColor
    ↓
    writePixels() ← NOT virtual - directly calls SPITFT::writePixels
```

## Why Large Rects (Blue Card) Work But Small Rects (Header Bar) Don't

The **large blue card probably uses different rendering code**:

1. If it uses `drawRect()` outline + `fillRect()` calls with different parameters
2. If it uses separate pixel operations that DO go through `drawPixel()`
3. If it uses `drawFastHLine()` or `drawFastVLine()` which DO call virtual methods

Let's trace `drawFastHLine()`:

```
User: tft.drawFastHLine()
    ↓
Adafruit_SPITFT::drawFastHLine() [line 1788]
    ↓
startWrite()
writeFillRectPreclipped(x, y, w, 1, color) ← SAME ISSUE
```

**BUT** if your header bar is drawn with a series of `drawPixel()` calls or with text background filling, those would be captured by your `MirroredST7789::drawPixel()` override.

## The Root Cause: SPITFT Optimization Strategy

The Adafruit library uses this strategy:

1. **Virtual functions for user-facing methods**: `fillRect()`, `drawPixel()`, `drawFastHLine()`, `drawFastVLine()`
   - These CAN be overridden by subclasses

2. **Non-virtual internal functions**: `writeFillRectPreclipped()`, `writeColor()`, `writePixels()`
   - These are SPITFT-specific optimizations NOT meant to be overridden
   - Direct dispatch for maximum performance

3. **This creates a two-tier system**:
   - **Tier 1**: User-facing drawing functions (virtual, can be overridden)
   - **Tier 2**: Internal pixel delivery (non-virtual, optimized for speed)

## Why Your writePixel() Override Doesn't Help

Your current override:
```cpp
void MirroredST7789::writePixel(int16_t x, int16_t y, uint16_t color) {
  // Your code
  Adafruit_ST7789::writePixel(x, y, color);
}
```

This works for calls that reach `writePixel()`, but `writeColor()` **never calls** `writePixel()`. It directly calls `writePixels()` (plural), which on ESP32 uses the specialized SPI function.

## Solutions

### Solution 1: Override writeFillRectPreclipped() (RECOMMENDED)

```cpp
// In MirroredST7789.h
class MirroredST7789 : public Adafruit_ST7789 {
  // ... existing code ...
  
  // Override the inline function - compiler will generate non-inline version
  inline void writeFillRectPreclipped(int16_t x, int16_t y, int16_t w, int16_t h,
                                      uint16_t color) override {
    // Mirror to local buffer
    mirrorPixelRegion(x, y, w, h, color);
    // Call parent
    Adafruit_ST7789::writeFillRectPreclipped(x, y, w, h, color);
  }
};
```

**Note**: Even though the parent is marked `inline`, your override can still work. The compiler will NOT inline a virtual function call (it defeats polymorphism).

### Solution 2: Override writeColor()

```cpp
// In MirroredST7789.h
void MirroredST7789::writeColor(uint16_t color, uint32_t len) {
  // Mirror current pixel region to buffer
  // (requires tracking current setAddrWindow state)
  Adafruit_ST7789::writeColor(color, len);
}
```

**Problem**: `writeColor()` doesn't know which pixels it's writing. You'd need to track the `setAddrWindow()` state separately.

### Solution 3: Override writePixels()

```cpp
void MirroredST7789::writePixels(uint16_t *colors, uint32_t len, bool block = true,
                                 bool bigEndian = false) {
  // Mirror pixels
  Adafruit_ST7789::writePixels(colors, len, block, bigEndian);
}
```

**Problem**: Same as above - you don't know the coordinates.

### Solution 4: Track setAddrWindow() State

```cpp
class MirroredST7789 : public Adafruit_ST7789 {
private:
  int16_t _lastAddrX, _lastAddrY, _lastAddrW, _lastAddrH;

public:
  void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) override {
    _lastAddrX = x;
    _lastAddrY = y;
    _lastAddrW = w;
    _lastAddrH = h;
    Adafruit_ST7789::setAddrWindow(x, y, w, h);
  }

  inline void writeFillRectPreclipped(int16_t x, int16_t y, int16_t w, int16_t h,
                                      uint16_t color) override {
    // Mirror using tracked address window
    mirrorRect(_lastAddrX, _lastAddrY, _lastAddrW, _lastAddrH, color);
    Adafruit_ST7789::writeFillRectPreclipped(x, y, w, h, color);
  }
};
```

**Best approach**: Combine this with `writeColor()` override to capture solid color fills.

## The Actual Call Chain for your fillRect() Issue

```cpp
// Your code:
tft.fillRect(0, 0, 320, 40, 0x1082);

// Adafruit_SPITFT::fillRect (line 1730-1770)
void Adafruit_SPITFT::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                               uint16_t color) {
  // ... clipping code ...
  startWrite();
  writeFillRectPreclipped(x, y, w, h, color);  // <-- STATIC DISPATCH TO SPITFT VERSION
  endWrite();
}

// Adafruit_SPITFT::writeFillRectPreclipped (line 1676-1681)
inline void Adafruit_SPITFT::writeFillRectPreclipped(...) {
  setAddrWindow(x, y, w, h);        // <-- Virtual, reaches parent'
