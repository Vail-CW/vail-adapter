// Test different FreeTouch configurations to find one that works for squeeze
// Tests various OVERSAMPLE and RESISTOR combinations

#include <Adafruit_FreeTouch.h>

#define QT_DIT_PIN A7
#define QT_DAH_PIN A6

Adafruit_FreeTouch qt_dit;
Adafruit_FreeTouch qt_dah;

const char* configNames[] = {
  "OVERSAMPLE_1, RESISTOR_0",
  "OVERSAMPLE_2, RESISTOR_0 (Current)",
  "OVERSAMPLE_4, RESISTOR_0",
  "OVERSAMPLE_8, RESISTOR_0",
  "OVERSAMPLE_2, RESISTOR_50K",
  "OVERSAMPLE_2, RESISTOR_100K",
  "OVERSAMPLE_4, RESISTOR_50K",
};

oversample_t oversampleVals[] = {OVERSAMPLE_1, OVERSAMPLE_2, OVERSAMPLE_4, OVERSAMPLE_8, OVERSAMPLE_2, OVERSAMPLE_2, OVERSAMPLE_4};
series_resistor_t resistorVals[] = {RESISTOR_0, RESISTOR_0, RESISTOR_0, RESISTOR_0, RESISTOR_50K, RESISTOR_100K, RESISTOR_50K};

int numConfigs = sizeof(configNames) / sizeof(configNames[0]);
int currentConfig = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000);

  Serial.println("\n╔════════════════════════════════════════════════╗");
  Serial.println("║      FREETOUCH CONFIGURATION TESTER           ║");
  Serial.println("╚════════════════════════════════════════════════╝\n");
  Serial.println("This tests different FreeTouch settings to find");
  Serial.println("one that doesn't collapse during squeeze.\n");
  delay(1000);

  startNextConfig();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'n' || cmd == 'N' || cmd == '\n' || cmd == '\r') {
      currentConfig++;
      if (currentConfig >= numConfigs) {
        Serial.println("\n✓ All configurations tested!");
        Serial.println("Review results and choose the best one.");
        while(1);
      }
      startNextConfig();
    }
  }

  // Continuous reading
  int ditVal = qt_dit.measure();
  int dahVal = qt_dah.measure();

  Serial.print("DIT: ");
  Serial.print(ditVal);
  Serial.print("\tDAH: ");
  Serial.println(dahVal);

  delay(100);
}

void startNextConfig() {
  Serial.println("\n┌────────────────────────────────────────────────┐");
  Serial.print("│ CONFIG ");
  Serial.print(currentConfig + 1);
  Serial.print("/");
  Serial.print(numConfigs);
  Serial.print(": ");
  Serial.print(configNames[currentConfig]);
  for (int i = strlen(configNames[currentConfig]); i < 33; i++) Serial.print(" ");
  Serial.println(" │");
  Serial.println("└────────────────────────────────────────────────┘");

  // Reinitialize with new settings
  qt_dit = Adafruit_FreeTouch(QT_DIT_PIN, oversampleVals[currentConfig],
                              resistorVals[currentConfig], FREQ_MODE_SPREAD);
  qt_dah = Adafruit_FreeTouch(QT_DAH_PIN, oversampleVals[currentConfig],
                              resistorVals[currentConfig], FREQ_MODE_SPREAD);

  if (!qt_dit.begin() || !qt_dah.begin()) {
    Serial.println("❌ Failed to initialize with this config!");
    Serial.println("Press ENTER to try next configuration...");
    return;
  }

  Serial.println("✓ Initialized");
  Serial.println("\nTest procedure:");
  Serial.println("  1. Watch baseline values (no touch)");
  Serial.println("  2. Touch DIT - note the value");
  Serial.println("  3. Touch DAH - note the value");
  Serial.println("  4. Touch BOTH - CHECK IF VALUES STAY HIGH!");
  Serial.println("\nPress ENTER for next configuration...\n");
}
