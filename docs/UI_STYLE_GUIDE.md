# VAIL SUMMIT UI Style Guide

**Version:** 2.0
**Last Updated:** 2025-01-10
**Design System:** Modern Minimal with Pastel Blue-Green Palette

---

## Overview

VAIL SUMMIT uses a clean, modern minimal design system inspired by iOS and contemporary mobile interfaces. This guide ensures visual consistency across all features and screens.

## Design Principles

1. **Solid Colors Only** - No banding or fake gradients (RGB565 limitation)
2. **Pastel Palette** - Subtle, muted blue-green colors throughout
3. **iOS-like Aesthetics** - Clean, minimal, modern design
4. **Clear Visual Hierarchy** - Text and element importance through color and size
5. **Context-Aware Accents** - Colors match content purpose (volume, brightness, status)

---

## Color Palette

### Background Colors

Foundation colors for the interface:

```cpp
COLOR_BG_DEEP           0x0841  // RGB(15, 20, 35) - Deep dark background
COLOR_BG_LAYER2         0x1082  // RGB(20, 30, 50) - Subtle raised layer
```

**Usage:**
- BG_DEEP: Main screen background
- BG_LAYER2: Card backgrounds, containers

### Card & Surface Colors (Pastel Blue-Green)

Solid pastel colors for cards and interactive elements:

```cpp
COLOR_CARD_BLUE         0x32AB  // RGB(50, 85, 90) - Soft muted blue
COLOR_CARD_TEAL         0x2B0E  // RGB(40, 95, 115) - Soft teal
COLOR_CARD_CYAN         0x3471  // RGB(50, 140, 140) - Muted cyan
COLOR_CARD_MINT         0x3D53  // RGB(60, 170, 155) - Soft mint
```

**Usage:**
- Use solid colors for cards (no gradients)
- Rotate through palette for variety (e.g., practice cards use teal, cyan, mint)
- Selection highlights: `COLOR_CARD_CYAN`

### Accent Colors (Subtle Highlights)

Brighter accents for important elements:

```cpp
COLOR_ACCENT_BLUE       0x4D5F  // RGB(75, 170, 255) - Soft bright blue
COLOR_ACCENT_CYAN       0x5EB9  // RGB(90, 215, 210) - Soft cyan accent
COLOR_ACCENT_GREEN      0x5712  // RGB(85, 225, 150) - Soft green accent
```

**Usage:**
- Icon circles, selected values, active states
- Use sparingly for emphasis

### Status Colors (Pastels)

Soft status indicators:

```cpp
COLOR_SUCCESS_PASTEL    0x4E91  // RGB(75, 210, 140) - Soft pastel green
COLOR_WARNING_PASTEL    0xFE27  // RGB(255, 195, 60) - Soft pastel orange
COLOR_ERROR_PASTEL      0xFACB  // RGB(250, 90, 90) - Soft pastel red
```

**Usage:**
- SUCCESS: Connected states, high battery, good values
- WARNING: Connecting, medium levels, caution
- ERROR: Disconnected, low battery, errors

### Border & Outline Colors

Subtle borders for depth:

```cpp
COLOR_BORDER_SUBTLE     0x39EC  // RGB(55, 65, 100) - Very subtle border
COLOR_BORDER_LIGHT      0x632C  // RGB(100, 100, 100) - Light neutral border
COLOR_BORDER_ACCENT     0x6D7F  // RGB(110, 170, 255) - Soft blue border
```

**Usage:**
- SUBTLE: Card outlines, container borders
- LIGHT: Progress bars, icon outlines
- ACCENT: Selection highlights, active borders

### Text Hierarchy Colors

Establish clear visual priority:

```cpp
COLOR_TEXT_PRIMARY      0xFFFF  // RGB(255, 255, 255) - White (main text)
COLOR_TEXT_SECONDARY    0xBDF7  // RGB(190, 190, 190) - Light gray (labels)
COLOR_TEXT_TERTIARY     0x8410  // RGB(130, 130, 130) - Medium gray (hints)
COLOR_TEXT_DISABLED     0x5ACB  // RGB(90, 90, 90) - Dark gray (inactive)
```

**Usage:**
- PRIMARY: Main content, values, headings, selected text
- SECONDARY: Labels, unselected items
- TERTIARY: Hints, placeholders
- DISABLED: Inactive states

### Background Color

```cpp
COLOR_BACKGROUND        COLOR_BG_DEEP  // Dark background (never pure black)
```

**Rule:** NEVER use pure black (0x0000). Always use COLOR_BACKGROUND or COLOR_BG_DEEP.

