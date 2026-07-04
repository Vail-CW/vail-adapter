/*
 * Button Hat Troubleshooting Tool
 *
 * This sketch tests the resistor ladder network on Vail Adapter button hats.
 * It prompts the user to press each button combination and records the analog values.
 *
 * Usage:
 * 1. Flash this firmware to your QT Py SAMD21
 * 2. Open Serial Monitor at 115200 baud
 * 3. Follow the prompts to test each button
 * 4. Review the results to diagnose button hat issues
 */

// Pin configuration for Advanced PCB (also works for other variants)
#define BUTTON_PIN 8  // R2R button ladder input (pin 3 on QT Py)
#define SAMPLE_COUNT 50  // Number of samples to average per button test

// Expected values for Advanced PCB (for reference)
const char* expectedValues =
  "Expected values for Advanced PCB:\n"
  "  None:   0-100   (typical: 0)\n"
  "  B3:     260-300 (typical: 280)\n"
  "  B2:     315-355 (typical: 335)\n"
  "  B2+B3:  377-405 (typical: 397)\n"
  "  B1:     406-435 (typical: 414)\n"
  "  B1+B3:  470-502 (typical: 490)\n"
  "  B1+B2:  503-650 (typical: 516)\n";

// Storage for test results
struct ButtonTest {
  const char* name;
  int minValue;
  int maxValue;
  int avgValue;
  bool tested;
};

ButtonTest tests[] = {
  {"None (no buttons)", 0, 0, 0, false},
  {"Button 3", 0, 0, 0, false},
  {"Button 2", 0, 0, 0, false},
  {"Button 1", 0, 0, 0, false},
  {"Buttons 2+3", 0, 0, 0, false},
  {"Buttons 1+3", 0, 0, 0, false},
  {"Buttons 1+2", 0, 0, 0, false}
};

const int NUM_TESTS = sizeof(tests) / sizeof(tests[0]);
int currentTest = 0;
bool waitingForInput = true;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);

  // Wait for serial connection
  while (!Serial) {
    delay(10);
  }

  delay(1000);  // Give user time to open serial monitor

  Serial.println("\n\n===========================================");
  Serial.println("  Vail Adapter Button Hat Diagnostic Tool");
  Serial.println("===========================================\n");

  Serial.println("This tool will test your button hat's resistor ladder network.");
  Serial.println("Follow the prompts to press each button combination.\n");

  Serial.println(expectedValues);
  Serial.println("\nReady to begin testing.\n");

  promptForNextTest();
}

void loop() {
  if (Serial.available() > 0) {
    // Clear the input buffer
    while (Serial.available() > 0) {
      Serial.read();
    }

    if (waitingForInput) {
      // Perform the test
      performTest();

      // Move to next test
      currentTest++;

      if (currentTest < NUM_TESTS) {
        promptForNextTest();
      } else {
        displayResults();
        currentTest = 0;  // Reset for another round if desired
        Serial.println("\n\nPress Enter to run tests again...");
      }
    }
  }
}

void promptForNextTest() {
  waitingForInput = true;
  Serial.println("------------------------------------------");
  Serial.print("Test ");
  Serial.print(currentTest + 1);
  Serial.print(" of ");
  Serial.print(NUM_TESTS);
  Serial.print(": ");
  Serial.println(tests[currentTest].name);

  if (currentTest == 0) {
    Serial.println("Release all buttons, then press Enter");
  } else {
    Serial.print("Hold ");
    Serial.print(tests[currentTest].name);
    Serial.println(", then press Enter");
  }
}

void performTest() {
  Serial.println("\nSampling...");

  long sum = 0;
  int minVal = 1023;
  int maxVal = 0;

  // Take multiple samples
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    int reading = analogRead(BUTTON_PIN);
    sum += reading;

    if (reading < minVal) minVal = reading;
    if (reading > maxVal) maxVal = reading;

    delay(10);  // Small delay between samples
  }

  int avgVal = sum / SAMPLE_COUNT;

  // Store results
  tests[currentTest].minValue = minVal;
  tests[currentTest].maxValue = maxVal;
  tests[currentTest].avgValue = avgVal;
  tests[currentTest].tested = true;

  // Display immediate feedback
  Serial.print("  Average: ");
  Serial.print(avgVal);
  Serial.print("  (Range: ");
  Serial.print(minVal);
  Serial.print(" - ");
  Serial.print(maxVal);
  Serial.println(")");

  waitingForInput = false;
}

void displayResults() {
  Serial.println("\n\n===========================================");
  Serial.println("           TEST RESULTS SUMMARY");
  Serial.println("===========================================\n");

  Serial.println("Button State       | Min   | Max   | Avg   | Status");
  Serial.println("-------------------|-------|-------|-------|--------");

  for (int i = 0; i < NUM_TESTS; i++) {
    if (tests[i].tested) {
      // Format button name with padding
      String name = String(tests[i].name);
      while (name.length() < 18) {
        name += " ";
      }
      Serial.print(name);
      Serial.print(" | ");

      // Print values with padding
      printPaddedNumber(tests[i].minValue, 5);
      Serial.print(" | ");
      printPaddedNumber(tests[i].maxValue, 5);
      Serial.print(" | ");
      printPaddedNumber(tests[i].avgValue, 5);
      Serial.print(" | ");

      // Check status based on expected ranges for Advanced PCB
      String status = getStatus(i, tests[i].avgValue);
      Serial.println(status);
    }
  }

  Serial.println("\n===========================================");
  Serial.println("\nDiagnostic Tips:");
  Serial.println("- Large range (Max-Min) indicates noisy connections");
  Serial.println("- Values outside expected range may indicate:");
  Serial.println("  * Wrong resistor values in ladder network");
  Serial.println("  * Poor solder joints");
  Serial.println("  * Damaged components");
  Serial.println("- Overlapping ranges between buttons will cause");
  Serial.println("  unreliable button detection");
  Serial.println("\nSave these results for troubleshooting!");
}

void printPaddedNumber(int num, int width) {
  String str = String(num);
  while (str.length() < width) {
    str = " " + str;
  }
  Serial.print(str);
}

String getStatus(int testIndex, int avgValue) {
  // Expected ranges for Advanced PCB
  switch (testIndex) {
    case 0:  // None
      if (avgValue >= 0 && avgValue <= 100) return "GOOD";
      return "CHECK";
    case 1:  // B3
      if (avgValue >= 260 && avgValue <= 300) return "GOOD";
      return "CHECK";
    case 2:  // B2
      if (avgValue >= 315 && avgValue <= 355) return "GOOD";
      return "CHECK";
    case 3:  // B1
      if (avgValue >= 406 && avgValue <= 435) return "GOOD";
      return "CHECK";
    case 4:  // B2+B3
      if (avgValue >= 377 && avgValue <= 405) return "GOOD";
      return "CHECK";
    case 5:  // B1+B3
      if (avgValue >= 470 && avgValue <= 502) return "GOOD";
      return "CHECK";
    case 6:  // B1+B2
      if (avgValue >= 503 && avgValue <= 650) return "GOOD";
      return "CHECK";
    default:
      return "UNKNOWN";
  }
}
