/*
 * BLE MIDI Mode
 * Bidirectional BLE MIDI for Vail repeater and MIDI-enabled morse tools
 * - Sends: Processed keyer output as MIDI Note On/Off
 * - Receives: Sidetone from computer, CC messages for speed/tone/keyer type
 *
 * MIDI Protocol (Vail Adapter Spec):
 * - Outgoing: Note 0 (C) for keyed output, Notes 1/2 for passthrough dit/dah
 * - Incoming CC1: Dit duration (speed), CC2: Sidetone note
 * - Incoming Program Change: Keyer type selection
 */

#ifndef BLE_MIDI_H
#define BLE_MIDI_H

#include <NimBLEDevice.h>
#include "ble_core.h"
#include "../core/config.h"
#include "../audio/i2s_audio.h"
#include "../settings/settings_cw.h"

// BLE MIDI Service and Characteristic UUIDs (standard BLE MIDI spec)
#define MIDI_SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define MIDI_CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

// MIDI message types
#define MIDI_NOTE_OFF     0x80
#define MIDI_NOTE_ON      0x90
#define MIDI_CONTROL_CHANGE 0xB0
#define MIDI_PROGRAM_CHANGE 0xC0

// MIDI notes (Vail Adapter spec)
#define MIDI_NOTE_STRAIGHT 0   // C - Straight key / keyed output
#define MIDI_NOTE_DIT      1   // C# - Dit (passthrough mode)
#define MIDI_NOTE_DAH      2   // D - Dah (passthrough mode)

// MIDI Control Change numbers
#define MIDI_CC_DIT_DURATION 1  // CC1 - Dit duration in 2ms units
#define MIDI_CC_SIDETONE_NOTE 2 // CC2 - Sidetone MIDI note number

// MIDI Keyer Programs (Vail Adapter spec)
#define MIDI_KEYER_PASSTHROUGH  0
#define MIDI_KEYER_STRAIGHT     1
#define MIDI_KEYER_BUG          2
#define MIDI_KEYER_IAMBIC_A     7
#define MIDI_KEYER_IAMBIC_B     8

// BLE MIDI state
struct BLEMIDIState {
  bool active = false;
  bool isKeying = false;           // Current key state (for output)
  bool lastDitPressed = false;
  bool lastDahPressed = false;

  // Keyer state machine (for Summit keying modes)
  bool keyerActive = false;
  bool sendingDit = false;
  bool sendingDah = false;
  bool inSpacing = false;
  bool ditMemory = false;
  bool dahMemory = false;
  unsigned long elementTimer = 0;
  unsigned long elementStartTime = 0;

  // MIDI-controlled settings
  int midiDitDuration = 0;         // 0 = use device settings
  int midiSidetoneNote = 69;       // A4 = 440Hz
  int midiKeyerProgram = MIDI_KEYER_IAMBIC_B;  // Default keyer type

  // BLE characteristics
  NimBLEService* midiService = nullptr;
  NimBLECharacteristic* midiCharacteristic = nullptr;

  unsigned long lastUpdateTime = 0;
};

BLEMIDIState btMIDI;

// Forward declarations
void startBTMIDI(LGFX& display);
void drawBTMIDIUI(LGFX& display);
int handleBTMIDIInput(char key, LGFX& display);
void updateBTMIDI();
void stopBTMIDI();
void sendMIDINoteOn(uint8_t note, uint8_t velocity);
void sendMIDINoteOff(uint8_t note);
void onMIDIReceived(uint8_t* data, size_t length);
void btMidiKeyerHandler();
void btMidiPassthroughHandler();
int midiNoteToFrequency(int note);
int getDitDuration();

// Convert MIDI note to frequency (equal temperament, A4=440Hz)
int midiNoteToFrequency(int note) {
  // f = 440 * 2^((n-69)/12)
  float freq = 440.0 * pow(2.0, (note - 69) / 12.0);
  return (int)freq;
}

