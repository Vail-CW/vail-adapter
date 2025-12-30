/*
 * VAIL SUMMIT - Band Plan Settings
 * Preferences persistence for user's license class and mode filter
 */

#ifndef SETTINGS_BAND_PLAN_H
#define SETTINGS_BAND_PLAN_H

#include <Preferences.h>
#include "../data/band_plan_data.h"

// ============================================
// Settings State
// ============================================

static LicenseClass bp_user_license = LICENSE_TECHNICIAN;
static uint8_t bp_mode_filter = BP_MODE_ALL;

// ============================================
// Load/Save Functions
// ============================================

inline void loadBandPlanSettings() {
    Preferences prefs;
    prefs.begin("bandplan", true);  // Read-only
    bp_user_license = (LicenseClass)prefs.getInt("license", LICENSE_TECHNICIAN);
    bp_mode_filter = prefs.getInt("modefilter", BP_MODE_ALL);
    prefs.end();

    // Validate loaded values
    if (bp_user_license > LICENSE_EXTRA) {
        bp_user_license = LICENSE_TECHNICIAN;
    }
    if (bp_mode_filter != BP_MODE_ALL && bp_mode_filter != BP_MODE_CW &&
        bp_mode_filter != BP_MODE_PHONE && bp_mode_filter != BP_MODE_DATA) {
        bp_mode_filter = BP_MODE_ALL;
    }
}

inline void saveBandPlanSettings() {
    Preferences prefs;
    prefs.begin("bandplan", false);  // Read-write
    prefs.putInt("license", (int)bp_user_license);
    prefs.putInt("modefilter", (int)bp_mode_filter);
    prefs.end();
}

// ============================================
// Getters/Setters
// ============================================

inline LicenseClass getBPUserLicense() {
    return bp_user_license;
}

inline void setBPUserLicense(LicenseClass lic) {
    bp_user_license = lic;
    saveBandPlanSettings();
}

inline uint8_t getBPModeFilter() {
    return bp_mode_filter;
}

inline void setBPModeFilter(uint8_t filter) {
    bp_mode_filter = filter;
    saveBandPlanSettings();
}

// Cycle to next license class
inline void cycleBPLicenseNext() {
    bp_user_license = (LicenseClass)((bp_user_license + 1) % 3);
    saveBandPlanSettings();
}

// Cycle to previous license class
inline void cycleBPLicensePrev() {
    bp_user_license = (LicenseClass)((bp_user_license + 2) % 3);
    saveBandPlanSettings();
}

// Cycle to next mode filter
inline void cycleBPModeFilterNext() {
    switch (bp_mode_filter) {
        case BP_MODE_ALL:   bp_mode_filter = BP_MODE_CW;    break;
        case BP_MODE_CW:    bp_mode_filter = BP_MODE_PHONE; break;
        case BP_MODE_PHONE: bp_mode_filter = BP_MODE_DATA;  break;
        case BP_MODE_DATA:  bp_mode_filter = BP_MODE_ALL;   break;
        default:            bp_mode_filter = BP_MODE_ALL;   break;
    }
    saveBandPlanSettings();
}

// Cycle to previous mode filter
inline void cycleBPModeFilterPrev() {
    switch (bp_mode_filter) {
        case BP_MODE_ALL:   bp_mode_filter = BP_MODE_DATA;  break;
        case BP_MODE_DATA:  bp_mode_filter = BP_MODE_PHONE; break;
        case BP_MODE_PHONE: bp_mode_filter = BP_MODE_CW;    break;
        case BP_MODE_CW:    bp_mode_filter = BP_MODE_ALL;   break;
        default:            bp_mode_filter = BP_MODE_ALL;   break;
    }
    saveBandPlanSettings();
}

#endif // SETTINGS_BAND_PLAN_H
