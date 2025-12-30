/*
 * VAIL SUMMIT - Band Plan Data
 * US Amateur Radio HF Band Allocations
 *
 * Data based on ARRL band plans and FCC Part 97 regulations
 * Future: Add UK, Canada, Japan, etc. band plans
 */

#ifndef BAND_PLAN_DATA_H
#define BAND_PLAN_DATA_H

#include <Arduino.h>

// ============================================
// Enums
// ============================================

typedef enum {
    LICENSE_TECHNICIAN = 0,
    LICENSE_GENERAL = 1,
    LICENSE_EXTRA = 2
} LicenseClass;

typedef enum {
    BP_MODE_CW      = 0x01,
    BP_MODE_PHONE   = 0x02,
    BP_MODE_DATA    = 0x04,
    BP_MODE_IMAGE   = 0x08,
    BP_MODE_ALL     = 0x0F
} BandPlanMode;

// ============================================
// Data Structures
// ============================================

typedef struct {
    float start_mhz;        // Start frequency in MHz
    float end_mhz;          // End frequency in MHz
    LicenseClass license;   // Minimum license class required
    uint8_t modes;          // Bitmask of allowed modes (BandPlanMode)
    const char* label;      // Short description
} BandPlanEntry;

typedef struct {
    const char* name;           // Full band name (e.g., "160 Meters")
    const char* short_name;     // Short name (e.g., "160m")
    float start_mhz;            // Band start frequency
    float end_mhz;              // Band end frequency
    int max_power_watts;        // Maximum power (typically 1500W)
    bool warc_band;             // True if WARC band (no contests)
    const BandPlanEntry* entries;
    int entry_count;
} BandDefinition;

typedef struct {
    const char* country_code;   // ISO country code (e.g., "US")
    const char* country_name;   // Full country name
    const BandDefinition* bands;
    int band_count;
} CountryBandPlan;

// ============================================
// Helper Functions
// ============================================

inline const char* getLicenseClassName(LicenseClass lic) {
    switch (lic) {
        case LICENSE_TECHNICIAN: return "Technician";
        case LICENSE_GENERAL:    return "General";
        case LICENSE_EXTRA:      return "Extra";
        default:                 return "Unknown";
    }
}

inline const char* getLicenseClassShort(LicenseClass lic) {
    switch (lic) {
        case LICENSE_TECHNICIAN: return "T";
        case LICENSE_GENERAL:    return "G";
        case LICENSE_EXTRA:      return "E";
        default:                 return "?";
    }
}

inline const char* getModeLabel(uint8_t modes) {
    if (modes == BP_MODE_ALL) return "All Modes";
    if (modes == BP_MODE_CW) return "CW Only";
    if (modes == (BP_MODE_CW | BP_MODE_DATA)) return "CW/Data";
    if (modes == (BP_MODE_CW | BP_MODE_PHONE)) return "CW/Phone";
    if (modes == (BP_MODE_CW | BP_MODE_PHONE | BP_MODE_DATA)) return "CW/Phone/Data";
    if (modes == BP_MODE_PHONE) return "Phone";
    if (modes == BP_MODE_DATA) return "Data";
    return "Mixed";
}

inline const char* getModeFilterLabel(uint8_t filter) {
    switch (filter) {
        case BP_MODE_ALL:   return "All Modes";
        case BP_MODE_CW:    return "CW Only";
        case BP_MODE_PHONE: return "Phone Only";
        case BP_MODE_DATA:  return "Data Only";
        default:            return "All Modes";
    }
}

// ============================================
// US Band Plan Data - 160 Meters (1.8-2.0 MHz)
// ============================================

static const BandPlanEntry us_160m_entries[] = {
    { 1.800f, 1.840f, LICENSE_EXTRA,      BP_MODE_CW | BP_MODE_DATA,   "CW/Data - E" },
    { 1.840f, 1.850f, LICENSE_GENERAL,    BP_MODE_CW | BP_MODE_DATA,   "CW/Data - G" },
    { 1.850f, 2.000f, LICENSE_GENERAL,    BP_MODE_ALL,                  "All Modes - G" }
};

// ============================================
// US Band Plan Data - 80 Meters (3.5-4.0 MHz)
// ============================================

static const BandPlanEntry us_80m_entries[] = {
    { 3.500f, 3.525f, LICENSE_EXTRA,      BP_MODE_CW | BP_MODE_DATA,   "CW/Data - E" },
    { 3.525f, 3.600f, LICENSE_GENERAL,    BP_MODE_CW | BP_MODE_DATA,   "CW/Data - G" },
    { 3.600f, 3.700f, LICENSE_EXTRA,      BP_MODE_ALL,                  "All Modes - E" },
    { 3.700f, 3.800f, LICENSE_GENERAL,    BP_MODE_ALL,                  "All Modes - G/A" },
    { 3.800f, 4.000f, LICENSE_EXTRA,      BP_MODE_ALL,                  "All Modes - E" }
};

