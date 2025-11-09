/*
 * CW Memories Module
 * Store and manage CW message presets (up to 10 slots)
 * Presets can be previewed on device speaker or transmitted via Radio Output mode
 */

#ifndef RADIO_CW_MEMORIES_H
#define RADIO_CW_MEMORIES_H

#include "config.h"
#include "morse_code.h"
#include <Preferences.h>

// External declarations
extern int cwSpeed;
extern int cwTone;

// ============================================
// Data Structures
// ============================================

#define CW_MEMORY_MAX_SLOTS 10
#define CW_MEMORY_LABEL_MAX_LENGTH 15
#define CW_MEMORY_MESSAGE_MAX_LENGTH 100

struct CWMemoryPreset {
  char label[CW_MEMORY_LABEL_MAX_LENGTH + 1];     // Label (15 chars + null)
  char message[CW_MEMORY_MESSAGE_MAX_LENGTH + 1]; // Message (100 chars + null)
  bool isEmpty;
};

// ============================================
// Global State
// ============================================

CWMemoryPreset cwMemories[CW_MEMORY_MAX_SLOTS] = {};  // Initialize all to zero
Preferences cwMemoryPrefs;

// UI State
int cwMemorySelection = 0;  // Currently selected slot (0-9)
bool cwMemoryScrollOffset = 0;  // For scrolling if needed

// Context menu state
enum CWMemoryContextMenu {
  CONTEXT_NONE,
  CONTEXT_EMPTY_SLOT,     // Create or Cancel
  CONTEXT_OCCUPIED_SLOT   // Preview, Edit, Delete, Cancel
};

CWMemoryContextMenu contextMenuActive = CONTEXT_NONE;
int contextMenuSelection = 0;

// Edit mode state
enum CWMemoryEditMode {
  EDIT_NONE,
  EDIT_CREATE_LABEL,
  EDIT_CREATE_MESSAGE,
  EDIT_EDIT_LABEL,
  EDIT_EDIT_MESSAGE
};

CWMemoryEditMode editMode = EDIT_NONE;
int editingSlot = -1;
char editBuffer[CW_MEMORY_MESSAGE_MAX_LENGTH + 1];
int editCursorPos = 0;

// Preview state
bool isPreviewingMemory = false;
int previewingSlot = -1;

// ============================================
// Storage Functions
// ============================================

// Load all CW memories from Preferences
void loadCWMemories() {
  cwMemoryPrefs.begin("cw_memories", true);  // Read-only

  for (int i = 0; i < CW_MEMORY_MAX_SLOTS; i++) {
    String labelKey = "label" + String(i + 1);
    String messageKey = "message" + String(i + 1);

    String label = cwMemoryPrefs.getString(labelKey.c_str(), "");
    String message = cwMemoryPrefs.getString(messageKey.c_str(), "");

    if (label.length() == 0 && message.length() == 0) {
      cwMemories[i].isEmpty = true;
      cwMemories[i].label[0] = '\0';
      cwMemories[i].message[0] = '\0';
    } else {
      cwMemories[i].isEmpty = false;
      strlcpy(cwMemories[i].label, label.c_str(), CW_MEMORY_LABEL_MAX_LENGTH + 1);
      strlcpy(cwMemories[i].message, message.c_str(), CW_MEMORY_MESSAGE_MAX_LENGTH + 1);

      // Debug logging
      Serial.print("Loaded slot ");
      Serial.print(i + 1);
      Serial.print(": Label='");
      Serial.print(cwMemories[i].label);
      Serial.print("' Message='");
      Serial.print(cwMemories[i].message);
      Serial.println("'");
    }
  }

  cwMemoryPrefs.end();

  Serial.println("CW Memories loaded from Preferences");
}

// Save a single CW memory to Preferences
void saveCWMemory(int slot) {
  if (slot < 0 || slot >= CW_MEMORY_MAX_SLOTS) {
    Serial.println("Error: Invalid slot number");
    return;
  }

  cwMemoryPrefs.begin("cw_memories", false);  // Read-write

  String labelKey = "label" + String(slot + 1);
  String messageKey = "message" + String(slot + 1);

  if (cwMemories[slot].isEmpty) {
    cwMemoryPrefs.putString(labelKey.c_str(), "");
    cwMemoryPrefs.putString(messageKey.c_str(), "");
    Serial.print("Cleared slot ");
    Serial.println(slot + 1);
  } else {
    cwMemoryPrefs.putString(labelKey.c_str(), cwMemories[slot].label);
    cwMemoryPrefs.putString(messageKey.c_str(), cwMemories[slot].message);

    // Debug logging
    Serial.print("Saved slot ");
    Serial.print(slot + 1);
    Serial.print(": Label='");
    Serial.print(cwMemories[slot].label);
    Serial.print("' Message='");
    Serial.print(cwMemories[slot].message);
    Serial.println("'");
  }

  cwMemoryPrefs.end();
}