---

## Layout Constants

### Screen Dimensions

```cpp
SCREEN_WIDTH            480     // Display width
SCREEN_HEIGHT           320     // Display height
HEADER_HEIGHT           45      // Top status bar (reduced for better proportions)
FOOTER_HEIGHT           30      // Bottom instruction bar
```

### Common Dimensions

```cpp
CARD_MAIN_WIDTH         450     // Main menu card width
CARD_MAIN_HEIGHT        80      // Main menu card height
CARD_STACK_WIDTH_1      405     // First stacked card
CARD_STACK_WIDTH_2      375     // Second stacked card
STATUS_ICON_SIZE        12      // Battery/WiFi icon height
```

---

## Component Patterns

### 1. Clean Container Card

**Standard Pattern:**

```cpp
// Example: Settings screen, info display

int cardX = 20;
int cardY = 60;
int cardW = SCREEN_WIDTH - 40;  // 20px margins
int cardH = 150;

// Solid card background (no gradients)
display.fillRoundRect(cardX, cardY, cardW, cardH, 10, COLOR_BG_LAYER2);

// Subtle border
display.drawRoundRect(cardX, cardY, cardW, cardH, 10, COLOR_BORDER_SUBTLE);
```

**Rules:**
- Use solid colors only (no banding/gradients)
- 10px radius for cards, 8px for smaller elements
- Single subtle border (no double borders or glow)
- No shadow layers or shimmer effects

### 2. Info Card (Practice Mode)

**For displaying key metrics (speed, WPM, key type):**

```cpp
// Example: Practice mode info cards

int cardX = 10;
int cardY = 75;
int cardW = 155;
int cardH = 50;

// Solid card background (use pastel color from palette)
display.fillRoundRect(cardX, cardY, cardW, cardH, 8, COLOR_CARD_TEAL);

// Subtle border
display.drawRoundRect(cardX, cardY, cardW, cardH, 8, COLOR_BORDER_SUBTLE);

// Label at top (small, inside card)
display.setTextSize(1);
display.setTextColor(COLOR_TEXT_SECONDARY);
display.setCursor(cardX + 8, cardY + 8);
display.print("SET WPM");

// Value text (centered, large)
display.setFont(&FreeSansBold18pt7b);
display.setTextSize(1);
display.setTextColor(COLOR_TEXT_PRIMARY);
display.setCursor(cardX + (cardW - w) / 2, cardY + 36);  // Properly centered
display.print(value);
display.setFont(nullptr);
```

**Card Color Rotation:**
- Card 1: `COLOR_CARD_TEAL`
- Card 2: `COLOR_CARD_CYAN`
- Card 3: `COLOR_CARD_MINT`

### 3. Progress Bar (Solid Fill)

**Clean progress bars with single color:**

```cpp
// Example: Volume/Brightness bars

int barX = cardX + 20;
int barY = cardY + 70;
int barW = cardW - 40;
int barH = 14;

// Bar container (dark background)
display.fillRoundRect(barX, barY, barW, barH, 7, COLOR_BG_DEEP);

// Filled portion (solid color based on level)
int fillW = (barW * value) / 100;
if (fillW > 0) {
  uint16_t fillColor;
  if (value > 70) {
    fillColor = COLOR_ACCENT_CYAN;      // High
  } else if (value > 30) {
    fillColor = COLOR_CARD_CYAN;        // Medium
  } else {
    fillColor = COLOR_CARD_TEAL;        // Low
  }

  display.fillRoundRect(barX + 2, barY + 2, fillW - 4, barH - 4, 5, fillColor);
}

// Border on bar
display.drawRoundRect(barX, barY, barW, barH, 7, COLOR_BORDER_LIGHT);
```

**Rules:**
- Use solid colors only (no banding)
- Choose color based on value/context
- Keep consistent with overall pastel palette

### 4. Selection Highlight

**For interactive lists, settings rows:**

```cpp
// When item is selected

if (isSelected) {
  // Solid highlight background
  display.fillRoundRect(itemX, itemY, itemW, itemH, 8, COLOR_CARD_CYAN);

  // Accent border
  display.drawRoundRect(itemX, itemY, itemW, itemH, 8, COLOR_BORDER_ACCENT);
}

// Text color changes
display.setTextColor(isSelected ? COLOR_TEXT_PRIMARY : COLOR_TEXT_SECONDARY);
```

**Rules:**
- Background: `COLOR_CARD_CYAN` for selections
- Single border with `COLOR_BORDER_ACCENT`
- Text: white when selected, light gray when not

### 5. Menu Card (Main Selected)

