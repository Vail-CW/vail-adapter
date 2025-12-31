/*
 * VAIL SUMMIT - Band Conditions API
 * Fetches solar/propagation data from hamqsl.com
 */

#ifndef BAND_CONDITIONS_H
#define BAND_CONDITIONS_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "internet_check.h"

// ============================================
// Band Condition Enums
// ============================================

enum BandCondition {
    BAND_UNKNOWN = 0,
    BAND_CLOSED = 1,
    BAND_POOR = 2,
    BAND_FAIR = 3,
    BAND_GOOD = 4
};

// ============================================
// Data Structures
// ============================================

// HF Band condition (day and night)
struct HFBandData {
    BandCondition day;
    BandCondition night;
};

// VHF Phenomenon (Aurora, E-skip, etc.)
struct VHFPhenomenon {
    char name[24];       // "Aurora", "6m E-Skip NA", etc.
    char location[32];   // "high latance", "north america", etc.
    bool closed;         // true = Band Closed
};

// Solar indices and space weather
struct SolarIndices {
    int solarFlux;           // SFI (typ 70-300)
    int aIndex;              // A-index (0-400)
    int kIndex;              // K-index (0-9)
    char xray[8];            // X-ray class (e.g., "B9.8", "M1.2")
    int sunspots;            // Sunspot count
    float solarWind;         // Solar wind speed (km/s)
    float magneticField;     // Bz magnetic field (nT)
    char geomagField[16];    // "Quiet", "Unsettled", "Storm", etc.
    char signalNoise[12];    // "S2-S3", etc.
    char muf[16];            // Maximum Usable Frequency
    char updated[36];        // Timestamp from XML
};

// Complete band conditions data
struct BandConditionsData {
    bool valid;              // Successfully parsed
    bool fetching;           // Currently fetching
    SolarIndices solar;

    // HF Bands (day/night conditions)
    HFBandData hf_80m_40m;
    HFBandData hf_30m_20m;
    HFBandData hf_17m_15m;
    HFBandData hf_12m_10m;

    // VHF Phenomena
    VHFPhenomenon vhf[12];   // Up to 12 VHF phenomena
    int vhfCount;
};

// Global data instance
static BandConditionsData bandConditionsData;

// ============================================
// XML Parsing Helpers
// ============================================

/*
 * Extract value between XML tags
 * Returns true if found, false otherwise
 */
bool extractXMLValue(const String& xml, const char* tagName, char* buffer, int bufSize) {
    String startTag = String("<") + tagName + ">";
    String endTag = String("</") + tagName + ">";

    int start = xml.indexOf(startTag);
    if (start < 0) return false;
    start += startTag.length();

    int end = xml.indexOf(endTag, start);
    if (end < 0) return false;

    String value = xml.substring(start, end);
    value.trim();
    strlcpy(buffer, value.c_str(), bufSize);
    return true;
}

/*
 * Extract integer value from XML
 */
int extractXMLInt(const String& xml, const char* tagName, int defaultVal = 0) {
    char buffer[16];
    if (extractXMLValue(xml, tagName, buffer, sizeof(buffer))) {
        return atoi(buffer);
    }
    return defaultVal;
}

/*
 * Extract float value from XML
 */
float extractXMLFloat(const String& xml, const char* tagName, float defaultVal = 0.0f) {
    char buffer[16];
    if (extractXMLValue(xml, tagName, buffer, sizeof(buffer))) {
        return atof(buffer);
    }
    return defaultVal;
}

/*
 * Parse band condition text to enum
 */
BandCondition parseBandCondition(const char* text) {
    if (!text || strlen(text) == 0) return BAND_UNKNOWN;

    // Convert to lowercase for comparison
    String lower = String(text);
    lower.toLowerCase();

    if (lower.indexOf("good") >= 0) return BAND_GOOD;
    if (lower.indexOf("fair") >= 0) return BAND_FAIR;
    if (lower.indexOf("poor") >= 0) return BAND_POOR;
    if (lower.indexOf("closed") >= 0 || lower.indexOf("close") >= 0) return BAND_CLOSED;

    return BAND_UNKNOWN;
}

/*
 * Extract HF band condition from calculatedconditions section
 * XML format: <band name="80m-40m" time="day">Good</band>
 */