// Delete a CW memory (clear the slot)
void deleteCWMemory(int slot) {
  if (slot < 0 || slot >= CW_MEMORY_MAX_SLOTS) {
    Serial.println("Error: Invalid slot number");
    return;
  }

  cwMemories[slot].isEmpty = true;
  cwMemories[slot].label[0] = '\0';
  cwMemories[slot].message[0] = '\0';

  saveCWMemory(slot);

  Serial.print("CW Memory deleted: Slot ");
  Serial.println(slot + 1);
}

// ============================================
// Preview Function
// ============================================

// Preview a memory on device speaker (non-blocking setup)
void previewCWMemory(int slot) {
  if (slot < 0 || slot >= CW_MEMORY_MAX_SLOTS) {
    Serial.println("Error: Invalid slot for preview");
    return;
  }

  if (cwMemories[slot].isEmpty || strlen(cwMemories[slot].message) == 0) {
    Serial.println("Error: Cannot preview empty memory");
    beep(TONE_ERROR, BEEP_SHORT);
    return;
  }

  isPreviewingMemory = true;
  previewingSlot = slot;

  Serial.print("Previewing memory slot ");
  Serial.print(slot + 1);
  Serial.print(": Label='");
  Serial.print(cwMemories[slot].label);
  Serial.print("' Message='");
  Serial.print(cwMemories[slot].message);
  Serial.print("' Length=");
  Serial.println(strlen(cwMemories[slot].message));

  // Play the message using existing morse code functions
  playMorseString(cwMemories[slot].message, cwSpeed, cwTone);

  // Preview complete
  isPreviewingMemory = false;
  previewingSlot = -1;
}

// ============================================
// Validation Functions
// ============================================

// Check if a character is valid for morse code
bool isValidMorseChar(char c) {
  c = toupper(c);
  if (c >= 'A' && c <= 'Z') return true;
  if (c >= '0' && c <= '9') return true;
  if (c == ' ') return true;
  if (c == '.' || c == ',' || c == '?' || c == '/' || c == '-') return true;
  // Prosigns entered as <AR>, <SK>, etc. are handled as regular characters
  if (c == '<' || c == '>') return true;
  return false;
}

// Validate message contains only valid morse characters
bool isValidMorseMessage(const char* message) {
  for (int i = 0; message[i] != '\0'; i++) {
    if (!isValidMorseChar(message[i])) {
      return false;
    }
  }
  return true;
}

// ============================================
// UI Drawing Functions
// ============================================

// Draw main CW Memories list screen
void drawCWMemoriesUI(Adafruit_ST7789 &display) {
  // Clear screen (preserve header)
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Title
  display.setTextSize(2);
  display.setTextColor(COLOR_TITLE);
  int16_t x1, y1;
  uint16_t w, h;
  String title = "CW MEMORIES";
  display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, 55);
  display.print(title);

  // Draw list of presets (show 5 at a time)
  int startY = 85;
  int itemHeight = 25;
  int visibleItems = 5;
  int startIdx = cwMemoryScrollOffset;

  for (int i = 0; i < visibleItems && (startIdx + i) < CW_MEMORY_MAX_SLOTS; i++) {
    int slot = startIdx + i;
    int yPos = startY + (i * itemHeight);

    bool isSelected = (slot == cwMemorySelection);

    // Highlight selected item
    if (isSelected) {
      display.fillRoundRect(10, yPos - 2, SCREEN_WIDTH - 20, itemHeight - 2, 6, 0x249F);
    }

    // Draw slot number and label
    display.setTextSize(1);
    display.setTextColor(isSelected ? ST77XX_WHITE : ST77XX_CYAN);
    display.setCursor(20, yPos + 8);
    display.print("[");
    display.print(slot + 1);
    display.print("] ");

    if (cwMemories[slot].isEmpty) {
      display.setTextColor(isSelected ? 0xC618 : 0x7BEF);  // Gray/light gray
      display.print("(empty)");
    } else {
      display.setTextColor(isSelected ? ST77XX_WHITE : ST77XX_CYAN);
      // Truncate label if too long
      if (strlen(cwMemories[slot].label) > 22) {
        char truncated[23];
        strlcpy(truncated, cwMemories[slot].label, 20);
        strcat(truncated, "...");
        display.print(truncated);
      } else {
        display.print(cwMemories[slot].label);
      }
    }
  }

  // Footer with instructions
  display.setTextSize(1);
  display.setTextColor(COLOR_WARNING);
  String helpText = "\x18\x19 Select  ENTER Menu  ESC Back";
  display.getTextBounds(helpText, 0, 0, &x1, &y1, &w, &h);
  centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, SCREEN_HEIGHT - 12);
  display.print(helpText);
}

