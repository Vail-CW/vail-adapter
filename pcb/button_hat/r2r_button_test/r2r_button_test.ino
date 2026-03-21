// R2R Button Test for Qt Py SAMD21
// Detects single and combo button presses from R2R ladder

const int BUTTON_PIN = 3;
const int NUM_SAMPLES = 10; // Reduced for faster response
const int DEBOUNCE_COUNT = 2; // Reduced for faster clicks

// Button states
const int BTN_NONE = 0;
const int BTN_B1 = 1;
const int BTN_B2 = 2;
const int BTN_B3 = 3;
const int BTN_B1_B2 = 4;
const int BTN_B1_B3 = 5;
const int BTN_B2_B3 = 6;

// Thresholds based on observed values:
// None: ~2, B1: ~512, B2: ~615, B3: ~682, B1+B2: ~768, B1+B3: ~820, B2+B3: ~830
const int NONE_MIN = 0;
const int NONE_MAX = 300;
const int B1_MIN = 400;
const int B1_MAX = 560;
const int B2_MIN = 580;
const int B2_MAX = 650;
const int B3_MIN = 665;
const int B3_MAX = 700;
const int B1_B2_MIN = 750;
const int B1_B2_MAX = 790;
const int B1_B3_MIN = 805;
const int B1_B3_MAX = 825;
const int B2_B3_MIN = 826;
const int B2_B3_MAX = 850;

int maxButtonState = BTN_NONE; // Tracks the highest button combo seen
int lastReportedState = BTN_NONE;
int debounceCounter = 0;
int pendingButton = BTN_NONE;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect
  }

  pinMode(BUTTON_PIN, INPUT);

  Serial.println("R2R Button Detector - Single & Combo");
  Serial.println("Supports: B1, B2, B3, B1+B2, B1+B3, B2+B3");
  Serial.println("---");
}

void printButtonState(int state) {
  switch(state) {
    case BTN_B1:
      Serial.println("B1 pressed");
      break;
    case BTN_B2:
      Serial.println("B2 pressed");
      break;
    case BTN_B3:
      Serial.println("B3 pressed");
      break;
    case BTN_B1_B2:
      Serial.println("B1+B2 pressed");
      break;
    case BTN_B1_B3:
      Serial.println("B1+B3 pressed");
      break;
    case BTN_B2_B3:
      Serial.println("B2+B3 pressed");
      break;
  }
}

void loop() {
  // Take multiple samples and average them
  long sum = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sum += analogRead(BUTTON_PIN);
    delay(1); // Faster sampling
  }
  int avgValue = sum / NUM_SAMPLES;

  // Detect current button state (check combos first, then singles)
  int currentButton = BTN_NONE;

  if (avgValue >= B2_B3_MIN && avgValue <= B2_B3_MAX) {
    currentButton = BTN_B2_B3;
  } else if (avgValue >= B1_B3_MIN && avgValue <= B1_B3_MAX) {
    currentButton = BTN_B1_B3;
  } else if (avgValue >= B1_B2_MIN && avgValue <= B1_B2_MAX) {
    currentButton = BTN_B1_B2;
  } else if (avgValue >= B3_MIN && avgValue <= B3_MAX) {
    currentButton = BTN_B3;
  } else if (avgValue >= B2_MIN && avgValue <= B2_MAX) {
    currentButton = BTN_B2;
  } else if (avgValue >= B1_MIN && avgValue <= B1_MAX) {
    currentButton = BTN_B1;
  } else if (avgValue >= NONE_MIN && avgValue <= NONE_MAX) {
    currentButton = BTN_NONE;
  }

  // Debounce logic - require consistent readings
  if (currentButton == pendingButton) {
    debounceCounter++;
    if (debounceCounter >= DEBOUNCE_COUNT) {
      // Confirmed stable reading

      // Track the maximum button state during this press sequence
      if (currentButton > maxButtonState) {
        maxButtonState = currentButton;
      }

      // Only report when ALL buttons are released
      if (currentButton == BTN_NONE && maxButtonState != BTN_NONE) {
        if (maxButtonState != lastReportedState) {
          printButtonState(maxButtonState);
          lastReportedState = maxButtonState;
        }
        maxButtonState = BTN_NONE; // Reset for next press
      }
    }
  } else {
    // New reading, reset counter
    pendingButton = currentButton;
    debounceCounter = 0;
  }

  delay(5); // Faster loop for quicker response
}