// Get current dit duration (MIDI override or device setting)
int getDitDuration() {
  if (btMIDI.midiDitDuration > 0) {
    return btMIDI.midiDitDuration;
  }
  return DIT_DURATION(cwSpeed);
}

// MIDI characteristic callback for receiving data
class MIDICharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    NimBLEAttValue value = pCharacteristic->getValue();
    Serial.print("MIDI onWrite: ");
    Serial.print(value.length());
    Serial.println(" bytes");
    if (value.length() > 0) {
      onMIDIReceived((uint8_t*)value.data(), value.length());
    }
  }

  void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    Serial.println("MIDI onRead");
  }

  void onNotify(NimBLECharacteristic* pCharacteristic) override {
    Serial.println("MIDI onNotify");
  }

  void onStatus(NimBLECharacteristic* pCharacteristic, int code) override {
    Serial.print("MIDI onStatus: code=");
    Serial.println(code);
  }
};

// Process received MIDI data
// BLE MIDI packet format: [Header][Timestamp][Status][Data...][Timestamp][Status][Data...]...
// Header byte: 1ttttttt (bit 7 always 1, bits 0-5 = timestamp high)
// Timestamp byte: 1ttttttt (bit 7 always 1, bits 0-6 = timestamp low)
// MIDI messages follow timestamps
void onMIDIReceived(uint8_t* data, size_t length) {
  if (length < 3) return;  // Minimum: header + timestamp + 1 MIDI byte

  Serial.print("BLE MIDI RX: ");
  for (size_t i = 0; i < length; i++) {
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // First byte is header (must have bit 7 set)
  if (!(data[0] & 0x80)) return;

  size_t pos = 1;  // Skip header byte
  uint8_t runningStatus = 0;

  while (pos < length) {
    // Check for timestamp byte (bit 7 set, but not a status byte pattern we care about)
    if ((data[pos] & 0x80) && pos + 1 < length) {
      // This could be a timestamp or a status byte
      uint8_t byte = data[pos];

      // Check if next byte looks like MIDI data (bit 7 clear) - then this is likely timestamp
      if (!(data[pos + 1] & 0x80)) {
        // Likely a timestamp followed by data using running status
        pos++;
        continue;
      }

      // Check if this is a MIDI status byte (0x80-0xEF for channel messages)
      uint8_t msgType = byte & 0xF0;
      if (msgType >= 0x80 && msgType <= 0xE0) {
        runningStatus = byte;
        pos++;

        // Parse MIDI message based on status
        if (msgType == MIDI_NOTE_ON && pos + 1 < length) {
          uint8_t note = data[pos] & 0x7F;
          uint8_t velocity = data[pos + 1] & 0x7F;
          pos += 2;

          if (velocity > 0) {
            int freq = midiNoteToFrequency(note);
            Serial.print("MIDI Note On: note=");
            Serial.print(note);
            Serial.print(" freq=");
            Serial.println(freq);
            startTone(freq);
          } else {
            Serial.println("MIDI Note Off (velocity 0)");
            stopTone();
          }

        } else if (msgType == MIDI_NOTE_OFF && pos + 1 < length) {
          pos += 2;  // Skip note and velocity
          Serial.println("MIDI Note Off");
          stopTone();

        } else if (msgType == MIDI_CONTROL_CHANGE && pos + 1 < length) {
          uint8_t cc = data[pos] & 0x7F;
          uint8_t value = data[pos + 1] & 0x7F;
          pos += 2;

          if (cc == MIDI_CC_DIT_DURATION) {
            btMIDI.midiDitDuration = value * 2;
            Serial.print("MIDI CC1 Dit Duration: ");
            Serial.print(btMIDI.midiDitDuration);
            Serial.println("ms");
          } else if (cc == MIDI_CC_SIDETONE_NOTE) {
            btMIDI.midiSidetoneNote = value;
            Serial.print("MIDI CC2 Sidetone Note: ");
            Serial.println(value);
          }

        } else if (msgType == MIDI_PROGRAM_CHANGE && pos < length) {
          uint8_t program = data[pos] & 0x7F;
          pos++;
          btMIDI.midiKeyerProgram = program;
          Serial.print("MIDI Program Change: ");
          Serial.println(program);

        } else {
          // Unknown message type, skip
          pos++;
        }
      } else {
        // Timestamp byte, skip it
        pos++;
      }
    } else {
      // Data byte or unknown, skip
      pos++;
    }
  }
}

// Send MIDI Note On
void sendMIDINoteOn(uint8_t note, uint8_t velocity) {
  if (!btMIDI.active || btMIDI.midiCharacteristic == nullptr) return;
  if (!isBLEConnected()) return;

  // BLE MIDI packet: [Header, Timestamp High, Status, Note, Velocity]
  uint8_t timestamp = (millis() & 0x1FFF) >> 7;  // 6-bit timestamp high
  uint8_t packet[5] = {
    (uint8_t)(0x80 | timestamp),    // Header with timestamp
    (uint8_t)(0x80 | (millis() & 0x7F)),  // Timestamp low
    (uint8_t)(MIDI_NOTE_ON | 0),    // Note On, channel 0
    note,
    velocity
  };

  btMIDI.midiCharacteristic->setValue(packet, sizeof(packet));
  btMIDI.midiCharacteristic->notify();

  Serial.print("Sent MIDI Note On: ");
  Serial.println(note);
}

// Send MIDI Note Off
void sendMIDINoteOff(uint8_t note) {
  if (!btMIDI.active || btMIDI.midiCharacteristic == nullptr) return;
  if (!isBLEConnected()) return;

  uint8_t timestamp = (millis() & 0x1FFF) >> 7;
  uint8_t packet[5] = {
    (uint8_t)(0x80 | timestamp),
    (uint8_t)(0x80 | (millis() & 0x7F)),
    (uint8_t)(MIDI_NOTE_OFF | 0),
    note,
    0x40  // Release velocity
  };

  btMIDI.midiCharacteristic->setValue(packet, sizeof(packet));
  btMIDI.midiCharacteristic->notify();

  Serial.print("Sent MIDI Note Off: ");
  Serial.println(note);
}

// Start BT MIDI mode
void startBTMIDI(LGFX& display) {
  Serial.println("Starting BT MIDI mode");

  // Reset state
  btMIDI.active = true;
  btMIDI.isKeying = false;
  btMIDI.lastDitPressed = false;
  btMIDI.lastDahPressed = false;
  btMIDI.keyerActive = false;
  btMIDI.inSpacing = false;
  btMIDI.ditMemory = false;
  btMIDI.dahMemory = false;
  btMIDI.midiDitDuration = 0;  // Use device settings by default
  btMIDI.midiSidetoneNote = 69;
  btMIDI.midiKeyerProgram = MIDI_KEYER_IAMBIC_B;
  btMIDI.lastUpdateTime = millis();

  // Initialize BLE core if not already done
  initBLECore();
  bleCore.activeMode = BLE_MODE_MIDI;

  // Create MIDI service
  btMIDI.midiService = bleCore.pServer->createService(MIDI_SERVICE_UUID);
  Serial.print("BLE MIDI: Service UUID = ");
  Serial.println(MIDI_SERVICE_UUID);

  // Create MIDI characteristic with standard BLE MIDI properties
  btMIDI.midiCharacteristic = btMIDI.midiService->createCharacteristic(
    MIDI_CHARACTERISTIC_UUID,
    NIMBLE_PROPERTY::READ |
    NIMBLE_PROPERTY::WRITE |
    NIMBLE_PROPERTY::NOTIFY |
    NIMBLE_PROPERTY::WRITE_NR
  );
  Serial.print("BLE MIDI: Characteristic UUID = ");
  Serial.println(MIDI_CHARACTERISTIC_UUID);

  // Set callback for receiving MIDI
  btMIDI.midiCharacteristic->setCallbacks(new MIDICharacteristicCallbacks());

  // Set security for bonding (required for some BLE MIDI clients)
  NimBLEDevice::setSecurityAuth(true, false, true);  // bonding, MITM, SC

  // Start service
  btMIDI.midiService->start();
  Serial.println("BLE MIDI: Service started");

  // Set up advertising
  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->addServiceUUID(btMIDI.midiService->getUUID());
  advertising->setAppearance(0x00);  // Generic appearance

  // Start advertising
  startBLEAdvertising("MIDI");

  drawBTMIDIUI(display);
}

// Stop BT MIDI mode
void stopBTMIDI() {
  Serial.println("Stopping BT MIDI mode");

  // Send note off if keying
  if (btMIDI.active && btMIDI.isKeying) {
    sendMIDINoteOff(MIDI_NOTE_STRAIGHT);
    btMIDI.isKeying = false;
  }

  // Stop any local sidetone
  stopTone();

  btMIDI.active = false;
  btMIDI.midiService = nullptr;
  btMIDI.midiCharacteristic = nullptr;

  // Deinit BLE
  deinitBLECore();
}

// Draw BT MIDI UI
void drawBTMIDIUI(LGFX& display) {
  // Clear screen (preserve header)
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Status card
  int cardX = 20;
  int cardY = 55;
  int cardW = SCREEN_WIDTH - 40;
  int cardH = 100;

  display.fillRoundRect(cardX, cardY, cardW, cardH, 12, 0x1082);
  display.drawRoundRect(cardX, cardY, cardW, cardH, 12, 0x34BF);

  // Connection status
  display.setFont(&FreeSansBold12pt7b);
  display.setTextSize(1);

  int yPos = cardY + 30;
  display.setCursor(cardX + 15, yPos);

  if (isBLEConnected()) {
    display.setTextColor(ST77XX_GREEN);
    display.print("Connected");
  } else if (isBLEAdvertising()) {
    display.setTextColor(ST77XX_YELLOW);
    display.print("Advertising...");
  } else {
    display.setTextColor(ST77XX_RED);
    display.print("Disconnected");
  }

  // Device name
  display.setFont(nullptr);
  display.setTextSize(2);
  display.setTextColor(ST77XX_CYAN);
  yPos += 35;
  display.setCursor(cardX + 15, yPos);
  display.print(getBLEDeviceName());

  // Settings card
  cardY = 170;
  cardH = 90;
  display.fillRoundRect(cardX, cardY, cardW, cardH, 12, 0x1082);
  display.drawRoundRect(cardX, cardY, cardW, cardH, 12, 0x34BF);

  display.setTextSize(1);
  display.setTextColor(0x7BEF);
  yPos = cardY + 12;
  display.setCursor(cardX + 15, yPos);
  display.print("Current Settings:");

  display.setTextSize(2);
  display.setTextColor(ST77XX_WHITE);

  // Speed
  yPos += 20;
  display.setCursor(cardX + 15, yPos);
  display.print("Speed: ");
  if (btMIDI.midiDitDuration > 0) {
    int wpm = 1200 / btMIDI.midiDitDuration;
    display.print(wpm);
    display.print(" WPM (MIDI)");
  } else {
    display.print(cwSpeed);
    display.print(" WPM");
  }

  // Keyer type
  yPos += 22;
  display.setCursor(cardX + 15, yPos);
  display.print("Keyer: ");
  switch (btMIDI.midiKeyerProgram) {
    case MIDI_KEYER_PASSTHROUGH: display.print("Passthrough"); break;
    case MIDI_KEYER_STRAIGHT: display.print("Straight"); break;
    case MIDI_KEYER_BUG: display.print("Bug"); break;
    case MIDI_KEYER_IAMBIC_A: display.print("Iambic A"); break;
    case MIDI_KEYER_IAMBIC_B: display.print("Iambic B"); break;
    default: display.print("Unknown"); break;
  }

  // Instructions
  display.setTextSize(1);
  display.setTextColor(ST77XX_YELLOW);
  display.setCursor(cardX + 15, SCREEN_HEIGHT - 50);
  display.print("macOS: Works with Web MIDI natively");
  display.setCursor(cardX + 15, SCREEN_HEIGHT - 35);
  display.print("Windows: Needs MIDIberry or similar app");

  display.setFont(nullptr);
}

// Handle BT MIDI keyboard input
int handleBTMIDIInput(char key, LGFX& display) {
  if (key == KEY_ESC) {
    stopBTMIDI();
    return -1;  // Exit mode
  }

  return 0;  // Normal input
}

// Passthrough handler (raw dit/dah)
void btMidiPassthroughHandler() {
  bool ditPressed = (digitalRead(DIT_PIN) == PADDLE_ACTIVE) ||
                    (touchRead(TOUCH_DIT_PIN) > TOUCH_THRESHOLD);
  bool dahPressed = (digitalRead(DAH_PIN) == PADDLE_ACTIVE) ||
                    (touchRead(TOUCH_DAH_PIN) > TOUCH_THRESHOLD);

  // Send dit state changes
  if (ditPressed != btMIDI.lastDitPressed) {
    if (ditPressed) {
      sendMIDINoteOn(MIDI_NOTE_DIT, 127);
      startTone(TONE_SIDETONE);
    } else {
      sendMIDINoteOff(MIDI_NOTE_DIT);
      if (!dahPressed) stopTone();
    }
    btMIDI.lastDitPressed = ditPressed;
  }

  // Send dah state changes
  if (dahPressed != btMIDI.lastDahPressed) {
    if (dahPressed) {
      sendMIDINoteOn(MIDI_NOTE_DAH, 127);
      startTone(TONE_SIDETONE);
    } else {
      sendMIDINoteOff(MIDI_NOTE_DAH);
      if (!ditPressed) stopTone();
    }
    btMIDI.lastDahPressed = dahPressed;
  }

  // Keep audio buffer filled while any paddle pressed
  if (ditPressed || dahPressed) {
    continueTone(TONE_SIDETONE);
  }
}

// Iambic keyer handler (processes paddle input, outputs keyed MIDI)
void btMidiKeyerHandler() {
  bool ditPressed = (digitalRead(DIT_PIN) == PADDLE_ACTIVE) ||
                    (touchRead(TOUCH_DIT_PIN) > TOUCH_THRESHOLD);
  bool dahPressed = (digitalRead(DAH_PIN) == PADDLE_ACTIVE) ||
                    (touchRead(TOUCH_DAH_PIN) > TOUCH_THRESHOLD);

  unsigned long currentTime = millis();
  int ditDuration = getDitDuration();
  int dahDuration = ditDuration * 3;

  // Straight key mode
  if (btMIDI.midiKeyerProgram == MIDI_KEYER_STRAIGHT) {
    bool keyDown = ditPressed || dahPressed;
    if (keyDown != btMIDI.isKeying) {
      if (keyDown) {
        sendMIDINoteOn(MIDI_NOTE_STRAIGHT, 127);
        startTone(TONE_SIDETONE);
      } else {
        sendMIDINoteOff(MIDI_NOTE_STRAIGHT);
        stopTone();
      }
      btMIDI.isKeying = keyDown;
    }
    btMIDI.lastDitPressed = ditPressed;
    btMIDI.lastDahPressed = dahPressed;
    return;
  }

  // Iambic keyer state machine
  if (!btMIDI.keyerActive && !btMIDI.inSpacing) {
    // IDLE state - check for paddle presses
    if (ditPressed || dahPressed) {
      if (ditPressed) {
        btMIDI.sendingDit = true;
        btMIDI.sendingDah = false;
      } else {
        btMIDI.sendingDit = false;
        btMIDI.sendingDah = true;
      }

      btMIDI.keyerActive = true;
      btMIDI.elementTimer = currentTime + (btMIDI.sendingDit ? ditDuration : dahDuration);
      btMIDI.elementStartTime = currentTime;

      // Key on
      sendMIDINoteOn(MIDI_NOTE_STRAIGHT, 127);
      startTone(TONE_SIDETONE);
      btMIDI.isKeying = true;
    }
  }
  else if (btMIDI.keyerActive) {
    // SENDING state - outputting dit or dah

    // Check for memory paddle presses
    if (ditPressed && !btMIDI.sendingDit) btMIDI.ditMemory = true;
    if (dahPressed && !btMIDI.sendingDah) btMIDI.dahMemory = true;

    // Check if element duration completed
    if (currentTime >= btMIDI.elementTimer) {
      // Key off
      sendMIDINoteOff(MIDI_NOTE_STRAIGHT);
      stopTone();
      btMIDI.isKeying = false;

      // Enter spacing state
      btMIDI.keyerActive = false;
      btMIDI.inSpacing = true;
      btMIDI.elementTimer = currentTime + ditDuration;  // Element gap
    }
  }
  else if (btMIDI.inSpacing) {
    // SPACING state - inter-element gap

    // Check for memory paddle presses
    if (ditPressed && !btMIDI.sendingDit) btMIDI.ditMemory = true;
    if (dahPressed && !btMIDI.sendingDah) btMIDI.dahMemory = true;

    // Check if spacing completed
    if (currentTime >= btMIDI.elementTimer) {
      btMIDI.inSpacing = false;

      // Check for queued element
      bool sendNextElement = false;
      bool nextIsDit = false;

      bool isIambicB = (btMIDI.midiKeyerProgram == MIDI_KEYER_IAMBIC_B);

      if (isIambicB) {
        // Iambic B: alternate on squeeze
        if (btMIDI.ditMemory && btMIDI.dahMemory) {
          nextIsDit = !btMIDI.sendingDit;
          sendNextElement = true;
        } else if (btMIDI.ditMemory) {
          nextIsDit = true;
          sendNextElement = true;
        } else if (btMIDI.dahMemory) {
          nextIsDit = false;
          sendNextElement = true;
        }
      } else {
        // Iambic A: memory only
        if (btMIDI.ditMemory) {
          nextIsDit = true;
          sendNextElement = true;
        } else if (btMIDI.dahMemory) {
          nextIsDit = false;
          sendNextElement = true;
        }
      }

      if (sendNextElement) {
        btMIDI.sendingDit = nextIsDit;
        btMIDI.sendingDah = !nextIsDit;
        btMIDI.keyerActive = true;
        btMIDI.elementTimer = currentTime + (nextIsDit ? ditDuration : dahDuration);
        btMIDI.elementStartTime = currentTime;

        if (nextIsDit) btMIDI.ditMemory = false;
        else btMIDI.dahMemory = false;

        // Key on
        sendMIDINoteOn(MIDI_NOTE_STRAIGHT, 127);
        startTone(TONE_SIDETONE);
        btMIDI.isKeying = true;
      } else {
        btMIDI.ditMemory = false;
        btMIDI.dahMemory = false;
      }
    }
  }

  btMIDI.lastDitPressed = ditPressed;
  btMIDI.lastDahPressed = dahPressed;
}

// Update BT MIDI (called from main loop)
void updateBTMIDI() {
  if (!btMIDI.active) return;

  // Route to appropriate handler based on keyer program
  if (btMIDI.midiKeyerProgram == MIDI_KEYER_PASSTHROUGH) {
    btMidiPassthroughHandler();
  } else {
    btMidiKeyerHandler();
  }

  // Keep audio buffer filled while keying
  if (btMIDI.isKeying) {
    continueTone(TONE_SIDETONE);
  }

  btMIDI.lastUpdateTime = millis();
}

#endif // BLE_MIDI_H
