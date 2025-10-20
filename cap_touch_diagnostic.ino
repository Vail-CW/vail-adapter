// Capacitive Touch Diagnostic Sketch
// Reads and displays QT_DIT and QT_DAH capacitive touch values
// Use this to determine proper threshold values

#include <Adafruit_FreeTouch.h>

// Advanced_PCB pin configuration
#define QT_DIT_PIN A7
#define QT_DAH_PIN A6

Adafruit_FreeTouch qt_dit = Adafruit_FreeTouch(QT_DIT_PIN, OVERSAMPLE_2, RESISTOR_0, FREQ_MODE_SPREAD);
Adafruit_FreeTouch qt_dah = Adafruit_FreeTouch(QT_DAH_PIN, OVERSAMPLE_2, RESISTOR_0, FREQ_MODE_SPREAD);

int ditMin = 9999, ditMax = 0;
int dahMin = 9999, dahMax = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000); // Wait for serial or 3 seconds

  Serial.println("\n=== Capacitive Touch Diagnostic ===");
  Serial.println("Touch pads individually and together to see values\n");

  if (!qt_dit.begin()) {
    Serial.println("ERROR: Failed to initialize QT_DIT");
  }
  if (!qt_dah.begin()) {
    Serial.println("ERROR: Failed to initialize QT_DAH");
  }

  delay(500);
  Serial.println("Starting measurements...\n");
  Serial.println("DIT\tDAH\t[DIT min-max]\t[DAH min-max]");
}

void loop() {
  int ditVal = qt_dit.measure();
  int dahVal = qt_dah.measure();

  // Track min/max
  if (ditVal < ditMin) ditMin = ditVal;
  if (ditVal > ditMax) ditMax = ditVal;
  if (dahVal < dahMin) dahMin = dahVal;
  if (dahVal > dahMax) dahMax = dahVal;

  // Print current values and ranges
  Serial.print(ditVal);
  Serial.print("\t");
  Serial.print(dahVal);
  Serial.print("\t[");
  Serial.print(ditMin);
  Serial.print("-");
  Serial.print(ditMax);
  Serial.print("]\t[");
  Serial.print(dahMin);
  Serial.print("-");
  Serial.print(dahMax);
  Serial.println("]");

  delay(100); // Update 10 times per second
}
