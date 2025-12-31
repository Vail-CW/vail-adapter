// POTA Active Spots API Module
// Fetches and manages live activator spots from api.pota.app

#ifndef POTA_SPOTS_H
#define POTA_SPOTS_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "internet_check.h"

// ============================================
// Configuration
// ============================================

#define MAX_POTA_SPOTS 200          // Maximum spots to store (PSRAM allows more)
#define POTA_SPOTS_TIMEOUT 15000    // API timeout in ms (increased for larger responses)
#define POTA_REFRESH_INTERVAL 60000 // Auto-refresh interval in ms

// ============================================
// POTA Spot Data Structure
// ============================================

struct POTASpot {
    uint32_t spotId;              // Unique spot identifier
    char activator[12];           // Callsign of activating station
    char frequency[12];           // Frequency in MHz (e.g., "14.062")
    char mode[8];                 // Operating mode (CW, SSB, FT8, etc.)
    char reference[12];           // Park reference (e.g., "K-0817")
    char parkName[51];            // Park name (truncated for display)
    char spotTime[24];            // ISO 8601 timestamp
    char spotter[12];             // Callsign of spotter
    char comments[61];            // Spot comments (truncated)
    char grid4[5];                // 4-character grid square
    char grid6[7];                // 6-character grid square
    float latitude;
    float longitude;
    char locationDesc[31];        // Location (e.g., "Indiana, US")
    int qsoCount;                 // Number of QSOs reported
};

// ============================================
// POTA Spot Filter
// ============================================

struct POTASpotFilter {
    char band[6];                 // "ALL", "20m", "40m", etc.
    char mode[8];                 // "ALL", "CW", "SSB", etc.
    char region[4];               // "ALL", "K", "VE", etc.
    bool active;                  // True if any filter is active
};

// ============================================
// POTA Spots Cache
// ============================================

struct POTASpotsCache {
    POTASpot* spots;              // Pointer to spots array (allocated in PSRAM)
    int count;                    // Number of spots in cache
    int maxSpots;                 // Allocated capacity
    unsigned long fetchTime;      // millis() when last fetched
    bool valid;                   // Data is valid
    bool fetching;                // Currently fetching
    bool initialized;             // PSRAM allocation done
};

// Global cache instance
// Using static since this header is only included in one translation unit chain
static POTASpotsCache potaSpotsCache = {
    nullptr, // spots (will be allocated in PSRAM)
    0,       // count
    0,       // maxSpots
    0,       // fetchTime
    false,   // valid
    false,   // fetching
    false    // initialized
};

// ============================================
// PSRAM Allocation
// ============================================

/**
 * Initialize POTA spots cache in PSRAM
 * Returns true if successful, false if PSRAM not available (falls back to heap)
 */
bool initPOTASpotsCache() {
    if (potaSpotsCache.initialized) {
        return true;  // Already initialized
    }

    size_t spotSize = sizeof(POTASpot);
    size_t totalSize = spotSize * MAX_POTA_SPOTS;

    Serial.printf("POTA Spots: Allocating cache for %d spots (%d bytes each, %d total)\n",
                  MAX_POTA_SPOTS, spotSize, totalSize);

    // Try PSRAM first
    if (psramFound()) {
        potaSpotsCache.spots = (POTASpot*)ps_malloc(totalSize);
        if (potaSpotsCache.spots) {
            Serial.printf("POTA Spots: Allocated %d bytes in PSRAM\n", totalSize);
            potaSpotsCache.maxSpots = MAX_POTA_SPOTS;
            potaSpotsCache.initialized = true;
            // Zero out the memory
            memset(potaSpotsCache.spots, 0, totalSize);
            return true;
        }
        Serial.println("POTA Spots: PSRAM allocation failed, trying heap...");
    }

    // Fall back to regular heap with smaller size
    int heapSpots = 50;  // Conservative limit for heap
    size_t heapSize = spotSize * heapSpots;

    potaSpotsCache.spots = (POTASpot*)malloc(heapSize);
    if (potaSpotsCache.spots) {
        Serial.printf("POTA Spots: Allocated %d bytes in heap (limit: %d spots)\n", heapSize, heapSpots);
        potaSpotsCache.maxSpots = heapSpots;
        potaSpotsCache.initialized = true;
        memset(potaSpotsCache.spots, 0, heapSize);
        return true;
    }

    Serial.println("POTA Spots: ERROR - Failed to allocate cache!");
    potaSpotsCache.maxSpots = 0;
    return false;
}

