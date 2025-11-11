/*
 * NTP Time Synchronization
 * Gets accurate UTC time from internet time servers
 */

#ifndef NTP_TIME_H
#define NTP_TIME_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

// ============================================
// NTP Configuration
// ============================================

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;           // UTC offset (0 for UTC)
const int   daylightOffset_sec = 0;      // No daylight saving for UTC

bool ntpSynced = false;
time_t lastNtpSync = 0;

// ============================================
// NTP Functions
// ============================================

/*
 * Initialize NTP time sync
 * Call this after WiFi is connected
 */
void initNTPTime() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping NTP sync");
    ntpSynced = false;
    return;
  }

  Serial.println("Syncing time with NTP server...");

  // Configure time with NTP server
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Wait up to 5 seconds for time sync
  int attempts = 0;
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo) && attempts < 10) {
    Serial.print(".");
    delay(500);
    attempts++;
  }

  if (attempts < 10) {
    Serial.println("\nNTP time synced successfully!");
    Serial.print("Current UTC time: ");
    Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
    ntpSynced = true;
    lastNtpSync = time(nullptr);
  } else {
    Serial.println("\nNTP sync failed - will use millis() approximation");
    ntpSynced = false;
  }
}

/*
 * Get current date/time string in UTC
 * Returns: "YYYYMMDD HHMM" format
 */
String getNTPDateTime() {
  if (!ntpSynced) {
    // Fallback to millis-based time
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = (minutes / 60) % 24;
    minutes = minutes % 60;

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "20250428 %02lu%02lu", hours, minutes);
    return String(buffer);
  }

  // Get time from NTP
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to get time");
    return "20250428 0000";
  }

  // Format: YYYYMMDD HHMM
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%04d%02d%02d %02d%02d",
           timeinfo.tm_year + 1900,
           timeinfo.tm_mon + 1,
           timeinfo.tm_mday,
           timeinfo.tm_hour,
           timeinfo.tm_min);

  return String(buffer);
}

/*
 * Get just the date in YYYYMMDD format
 */
String getNTPDate() {
  String dateTime = getNTPDateTime();
  return dateTime.substring(0, 8);
}

/*
 * Get just the time in HHMM format
 */
String getNTPTime() {
  String dateTime = getNTPDateTime();
  return dateTime.substring(9, 13);
}

/*
 * Check if NTP time is synced and not too old
 * Re-sync if more than 24 hours old
 */
bool isNTPTimeCurrent() {
  if (!ntpSynced) {
    return false;
  }

  time_t now = time(nullptr);
  time_t age = now - lastNtpSync;

  // Re-sync if older than 24 hours (86400 seconds)
  if (age > 86400) {
    Serial.println("NTP time is stale, re-syncing...");
    initNTPTime();
  }

  return ntpSynced;
}

/*
 * Get human-readable time string for display
 */
String getTimeString() {
  if (!ntpSynced) {
    return "Time not synced";
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Time error";
  }

  char buffer[30];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d UTC",
           timeinfo.tm_year + 1900,
           timeinfo.tm_mon + 1,
           timeinfo.tm_mday,
           timeinfo.tm_hour,
           timeinfo.tm_min);

  return String(buffer);
}

#endif // NTP_TIME_H
