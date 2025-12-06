// QSO Logger View Module
// Handles viewing saved QSO logs on-device with list and detail views

#ifndef QSO_LOGGER_VIEW_H
#define QSO_LOGGER_VIEW_H

#include <Arduino.h>
#include "../core/config.h"
#include "qso_logger.h"  // Same folder
#include "qso_logger_storage.h"  // Same folder

// View state
enum ViewMode {
  VIEW_MODE_LIST = 0,
  VIEW_MODE_DETAIL = 1,
  VIEW_MODE_DELETE_CONFIRM = 2
};

struct ViewState {
  ViewMode mode;
  int selectedIndex;        // Currently selected QSO in list
  int scrollOffset;         // Top visible item in list
  int totalQSOs;           // Total number of QSOs loaded
  QSO* qsos;               // Array of loaded QSOs (dynamically allocated)
  int detailScrollOffset;  // Scroll position in detail view
  bool deleteConfirm;      // Waiting for delete confirmation
};

ViewState viewState;

// Constants
#define MAX_VISIBLE_LIST_ITEMS 6
#define LIST_ITEM_HEIGHT 30
#define LIST_START_Y 48
#define DETAIL_START_Y 48
#define DETAIL_LINE_HEIGHT 20

// Forward declarations
void loadQSOsForView();
void freeQSOsFromView();
void drawListView(LGFX& tft);
void drawDetailView(LGFX& tft);
void scrollListUp();
void scrollListDown();
void scrollDetailUp();
void scrollDetailDown();

// Initialize view state and load QSOs
void startViewLogs(LGFX& tft) {
  Serial.println("Starting View Logs mode");

  viewState.mode = VIEW_MODE_LIST;
  viewState.selectedIndex = 0;
  viewState.scrollOffset = 0;
  viewState.detailScrollOffset = 0;
  viewState.totalQSOs = 0;
  viewState.qsos = nullptr;
  viewState.deleteConfirm = false;

  // Load all QSOs from storage
  loadQSOsForView();

  // Draw initial UI
  tft.fillScreen(COLOR_BACKGROUND);
  drawListView(tft);
}

// Load all QSOs from storage into memory
void loadQSOsForView() {
  Serial.println("Loading QSOs for view...");

  // First, count total QSOs
  int totalCount = 0;
  File root = SD.open("/logs");
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open /logs directory");
    return;
  }

  // Count QSOs across all daily log files
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String filename = String(file.name());
      Serial.print("Checking file: ");
      Serial.println(filename);

      // Extract just the filename from full path if needed
      int lastSlash = filename.lastIndexOf('/');
      if (lastSlash >= 0) {
        filename = filename.substring(lastSlash + 1);
      }

      Serial.print("  Basename: ");
      Serial.println(filename);

      if (filename.startsWith("qso_") && filename.endsWith(".json")) {
        Serial.println("  -> Loading this file");
        // Load this file and count entries
        File logFile = SD.open(file.path(), "r");
        if (logFile) {
          String content = logFile.readString();
          logFile.close();

          Serial.print("    File content length: ");
          Serial.println(content.length());
          Serial.print("    First 200 chars: ");
          Serial.println(content.substring(0, min(200, (int)content.length())));

          StaticJsonDocument<8192> doc;
          DeserializationError error = deserializeJson(doc, content);

          Serial.print("    JSON parse result: ");
          Serial.println(error.c_str());
          Serial.print("    Contains 'logs' key: ");
          Serial.println(doc.containsKey("logs") ? "YES" : "NO");

          if (!error && doc.containsKey("logs")) {
            JsonArray qsos = doc["logs"].as<JsonArray>();
            int qsoCount = qsos.size();
            Serial.print("    Found ");
            Serial.print(qsoCount);
            Serial.println(" QSOs in this file");
            totalCount += qsoCount;
          } else {
            Serial.print("    JSON error or no qsos key: ");
            Serial.println(error.c_str());
          }
        }
      }
    }
    file = root.openNextFile();
  }
  root.close();

  Serial.print("Total QSOs found: ");
  Serial.println(totalCount);

  if (totalCount == 0) {
    viewState.totalQSOs = 0;
    return;
  }

  // Allocate memory for QSOs
  viewState.qsos = new QSO[totalCount];
  viewState.totalQSOs = totalCount;

  // Load QSOs into array
  int qsoIndex = 0;
  root = SD.open("/logs");
  file = root.openNextFile();

  while (file && qsoIndex < totalCount) {
    if (!file.isDirectory()) {
      String filename = String(file.name());

      // Extract just the filename from full path if needed
      int lastSlash = filename.lastIndexOf('/');
      if (lastSlash >= 0) {
        filename = filename.substring(lastSlash + 1);
      }

      if (filename.startsWith("qso_") && filename.endsWith(".json")) {
        File logFile = SD.open(file.path(), "r");
        if (logFile) {
          String content = logFile.readString();
          logFile.close();

          StaticJsonDocument<8192> doc;
          DeserializationError error = deserializeJson(doc, content);

          if (!error && doc.containsKey("logs")) {
            JsonArray qsos = doc["logs"].as<JsonArray>();
            for (JsonObject qsoObj : qsos) {
              if (qsoIndex >= totalCount) break;

              // Use the standard jsonToQso function to load all fields
              jsonToQso(qsoObj, viewState.qsos[qsoIndex]);

              qsoIndex++;
            }
          }
        }
      }
    }
    file = root.openNextFile();
  }
  root.close();

  Serial.print("Loaded ");
  Serial.print(qsoIndex);
  Serial.println(" QSOs into memory");
}

