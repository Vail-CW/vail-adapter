#include "settings_eeprom.h"
#include "config.h"
#include <FlashStorage_SAMD.h>
#include <MIDIUSB.h>

// ============================================================================
// Adapter Settings EEPROM Functions
// ============================================================================

uint8_t loadToneFromEEPROM() {
  if (EEPROM.read(EEPROM_VALID_FLAG_ADDR) == EEPROM_VALID_VALUE) {
    uint8_t txNote = EEPROM.read(EEPROM_TX_NOTE_ADDR);
    return txNote;
  } else {
    Serial.println("EEPROM not initialized, using default tone");
    return DEFAULT_TONE_NOTE;
  }
}

void saveSettingsToEEPROM(uint8_t keyerType, uint16_t ditDuration, uint8_t txNote) {
  EEPROM.write(EEPROM_KEYER_TYPE_ADDR, keyerType);
  EEPROM.put(EEPROM_DIT_DURATION_ADDR, ditDuration);
  EEPROM.write(EEPROM_TX_NOTE_ADDR, txNote);
  EEPROM.write(EEPROM_VALID_FLAG_ADDR, EEPROM_VALID_VALUE);
  EEPROM.commit();
  Serial.print("Saved to EEPROM - Keyer: "); Serial.print(keyerType);
  Serial.print(", Dit Duration: "); Serial.print(ditDuration);
  Serial.print(", TX Note: "); Serial.println(txNote);
}

void saveRadioKeyerModeToEEPROM(bool radioKeyerMode) {
  EEPROM.write(EEPROM_RADIO_KEYER_MODE_ADDR, radioKeyerMode ? 1 : 0);
  EEPROM.commit();
  Serial.print("Saved Radio Keyer Mode to EEPROM: "); Serial.println(radioKeyerMode ? "ON" : "OFF");
}

void loadRadioKeyerModeFromEEPROM(VailAdapter& adapter) {
#ifdef HAS_RADIO_OUTPUT
  if (EEPROM.read(EEPROM_VALID_FLAG_ADDR) == EEPROM_VALID_VALUE) {
    uint8_t radioKeyerModeVal = EEPROM.read(EEPROM_RADIO_KEYER_MODE_ADDR);
    bool radioKeyerMode = (radioKeyerModeVal == 1);

    adapter.SetRadioKeyerMode(radioKeyerMode);
    Serial.print("Loaded Radio Keyer Mode from EEPROM: ");
    Serial.println(radioKeyerMode ? "ON" : "OFF");
  }
#endif
}

void loadSettingsFromEEPROM(VailAdapter& adapter) {
  if (EEPROM.read(EEPROM_VALID_FLAG_ADDR) == EEPROM_VALID_VALUE) {
    uint8_t keyerType = EEPROM.read(EEPROM_KEYER_TYPE_ADDR);
    uint16_t ditDurationVal;
    EEPROM.get(EEPROM_DIT_DURATION_ADDR, ditDurationVal);
    uint8_t txNoteVal = EEPROM.read(EEPROM_TX_NOTE_ADDR);

    Serial.print("EEPROM values - Keyer: "); Serial.print(keyerType);
    Serial.print(", Dit Duration: "); Serial.print(ditDurationVal);
    Serial.print(", TX Note: "); Serial.println(txNoteVal);

    midiEventPacket_t event;
    event.header = 0x0B; event.byte1 = 0xB0;
    event.byte2 = 1;
    event.byte3 = ditDurationVal / (2 * MILLISECOND);
    adapter.HandleMIDI(event);

    event.byte2 = 2;
    event.byte3 = txNoteVal;
    adapter.HandleMIDI(event);

    if (keyerType >= 0 && keyerType <= 9) {
      event.header = 0x0C; event.byte1 = 0xC0;
      event.byte2 = keyerType; event.byte3 = 0;
      adapter.HandleMIDI(event);
    }
  } else {
    Serial.println("EEPROM initializing with default values...");
    EEPROM.write(EEPROM_KEYER_TYPE_ADDR, 8);  // Default to Iambic B
    EEPROM.put(EEPROM_DIT_DURATION_ADDR, (uint16_t)DEFAULT_ADAPTER_DIT_DURATION_MS);
    EEPROM.write(EEPROM_TX_NOTE_ADDR, DEFAULT_TONE_NOTE);
    EEPROM.write(EEPROM_RADIO_KEYER_MODE_ADDR, 0); // Default: Radio Keyer Mode OFF
    EEPROM.write(EEPROM_VALID_FLAG_ADDR, EEPROM_VALID_VALUE);
    EEPROM.commit();
    Serial.println("EEPROM initialized. Loading these defaults now.");
    loadSettingsFromEEPROM(adapter);
  }
}