// Draw context menu
void drawContextMenu(Adafruit_ST7789 &display) {
  // Modal overlay
  display.fillRoundRect(40, 80, SCREEN_WIDTH - 80, 100, 12, 0x1082);  // Dark blue fill
  display.drawRoundRect(40, 80, SCREEN_WIDTH - 80, 100, 12, 0x34BF);  // Light blue outline

  display.setTextSize(1);
  display.setTextColor(ST77XX_CYAN);
  int yPos = 95;

  if (contextMenuActive == CONTEXT_EMPTY_SLOT) {
    // Empty slot menu
    bool sel0 = (contextMenuSelection == 0);
    bool sel1 = (contextMenuSelection == 1);

    if (sel0) display.fillRoundRect(50, yPos - 3, SCREEN_WIDTH - 100, 20, 6, 0x249F);
    display.setTextColor(sel0 ? ST77XX_WHITE : ST77XX_CYAN);
    display.setCursor(60, yPos + 5);
    display.print("Create Preset");

    yPos += 30;
    if (sel1) display.fillRoundRect(50, yPos - 3, SCREEN_WIDTH - 100, 20, 6, 0x249F);
    display.setTextColor(sel1 ? ST77XX_WHITE : ST77XX_CYAN);
    display.setCursor(60, yPos + 5);
    display.print("Cancel");

  } else if (contextMenuActive == CONTEXT_OCCUPIED_SLOT) {
    // Occupied slot menu
    const char* options[] = {"Preview", "Edit Preset", "Delete Preset", "Cancel"};

    for (int i = 0; i < 4; i++) {
      bool sel = (contextMenuSelection == i);
      if (sel) display.fillRoundRect(50, yPos - 3, SCREEN_WIDTH - 100, 20, 6, 0x249F);
      display.setTextColor(sel ? ST77XX_WHITE : ST77XX_CYAN);
      display.setCursor(60, yPos + 5);
      display.print(options[i]);
      yPos += 20;
    }
  }
}