// Free QSO memory when exiting view
void freeQSOsFromView() {
  if (viewState.qsos != nullptr) {
    delete[] viewState.qsos;
    viewState.qsos = nullptr;
  }
  viewState.totalQSOs = 0;
}

// Draw list view with scrollable QSOs
void drawListView(LGFX& tft) {
  // Header
  tft.fillRect(0, 0, SCREEN_WIDTH, 40, 0x1082); // Dark blue header
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 12);
  tft.print("View Logs");

  // Show count
  tft.setTextSize(1);
  tft.setCursor(250, 18);
  tft.print(viewState.totalQSOs);
  tft.print(" QSOs");

  // Clear content area
  tft.fillRect(0, 40, SCREEN_WIDTH, SCREEN_HEIGHT - 40 - 20, COLOR_BACKGROUND);

  if (viewState.totalQSOs == 0) {
    // No QSOs message
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(60, 120);
    tft.print("No logs found");

    // Footer
    tft.fillRect(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH, 20, COLOR_BACKGROUND);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_WARNING);
    tft.setCursor(10, SCREEN_HEIGHT - 16);
    tft.print("ESC Back");
    return;
  }

  // Draw visible list items
  int maxVisible = min(MAX_VISIBLE_LIST_ITEMS, viewState.totalQSOs);

  for (int i = 0; i < maxVisible; i++) {
    int qsoIndex = viewState.scrollOffset + i;
    if (qsoIndex >= viewState.totalQSOs) break;

    QSO& qso = viewState.qsos[qsoIndex];
    int y = LIST_START_Y + (i * LIST_ITEM_HEIGHT);

    // Highlight selected item
    bool isSelected = (qsoIndex == viewState.selectedIndex);
    uint16_t bgColor = isSelected ? 0x1082 : COLOR_BACKGROUND;
    uint16_t borderColor = isSelected ? ST77XX_CYAN : 0x39C7;

    // Draw item card
    tft.fillRoundRect(5, y, 310, 26, 4, bgColor);
    tft.drawRoundRect(5, y, 310, 26, 4, borderColor);

    // Draw content
    tft.setTextSize(1);

    // Date (left side)
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(10, y + 4);
    // Format date YYYYMMDD to MM/DD
    if (strlen(qso.date) >= 8) {
      tft.print(qso.date[4]);
      tft.print(qso.date[5]);
      tft.print("/");
      tft.print(qso.date[6]);
      tft.print(qso.date[7]);
    } else {
      tft.print(qso.date);
    }

    // Time
    tft.setCursor(10, y + 14);
    tft.setTextColor(0x7BEF); // Gray
    tft.print(qso.time_on);

    // Callsign (center)
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_CYAN);
    tft.setCursor(70, y + 5);
    tft.print(qso.callsign);

    // Band/Mode (right side)
    tft.setTextSize(1);
    tft.setTextColor(COLOR_WARNING);
    tft.setCursor(220, y + 4);
    tft.print(qso.band);
    tft.setCursor(220, y + 14);
    tft.setTextColor(ST77XX_WHITE);
    tft.print(qso.mode);
  }

  // Scroll indicators
  if (viewState.scrollOffset > 0) {
    // Up arrow
    tft.fillTriangle(SCREEN_WIDTH - 15, LIST_START_Y + 5,
                     SCREEN_WIDTH - 10, LIST_START_Y + 10,
                     SCREEN_WIDTH - 20, LIST_START_Y + 10,
                     ST77XX_CYAN);
  }

  if (viewState.scrollOffset + MAX_VISIBLE_LIST_ITEMS < viewState.totalQSOs) {
    // Down arrow
    int arrowY = LIST_START_Y + (MAX_VISIBLE_LIST_ITEMS * LIST_ITEM_HEIGHT) - 10;
    tft.fillTriangle(SCREEN_WIDTH - 15, arrowY,
                     SCREEN_WIDTH - 10, arrowY - 5,
                     SCREEN_WIDTH - 20, arrowY - 5,
                     ST77XX_CYAN);
  }

  // Footer
  tft.fillRect(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH, 20, COLOR_BACKGROUND);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(10, SCREEN_HEIGHT - 16);
  tft.print("UP/DOWN Select  ENT View  ESC Back");
}

