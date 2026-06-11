// R2R Button Continuous ADC Reader
// Shows real-time ADC values to help diagnose wiring issues

const int BUTTON_PIN = 3;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect
  }

  pinMode(BUTTON_PIN, INPUT);

  Serial.println("===========================================");
  Serial.println("R2R Continuous ADC Reader");
  Serial.println("===========================================");
  Serial.println();
  Serial.println("Showing ADC values in real-time.");
  Serial.println("Press different button combinations and watch the values.");
  Serial.println();
  Serial.println("Format: Raw ADC | Voltage (assuming 3.3V reference)");
  Serial.println("===========================================");
  Serial.println();
}

void loop() {
  // Read raw ADC value
  int rawValue = analogRead(BUTTON_PIN);

  // Calculate voltage (SAMD21 uses 3.3V reference by default)
  float voltage = (rawValue / 1023.0) * 3.3;

  // Print with formatting
  Serial.print("ADC: ");
  Serial.print(rawValue);
  Serial.print("\t| Voltage: ");
  Serial.print(voltage, 3);
  Serial.println(" V");

  delay(200); // Update 5 times per second
}