bool extractHFBand(const String& xml, const char* bandName, const char* timeOfDay, BandCondition& condition) {
    // Build search pattern: name="80m-40m" time="day"
    String searchPattern = String("name=\"") + bandName + "\" time=\"" + timeOfDay + "\"";

    int pos = xml.indexOf(searchPattern);
    if (pos < 0) return false;

    // Find the closing > of this tag
    int tagEnd = xml.indexOf(">", pos);
    if (tagEnd < 0) return false;

    // Find the </band> closing tag
    int valueEnd = xml.indexOf("</band>", tagEnd);
    if (valueEnd < 0) return false;

    // Extract the value between > and </band>
    String value = xml.substring(tagEnd + 1, valueEnd);
    value.trim();

    condition = parseBandCondition(value.c_str());
    return true;
}

/*
 * Extract VHF phenomena from calculatedvhfconditions section
 * XML format: <phenomenon name="Aurora" location="high latance">Band Closed</phenomenon>
 */
int extractVHFPhenomena(const String& xml, VHFPhenomenon* phenomena, int maxCount) {
    int count = 0;
    int searchPos = 0;

    while (count < maxCount) {
        // Find next <phenomenon tag
        int tagStart = xml.indexOf("<phenomenon ", searchPos);
        if (tagStart < 0) break;

        // Find name attribute
        int nameStart = xml.indexOf("name=\"", tagStart);
        if (nameStart < 0) break;
        nameStart += 6;

        int nameEnd = xml.indexOf("\"", nameStart);
        if (nameEnd < 0) break;

        // Find location attribute
        int locStart = xml.indexOf("location=\"", tagStart);
        int locEnd = -1;
        if (locStart > 0 && locStart < tagStart + 200) {
            locStart += 10;
            locEnd = xml.indexOf("\"", locStart);
        }

        // Find the value (between > and </phenomenon>)
        int valueStart = xml.indexOf(">", tagStart);
        if (valueStart < 0) break;
        valueStart++;

        int valueEnd = xml.indexOf("</phenomenon>", valueStart);
        if (valueEnd < 0) break;

        // Extract values
        String name = xml.substring(nameStart, nameEnd);
        String location = (locEnd > 0) ? xml.substring(locStart, locEnd) : "";
        String value = xml.substring(valueStart, valueEnd);
        value.trim();

        // Store in struct
        strlcpy(phenomena[count].name, name.c_str(), sizeof(phenomena[count].name));
        strlcpy(phenomena[count].location, location.c_str(), sizeof(phenomena[count].location));
        phenomena[count].closed = (value.indexOf("Closed") >= 0);

        count++;
        searchPos = valueEnd + 13; // Move past </phenomenon>
    }

    return count;
}

/*
 * Parse complete XML response into BandConditionsData
 */
bool parseXMLResponse(const String& xml, BandConditionsData& data) {
    // Reset data
    memset(&data, 0, sizeof(BandConditionsData));

    // Extract solar indices
    data.solar.solarFlux = extractXMLInt(xml, "solarflux");
    data.solar.aIndex = extractXMLInt(xml, "aindex");
    data.solar.kIndex = extractXMLInt(xml, "kindex");
    data.solar.sunspots = extractXMLInt(xml, "sunspots");
    data.solar.solarWind = extractXMLFloat(xml, "solarwind");
    data.solar.magneticField = extractXMLFloat(xml, "magneticfield");

    extractXMLValue(xml, "xray", data.solar.xray, sizeof(data.solar.xray));
    extractXMLValue(xml, "geomagfield", data.solar.geomagField, sizeof(data.solar.geomagField));
    extractXMLValue(xml, "signalnoise", data.solar.signalNoise, sizeof(data.solar.signalNoise));
    extractXMLValue(xml, "muf", data.solar.muf, sizeof(data.solar.muf));
    extractXMLValue(xml, "updated", data.solar.updated, sizeof(data.solar.updated));

    // Extract HF band conditions
    extractHFBand(xml, "80m-40m", "day", data.hf_80m_40m.day);
    extractHFBand(xml, "80m-40m", "night", data.hf_80m_40m.night);
    extractHFBand(xml, "30m-20m", "day", data.hf_30m_20m.day);
    extractHFBand(xml, "30m-20m", "night", data.hf_30m_20m.night);
    extractHFBand(xml, "17m-15m", "day", data.hf_17m_15m.day);
    extractHFBand(xml, "17m-15m", "night", data.hf_17m_15m.night);
    extractHFBand(xml, "12m-10m", "day", data.hf_12m_10m.day);
    extractHFBand(xml, "12m-10m", "night", data.hf_12m_10m.night);

    // Extract VHF phenomena
    data.vhfCount = extractVHFPhenomena(xml, data.vhf, 12);

    // Mark as valid if we got at least solar flux
    data.valid = (data.solar.solarFlux > 0);

    Serial.printf("[BandConditions] Parsed: SFI=%d, A=%d, K=%d, VHF phenomena=%d\n",
        data.solar.solarFlux, data.solar.aIndex, data.solar.kIndex, data.vhfCount);

    return data.valid;
}

