#ifndef SETTINGS_EEPROM_H
#define SETTINGS_EEPROM_H

#include <Arduino.h>
#include "adapter.h"
#include "memory.h"

// EEPROM operations for adapter settings
void saveSettingsToEEPROM(uint8_t keyerType, uint16_t ditDuration, uint8_t txNote);
void saveRadioKeyerModeToEEPROM(bool radioKeyerMode);
void loadSettingsFromEEPROM(VailAdapter& adapter);
void loadRadioKeyerModeFromEEPROM(VailAdapter& adapter);
uint8_t loadToneFromEEPROM();

// EEPROM operations for CW memory slots
uint16_t getEEPROMAddressForSlot(uint8_t slotNumber);
void saveMemoryToEEPROM(uint8_t slotNumber, const CWMemory& memory);
void loadMemoryFromEEPROM(uint8_t slotNumber, CWMemory& memory);
void clearMemoryInEEPROM(uint8_t slotNumber);
void loadMemoriesFromEEPROM(CWMemory memorySlots[]);

#endif // SETTINGS_EEPROM_H
