/*
 * VAIL SUMMIT - Internet Connectivity Check
 *
 * Provides actual internet connectivity verification beyond WiFi association.
 * WiFi.status() == WL_CONNECTED only indicates association with access point,
 * not actual internet reachability. This module verifies internet access
 * using lightweight HTTP checks.
 */

#ifndef INTERNET_CHECK_H
#define INTERNET_CHECK_H

#include <WiFi.h>
#include <HTTPClient.h>

// Connectivity states
enum InternetStatus {
    INET_DISCONNECTED = 0,    // WiFi not connected
    INET_WIFI_ONLY = 1,       // WiFi connected, no internet
    INET_CONNECTED = 2        // Full connectivity
};

// Configuration
#define INET_CHECK_INTERVAL_SUCCESS   60000   // Check every 60s when working
#define INET_CHECK_INTERVAL_FAIL      15000   // Check every 15s when failing
#define INET_CHECK_TIMEOUT            5000    // 5 second HTTP timeout
#define INET_RECONNECT_COOLDOWN       30000   // Wait 30s between reconnect attempts
#define INET_MAX_CONSECUTIVE_FAILS    3       // Trigger reconnect after 3 fails

// Lightweight check endpoints (return small responses)
// Use multiple to avoid single point of failure
static const char* INET_CHECK_URLS[] = {
    "http://clients3.google.com/generate_204",      // Google connectivity check
    "http://connectivitycheck.gstatic.com/generate_204",  // Android check
    "http://captive.apple.com/hotspot-detect.html"  // Apple check
};
#define INET_CHECK_URL_COUNT 3

// State tracking
static InternetStatus internetStatus = INET_DISCONNECTED;
static unsigned long lastInternetCheck = 0;
static unsigned long lastReconnectAttempt = 0;
static int consecutiveCheckFails = 0;
static int currentCheckUrlIndex = 0;
static bool internetCheckEnabled = true;  // Can be disabled during audio-critical modes

// Forward declarations
InternetStatus getInternetStatus();
void updateInternetStatus();
bool checkInternetConnectivity();
void triggerWiFiReconnect();
void setInternetCheckEnabled(bool enabled);
void forceInternetCheck();
bool isInternetAvailable();

/*
 * Get current internet connectivity status
 * Called frequently by UI - returns cached value
 */
InternetStatus getInternetStatus() {
    return internetStatus;
}

/*
 * Enable or disable internet checking
 * Disable during audio-critical modes to prevent glitches
 */
void setInternetCheckEnabled(bool enabled) {
    internetCheckEnabled = enabled;
}

/*
 * Check if internet is actually reachable (blocking, use sparingly)
 * Uses HTTP 204 response check - very lightweight
 */
bool checkInternetConnectivity() {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    HTTPClient http;
    http.setTimeout(INET_CHECK_TIMEOUT);

    // Rotate through check URLs to avoid single point of failure
    const char* url = INET_CHECK_URLS[currentCheckUrlIndex];
    currentCheckUrlIndex = (currentCheckUrlIndex + 1) % INET_CHECK_URL_COUNT;

    bool success = false;

    if (http.begin(url)) {
        int httpCode = http.GET();
        http.end();

        // 204 = success for generate_204 endpoints
        // 200 = success for Apple endpoint
        if (httpCode == 204 || httpCode == 200) {
            Serial.printf("[InetCheck] Success via %s (HTTP %d)\n", url, httpCode);
            success = true;
        } else {
            Serial.printf("[InetCheck] Failed via %s (HTTP %d)\n", url, httpCode);
        }
    } else {
        Serial.printf("[InetCheck] Failed to begin HTTP connection to %s\n", url);
    }

    return success;
}

/*
 * Trigger WiFi reconnection (full disconnect/reconnect cycle)
 * Called when internet check fails repeatedly
 */
void triggerWiFiReconnect() {
    unsigned long now = millis();

    // Cooldown to prevent rapid reconnect attempts
    if (now - lastReconnectAttempt < INET_RECONNECT_COOLDOWN) {
        Serial.println("[InetCheck] Reconnect on cooldown, skipping");
        return;
    }

    lastReconnectAttempt = now;
    Serial.println("[InetCheck] Triggering WiFi reconnect...");

    // Full disconnect/reconnect cycle
    WiFi.disconnect(true);
    delay(100);
    WiFi.reconnect();

    // Reset fail counter after reconnect attempt
    consecutiveCheckFails = 0;
}

/*
 * Update internet connectivity status (non-blocking pattern)
 * Call this from main loop - manages timing internally
 */
void updateInternetStatus() {
    // Skip if checking is disabled (e.g., during audio playback)
    if (!internetCheckEnabled) {
        return;
    }

    unsigned long now = millis();

    // Determine check interval based on current status
    unsigned long checkInterval = (internetStatus == INET_CONNECTED)
        ? INET_CHECK_INTERVAL_SUCCESS
        : INET_CHECK_INTERVAL_FAIL;

    // Check if it's time for a connectivity check
    if (now - lastInternetCheck < checkInterval) {
        // Not time yet - just update basic WiFi status
        if (WiFi.status() != WL_CONNECTED) {
            internetStatus = INET_DISCONNECTED;
            consecutiveCheckFails = 0;
        }
        return;
    }

    lastInternetCheck = now;

    // If WiFi is disconnected, no internet check needed
    if (WiFi.status() != WL_CONNECTED) {
        internetStatus = INET_DISCONNECTED;
        consecutiveCheckFails = 0;
        return;
    }

    // Perform internet connectivity check
    if (checkInternetConnectivity()) {
        internetStatus = INET_CONNECTED;
        consecutiveCheckFails = 0;
        Serial.println("[InetCheck] Internet connectivity confirmed");
    } else {
        internetStatus = INET_WIFI_ONLY;
        consecutiveCheckFails++;
        Serial.printf("[InetCheck] No internet (fail #%d)\n", consecutiveCheckFails);

        // Trigger reconnect after multiple failures
        if (consecutiveCheckFails >= INET_MAX_CONSECUTIVE_FAILS) {
            triggerWiFiReconnect();
        }
    }
}

/*
 * Force an immediate internet check (for use after user actions)
 */
void forceInternetCheck() {
    lastInternetCheck = 0;  // Reset timer to force immediate check on next update
}

/*
 * Check if internet is available (convenience function)
 */
bool isInternetAvailable() {
    return internetStatus == INET_CONNECTED;
}

/*
 * Get status as human-readable string (for debugging)
 */
const char* getInternetStatusString() {
    switch (internetStatus) {
        case INET_DISCONNECTED: return "Disconnected";
        case INET_WIFI_ONLY: return "WiFi Only";
        case INET_CONNECTED: return "Connected";
        default: return "Unknown";
    }
}

#endif // INTERNET_CHECK_H