**The signature VAIL SUMMIT card design:**

```cpp
int mainCardX = (SCREEN_WIDTH - CARD_MAIN_WIDTH) / 2;
int mainCardY = 110;

// Solid pastel background (no gradients)
tft.fillRoundRect(mainCardX, mainCardY, CARD_MAIN_WIDTH, CARD_MAIN_HEIGHT, 12, COLOR_CARD_CYAN);

// Clean border
tft.drawRoundRect(mainCardX, mainCardY, CARD_MAIN_WIDTH, CARD_MAIN_HEIGHT, 12, COLOR_BORDER_ACCENT);

// Icon circle (simple, clean)
int iconX = mainCardX + 40;
int iconY = mainCardY + 40;

// Solid icon circle
tft.fillCircle(iconX, iconY, 26, COLOR_ACCENT_BLUE);
tft.drawCircle(iconX, iconY, 26, COLOR_BORDER_LIGHT);

// Icon letter (clean white, no shadow)
tft.setFont(&FreeSansBold18pt7b);
tft.setTextColor(ST77XX_WHITE);
tft.setCursor(mainCardX + 30, mainCardY + 25);
tft.print(icon);

// Menu text (clean, no shadow)
tft.setCursor(mainCardX + 85, mainCardY + 25);
tft.print(menuText);

// Simple arrow indicator
tft.fillTriangle(mainCardX + CARD_MAIN_WIDTH - 30, mainCardY + 32,
                 mainCardX + CARD_MAIN_WIDTH - 30, mainCardY + 48,
                 mainCardX + CARD_MAIN_WIDTH - 15, mainCardY + 40, COLOR_TEXT_PRIMARY);
```

**Critical Pattern:**
- Solid pastel card color
- Single clean border
- Simple solid icon circle
- No shadows, no glow effects
- Clean text alignment

### 6. Stacked Cards (Background/Preview)

**For carousel/preview effects:**

```cpp
// Stacked card (next/previous item)
int stackY = mainCardY + CARD_MAIN_HEIGHT + 12;

// Solid dimmed background
display.fillRoundRect(stackX, stackY, stackW, stackH, 8, COLOR_BG_LAYER2);

// Subtle border
display.drawRoundRect(stackX, stackY, stackW, stackH, 8, COLOR_BORDER_SUBTLE);

// Small icon circle
display.fillCircle(stackX + 18, stackY + 16, 12, COLOR_CARD_BLUE);
display.drawCircle(stackX + 18, stackY + 16, 12, COLOR_BORDER_SUBTLE);

// Icon and text use secondary/tertiary colors
display.setTextColor(COLOR_TEXT_SECONDARY);
```

**Rules:**
- Darker solid backgrounds = further back in visual stack
- Smaller size, less detail = visual hierarchy
- Text: secondary or tertiary colors only

---

## Typography

### Font Sizes

```cpp
FreeSansBold18pt7b  // Main headings, selected items, large values
FreeSansBold12pt7b  // Menu text, settings labels, card headers
FreeSansBold9pt7b   // Secondary items, stacked cards, footer text
nullptr (default)   // Fallback for basic UI elements
```

### Text Color Usage

| Context | Color | Use Case |
|---------|-------|----------|
| Primary content | `COLOR_TEXT_PRIMARY` | Main values, headings, selected items |
| Labels | `COLOR_TEXT_SECONDARY` | Field labels, descriptions |
| Hints | `COLOR_TEXT_TERTIARY` | Placeholder text, less important info |
| Disabled | `COLOR_TEXT_DISABLED` | Inactive elements, shadows |
| Success | `COLOR_GREEN_ACCENT` | Positive values, connected states |
| Warning | `COLOR_WARNING_NEW` | Caution, medium states |
| Error | `COLOR_ERROR_NEW` | Errors, low values, disconnected |
| Info | `COLOR_CYAN` | Neutral highlight, set values |

### Text Shadow (Optional Depth)

For large text on gradients, add subtle shadow:

```cpp
// Shadow (1px offset, dark color)
display.setTextColor(COLOR_TEXT_DISABLED);
display.setCursor(x + 1, y + 1);
display.print(text);

// Main text (white)
display.setTextColor(COLOR_TEXT_PRIMARY);
display.setCursor(x, y);
display.print(text);
```

---

## Spacing & Dimensions

### Corner Radius Standards

| Element Type | Radius | Use Case |
|--------------|--------|----------|
| Large cards | 10-12px | Settings containers, main cards |
| Medium cards | 8-10px | Info cards, decoder boxes |
| Small elements | 7-8px | Progress bars, badges |
| Icon circles | 26px (diameter 52px) | Menu icon circles |

