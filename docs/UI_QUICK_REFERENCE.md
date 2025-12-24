# VAIL SUMMIT UI Quick Reference

**Fast lookup for common UI patterns - Version 2.0 (Clean Minimal)**

---

## Color Palette (Copy-Paste Ready)

### Background Colors
```cpp
COLOR_BG_DEEP, COLOR_BG_LAYER2
```

### Card Colors (Pastel Blue-Green)
```cpp
COLOR_CARD_BLUE, COLOR_CARD_TEAL, COLOR_CARD_CYAN, COLOR_CARD_MINT
```

### Accent Colors
```cpp
COLOR_ACCENT_BLUE, COLOR_ACCENT_CYAN, COLOR_ACCENT_GREEN
```

### Status Colors (Pastels)
```cpp
COLOR_SUCCESS_PASTEL, COLOR_WARNING_PASTEL, COLOR_ERROR_PASTEL
```

### Border Colors
```cpp
COLOR_BORDER_SUBTLE, COLOR_BORDER_LIGHT, COLOR_BORDER_ACCENT
```

### Text Hierarchy
```cpp
COLOR_TEXT_PRIMARY, COLOR_TEXT_SECONDARY, COLOR_TEXT_TERTIARY, COLOR_TEXT_DISABLED
```

---

## Standard Container (Copy-Paste Template)

```cpp
// Clean minimal container card
int cardX = 20;
int cardY = 60;
int cardW = SCREEN_WIDTH - 40;
int cardH = 150;

// Solid card background (no gradients)
display.fillRoundRect(cardX, cardY, cardW, cardH, 10, COLOR_BG_LAYER2);

// Subtle border
display.drawRoundRect(cardX, cardY, cardW, cardH, 10, COLOR_BORDER_SUBTLE);
```

---

## Info Card (Copy-Paste Template)

```cpp
// Info card with label inside
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
display.setCursor(cardX + (cardW - w) / 2, cardY + 36);
display.print(value);
display.setFont(nullptr);
```

---

## Progress Bar (Copy-Paste Template)

```cpp
// Clean progress bar with solid fill
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

---

## Selection Highlight (Copy-Paste Template)

```cpp
// For list items, settings rows
if (isSelected) {
  display.fillRoundRect(itemX, itemY, itemW, itemH, 8, COLOR_CARD_CYAN);
  display.drawRoundRect(itemX, itemY, itemW, itemH, 8, COLOR_BORDER_ACCENT);
}

display.setTextColor(isSelected ? COLOR_TEXT_PRIMARY : COLOR_TEXT_SECONDARY);
```

---

## Text Hierarchy Pattern

```cpp
// Label
display.setTextSize(1);
display.setTextColor(COLOR_TEXT_SECONDARY);
display.print("Label");

// Value
display.setTextSize(2);
display.setTextColor(COLOR_TEXT_PRIMARY);
display.print(value);

// Hint/description
display.setTextSize(1);
display.setTextColor(COLOR_TEXT_TERTIARY);
display.print("Hint text");
```

---

## Common Dimensions

| Element | Width | Height | Radius | Margins |
|---------|-------|--------|--------|---------|
| Full-width card | `SCREEN_WIDTH - 40` | varies | 10px | 20px L/R |
| Info card (3-up) | 155px | 50px | 8px | varies |
| Progress bar | container - 40 | 14px | 7px | 20px L/R |
| Icon circle | 52px diameter | 52px | 26px | - |
| Battery icon | 24px + 2px nub | 12px | 2px | - |
| WiFi icon | 18px | 12px | - | - |

---

## Safe Zones

```
Screen: 480 × 320

Header:        y = 0-45
Content area:  y = 47-306
Footer:        y = 308-320
```

**Header title:** `y = 28` (vertically centered in 45px header)
**Footer text:** `y = SCREEN_HEIGHT - 22` (298px)

---

## Corner Radius Standards

| Size | Radius | Use |
|------|--------|-----|
| Large | 10-12px | Main cards, settings containers |
| Medium | 8-10px | Info cards, decoder boxes |
| Small | 7-8px | Progress bars, badges |

---

## Font Selection

```cpp
&FreeSansBold18pt7b  // Large values (centered in cards)
&FreeSansBold12pt7b  // Header titles, menu text, card headers
&FreeSansBold9pt7b   // Secondary text, footer
nullptr              // Default/fallback (labels)
```

---

## Status Color Mapping

```cpp
// Connected/Good/High
display.setTextColor(COLOR_SUCCESS_PASTEL);

// Connecting/Caution/Medium
display.setTextColor(COLOR_WARNING_PASTEL);

// Disconnected/Error/Low
display.setTextColor(COLOR_ERROR_PASTEL);

// Neutral/Info
display.setTextColor(COLOR_ACCENT_CYAN);
```

---

## Pre-Flight Checklist

Before committing UI code:

- [ ] No pure black (`0x0000`) - use `COLOR_BACKGROUND`
- [ ] No hardcoded colors - use named constants
- [ ] No banded gradients - use solid colors only
- [ ] Cards have subtle borders
- [ ] Text hierarchy (primary/secondary/tertiary)
- [ ] Rounded corners (10px/8px/7px)
- [ ] Elements within safe zones (47-306)
- [ ] Progress bars use solid colors (not banding)
- [ ] Labels inside cards (not floating pills)
- [ ] Header is 45px tall
- [ ] Header title uses FreeSansBold12pt7b at y=28

---

**Full documentation:** See `UI_STYLE_GUIDE.md`
