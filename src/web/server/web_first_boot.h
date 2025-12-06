/*
 * Web First Boot Handler
 * Prompts user to download web files on first WiFi connection
 */

#ifndef WEB_FIRST_BOOT_H
#define WEB_FIRST_BOOT_H

#include <Arduino.h>
#include <Preferences.h>
#include "../../core/config.h"
#include "../../storage/sd_card.h"
#include "web_file_downloader.h"

// First boot state tracking
bool webFilesDownloadPromptShown = false;
bool webFilesDownloading = false;
bool webFilesPromptPending = false;  // Flag set by WiFi event, checked by main loop

// Preferences for tracking first boot
Preferences webFilesPrefs;

// Forward declaration of display functions (implemented in main sketch)
extern void beep(int frequency, int duration);
extern LGFX tft;

// ============================================
// First Boot Detection
// ============================================

/**
 * Check if this is first WiFi connection with SD card but no web files
 * @return true if should prompt for web files download
 */
bool shouldPromptForWebFilesDownload() {
  // Don't prompt if already shown this session
  if (webFilesDownloadPromptShown) {
    return false;
  }

  // Must have WiFi connected
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  // Check if SD card is available
  if (!sdCardAvailable) {
    // Try to initialize SD card
    if (!initSDCard()) {
      Serial.println("No SD card available - skipping web files prompt");
      return false;
    }
  }

  // Check if web files already exist
  String indexPath = String(WEB_FILES_PATH) + "index.html";
  if (SD.exists(indexPath.c_str())) {
    Serial.println("Web files already exist on SD card");
    return false;
  }

  // Check if user has declined before (stored in preferences)
  webFilesPrefs.begin("webfiles", true);
  bool declined = webFilesPrefs.getBool("declined", false);
  webFilesPrefs.end();

  if (declined) {
    Serial.println("User previously declined web files download");
    return false;
  }

  return true;
}

/**
 * Mark that user declined the download prompt
 */
void declineWebFilesDownload() {
  webFilesPrefs.begin("webfiles", false);
  webFilesPrefs.putBool("declined", true);
  webFilesPrefs.end();
  webFilesDownloadPromptShown = true;
}

/**
 * Reset the declined flag (e.g., when user manually deletes web files)
 */
void resetWebFilesDeclined() {
  webFilesPrefs.begin("webfiles", false);
  webFilesPrefs.putBool("declined", false);
  webFilesPrefs.end();
}

// ============================================
// LCD Prompt UI
// ============================================

/**
 * Draw the download prompt on LCD
 */
void drawWebFilesPrompt() {
  // Clear screen
  tft.fillScreen(COLOR_BACKGROUND);

  // Header
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);

  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(tft, "Web Interface Setup", 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(centerX, 40);
  tft.print("Web Interface Setup");

  // Info box
  tft.drawRect(20, 80, SCREEN_WIDTH - 40, 100, ST77XX_CYAN);
  tft.fillRect(22, 82, SCREEN_WIDTH - 44, 96, 0x0841);  // Dark blue background

  // Message
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(30, 95);
  tft.print("SD card detected but web interface");
  tft.setCursor(30, 110);
  tft.print("files are missing.");

  tft.setCursor(30, 130);
  tft.print("Would you like to download the web");
  tft.setCursor(30, 145);
  tft.print("interface files from the internet?");
  tft.setCursor(30, 160);
  tft.print("(Requires active WiFi connection)");

  // Buttons
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING);

  String footer = "Y: Download Now   N: Skip (don't ask again)";
  getTextBounds_compat(tft, footer.c_str(), 0, 0, &x1, &y1, &w, &h);
  centerX = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(centerX, SCREEN_HEIGHT - 30);
  tft.print(footer);
}

/**
 * Draw download progress on LCD
 */
