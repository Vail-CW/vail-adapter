// R2R Button Combination Tester for Qt Py SAMD21
// Tests all two-button combinations

const int BUTTON_PIN = 3;
const int NUM_SAMPLES = 20;

// Combinations to test
const char* combos[] = {
  "B1 + B2",
  "B1 + B3",
  "B2 + B3"
};
const int NUM_COMBOS = 3;

int currentCombo = 0;
bool waitingForPress = true;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect
  }

  pinMode(BUTTON_PIN, INPUT);

  Serial.println("R2R Button Combination Tester");
  Serial.println("================================");
  Serial.println();
  promptForCombo();
}

void promptForCombo() {
  if (currentCombo < NUM_COMBOS) {
    Serial.print("Press and HOLD: ");
    Serial.println(combos[currentCombo]);
    Serial.println("(Hold steady for 2 seconds)");
    waitingForPress = true;
  } else {
    Serial.println();
    Serial.println("All combinations tested!");
    Serial.println("================================");
  }
}

void loop() {
  if (currentCombo >= NUM_COMBOS) {
    return; // Done testing
  }

  if (waitingForPress) {
    delay(2000); // Wait 2 seconds for user to hold buttons

    // Now take reading
    long sum = 0;
    int minVal = 1023;
    int maxVal = 0;

    Serial.print("Reading...");

    for (int i = 0; i < NUM_SAMPLES; i++) {
      int reading = analogRead(BUTTON_PIN);
      sum += reading;
      minVal = min(minVal, reading);
      maxVal = max(maxVal, reading);
      delay(10);
    }

    int avgValue = sum / NUM_SAMPLES;
    float voltage = (avgValue / 1023.0) * 3.3;

    Serial.println(" Done!");
    Serial.print("  Raw: ");
    Serial.print(avgValue);
    Serial.print("\t Voltage: ");
    Serial.print(voltage, 3);
    Serial.print("V \t Range: ");
    Serial.print(minVal);
    Serial.print("-");
    Serial.println(maxVal);
    Serial.println();

    waitingForPress = false;
    currentCombo++;

    delay(1000); // Give time to release buttons
    promptForCombo();
  }
}