// Draw detail view for selected QSO
void drawDetailView(LGFX& tft) {
  if (viewState.selectedIndex < 0 || viewState.selectedIndex >= viewState.totalQSOs) {
    return;
  }

  QSO& qso = viewState.qsos[viewState.selectedIndex];

  // Debug: Print location field contents
  Serial.println("=== Detail View Debug ===");
  Serial.print("my_gridsquare: [");
  Serial.print(qso.my_gridsquare);
  Serial.print("] len=");
  Serial.println(strlen(qso.my_gridsquare));
  Serial.print("my_pota_ref: [");
  Serial.print(qso.my_pota_ref);
  Serial.print("] len=");
  Serial.println(strlen(qso.my_pota_ref));
  Serial.print("gridsquare (their): [");
  Serial.print(qso.gridsquare);
  Serial.print("] len=");
  Serial.println(strlen(qso.gridsquare));
  Serial.print("their_pota_ref: [");
  Serial.print(qso.their_pota_ref);
  Serial.print("] len=");
  Serial.println(strlen(qso.their_pota_ref));

  // Clear entire screen first
  tft.fillScreen(COLOR_BACKGROUND);

  // Draw fields with scroll offset applied
  tft.setTextSize(1);
  int y = DETAIL_START_Y - viewState.detailScrollOffset;
  int lineHeight = DETAIL_LINE_HEIGHT;

  // Callsign (large)
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(10, y);
  tft.print(qso.callsign);
  y += 30;

  tft.setTextSize(1);

  // Date/Time
  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(10, y);
  tft.print("Date/Time:");
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(100, y);
  // Format date YYYYMMDD to YYYY-MM-DD
  if (strlen(qso.date) >= 8) {
    tft.print(qso.date[0]);
    tft.print(qso.date[1]);
    tft.print(qso.date[2]);
    tft.print(qso.date[3]);
    tft.print("-");
    tft.print(qso.date[4]);
    tft.print(qso.date[5]);
    tft.print("-");
    tft.print(qso.date[6]);
    tft.print(qso.date[7]);
  } else {
    tft.print(qso.date);
  }
  tft.print(" ");
  // Format time HHMM to HH:MM
  if (strlen(qso.time_on) >= 4) {
    tft.print(qso.time_on[0]);
    tft.print(qso.time_on[1]);
    tft.print(":");
    tft.print(qso.time_on[2]);
    tft.print(qso.time_on[3]);
  } else {
    tft.print(qso.time_on);
  }
  tft.print(" UTC");
  y += lineHeight;

  // Frequency/Band
  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(10, y);
  tft.print("Frequency:");
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(100, y);
  tft.print(qso.frequency, 3);
  tft.print(" MHz (");
  tft.print(qso.band);
  tft.print(")");
  y += lineHeight;

  // Mode
  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(10, y);
  tft.print("Mode:");
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(100, y);
  tft.print(qso.mode);
  y += lineHeight;

  // RST Sent
  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(10, y);
  tft.print("RST Sent:");
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(100, y);
  tft.print(qso.rst_sent);
  y += lineHeight;

  // RST Received
  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(10, y);
  tft.print("RST Rcvd:");
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(100, y);
  tft.print(qso.rst_rcvd);
  y += lineHeight;

  // My Grid
  if (strlen(qso.my_gridsquare) > 0) {
    tft.setTextColor(COLOR_WARNING);
    tft.setCursor(10, y);
    tft.print("My Grid:");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(100, y);
    tft.print(qso.my_gridsquare);
    y += lineHeight;
  }

  // My POTA
  if (strlen(qso.my_pota_ref) > 0) {
    tft.setTextColor(COLOR_WARNING);
    tft.setCursor(10, y);
    tft.print("My POTA:");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(100, y);
    tft.print(qso.my_pota_ref);
    y += lineHeight;
  }

  // Their Grid
  if (strlen(qso.gridsquare) > 0) {
    tft.setTextColor(COLOR_WARNING);
    tft.setCursor(10, y);
    tft.print("Their Grid:");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(100, y);
    tft.print(qso.gridsquare);
    y += lineHeight;
  }

  // Their POTA
  if (strlen(qso.their_pota_ref) > 0) {
    tft.setTextColor(COLOR_WARNING);
    tft.setCursor(10, y);
    tft.print("Their POTA:");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(100, y);
    tft.print(qso.their_pota_ref);
    y += lineHeight;
  }

  // Notes (wrapped if long)
  if (strlen(qso.notes) > 0) {
    tft.setTextColor(COLOR_WARNING);
    tft.setCursor(10, y);
    tft.print("Notes:");
    y += lineHeight;

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(10, y);

    // Simple word wrapping for notes
    String notesStr = String(qso.notes);
    int maxCharsPerLine = 52; // Approximate for size 1 text
    int startPos = 0;

    while (startPos < notesStr.length()) {
      String line = notesStr.substring(startPos, min((int)notesStr.length(), startPos + maxCharsPerLine));
      tft.setCursor(10, y);
      tft.print(line);
      y += 10; // Smaller line height for wrapped text
      startPos += maxCharsPerLine;

      if (y > SCREEN_HEIGHT - 30) break; // Don't overflow screen
    }
  }

  // Redraw header and footer on top of content to prevent overlap
  // Header
  tft.fillRect(0, 0, SCREEN_WIDTH, 40, 0x1082); // Dark blue header
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 12);
  tft.print("QSO Details");

  // Footer
  tft.fillRect(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH, 20, COLOR_BACKGROUND);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(10, SCREEN_HEIGHT - 16);
  tft.print("D Delete  UP/DN Scroll  ESC Back");
}