// ============================================
// HTTP Fetch Function
// ============================================

#define BAND_CONDITIONS_URL "https://www.hamqsl.com/solarxml.php"

/*
 * Fetch band conditions from hamqsl.com
 * Returns true if successful, false otherwise
 */
bool fetchBandConditions(BandConditionsData& data) {
    data.fetching = true;
    data.valid = false;

    // Check internet connectivity (not just WiFi association)
    InternetStatus inetStatus = getInternetStatus();
    if (inetStatus != INET_CONNECTED) {
        if (inetStatus == INET_WIFI_ONLY) {
            Serial.println("[BandConditions] WiFi connected but no internet");
        } else {
            Serial.println("[BandConditions] No WiFi connection");
        }
        data.fetching = false;
        return false;
    }

    Serial.println("[BandConditions] Fetching from hamqsl.com...");

    HTTPClient http;
    http.begin(BAND_CONDITIONS_URL);
    http.setTimeout(10000);  // 10 second timeout

    int httpCode = http.GET();

    if (httpCode == 200) {
        String xml = http.getString();
        Serial.printf("[BandConditions] Received %d bytes\n", xml.length());

        bool success = parseXMLResponse(xml, data);
        http.end();
        data.fetching = false;
        return success;
    } else {
        Serial.printf("[BandConditions] HTTP error: %d\n", httpCode);
        http.end();
        data.fetching = false;
        return false;
    }
}

// ============================================
// Helper Functions
// ============================================

/*
 * Get color for band condition (returns LVGL-compatible hex)
 */
uint32_t getBandConditionColorHex(BandCondition cond) {
    switch (cond) {
        case BAND_GOOD:    return 0x00FF00;  // Green
        case BAND_FAIR:    return 0xFFFF00;  // Yellow
        case BAND_POOR:    return 0xFF8C00;  // Orange
        case BAND_CLOSED:  return 0xFF0000;  // Red
        default:           return 0x808080;  // Gray for unknown
    }
}

/*
 * Get short text for band condition
 */
const char* getBandConditionText(BandCondition cond) {
    switch (cond) {
        case BAND_GOOD:    return "Good";
        case BAND_FAIR:    return "Fair";
        case BAND_POOR:    return "Poor";
        case BAND_CLOSED:  return "Clsd";
        default:           return "---";
    }
}

/*
 * Get geomagnetic field color
 */
uint32_t getGeomagColorHex(const char* field) {
    String f = String(field);
    f.toLowerCase();

    if (f.indexOf("quiet") >= 0) return 0x00FF00;      // Green
    if (f.indexOf("unsettl") >= 0) return 0xFFFF00;    // Yellow
    if (f.indexOf("active") >= 0) return 0xFF8C00;     // Orange
    if (f.indexOf("storm") >= 0) return 0xFF0000;      // Red
    return 0x808080;  // Gray for unknown
}

/*
 * Get K-index color (0-9 scale)
 */
uint32_t getKIndexColorHex(int k) {
    if (k <= 1) return 0x00FF00;       // Green - Quiet
    if (k <= 3) return 0xFFFF00;       // Yellow - Unsettled
    if (k <= 4) return 0xFF8C00;       // Orange - Active
    return 0xFF0000;                   // Red - Storm
}

#endif // BAND_CONDITIONS_H