// Draw edit screen (label or message input)
void drawEditScreen(Adafruit_ST7789 &display) {
  // Clear screen (preserve header)
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Title
  display.setTextSize(2);
  display.setTextColor(COLOR_TITLE);
  int16_t x1, y1;
  uint16_t w, h;
  String title = (editMode == EDIT_CREATE_LABEL || editMode == EDIT_CREATE_MESSAGE) ? "CREATE PRESET" : "EDIT PRESET";
  display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, 55);
  display.print(title);

  // Prompt label
  display.setTextSize(1);
  display.setTextColor(ST77XX_CYAN);
  display.setCursor(20, 85);
  if (editMode == EDIT_CREATE_LABEL || editMode == EDIT_EDIT_LABEL) {
    display.print("Label (max 15 chars):");
  } else {
    display.print("Message (max 100 chars):");
  }

  // Input box
  display.fillRoundRect(20, 105, SCREEN_WIDTH - 40, 60, 8, 0x1082);  // Dark blue
  display.drawRoundRect(20, 105, SCREEN_WIDTH - 40, 60, 8, 0x34BF);  // Light blue outline

  // Display edit buffer (word wrap for messages)
  display.setTextSize(1);
  display.setTextColor(ST77XX_WHITE);

  if (editMode == EDIT_CREATE_LABEL || editMode == EDIT_EDIT_LABEL) {
    // Single line for label
    display.setCursor(30, 120);
    display.print(editBuffer);

    // Cursor indicator
    if (millis() % 1000 < 500) {
      display.setCursor(30 + (strlen(editBuffer) * 6), 120);
      display.print("_");
    }
  } else {
    // Multi-line for message (word wrap)
    int xPos = 30;
    int yPos = 115;
    int lineHeight = 12;
    int charsPerLine = 45;

    for (int i = 0; i < strlen(editBuffer); i++) {
      if (xPos > SCREEN_WIDTH - 60 || (i > 0 && i % charsPerLine == 0)) {
        xPos = 30;
        yPos += lineHeight;
      }
      display.setCursor(xPos, yPos);
      display.print(editBuffer[i]);
      xPos += 6;
    }

    // Cursor indicator
    if (millis() % 1000 < 500) {
      if (xPos > SCREEN_WIDTH - 60) {
        xPos = 30;
        yPos += lineHeight;
      }
      display.setCursor(xPos, yPos);
      display.print("_");
    }
  }

  // Character count
  display.setTextSize(1);
  display.setTextColor(0x7BEF);  // Light gray
  display.setCursor(20, 175);
  display.print(strlen(editBuffer));
  display.print(" / ");
  if (editMode == EDIT_CREATE_LABEL || editMode == EDIT_EDIT_LABEL) {
    display.print(CW_MEMORY_LABEL_MAX_LENGTH);
  } else {
    display.print(CW_MEMORY_MESSAGE_MAX_LENGTH);
  }
  display.print(" chars");

  // Footer with instructions
  display.setTextSize(1);
  display.setTextColor(COLOR_WARNING);
  String helpText = "Type text  ENTER Save  ESC Cancel";
  display.getTextBounds(helpText, 0, 0, &x1, &y1, &w, &h);
  centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, SCREEN_HEIGHT - 12);
  display.print(helpText);
}

// Draw delete confirmation dialog
void drawDeleteConfirmation(Adafruit_ST7789 &display, int slot) {
  // Modal overlay
  display.fillRoundRect(30, 70, SCREEN_WIDTH - 60, 110, 12, 0x1082);
  display.drawRoundRect(30, 70, SCREEN_WIDTH - 60, 110, 12, COLOR_ERROR);

  // Title
  display.setTextSize(1);
  display.setTextColor(COLOR_ERROR);
  display.setCursor(50, 85);
  display.print("DELETE PRESET");

  // Message (wrap if needed)
  display.setTextSize(1);
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(50, 105);
  display.print("Delete \"");

  // Truncate label if needed
  if (strlen(cwMemories[slot].label) > 18) {
    char truncated[19];
    strlcpy(truncated, cwMemories[slot].label, 16);
    strcat(truncated, "...");
    display.print(truncated);
  } else {
    display.print(cwMemories[slot].label);
  }
  display.print("\"?");

  // Yes/No options
  int yPos = 135;
  bool selYes = (contextMenuSelection == 0);
  bool selNo = (contextMenuSelection == 1);

  if (selYes) display.fillRoundRect(50, yPos, 80, 25, 6, COLOR_ERROR);
  display.setTextColor(selYes ? ST77XX_BLACK : ST77XX_WHITE);
  display.setCursor(70, yPos + 10);
  display.print("Yes");

  if (selNo) display.fillRoundRect(150, yPos, 80, 25, 6, 0x249F);
  display.setTextColor(selNo ? ST77XX_WHITE : 0x7BEF);
  display.setCursor(170, yPos + 10);
  display.print("No");
}

// ============================================
// Mode Entry Function
// ============================================

void startCWMemoriesMode(Adafruit_ST7789 &display) {
  cwMemorySelection = 0;
  cwMemoryScrollOffset = 0;
  contextMenuActive = CONTEXT_NONE;
  contextMenuSelection = 0;
  editMode = EDIT_NONE;
  editingSlot = -1;
  editBuffer[0] = '\0';
  editCursorPos = 0;

  drawCWMemoriesUI(display);
  beep(TONE_SELECT, BEEP_SHORT);
}

// ============================================
// Input Handling Functions
// ============================================