// ============================================================================
// CW Memory EEPROM Functions
// ============================================================================

uint16_t getEEPROMAddressForSlot(uint8_t slotNumber) {
  // Returns the starting EEPROM address for the given slot (0-2)
  switch (slotNumber) {
    case 0: return EEPROM_MEMORY_1_ADDR;
    case 1: return EEPROM_MEMORY_2_ADDR;
    case 2: return EEPROM_MEMORY_3_ADDR;
    default: return EEPROM_MEMORY_1_ADDR;  // Safety fallback
  }
}

void saveMemoryToEEPROM(uint8_t slotNumber, const CWMemory& memory) {
  if (slotNumber >= MAX_MEMORY_SLOTS) return;  // Safety check

  uint16_t baseAddr = getEEPROMAddressForSlot(slotNumber);

  // Write the transition count (2 bytes)
  EEPROM.put(baseAddr, memory.transitionCount);

  // Write the transition data
  uint16_t dataAddr = baseAddr + MEMORY_LENGTH_SIZE;
  for (uint16_t i = 0; i < memory.transitionCount && i < MAX_TRANSITIONS_PER_MEMORY; i++) {
    EEPROM.put(dataAddr + (i * 2), memory.transitions[i]);
  }

  EEPROM.commit();

  Serial.print("Saved memory slot ");
  Serial.print(slotNumber + 1);
  Serial.print(" - ");
  Serial.print(memory.transitionCount);
  Serial.print(" transitions, ");
  Serial.print(memory.getDurationMs());
  Serial.println("ms duration");
}

void loadMemoryFromEEPROM(uint8_t slotNumber, CWMemory& memory) {
  if (slotNumber >= MAX_MEMORY_SLOTS) {
    memory.clear();
    return;
  }

  uint16_t baseAddr = getEEPROMAddressForSlot(slotNumber);

  // Read the transition count
  uint16_t count;
  EEPROM.get(baseAddr, count);

  // Validate the count
  if (count > MAX_TRANSITIONS_PER_MEMORY) {
    // Invalid data, clear the memory
    memory.clear();
    Serial.print("Memory slot ");
    Serial.print(slotNumber + 1);
    Serial.println(" - invalid data, cleared");
    return;
  }

  memory.transitionCount = count;

  // Read the transition data
  uint16_t dataAddr = baseAddr + MEMORY_LENGTH_SIZE;
  for (uint16_t i = 0; i < count; i++) {
    EEPROM.get(dataAddr + (i * 2), memory.transitions[i]);
  }

  Serial.print("Loaded memory slot ");
  Serial.print(slotNumber + 1);
  Serial.print(" - ");
  Serial.print(memory.transitionCount);
  Serial.print(" transitions, ");
  Serial.print(memory.getDurationMs());
  Serial.println("ms duration");
}

void clearMemoryInEEPROM(uint8_t slotNumber) {
  if (slotNumber >= MAX_MEMORY_SLOTS) return;

  uint16_t baseAddr = getEEPROMAddressForSlot(slotNumber);

  // Write 0 for the transition count
  uint16_t zero = 0;
  EEPROM.put(baseAddr, zero);
  EEPROM.commit();

  Serial.print("Cleared memory slot ");
  Serial.println(slotNumber + 1);
}

void loadMemoriesFromEEPROM(CWMemory memorySlots[]) {
  Serial.println("Loading CW memories from EEPROM...");
  for (uint8_t i = 0; i < MAX_MEMORY_SLOTS; i++) {
    loadMemoryFromEEPROM(i, memorySlots[i]);
  }
}
