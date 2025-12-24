/*
 * VAIL SUMMIT - Theme Manager
 * Manages multiple UI themes with dynamic switching
 *
 * Themes:
 * - SUMMIT: Modern dark theme with cyan/teal accents (default)
 * - ENIGMA: Military-inspired theme with green/brass accents
 */

#ifndef LV_THEME_MANAGER_H
#define LV_THEME_MANAGER_H

#include <lvgl.h>

// Include custom fonts for Enigma theme
#include "../fonts/font_special_elite_14.c"
#include "../fonts/font_special_elite_18.c"
#include "../fonts/font_special_elite_24.c"
#include "../fonts/font_special_elite_28.c"

// ============================================
// Theme Types
// ============================================

typedef enum {
    THEME_SUMMIT = 0,    // Modern dark theme with cyan/teal accents
    THEME_ENIGMA = 1     // Military-inspired theme with green/brass accents
} ThemeType;

// ============================================
// Theme Color Palette Structure
// ============================================

typedef struct {
    // Background colors
    lv_color_t bg_deep;           // Main background
    lv_color_t bg_layer2;         // Elevated surfaces

    // Card & surface colors
    lv_color_t card_primary;      // Standard card background
    lv_color_t card_secondary;    // Dark card
    lv_color_t card_focused;      // Highlighted/focused card
    lv_color_t card_border;       // Card border color

    // Accent colors
    lv_color_t accent_primary;    // Primary accent (focus states)
    lv_color_t accent_secondary;  // Secondary accent
    lv_color_t accent_glow;       // Glow/border accent

    // Text colors
    lv_color_t text_primary;      // Main text
    lv_color_t text_secondary;    // Secondary text
    lv_color_t text_tertiary;     // Tertiary/hint text
    lv_color_t text_disabled;     // Disabled text
    lv_color_t text_on_accent;    // Text on accent background

    // Status colors
    lv_color_t success;           // Green/success
    lv_color_t warning;           // Orange/warning
    lv_color_t error;             // Red/error

    // Border colors
    lv_color_t border_subtle;     // Subtle border
    lv_color_t border_light;      // Lighter border
    lv_color_t border_accent;     // Focused border glow
} ThemeColors;

// ============================================
// Theme Font Set Structure
// ============================================

typedef struct {
    const lv_font_t* font_small;      // 12pt - help text, footer
    const lv_font_t* font_body;       // 14pt - body text, buttons
    const lv_font_t* font_input;      // 16pt - input fields
    const lv_font_t* font_subtitle;   // 18pt - section headers
    const lv_font_t* font_title;      // 24pt - titles
    const lv_font_t* font_large;      // 28pt - large display values
} ThemeFonts;

// ============================================
// Global Theme State
// ============================================

static ThemeType currentTheme = THEME_SUMMIT;
static ThemeColors activeColors;
static ThemeFonts activeFonts;
static bool themeManagerInitialized = false;

// ============================================
// Summit Theme Color Palette (Cyan/Teal Modern)
// ============================================

static const ThemeColors SUMMIT_COLORS = {
    // Background colors
    .bg_deep = {0},          // Will be set in init (0x1A1A2E)
    .bg_layer2 = {0},        // 0x252542

    // Card & surface colors
    .card_primary = {0},     // 0x252542
    .card_secondary = {0},   // 0x2D3250
    .card_focused = {0},     // 0x00D4AA
    .card_border = {0},      // 0x3A3A5C

    // Accent colors
    .accent_primary = {0},   // 0x00D4AA
    .accent_secondary = {0}, // 0x4A90D9
    .accent_glow = {0},      // 0x00FFCC

    // Text colors
    .text_primary = {0},     // 0xE8E8F0
    .text_secondary = {0},   // 0x8888AA
    .text_tertiary = {0},    // 0x666688
    .text_disabled = {0},    // 0x4A4A6A
    .text_on_accent = {0},   // 0x1A1A2E

    // Status colors
    .success = {0},          // 0x50C878
    .warning = {0},          // 0xFFB347
    .error = {0},            // 0xFF6B6B

    // Border colors
    .border_subtle = {0},    // 0x3A3A5C
    .border_light = {0},     // 0x5A5A7C
    .border_accent = {0}     // 0x00FFCC
};

// ============================================
// Enigma Theme Color Palette (Military Green/Brass)
// ============================================