// Handle input in edit mode (label or message entry)
int handleEditModeInput(char key, Adafruit_ST7789 &display) {
  int maxLength = (editMode == EDIT_CREATE_LABEL || editMode == EDIT_EDIT_LABEL)
                  ? CW_MEMORY_LABEL_MAX_LENGTH
                  : CW_MEMORY_MESSAGE_MAX_LENGTH;

  if (key == KEY_ESC) {
    // Cancel edit
    editMode = EDIT_NONE;
    editBuffer[0] = '\0';
    contextMenuActive = CONTEXT_NONE;
    drawCWMemoriesUI(display);
    beep(TONE_MENU_NAV, BEEP_SHORT);
    return 2;
  }
  else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    // Save current field
    if (strlen(editBuffer) == 0) {
      // Empty field - show error
      beep(TONE_ERROR, BEEP_SHORT);
      return 0;
    }

    if (editMode == EDIT_CREATE_LABEL || editMode == EDIT_EDIT_LABEL) {
      // Save label, move to message
      strlcpy(cwMemories[editingSlot].label, editBuffer, CW_MEMORY_LABEL_MAX_LENGTH + 1);

      // Move to message entry
      editBuffer[0] = '\0';
      if (editMode == EDIT_CREATE_LABEL) {
        editMode = EDIT_CREATE_MESSAGE;
      } else {
        editMode = EDIT_EDIT_MESSAGE;
      }

      // Pre-fill message if editing
      if (editMode == EDIT_EDIT_MESSAGE && !cwMemories[editingSlot].isEmpty) {
        strlcpy(editBuffer, cwMemories[editingSlot].message, CW_MEMORY_MESSAGE_MAX_LENGTH + 1);
      }

      drawEditScreen(display);
      beep(TONE_SELECT, BEEP_SHORT);
      return 2;
    }
    else {
      // Save message and complete
      if (!isValidMorseMessage(editBuffer)) {
        // Invalid characters
        beep(TONE_ERROR, BEEP_LONG);
        return 0;
      }

      strlcpy(cwMemories[editingSlot].message, editBuffer, CW_MEMORY_MESSAGE_MAX_LENGTH + 1);
      cwMemories[editingSlot].isEmpty = false;

      saveCWMemory(editingSlot);

      // Return to main list
      editMode = EDIT_NONE;
      editBuffer[0] = '\0';
      contextMenuActive = CONTEXT_NONE;
      drawCWMemoriesUI(display);
      beep(TONE_SUCCESS, BEEP_MEDIUM);
      return 2;
    }
  }
  else if (key == KEY_BACKSPACE) {
    // Delete character
    int len = strlen(editBuffer);
    if (len > 0) {
      editBuffer[len - 1] = '\0';
      drawEditScreen(display);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;
    }
  }
  else if (key >= 32 && key <= 126) {
    // Printable character
    int len = strlen(editBuffer);
    if (len < maxLength) {
      // Auto-uppercase for consistency
      char c = toupper(key);
      editBuffer[len] = c;
      editBuffer[len + 1] = '\0';
      drawEditScreen(display);
      return 2;
    } else {
      beep(TONE_ERROR, BEEP_SHORT);
    }
  }

  return 0;
}

// Handle context menu input
int handleContextMenuInput(char key, Adafruit_ST7789 &display) {
  int maxOptions = (contextMenuActive == CONTEXT_EMPTY_SLOT) ? 2 : 4;

  if (key == KEY_UP) {
    if (contextMenuSelection > 0) {
      contextMenuSelection--;
      drawContextMenu(display);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;
    }
  }
  else if (key == KEY_DOWN) {
    if (contextMenuSelection < maxOptions - 1) {
      contextMenuSelection++;
      drawContextMenu(display);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;
    }
  }
  else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    if (contextMenuActive == CONTEXT_EMPTY_SLOT) {
      if (contextMenuSelection == 0) {
        // Create preset
        editingSlot = cwMemorySelection;
        editMode = EDIT_CREATE_LABEL;
        editBuffer[0] = '\0';
        contextMenuActive = CONTEXT_NONE;
        drawEditScreen(display);
        beep(TONE_SELECT, BEEP_SHORT);
        return 2;
      } else {
        // Cancel
        contextMenuActive = CONTEXT_NONE;
        drawCWMemoriesUI(display);
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 2;
      }
    }
    else if (contextMenuActive == CONTEXT_OCCUPIED_SLOT) {
      if (contextMenuSelection == 0) {
        // Preview
        contextMenuActive = CONTEXT_NONE;
        drawCWMemoriesUI(display);
        previewCWMemory(cwMemorySelection);
        beep(TONE_SELECT, BEEP_SHORT);
        return 2;
      }
      else if (contextMenuSelection == 1) {
        // Edit
        editingSlot = cwMemorySelection;
        editMode = EDIT_EDIT_LABEL;
        strlcpy(editBuffer, cwMemories[editingSlot].label, CW_MEMORY_LABEL_MAX_LENGTH + 1);
        contextMenuActive = CONTEXT_NONE;
        drawEditScreen(display);
        beep(TONE_SELECT, BEEP_SHORT);
        return 2;
      }
      else if (contextMenuSelection == 2) {
        // Delete - show confirmation
        contextMenuSelection = 1;  // Default to "No"
        drawDeleteConfirmation(display, cwMemorySelection);
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 2;
      }
      else {
        // Cancel
        contextMenuActive = CONTEXT_NONE;
        drawCWMemoriesUI(display);
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 2;
      }
    }
  }
  else if (key == KEY_ESC) {
    // Cancel menu
    contextMenuActive = CONTEXT_NONE;
    drawCWMemoriesUI(display);
    beep(TONE_MENU_NAV, BEEP_SHORT);
    return 2;
  }

  return 0;
}

