// POTA API Integration Module
// Interfaces with Parks on the Air public API

#ifndef POTA_API_H
#define POTA_API_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "internet_check.h"

// ============================================
// POTA Park Data Structure
// ============================================

struct POTAPark {
  char reference[11];       // Park reference (e.g., "K-0817")
  char name[61];            // Park name
  char locationDesc[41];    // Location description (e.g., "IN, US")
  char grid4[5];            // 4-character grid square
  char grid6[7];            // 6-character grid square
  float latitude;
  float longitude;
  bool valid;               // Successfully loaded from API
};

// ============================================
// POTA API Functions
// ============================================

/**
 * Lookup a POTA park by reference
 * @param reference Park reference (e.g., "K-0817")
 * @param park Output structure to fill
 * @return true if successful, false if failed or no WiFi
 */
bool lookupPOTAPark(const char* reference, POTAPark& park) {
  // Initialize park as invalid
  park.valid = false;
  strlcpy(park.reference, reference, sizeof(park.reference));

  // Check internet connectivity (not just WiFi association)
  InternetStatus inetStatus = getInternetStatus();
  if (inetStatus != INET_CONNECTED) {
    if (inetStatus == INET_WIFI_ONLY) {
      Serial.println("POTA API: WiFi connected but no internet");
    } else {
      Serial.println("POTA API: No WiFi connection");
    }
    return false;
  }

  // Build URL
  String url = "https://api.pota.app/park/";
  url += reference;

  Serial.print("POTA API: Looking up ");
  Serial.println(url);

  HTTPClient http;
  http.begin(url);
  http.setTimeout(5000);  // 5 second timeout

  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    Serial.print("POTA API Response: ");
    Serial.println(payload.substring(0, 200));

    // Parse JSON
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      // Extract park data
      strlcpy(park.name, doc["name"] | "", sizeof(park.name));
      strlcpy(park.locationDesc, doc["locationDesc"] | "", sizeof(park.locationDesc));
      strlcpy(park.grid4, doc["grid4"] | "", sizeof(park.grid4));
      strlcpy(park.grid6, doc["grid6"] | "", sizeof(park.grid6));
      park.latitude = doc["latitude"] | 0.0f;
      park.longitude = doc["longitude"] | 0.0f;
      park.valid = true;

      Serial.print("POTA API: Success - ");
      Serial.print(park.name);
      Serial.print(" @ ");
      Serial.println(park.grid6);

      http.end();
      return true;
    } else {
      Serial.print("POTA API: JSON parse error - ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.print("POTA API: HTTP error ");
    Serial.println(httpCode);
  }

  http.end();
  return false;
}

/**
 * Validate POTA reference format
 * Format: Prefix + dash + number (e.g., "K-0817", "US-2256", "VE-1234")
 * Prefix can be letters or alphanumeric (e.g., US, K, G, VE, etc.)
 * @param reference Reference to validate
 * @return true if format is valid
 */
bool validatePOTAReference(const char* reference) {
  int len = strlen(reference);
  if (len < 3 || len > 12) return false;  // Too short or too long

  // Find dash
  const char* dash = strchr(reference, '-');
  if (!dash) return false;  // No dash

  // Check prefix (1-4 alphanumeric characters before dash)
  int prefixLen = dash - reference;
  if (prefixLen < 1 || prefixLen > 4) return false;

  for (int i = 0; i < prefixLen; i++) {
    if (!isalnum(reference[i])) return false;  // Allow letters and digits
  }

  // Check suffix (1-5 digits after dash)
  int suffixLen = len - prefixLen - 1;
  if (suffixLen < 1 || suffixLen > 5) return false;

  for (int i = prefixLen + 1; i < len; i++) {
    if (!isdigit(reference[i])) return false;
  }

  return true;
}

// Note: validateGridSquare() is defined in qso_logger_validation.h

#endif // POTA_API_H