### Padding & Margins

| Element | Spacing | Notes |
|---------|---------|-------|
| Screen margins | 20px | Left/right margins for full-width cards |
| Card padding | 15px | Internal padding from edges |
| Label position | 8px from top | Labels inside cards |
| Line spacing | 45px | Vertical spacing between settings rows |
| Card gap | 4-12px | Space between stacked cards |

### Safe Zones

```
Screen: 480×320

Header: 0-45 (reserved - reduced height)
Footer: 308-320 (reserved for help text)
Content area: 47-306 (safe zone for UI elements)

Horizontal margins: 15-30px recommended
```

---

## Header & Footer Patterns

### Clean Minimal Header

```cpp
void drawHeader() {
  // Clean solid background
  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BG_LAYER2);

  // Subtle bottom border
  tft.drawLine(0, HEADER_HEIGHT, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BORDER_SUBTLE);

  // Title (vertically centered)
  tft.setFont(&FreeSansBold12pt7b);  // Smaller font
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  tft.setCursor(15, 28);  // y=28 for vertical center in 45px header
  tft.print(title);
  tft.setFont(nullptr);

  // Status icons (properly sized and positioned)
  drawStatusIcons();
}
```

**Rules:**
- Always 45px tall (HEADER_HEIGHT - reduced for better proportions)
- Use FreeSansBold12pt7b for title (not 18pt)
- Title at y=28 for perfect vertical centering
- Single subtle border at bottom

### Footer

```cpp
void drawFooter() {
  display.setFont(&FreeSansBold9pt7b);
  display.setTextColor(COLOR_WARNING_PASTEL);  // Soft pastel orange

  String helpText = "UP/DN Navigate   ENTER Select   ESC Back";

  // Center text
  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(display, helpText.c_str(), 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;

  display.setCursor(centerX, SCREEN_HEIGHT - 22);
  display.print(helpText);
  display.setFont(nullptr);
}
```

**Rules:**
- Use COLOR_WARNING_PASTEL (soft pastel orange)
- Single line, centered
- Position at y = SCREEN_HEIGHT - 22 (298px)

---

## Status Icons

### Battery Icon (Clean Minimal)

```cpp
// Clean battery outline (24x12)
int battW = 24;
int battH = 12;

tft.drawRoundRect(x, y, battW, battH, 2, COLOR_BORDER_LIGHT);

// Battery nub (small terminal)
tft.fillRect(x + battW, y + 4, 2, 4, COLOR_BORDER_LIGHT);

// Determine fill color based on level
uint16_t fillColor;
if (batteryPercent > 60) {
  fillColor = COLOR_SUCCESS_PASTEL;  // Soft green for high
} else if (batteryPercent > 20) {
  fillColor = COLOR_ACCENT_CYAN;     // Soft cyan for medium
} else {
  fillColor = COLOR_ERROR_PASTEL;    // Soft red for low
}

// Solid fill (no banding)
int fillWidth = (batteryPercent * (battW - 4)) / 100;
if (fillWidth > 0) {
  tft.fillRect(x + 2, y + 2, fillWidth, battH - 4, fillColor);
}
```

### WiFi Icon (Clean Simple Bars)

```cpp
// Simple WiFi bars (4 bars, increasing height)
uint16_t barColor = wifiConnected ? COLOR_ACCENT_CYAN : COLOR_TEXT_DISABLED;

// Draw 4 signal bars (3px wide each)
tft.fillRect(x, y + 8, 3, 4, barColor);
tft.fillRect(x + 5, y + 5, 3, 7, barColor);
tft.fillRect(x + 10, y + 2, 3, 10, barColor);
tft.fillRect(x + 15, y, 3, 12, barColor);
```

---

## Common Pitfalls & Solutions

### ❌ DON'T: Use Pure Black Background

```cpp
// WRONG
display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x0000);
```

### ✅ DO: Use Deep Dark Background

```cpp
// CORRECT
display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BACKGROUND);
```

---

### ❌ DON'T: Use Hardcoded Colors

```cpp
// WRONG - Hardcoded hex values
display.setTextColor(0x7BEF);
display.fillRect(x, y, w, h, 0x1082);
```

### ✅ DO: Use Named Color Constants

```cpp
// CORRECT - Named constants from palette
display.setTextColor(COLOR_TEXT_SECONDARY);
display.fillRoundRect(x, y, w, h, 10, COLOR_BG_LAYER2);
```

---

### ❌ DON'T: Use Banded Gradients