// Handle delete confirmation input
int handleDeleteConfirmationInput(char key, Adafruit_ST7789 &display) {
  if (key == KEY_LEFT || key == KEY_RIGHT) {
    contextMenuSelection = (contextMenuSelection == 0) ? 1 : 0;
    drawDeleteConfirmation(display, cwMemorySelection);
    beep(TONE_MENU_NAV, BEEP_SHORT);
    return 2;
  }
  else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    if (contextMenuSelection == 0) {
      // Confirm delete
      deleteCWMemory(cwMemorySelection);
      contextMenuActive = CONTEXT_NONE;
      drawCWMemoriesUI(display);
      beep(TONE_SUCCESS, BEEP_MEDIUM);
      return 2;
    } else {
      // Cancel delete
      contextMenuActive = CONTEXT_OCCUPIED_SLOT;
      contextMenuSelection = 0;
      drawContextMenu(display);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;
    }
  }
  else if (key == KEY_ESC) {
    // Cancel delete
    contextMenuActive = CONTEXT_OCCUPIED_SLOT;
    contextMenuSelection = 0;
    drawContextMenu(display);
    beep(TONE_MENU_NAV, BEEP_SHORT);
    return 2;
  }

  return 0;
}

// Main input handler
int handleCWMemoriesInput(char key, Adafruit_ST7789 &display) {
  // Handle edit mode
  if (editMode != EDIT_NONE) {
    return handleEditModeInput(key, display);
  }

  // Handle delete confirmation
  if (contextMenuActive == CONTEXT_OCCUPIED_SLOT && contextMenuSelection == 2) {
    // Check if we're showing delete confirmation (rough heuristic)
    // This would be cleaner with a separate state variable
    static bool showingDeleteConfirm = false;
    if (key == KEY_LEFT || key == KEY_RIGHT) {
      showingDeleteConfirm = true;
    }
    if (showingDeleteConfirm) {
      int result = handleDeleteConfirmationInput(key, display);
      if (result != 0) {
        showingDeleteConfirm = false;
      }
      return result;
    }
  }

  // Handle context menu
  if (contextMenuActive != CONTEXT_NONE) {
    return handleContextMenuInput(key, display);
  }

  // Handle main list navigation
  if (key == KEY_UP) {
    if (cwMemorySelection > 0) {
      cwMemorySelection--;

      // Adjust scroll offset if needed
      if (cwMemorySelection < cwMemoryScrollOffset) {
        cwMemoryScrollOffset = cwMemorySelection;
      }

      drawCWMemoriesUI(display);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;
    }
  }
  else if (key == KEY_DOWN) {
    if (cwMemorySelection < CW_MEMORY_MAX_SLOTS - 1) {
      cwMemorySelection++;

      // Adjust scroll offset if needed
      if (cwMemorySelection >= cwMemoryScrollOffset + 5) {
        cwMemoryScrollOffset = cwMemorySelection - 4;
      }

      drawCWMemoriesUI(display);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;
    }
  }
  else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    // Open context menu
    contextMenuSelection = 0;
    if (cwMemories[cwMemorySelection].isEmpty) {
      contextMenuActive = CONTEXT_EMPTY_SLOT;
    } else {
      contextMenuActive = CONTEXT_OCCUPIED_SLOT;
    }
    drawContextMenu(display);
    beep(TONE_SELECT, BEEP_SHORT);
    return 2;
  }
  else if (key == KEY_ESC) {
    // Exit to radio menu
    return -1;
  }

  return 0;
}

#endif // RADIO_CW_MEMORIES_H