// ============================================
// US Band Plan Data - 60 Meters (5 Channels)
// Note: 60m is channelized, not continuous
// ============================================

static const BandPlanEntry us_60m_entries[] = {
    { 5.332f, 5.333f, LICENSE_GENERAL,    BP_MODE_CW | BP_MODE_PHONE | BP_MODE_DATA, "Ch 1 - 5332.0" },
    { 5.348f, 5.349f, LICENSE_GENERAL,    BP_MODE_CW | BP_MODE_PHONE | BP_MODE_DATA, "Ch 2 - 5348.0" },
    { 5.358f, 5.359f, LICENSE_GENERAL,    BP_MODE_CW | BP_MODE_PHONE | BP_MODE_DATA, "Ch 3 - 5358.5" },
    { 5.373f, 5.374f, LICENSE_GENERAL,    BP_MODE_CW | BP_MODE_PHONE | BP_MODE_DATA, "Ch 4 - 5373.0" },
    { 5.405f, 5.406f, LICENSE_GENERAL,    BP_MODE_CW | BP_MODE_PHONE | BP_MODE_DATA, "Ch 5 - 5405.0" }
};

// ============================================
// US Band Plan Data - 40 Meters (7.0-7.3 MHz)
// ============================================

static const BandPlanEntry us_40m_entries[] = {
    { 7.000f, 7.025f, LICENSE_EXTRA,      BP_MODE_CW | BP_MODE_DATA,   "CW/Data - E" },
    { 7.025f, 7.125f, LICENSE_GENERAL,    BP_MODE_CW | BP_MODE_DATA,   "CW/Data - G" },
    { 7.125f, 7.175f, LICENSE_EXTRA,      BP_MODE_ALL,                  "All Modes - E" },
    { 7.175f, 7.300f, LICENSE_GENERAL,    BP_MODE_ALL,                  "All Modes - G" }
};

// ============================================
// US Band Plan Data - 30 Meters (10.1-10.15 MHz)
// WARC Band - No contests
// ============================================

static const BandPlanEntry us_30m_entries[] = {
    { 10.100f, 10.150f, LICENSE_GENERAL,  BP_MODE_CW | BP_MODE_DATA,   "CW/Data - G (200W)" }
};

// ============================================
// US Band Plan Data - 20 Meters (14.0-14.35 MHz)
// ============================================

static const BandPlanEntry us_20m_entries[] = {
    { 14.000f, 14.025f, LICENSE_EXTRA,    BP_MODE_CW | BP_MODE_DATA,   "CW/Data - E" },
    { 14.025f, 14.150f, LICENSE_GENERAL,  BP_MODE_CW | BP_MODE_DATA,   "CW/Data - G" },
    { 14.150f, 14.175f, LICENSE_EXTRA,    BP_MODE_ALL,                  "All Modes - E" },
    { 14.175f, 14.225f, LICENSE_GENERAL,  BP_MODE_ALL,                  "All Modes - G" },
    { 14.225f, 14.350f, LICENSE_EXTRA,    BP_MODE_ALL,                  "All Modes - E" }
};

// ============================================
// US Band Plan Data - 17 Meters (18.068-18.168 MHz)
// WARC Band - No contests
// ============================================

static const BandPlanEntry us_17m_entries[] = {
    { 18.068f, 18.110f, LICENSE_GENERAL,  BP_MODE_CW | BP_MODE_DATA,   "CW/Data - G" },
    { 18.110f, 18.168f, LICENSE_GENERAL,  BP_MODE_ALL,                  "All Modes - G" }
};

// ============================================
// US Band Plan Data - 15 Meters (21.0-21.45 MHz)
// ============================================

static const BandPlanEntry us_15m_entries[] = {
    { 21.000f, 21.025f, LICENSE_EXTRA,    BP_MODE_CW | BP_MODE_DATA,   "CW/Data - E" },
    { 21.025f, 21.200f, LICENSE_GENERAL,  BP_MODE_CW | BP_MODE_DATA,   "CW/Data - G" },
    { 21.200f, 21.225f, LICENSE_EXTRA,    BP_MODE_ALL,                  "All Modes - E" },
    { 21.225f, 21.275f, LICENSE_GENERAL,  BP_MODE_ALL,                  "All Modes - G" },
    { 21.275f, 21.450f, LICENSE_EXTRA,    BP_MODE_ALL,                  "All Modes - E" }
};

// ============================================
// US Band Plan Data - 12 Meters (24.89-24.99 MHz)
// WARC Band - No contests
// ============================================