```cpp
// WRONG - 16-bit banding looks tacky
int bandW = w / 3;
display.fillRect(x, y, bandW, h, COLOR_BLUE_MED);
display.fillRect(x + bandW, y, bandW, h, COLOR_BLUE_CYAN);
display.fillRect(x + 2*bandW, y, w - 2*bandW, h, COLOR_CYAN_GREEN);
```

### ✅ DO: Use Solid Pastel Colors

```cpp
// CORRECT - Single solid pastel color
display.fillRoundRect(x, y, w, h, 10, COLOR_CARD_CYAN);
display.drawRoundRect(x, y, w, h, 10, COLOR_BORDER_SUBTLE);
```

---

### ❌ DON'T: Text Without Hierarchy

```cpp
// WRONG - Everything is white
display.setTextColor(ST77XX_WHITE);
display.print("Label");  // Should be secondary
display.print(value);    // Should be primary
```

### ✅ DO: Clear Text Hierarchy

```cpp
// CORRECT - Labels vs values
display.setTextColor(COLOR_TEXT_SECONDARY);
display.print("Label");

display.setTextColor(COLOR_TEXT_PRIMARY);
display.print(value);
```

---

## Checklist for New Features

Before submitting new UI code, verify:

- [ ] Uses `COLOR_BACKGROUND` instead of pure black
- [ ] All cards have shadow layer (+2 or +3 offset)
- [ ] Glass borders include both glow and border layers
- [ ] Shimmer highlights on glass elements (top-left)
- [ ] Text uses hierarchy (PRIMARY/SECONDARY/TERTIARY)
- [ ] Progress bars use gradient fills (not flat colors)
- [ ] Rounded corners: 16px (large), 12px (medium), 8px (small)
- [ ] Status colors use modern palette (SUCCESS_NEW, WARNING_NEW, ERROR_NEW)
- [ ] No hardcoded color hex values (use named constants)
- [ ] Badges hover above cards (-8px offset)
- [ ] All elements fit within safe zones (62-306 vertical)
- [ ] Footer uses `COLOR_WARNING` (warm orange)
- [ ] Header title at y=35 for vertical centering
- [ ] Gradients use blue→cyan→green spectrum appropriately

---

## Quick Reference: Color Selection

**When to use which color:**

| Need | Color Constant | Notes |
|------|----------------|-------|
| Main container background | `COLOR_GLASS_MED` | Standard glass body |
| Deep background | `COLOR_BACKGROUND` or `COLOR_BG_DEEP` | Never pure black |
| Shadow layer | `COLOR_GLASS_DARK` | Offset +2 or +3 |
| Light overlay | `COLOR_GLASS_LIGHT` | Top portion of containers |
| Border glow | `COLOR_GLASS_BORDER_GLOW` | Outer border |
| Border line | `COLOR_GLASS_BORDER` | Inner border |
| Shimmer | `COLOR_GLASS_SHIMMER` | Top-left highlights |
| Main text | `COLOR_TEXT_PRIMARY` | White, most important |
| Label text | `COLOR_TEXT_SECONDARY` | Light gray, labels |
| Hint text | `COLOR_TEXT_TERTIARY` | Medium gray, hints |
| Inactive text | `COLOR_TEXT_DISABLED` | Dark gray, shadows |
| Success state | `COLOR_GREEN_ACCENT` | Connected, high, good |
| Warning state | `COLOR_WARNING_NEW` | Caution, medium |
| Error state | `COLOR_ERROR_NEW` | Error, low, disconnected |
| Info badge | `COLOR_CYAN` | Neutral information |
| Progress start | `COLOR_BLUE_CYAN` | Blue end of spectrum |
| Progress mid | `COLOR_CYAN_GREEN` | Middle of spectrum |
| Progress end | `COLOR_GREEN_ACCENT` | Green end of spectrum |

---

## Examples

See these files for reference implementations:

- **Menu cards**: `src/ui/menu_ui.h` (lines 452-520)
- **Settings container**: `src/settings/settings_cw.h` (lines 94-108)
- **Info cards**: `src/training/training_practice.h` (lines 180-294)
- **Progress bars**: `src/settings/settings_volume.h` (lines 96-125)
- **Header**: `src/ui/menu_ui.h` (lines 294-410)
- **Status icons**: `src/ui/status_bar.h` (lines 29-119)

---

## Version History

**v1.0 (2025-01-10)**
- Initial glassmorphic design system
- Blue-green gradient spectrum
- Glass effect simulation patterns
- Text hierarchy system
- Complete component library

---

**Questions?** Refer to existing implementations or contact the design team.