/**
 * Free POTA spots cache memory
 */
void freePOTASpotsCache() {
    if (potaSpotsCache.spots) {
        free(potaSpotsCache.spots);
        potaSpotsCache.spots = nullptr;
    }
    potaSpotsCache.count = 0;
    potaSpotsCache.maxSpots = 0;
    potaSpotsCache.initialized = false;
    potaSpotsCache.valid = false;
}

// Global filter instance
static POTASpotFilter potaSpotFilter = {
    "ALL",  // band
    "ALL",  // mode
    "ALL",  // region
    false   // active
};

// Selected spot index for detail view
static int selectedSpotIndex = -1;

// ============================================
// Band Frequency Ranges
// ============================================

struct BandRange {
    const char* name;
    float minFreq;
    float maxFreq;
};

static const BandRange bandRanges[] = {
    {"160m", 1.8, 2.0},
    {"80m", 3.5, 4.0},
    {"60m", 5.06, 5.45},
    {"40m", 7.0, 7.3},
    {"30m", 10.1, 10.15},
    {"20m", 14.0, 14.35},
    {"17m", 18.068, 18.168},
    {"15m", 21.0, 21.45},
    {"12m", 24.89, 24.99},
    {"10m", 28.0, 29.7},
    {"6m", 50.0, 54.0},
    {"2m", 144.0, 148.0},
    {"70cm", 420.0, 450.0}
};
#define NUM_BAND_RANGES (sizeof(bandRanges) / sizeof(bandRanges[0]))

// Filter options
static const char* bandFilterOptions[] = {"ALL", "160m", "80m", "60m", "40m", "30m", "20m", "17m", "15m", "12m", "10m", "6m", "2m"};
#define NUM_BAND_FILTERS (sizeof(bandFilterOptions) / sizeof(bandFilterOptions[0]))

static const char* modeFilterOptions[] = {"ALL", "CW", "SSB", "FT8", "FT4", "FM", "RTTY", "PSK"};
#define NUM_MODE_FILTERS (sizeof(modeFilterOptions) / sizeof(modeFilterOptions[0]))

static const char* regionFilterOptions[] = {"ALL", "K", "VE", "G", "DL", "F", "I", "JA", "VK", "ZL"};
#define NUM_REGION_FILTERS (sizeof(regionFilterOptions) / sizeof(regionFilterOptions[0]))

// ============================================
// Helper Functions
// ============================================

/**
 * Parse frequency string and return as float
 * Handles POTA API quirks (sometimes kHz, sometimes MHz)
 */
float parseFrequency(const char* freqStr) {
    if (!freqStr || strlen(freqStr) == 0) return 0.0f;

    float freq = atof(freqStr);

    // POTA API sometimes returns frequency in kHz if > 1000
    if (freq > 1000.0f) {
        freq = freq / 1000.0f;
    }

    return freq;
}

/**
 * Convert frequency to band name (POTA-specific version)
 * @param frequency Frequency in MHz
 * @return Band name (e.g., "20m") or "?" if unknown
 */
const char* potaFrequencyToBand(float frequency) {
    for (int i = 0; i < NUM_BAND_RANGES; i++) {
        if (frequency >= bandRanges[i].minFreq && frequency <= bandRanges[i].maxFreq) {
            return bandRanges[i].name;
        }
    }
    return "?";
}

/**
 * Get spot age as human-readable string
 * @param spotTime ISO 8601 timestamp string
 * @param buffer Output buffer
 * @param bufSize Buffer size
 */