// Draw delete confirmation dialog
void drawDeleteConfirmation(LGFX& tft) {
  // Semi-transparent overlay effect (dark rectangle)
  tft.fillRect(20, 80, SCREEN_WIDTH - 40, 80, 0x2104); // Dark blue-gray
  tft.drawRect(20, 80, SCREEN_WIDTH - 40, 80, ST77XX_RED);
  tft.drawRect(21, 81, SCREEN_WIDTH - 42, 78, ST77XX_RED);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(60, 90);
  tft.print("DELETE QSO?");

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(30, 115);
  tft.print("This cannot be undone!");

  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(30, 135);
  tft.print("Y = Confirm   N = Cancel");
}

// Delete the currently viewed QSO from storage
bool deleteCurrentQSO() {
  if (viewState.selectedIndex < 0 || viewState.selectedIndex >= viewState.totalQSOs) {
    return false;
  }

  QSO& qsoToDelete = viewState.qsos[viewState.selectedIndex];

  Serial.print("Deleting QSO: ");
  Serial.print(qsoToDelete.callsign);
  Serial.print(" ID: ");
  Serial.println(qsoToDelete.id);

  // Get the log filename for this QSO's date
  String filename = getLogFilename(qsoToDelete.date);

  if (!SD.exists(filename)) {
    Serial.println("Log file doesn't exist!");
    return false;
  }

  // Read the entire log file
  File file = SD.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open log file for reading");
    return false;
  }

  String content = file.readString();
  file.close();

  // Parse JSON
  StaticJsonDocument<8192> doc;
  DeserializationError error = deserializeJson(doc, content);

  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    return false;
  }

  if (!doc.containsKey("logs")) {
    Serial.println("No logs array found");
    return false;
  }

  // Create new array without the deleted QSO
  JsonArray oldLogs = doc["logs"].as<JsonArray>();
  StaticJsonDocument<8192> newDoc;
  JsonArray newLogs = newDoc.createNestedArray("logs");

  int removedCount = 0;
  for (JsonObject qsoObj : oldLogs) {
    unsigned long id = qsoObj["id"] | 0;
    if (id != qsoToDelete.id) {
      // Keep this QSO
      newLogs.add(qsoObj);
    } else {
      // Skip this QSO (delete it)
      removedCount++;
      Serial.print("Removed QSO with ID: ");
      Serial.println(id);
    }
  }

  if (removedCount == 0) {
    Serial.println("QSO not found in file!");
    return false;
  }

  // Write back to file
  file = SD.open(filename, "w");
  if (!file) {
    Serial.println("Failed to open log file for writing");
    return false;
  }

  serializeJson(newDoc, file);
  file.close();

  // Update metadata to decrement total count
  if (storageStats.totalLogs > 0) {
    storageStats.totalLogs--;
    saveMetadata();
    Serial.print("Updated metadata: totalLogs now = ");
    Serial.println(storageStats.totalLogs);
  }

  Serial.println("QSO deleted successfully");
  return true;
}