static const BandPlanEntry us_12m_entries[] = {
    { 24.890f, 24.930f, LICENSE_GENERAL,  BP_MODE_CW | BP_MODE_DATA,   "CW/Data - G" },
    { 24.930f, 24.990f, LICENSE_GENERAL,  BP_MODE_ALL,                  "All Modes - G" }
};

// ============================================
// US Band Plan Data - 10 Meters (28.0-29.7 MHz)
// ============================================

static const BandPlanEntry us_10m_entries[] = {
    { 28.000f, 28.300f, LICENSE_TECHNICIAN, BP_MODE_CW | BP_MODE_DATA, "CW/Data - T (200W)" },
    { 28.300f, 28.500f, LICENSE_TECHNICIAN, BP_MODE_ALL,               "All Modes - T" },
    { 28.500f, 29.700f, LICENSE_GENERAL,    BP_MODE_ALL,               "All Modes - G" }
};

// ============================================
// US HF Band Definitions Array
// ============================================

static const BandDefinition us_hf_bands[] = {
    {
        "160 Meters", "160m",
        1.800f, 2.000f,
        1500,
        false,  // Not WARC
        us_160m_entries,
        sizeof(us_160m_entries) / sizeof(BandPlanEntry)
    },
    {
        "80 Meters", "80m",
        3.500f, 4.000f,
        1500,
        false,
        us_80m_entries,
        sizeof(us_80m_entries) / sizeof(BandPlanEntry)
    },
    {
        "60 Meters", "60m",
        5.330f, 5.410f,
        100,  // 60m is 100W ERP max
        false,
        us_60m_entries,
        sizeof(us_60m_entries) / sizeof(BandPlanEntry)
    },
    {
        "40 Meters", "40m",
        7.000f, 7.300f,
        1500,
        false,
        us_40m_entries,
        sizeof(us_40m_entries) / sizeof(BandPlanEntry)
    },
    {
        "30 Meters", "30m",
        10.100f, 10.150f,
        200,  // 30m is 200W max
        true,  // WARC band
        us_30m_entries,
        sizeof(us_30m_entries) / sizeof(BandPlanEntry)
    },
    {
        "20 Meters", "20m",
        14.000f, 14.350f,
        1500,
        false,
        us_20m_entries,
        sizeof(us_20m_entries) / sizeof(BandPlanEntry)
    },
    {
        "17 Meters", "17m",
        18.068f, 18.168f,
        1500,
        true,  // WARC band
        us_17m_entries,
        sizeof(us_17m_entries) / sizeof(BandPlanEntry)
    },
    {
        "15 Meters", "15m",
        21.000f, 21.450f,
        1500,
        false,
        us_15m_entries,
        sizeof(us_15m_entries) / sizeof(BandPlanEntry)
    },
    {
        "12 Meters", "12m",
        24.890f, 24.990f,
        1500,
        true,  // WARC band
        us_12m_entries,
        sizeof(us_12m_entries) / sizeof(BandPlanEntry)
    },
    {
        "10 Meters", "10m",
        28.000f, 29.700f,
        1500,
        false,
        us_10m_entries,
        sizeof(us_10m_entries) / sizeof(BandPlanEntry)
    }
};

#define US_HF_BAND_COUNT (sizeof(us_hf_bands) / sizeof(BandDefinition))

// ============================================
// US Country Band Plan
// ============================================

static const CountryBandPlan us_band_plan = {
    "US",
    "United States",
    us_hf_bands,
    US_HF_BAND_COUNT
};

// ============================================
// Future Country Placeholders
// ============================================

// Placeholder for UK band plan
// static const CountryBandPlan uk_band_plan = { "UK", "United Kingdom", NULL, 0 };

// Placeholder for Canada band plan
// static const CountryBandPlan ca_band_plan = { "CA", "Canada", NULL, 0 };

// Placeholder for Japan band plan
// static const CountryBandPlan ja_band_plan = { "JA", "Japan", NULL, 0 };

// ============================================
// Access Functions
// ============================================

inline const CountryBandPlan* getUSBandPlan() {
    return &us_band_plan;
}

inline const BandDefinition* getBandByIndex(int index) {
    if (index >= 0 && index < (int)US_HF_BAND_COUNT) {
        return &us_hf_bands[index];
    }
    return NULL;
}

inline int getBandCount() {
    return US_HF_BAND_COUNT;
}

// Check if user can operate in a specific segment
inline bool canOperate(const BandPlanEntry* entry, LicenseClass user_license) {
    return user_license >= entry->license;
}

// Check if a mode matches the filter
inline bool modeMatchesFilter(uint8_t entry_modes, uint8_t filter) {
    if (filter == BP_MODE_ALL) return true;
    return (entry_modes & filter) != 0;
}

#endif // BAND_PLAN_DATA_H