static const ThemeColors ENIGMA_COLORS = {
    // Background colors - Near-black with green tint
    .bg_deep = {0},          // Will be set in init (0x1A1C1A)
    .bg_layer2 = {0},        // 0x252825

    // Card & surface colors - Army green tones
    .card_primary = {0},     // 0x2D3A2A (dark army green)
    .card_secondary = {0},   // 0x252825 (surface)
    .card_focused = {0},     // 0x4A5A47 (lighter army green)
    .card_border = {0},      // 0x3A3A3A

    // Accent colors - Aged brass
    .accent_primary = {0},   // 0x8B7355 (aged brass)
    .accent_secondary = {0}, // 0x3D4A3A (army green)
    .accent_glow = {0},      // 0xA08060 (light brass)

    // Text colors - Aged paper
    .text_primary = {0},     // 0xD4CFC4 (aged paper)
    .text_secondary = {0},   // 0x8A8578 (muted)
    .text_tertiary = {0},    // 0x6A6558
    .text_disabled = {0},    // 0x4A4A4A
    .text_on_accent = {0},   // 0x1A1C1A (dark on brass)

    // Status colors - Muted military
    .success = {0},          // 0x3A5A3A (dark green)
    .warning = {0},          // 0x8B7A3A (mustard)
    .error = {0},            // 0x8B3A3A (muted red)

    // Border colors
    .border_subtle = {0},    // 0x3A3A3A
    .border_light = {0},     // 0x5A5A5A
    .border_accent = {0}     // 0xA08060 (brass)
};

// ============================================
// Theme Initialization Functions
// ============================================

/*
 * Initialize Summit color palette
 */
void initSummitColors(ThemeColors* colors) {
    // Background colors
    colors->bg_deep = lv_color_hex(0x1A1A2E);
    colors->bg_layer2 = lv_color_hex(0x252542);

    // Card & surface colors
    colors->card_primary = lv_color_hex(0x252542);
    colors->card_secondary = lv_color_hex(0x2D3250);
    colors->card_focused = lv_color_hex(0x00D4AA);
    colors->card_border = lv_color_hex(0x3A3A5C);

    // Accent colors
    colors->accent_primary = lv_color_hex(0x00D4AA);
    colors->accent_secondary = lv_color_hex(0x4A90D9);
    colors->accent_glow = lv_color_hex(0x00FFCC);

    // Text colors
    colors->text_primary = lv_color_hex(0xE8E8F0);
    colors->text_secondary = lv_color_hex(0x8888AA);
    colors->text_tertiary = lv_color_hex(0x666688);
    colors->text_disabled = lv_color_hex(0x4A4A6A);
    colors->text_on_accent = lv_color_hex(0x1A1A2E);

    // Status colors
    colors->success = lv_color_hex(0x50C878);
    colors->warning = lv_color_hex(0xFFB347);
    colors->error = lv_color_hex(0xFF6B6B);

    // Border colors
    colors->border_subtle = lv_color_hex(0x3A3A5C);
    colors->border_light = lv_color_hex(0x5A5A7C);
    colors->border_accent = lv_color_hex(0x00FFCC);
}

/*
 * Initialize Enigma color palette
 */
void initEnigmaColors(ThemeColors* colors) {
    // Background colors - Near-black with green tint
    colors->bg_deep = lv_color_hex(0x1A1C1A);
    colors->bg_layer2 = lv_color_hex(0x252825);

    // Card & surface colors - Army green tones
    colors->card_primary = lv_color_hex(0x2D3A2A);
    colors->card_secondary = lv_color_hex(0x252825);
    colors->card_focused = lv_color_hex(0x4A5A47);
    colors->card_border = lv_color_hex(0x3A3A3A);

    // Accent colors - Aged brass
    colors->accent_primary = lv_color_hex(0x8B7355);
    colors->accent_secondary = lv_color_hex(0x3D4A3A);
    colors->accent_glow = lv_color_hex(0xA08060);

    // Text colors - Aged paper
    colors->text_primary = lv_color_hex(0xD4CFC4);
    colors->text_secondary = lv_color_hex(0x8A8578);
    colors->text_tertiary = lv_color_hex(0x6A6558);
    colors->text_disabled = lv_color_hex(0x4A4A4A);
    colors->text_on_accent = lv_color_hex(0x1A1C1A);

    // Status colors - Muted military
    colors->success = lv_color_hex(0x3A5A3A);
    colors->warning = lv_color_hex(0x8B7A3A);
    colors->error = lv_color_hex(0x8B3A3A);

    // Border colors
    colors->border_subtle = lv_color_hex(0x3A3A3A);
    colors->border_light = lv_color_hex(0x5A5A5A);
    colors->border_accent = lv_color_hex(0xA08060);
}

