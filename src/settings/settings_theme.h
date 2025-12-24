/*
 * VAIL SUMMIT - Theme Settings Persistence
 * Manages saving and loading UI theme preference
 */

#ifndef SETTINGS_THEME_H
#define SETTINGS_THEME_H

#include <Preferences.h>
#include "../lvgl/lv_theme_manager.h"

// Preferences namespace for theme settings
static Preferences themePrefs;

// ============================================
// Theme Settings API
// ============================================

/*
 * Load theme settings from flash storage
 * Call this during boot after initThemeManager()
 */
void loadThemeSettings() {
    themePrefs.begin("theme", true);  // Read-only mode
    int savedTheme = themePrefs.getInt("type", 0);  // Default: Summit (0)
    themePrefs.end();

    Serial.printf("[ThemeSettings] Loaded theme preference: %d\n", savedTheme);

    // Apply theme without screen refresh (boot sequence)
    ThemeType theme = (savedTheme == 1) ? THEME_ENIGMA : THEME_SUMMIT;
    applyThemeWithoutRefresh(theme);
}

/*
 * Save theme setting to flash storage
 * Call this when user changes theme
 */
void saveThemeSetting(ThemeType theme) {
    themePrefs.begin("theme", false);  // Read-write mode
    themePrefs.putInt("type", (int)theme);
    themePrefs.end();

    Serial.printf("[ThemeSettings] Saved theme preference: %d (%s)\n",
                  (int)theme, getThemeName(theme));
}

/*
 * Reset theme to default (Summit)
 */
void resetThemeSettings() {
    themePrefs.begin("theme", false);
    themePrefs.clear();
    themePrefs.end();

    applyThemeWithoutRefresh(THEME_SUMMIT);
    Serial.println("[ThemeSettings] Theme reset to default (Summit)");
}

#endif // SETTINGS_THEME_H