// Scroll functions
void scrollListUp() {
  if (viewState.selectedIndex > 0) {
    viewState.selectedIndex--;

    // Adjust scroll offset if needed
    if (viewState.selectedIndex < viewState.scrollOffset) {
      viewState.scrollOffset = viewState.selectedIndex;
    }
  }
}

void scrollListDown() {
  if (viewState.selectedIndex < viewState.totalQSOs - 1) {
    viewState.selectedIndex++;

    // Adjust scroll offset if needed
    if (viewState.selectedIndex >= viewState.scrollOffset + MAX_VISIBLE_LIST_ITEMS) {
      viewState.scrollOffset = viewState.selectedIndex - MAX_VISIBLE_LIST_ITEMS + 1;
    }
  }
}

// Handle input for view logs mode
int handleViewLogsInput(char key, LGFX& tft) {
  Serial.print("View Logs key: 0x");
  Serial.println(key, HEX);

  // ESC: exit mode
  if (key == 0x1B) {
    freeQSOsFromView();
    return -1; // Exit to menu
  }

  if (viewState.mode == VIEW_MODE_LIST) {
    // List view input
    if (key == 0xB5) { // UP arrow
      scrollListUp();
      drawListView(tft);
      return 2; // Redraw
    }

    if (key == 0xB6) { // DOWN arrow
      scrollListDown();
      drawListView(tft);
      return 2; // Redraw
    }

    if (key == 0x0D || key == '\n') { // ENTER
      if (viewState.totalQSOs > 0) {
        viewState.mode = VIEW_MODE_DETAIL;
        tft.fillScreen(COLOR_BACKGROUND);
        drawDetailView(tft);
        return 2; // Redraw
      }
    }
  } else if (viewState.mode == VIEW_MODE_DETAIL) {
    // Detail view input
    if (key == 0x1B) { // ESC - back to list
      viewState.mode = VIEW_MODE_LIST;
      viewState.detailScrollOffset = 0; // Reset scroll
      tft.fillScreen(COLOR_BACKGROUND);
      drawListView(tft);
      return 2; // Redraw
    }

    if (key == 0xB5) { // UP arrow - scroll up
      if (viewState.detailScrollOffset > 0) {
        viewState.detailScrollOffset -= DETAIL_LINE_HEIGHT;
        if (viewState.detailScrollOffset < 0) {
          viewState.detailScrollOffset = 0;
        }
        tft.fillScreen(COLOR_BACKGROUND);
        drawDetailView(tft);
        return 2; // Redraw
      }
    }

    if (key == 0xB6) { // DOWN arrow - scroll down
      // Allow scrolling down (max scroll calculated dynamically based on content)
      viewState.detailScrollOffset += DETAIL_LINE_HEIGHT;
      tft.fillScreen(COLOR_BACKGROUND);
      drawDetailView(tft);
      return 2; // Redraw
    }

    if (key == 'D' || key == 'd') { // Delete key
      viewState.mode = VIEW_MODE_DELETE_CONFIRM;
      drawDeleteConfirmation(tft);
      return 2; // Redraw
    }
  } else if (viewState.mode == VIEW_MODE_DELETE_CONFIRM) {
    // Delete confirmation input
    if (key == 'Y' || key == 'y') { // Confirm delete
      if (deleteCurrentQSO()) {
        // Successfully deleted - reload QSOs and return to list
        freeQSOsFromView();
        loadQSOsForView();

        // Adjust selected index if needed
        if (viewState.selectedIndex >= viewState.totalQSOs) {
          viewState.selectedIndex = viewState.totalQSOs - 1;
        }
        if (viewState.selectedIndex < 0) {
          viewState.selectedIndex = 0;
        }

        viewState.mode = VIEW_MODE_LIST;
        viewState.detailScrollOffset = 0;
        tft.fillScreen(COLOR_BACKGROUND);
        drawListView(tft);
        return 2; // Redraw
      } else {
        // Delete failed - show error and go back to detail
        beep(600, 200); // Error beep
        viewState.mode = VIEW_MODE_DETAIL;
        tft.fillScreen(COLOR_BACKGROUND);
        drawDetailView(tft);
        return 2; // Redraw
      }
    }

    if (key == 'N' || key == 'n' || key == 0x1B) { // Cancel delete
      viewState.mode = VIEW_MODE_DETAIL;
      tft.fillScreen(COLOR_BACKGROUND);
      drawDetailView(tft);
      return 2; // Redraw
    }
  }

  return 0; // No action
}

#endif // QSO_LOGGER_VIEW_H