/*
 * Initialize Summit fonts (Montserrat)
 */
void initSummitFonts(ThemeFonts* fonts) {
    fonts->font_small = &lv_font_montserrat_12;
    fonts->font_body = &lv_font_montserrat_14;
    fonts->font_input = &lv_font_montserrat_16;
    fonts->font_subtitle = &lv_font_montserrat_18;
    fonts->font_title = &lv_font_montserrat_24;
    fonts->font_large = &lv_font_montserrat_28;
}

/*
 * Initialize Enigma fonts (Special Elite typewriter)
 */
void initEnigmaFonts(ThemeFonts* fonts) {
    fonts->font_small = &font_special_elite_14;
    fonts->font_body = &font_special_elite_14;
    fonts->font_input = &font_special_elite_18;
    fonts->font_subtitle = &font_special_elite_18;
    fonts->font_title = &font_special_elite_24;
    fonts->font_large = &font_special_elite_28;
}

/*
 * Load color palette for the specified theme
 */
void loadThemePalette(ThemeType theme) {
    if (theme == THEME_ENIGMA) {
        initEnigmaColors(&activeColors);
    } else {
        initSummitColors(&activeColors);
    }
}

/*
 * Load font set for the specified theme
 */
void loadThemeFonts(ThemeType theme) {
    if (theme == THEME_ENIGMA) {
        initEnigmaFonts(&activeFonts);
    } else {
        initSummitFonts(&activeFonts);
    }
}

// ============================================
// Theme Manager API
// ============================================

/*
 * Initialize the theme manager
 * Call this once after lv_init() but before initSummitTheme()
 */
void initThemeManager() {
    if (themeManagerInitialized) return;

    Serial.println("[ThemeManager] Initializing...");

    // Set default theme (Summit)
    currentTheme = THEME_SUMMIT;
    loadThemePalette(THEME_SUMMIT);
    loadThemeFonts(THEME_SUMMIT);

    themeManagerInitialized = true;
    Serial.println("[ThemeManager] Initialization complete (default: Summit)");
}

/*
 * Get the current active theme
 */
ThemeType getCurrentTheme() {
    return currentTheme;
}

/*
 * Get the current color palette
 */
const ThemeColors* getThemeColors() {
    return &activeColors;
}

/*
 * Get the current font set
 */
const ThemeFonts* getThemeFonts() {
    return &activeFonts;
}

/*
 * Get theme name as string
 */
const char* getThemeName(ThemeType theme) {
    switch (theme) {
        case THEME_ENIGMA:
            return "Enigma";
        case THEME_SUMMIT:
        default:
            return "Summit";
    }
}

// Forward declarations for theme switching
// (These functions will be implemented after lv_theme_summit.h is refactored)
extern void initSummitTheme();
extern void refreshCurrentLVGLScreen();

/*
 * Switch to a new theme
 * This reinitializes all styles and reloads the current screen
 */
void setTheme(ThemeType theme) {
    if (theme == currentTheme) {
        Serial.printf("[ThemeManager] Theme already set to %s, skipping\n", getThemeName(theme));
        return;
    }

    Serial.printf("[ThemeManager] Switching theme from %s to %s\n",
                  getThemeName(currentTheme), getThemeName(theme));

    // Update current theme
    currentTheme = theme;

    // Load new color palette
    loadThemePalette(theme);

    // Load new fonts
    loadThemeFonts(theme);

    // Reinitialize all styles with new colors
    initSummitTheme();

    // Force LVGL to refresh all style caches
    lv_obj_report_style_change(NULL);

    // Refresh the current screen to apply changes
    refreshCurrentLVGLScreen();

    Serial.printf("[ThemeManager] Theme switched to %s\n", getThemeName(theme));
}

/*
 * Apply a theme without screen refresh (for boot sequence)
 * Use setTheme() for runtime switching
 */
void applyThemeWithoutRefresh(ThemeType theme) {
    currentTheme = theme;
    loadThemePalette(theme);
    loadThemeFonts(theme);
    Serial.printf("[ThemeManager] Applied theme: %s (no refresh)\n", getThemeName(theme));
}

#endif // LV_THEME_MANAGER_H