void drawWebFilesProgress() {
  // Clear progress area
  tft.fillRect(20, 80, SCREEN_WIDTH - 40, 120, COLOR_BACKGROUND);

  // Progress box
  tft.drawRect(20, 80, SCREEN_WIDTH - 40, 120, ST77XX_CYAN);
  tft.fillRect(22, 82, SCREEN_WIDTH - 44, 116, 0x0841);

  // Status text
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(30, 95);
  tft.print("Downloading web interface files...");

  // Current file
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(30, 115);
  String fileText = "File " + String(webDownloadProgress.currentFile) + "/" +
                    String(webDownloadProgress.totalFiles);
  tft.print(fileText);

  tft.setCursor(30, 130);
  String fileName = webDownloadProgress.currentFileName;
  if (fileName.length() > 35) {
    fileName = fileName.substring(0, 32) + "...";
  }
  tft.print(fileName);

  // Progress bar
  int barWidth = SCREEN_WIDTH - 80;
  int barX = 40;
  int barY = 155;
  int barHeight = 20;

  // Background
  tft.drawRect(barX, barY, barWidth, barHeight, ST77XX_WHITE);

  // Fill
  int progress = 0;
  if (webDownloadProgress.totalFiles > 0) {
    progress = (webDownloadProgress.currentFile * 100) / webDownloadProgress.totalFiles;
  }
  int fillWidth = (barWidth - 4) * progress / 100;
  tft.fillRect(barX + 2, barY + 2, fillWidth, barHeight - 4, ST77XX_GREEN);

  // Percentage
  tft.setTextColor(ST77XX_WHITE);
  int16_t x1, y1;
  uint16_t w, h;
  String pctText = String(progress) + "%";
  getTextBounds_compat(tft, pctText.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(barX + (barWidth - w) / 2, barY + 5);
  tft.print(pctText);

  // Cancel hint
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(30, 185);
  tft.print("Press ESC to cancel");
}

/**
 * Draw download complete message
 */
void drawWebFilesComplete(bool success) {
  tft.fillRect(20, 80, SCREEN_WIDTH - 40, 120, COLOR_BACKGROUND);

  tft.drawRect(20, 80, SCREEN_WIDTH - 40, 120, success ? ST77XX_GREEN : ST77XX_RED);
  tft.fillRect(22, 82, SCREEN_WIDTH - 44, 116, 0x0841);

  tft.setTextSize(2);
  tft.setTextColor(success ? ST77XX_GREEN : ST77XX_RED);

  int16_t x1, y1;
  uint16_t w, h;
  String title = success ? "Download Complete!" : "Download Failed";
  getTextBounds_compat(tft, title.c_str(), 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(centerX, 105);
  tft.print(title);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);

  if (success) {
    tft.setCursor(30, 140);
    tft.print("Web interface is now available at:");
    tft.setCursor(30, 155);
    tft.setTextColor(ST77XX_CYAN);
    tft.print("http://vail-summit.local");
  } else {
    tft.setCursor(30, 140);
    tft.print("Error: ");
    tft.print(webDownloadProgress.errorMessage);
    tft.setCursor(30, 160);
    tft.print("You can try again via Settings menu");
  }

  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(30, 185);
  tft.print("Press any key to continue...");
}

// ============================================
// Main First Boot Handler
// ============================================

/**
 * Show first boot download prompt and handle user input
 * This is a blocking function that waits for user response
 * @param getKey Function pointer to get keyboard input
 * @return true if download was started (or completed)
 */
bool handleFirstBootWebFilesPrompt(char (*getKey)()) {
  if (!shouldPromptForWebFilesDownload()) {
    return false;
  }

  webFilesDownloadPromptShown = true;

  // Play notification sound
  beep(TONE_MENU_NAV, BEEP_MEDIUM);

  // Draw prompt
  drawWebFilesPrompt();

  // Wait for user input
  while (true) {
    char key = getKey();

    if (key == 'y' || key == 'Y') {
      // User wants to download
      beep(TONE_SELECT, BEEP_MEDIUM);

      // Start download
      webFilesDownloading = true;
      drawWebFilesProgress();

      // Download in chunks, updating UI periodically
      bool downloadStarted = false;

      // Start the download in the background would be ideal,
      // but for simplicity we'll do it synchronously with UI updates

      // Create a task or use the existing download function
      bool success = downloadWebFilesFromGitHub();

      webFilesDownloading = false;

      // Show result
      drawWebFilesComplete(success);
      beep(success ? TONE_SELECT : TONE_ERROR, BEEP_LONG);

      // Wait for any key
      while (true) {
        key = getKey();
        if (key != 0) break;
        delay(50);
      }

      return true;
    }
    else if (key == 'n' || key == 'N' || key == KEY_ESC) {
      // User declined
      beep(TONE_MENU_NAV, BEEP_SHORT);
      declineWebFilesDownload();
      return false;
    }

    // Update progress if downloading
    if (webFilesDownloading) {
      drawWebFilesProgress();

      // Check for ESC to cancel
      if (key == KEY_ESC) {
        cancelWebFileDownload();
        beep(TONE_ERROR, BEEP_MEDIUM);
      }
    }

    delay(50);  // Small delay to prevent busy loop
  }

  return false;
}

/**
 * Check and set flag if web files prompt should be shown (non-blocking version)
 * Call this after WiFi connects - sets a flag for the main loop to handle
 */
void checkAndShowWebFilesPrompt() {
  // This is called from WiFi event handler, so we just set a flag
  // The main loop will check this flag and show the prompt with keyboard access
  if (shouldPromptForWebFilesDownload()) {
    Serial.println("Web files missing - setting prompt flag for main loop");
    webFilesPromptPending = true;
  }
}

/**
 * Check if web files prompt is pending (call from main loop)
 * @return true if prompt should be shown
 */
bool isWebFilesPromptPending() {
  return webFilesPromptPending && !webFilesDownloadPromptShown;
}

/**
 * Clear the pending prompt flag
 */
void clearWebFilesPromptPending() {
  webFilesPromptPending = false;
}

#endif // WEB_FIRST_BOOT_H