void getSpotAge(const char* spotTime, char* buffer, int bufSize) {
    if (!spotTime || strlen(spotTime) == 0) {
        snprintf(buffer, bufSize, "?");
        return;
    }

    // Parse ISO 8601 timestamp (YYYY-MM-DDTHH:MM:SSZ or similar)
    // For simplicity, we'll use a basic approach
    struct tm tm = {};
    int year, month, day, hour, minute, second;

    // Try parsing common formats
    if (sscanf(spotTime, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = second;

        time_t spotEpoch = mktime(&tm);
        time_t now;
        time(&now);

        // Adjust for UTC (mktime assumes local time)
        // This is approximate - ESP32 time handling is tricky
        long diffSeconds = difftime(now, spotEpoch);

        if (diffSeconds < 0) {
            snprintf(buffer, bufSize, "now");
        } else if (diffSeconds < 60) {
            snprintf(buffer, bufSize, "now");
        } else if (diffSeconds < 3600) {
            int mins = diffSeconds / 60;
            snprintf(buffer, bufSize, "%dm", mins);
        } else {
            int hours = diffSeconds / 3600;
            snprintf(buffer, bufSize, "%dh", hours);
        }
    } else {
        snprintf(buffer, bufSize, "?");
    }
}

/**
 * Get spot age in minutes
 * @param spotTime ISO 8601 timestamp string
 * @return Age in minutes, or -1 if parse error
 */
int getSpotAgeMinutes(const char* spotTime) {
    if (!spotTime || strlen(spotTime) == 0) return -1;

    struct tm tm = {};
    int year, month, day, hour, minute, second;

    if (sscanf(spotTime, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = second;

        time_t spotEpoch = mktime(&tm);
        time_t now;
        time(&now);

        long diffSeconds = difftime(now, spotEpoch);
        return (diffSeconds > 0) ? (diffSeconds / 60) : 0;
    }

    return -1;
}

/**
 * Check if spot matches QRT keywords (activator signing off)
 */
bool isSpotQRT(const POTASpot& spot) {
    const char* qrtKeywords[] = {"qrt", "off", "done", "clear", "closing"};

    // Check comments
    if (strlen(spot.comments) > 0) {
        char lowerComments[61];
        strncpy(lowerComments, spot.comments, sizeof(lowerComments));
        for (int i = 0; lowerComments[i]; i++) {
            lowerComments[i] = tolower(lowerComments[i]);
        }

        for (int i = 0; i < 5; i++) {
            if (strstr(lowerComments, qrtKeywords[i]) != NULL) {
                return true;
            }
        }
    }

    // Check mode field
    if (strlen(spot.mode) > 0) {
        char lowerMode[8];
        strncpy(lowerMode, spot.mode, sizeof(lowerMode));
        for (int i = 0; lowerMode[i]; i++) {
            lowerMode[i] = tolower(lowerMode[i]);
        }

        if (strstr(lowerMode, "qrt") != NULL) {
            return true;
        }
    }

    return false;
}

/**
 * Check if spot matches current filter criteria
 */
bool spotMatchesFilter(const POTASpot& spot, const POTASpotFilter& filter) {
    // Band filter
    if (strcmp(filter.band, "ALL") != 0) {
        float freq = parseFrequency(spot.frequency);
        const char* spotBand = potaFrequencyToBand(freq);
        if (strcmp(spotBand, filter.band) != 0) {
            return false;
        }
    }

    // Mode filter
    if (strcmp(filter.mode, "ALL") != 0) {
        // Handle SSB variants
        if (strcmp(filter.mode, "SSB") == 0) {
            if (strcasecmp(spot.mode, "SSB") != 0 &&
                strcasecmp(spot.mode, "USB") != 0 &&
                strcasecmp(spot.mode, "LSB") != 0) {
                return false;
            }
        } else if (strcasecmp(spot.mode, filter.mode) != 0) {
            return false;
        }
    }

    // Region filter (first character(s) of park reference)
    if (strcmp(filter.region, "ALL") != 0) {
        // Extract prefix from reference (e.g., "K" from "K-1234", "VE" from "VE-1234")
        const char* dash = strchr(spot.reference, '-');
        if (dash) {
            int prefixLen = dash - spot.reference;
            char prefix[5] = "";
            if (prefixLen > 0 && prefixLen < 5) {
                strncpy(prefix, spot.reference, prefixLen);
                prefix[prefixLen] = '\0';

                // For single-letter regions (K, G, I, F), match first char
                // For multi-letter (VE, DL, JA, VK, ZL), match whole prefix
                if (strlen(filter.region) == 1) {
                    if (prefix[0] != filter.region[0]) {
                        return false;
                    }
                } else {
                    if (strcasecmp(prefix, filter.region) != 0) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

/**
 * Get filtered spot indices
 * @param cache Source cache
 * @param filter Filter criteria
 * @param indices Output array of indices
 * @param maxResults Maximum results to return
 * @return Number of matching spots
 */
int filterSpots(const POTASpotsCache& cache, const POTASpotFilter& filter,
                int* indices, int maxResults) {
    int count = 0;

    // Safety check
    if (!cache.spots || !cache.initialized || cache.count == 0) {
        return 0;
    }

    for (int i = 0; i < cache.count && count < maxResults; i++) {
        if (spotMatchesFilter(cache.spots[i], filter)) {
            indices[count++] = i;
        }
    }

    return count;
}

// ============================================
// API Functions
// ============================================

/**
 * Fetch active spots from POTA API
 * @param cache Cache to populate
 * @return Number of spots fetched, or -1 on error
 */
int fetchActiveSpots(POTASpotsCache& cache) {
    // Check internet connectivity (not just WiFi association)
    InternetStatus inetStatus = getInternetStatus();
    if (inetStatus != INET_CONNECTED) {
        if (inetStatus == INET_WIFI_ONLY) {
            Serial.println("POTA Spots: WiFi connected but no internet");
        } else {
            Serial.println("POTA Spots: No WiFi connection");
        }
        return -1;
    }

    // Initialize cache if not already done
    if (!cache.initialized) {
        if (!initPOTASpotsCache()) {
            Serial.println("POTA Spots: Failed to initialize cache");
            return -1;
        }
    }

    // Verify we have a valid spots array
    if (!cache.spots || cache.maxSpots == 0) {
        Serial.println("POTA Spots: Cache not properly allocated");
        return -1;
    }

    cache.fetching = true;

    Serial.println("POTA Spots: Fetching active spots...");
    Serial.printf("POTA Spots: Free heap: %d, PSRAM free: %d\n", ESP.getFreeHeap(), ESP.getFreePsram());
    Serial.printf("POTA Spots: Cache can hold up to %d spots\n", cache.maxSpots);

    HTTPClient http;
    http.begin("https://api.pota.app/spot/activator");
    http.setTimeout(POTA_SPOTS_TIMEOUT);
    http.addHeader("Accept", "application/json");

    int httpCode = http.GET();

    if (httpCode != 200) {
        Serial.printf("POTA Spots: HTTP error %d\n", httpCode);
        http.end();
        cache.fetching = false;
        return -1;
    }

    // Check if we have enough memory before reading response
    int contentLength = http.getSize();
    Serial.printf("POTA Spots: Content-Length: %d\n", contentLength);

    // Use PSRAM for large responses if available
    size_t requiredMem = contentLength + 65536;  // Response + JSON buffer headroom
    bool havePsram = psramFound() && ESP.getFreePsram() > requiredMem;

    if (!havePsram && contentLength > 0 && ESP.getFreeHeap() < requiredMem) {
        Serial.println("POTA Spots: Not enough memory for response!");
        http.end();
        cache.fetching = false;
        return -1;
    }

    String payload = http.getString();
    http.end();

    Serial.printf("POTA Spots: Received %d bytes\n", payload.length());
    Serial.printf("POTA Spots: Free heap after receive: %d\n", ESP.getFreeHeap());

    // Use larger JSON buffer for more spots
    // Each spot is ~500 bytes in JSON, so 200 spots = ~100KB
    // Allocate in PSRAM if available
    size_t jsonBufferSize = 131072;  // 128KB for ~200 spots
    DynamicJsonDocument* doc;

    if (psramFound()) {
        // Allocate JSON document in PSRAM
        doc = new (ps_malloc(sizeof(DynamicJsonDocument))) DynamicJsonDocument(jsonBufferSize);
        Serial.printf("POTA Spots: JSON buffer allocated in PSRAM (%d bytes)\n", jsonBufferSize);
    } else {
        // Fall back to heap with smaller buffer
        jsonBufferSize = 32768;  // 32KB for ~50 spots
        doc = new DynamicJsonDocument(jsonBufferSize);
        Serial.printf("POTA Spots: JSON buffer allocated in heap (%d bytes)\n", jsonBufferSize);
    }

    DeserializationError error = deserializeJson(*doc, payload);

    // Free the payload string ASAP to recover memory
    payload = String();

    if (error) {
        Serial.printf("POTA Spots: JSON parse error - %s\n", error.c_str());
        delete doc;
        cache.fetching = false;
        return -1;
    }

    Serial.printf("POTA Spots: Free heap after parse: %d\n", ESP.getFreeHeap());

    JsonArray spotsArray = doc->as<JsonArray>();
    Serial.printf("POTA Spots: API returned %d spots\n", spotsArray.size());

    // Clear cache
    cache.count = 0;

    // Parse each spot
    for (JsonObject spotObj : spotsArray) {
        if (cache.count >= cache.maxSpots) {
            Serial.printf("POTA Spots: Cache full at %d spots\n", cache.maxSpots);
            break;
        }

        // Skip invalid spots
        if (spotObj["invalid"].as<bool>()) {
            continue;
        }

        POTASpot& spot = cache.spots[cache.count];

        spot.spotId = spotObj["spotId"] | 0;
        strlcpy(spot.activator, spotObj["activator"] | "", sizeof(spot.activator));
        strlcpy(spot.frequency, spotObj["frequency"] | "", sizeof(spot.frequency));
        strlcpy(spot.mode, spotObj["mode"] | "", sizeof(spot.mode));
        strlcpy(spot.reference, spotObj["reference"] | "", sizeof(spot.reference));
        strlcpy(spot.parkName, spotObj["name"] | spotObj["parkName"] | "", sizeof(spot.parkName));
        strlcpy(spot.spotTime, spotObj["spotTime"] | "", sizeof(spot.spotTime));
        strlcpy(spot.spotter, spotObj["spotter"] | "", sizeof(spot.spotter));
        strlcpy(spot.comments, spotObj["comments"] | "", sizeof(spot.comments));
        strlcpy(spot.grid4, spotObj["grid4"] | "", sizeof(spot.grid4));
        strlcpy(spot.grid6, spotObj["grid6"] | "", sizeof(spot.grid6));
        spot.latitude = spotObj["latitude"] | 0.0f;
        spot.longitude = spotObj["longitude"] | 0.0f;
        strlcpy(spot.locationDesc, spotObj["locationDesc"] | "", sizeof(spot.locationDesc));
        spot.qsoCount = spotObj["count"] | 0;

        // Convert callsigns to uppercase
        for (int i = 0; spot.activator[i]; i++) {
            spot.activator[i] = toupper(spot.activator[i]);
        }
        for (int i = 0; spot.spotter[i]; i++) {
            spot.spotter[i] = toupper(spot.spotter[i]);
        }

        // Skip QRT spots
        if (isSpotQRT(spot)) {
            continue;
        }

        cache.count++;
    }

    // Free JSON document
    delete doc;

    cache.fetchTime = millis();
    cache.valid = true;
    cache.fetching = false;

    Serial.printf("POTA Spots: Loaded %d spots\n", cache.count);
    Serial.printf("POTA Spots: Free heap after cleanup: %d, PSRAM free: %d\n", ESP.getFreeHeap(), ESP.getFreePsram());

    return cache.count;
}

/**
 * Check if cache needs refresh
 */
bool spotsCacheNeedsRefresh() {
    if (!potaSpotsCache.valid) return true;
    return (millis() - potaSpotsCache.fetchTime) > POTA_REFRESH_INTERVAL;
}

/**
 * Get cache age in minutes
 */
int getCacheAgeMinutes() {
    if (!potaSpotsCache.valid || potaSpotsCache.fetchTime == 0) {
        return -1;
    }
    return (millis() - potaSpotsCache.fetchTime) / 60000;
}

/**
 * Reset filter to default (ALL)
 */
void resetSpotFilter() {
    strcpy(potaSpotFilter.band, "ALL");
    strcpy(potaSpotFilter.mode, "ALL");
    strcpy(potaSpotFilter.region, "ALL");
    potaSpotFilter.active = false;
}

/**
 * Update filter active status
 */
void updateFilterActiveStatus() {
    potaSpotFilter.active = (strcmp(potaSpotFilter.band, "ALL") != 0 ||
                              strcmp(potaSpotFilter.mode, "ALL") != 0 ||
                              strcmp(potaSpotFilter.region, "ALL") != 0);
}

#endif // POTA_SPOTS_H
