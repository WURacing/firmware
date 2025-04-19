const int numSensors = 1;  // Only one pitot tube sensor
const int sensorPins[numSensors] = {A0};  // Assuming the pitot tube is connected to A0

void setup() {
  Serial.begin(9600);  // Initialize serial communication
}

void loop() {
  // Check for serial input first to ensure immediate response
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.equalsIgnoreCase("END")) {
      Serial.println("DATA_END");  // Termination signal for Python
      return;
    }
    else {
      // Read sensor immediately when input is received
      float voltages[numSensors];
      for (int i = 0; i < numSensors; i++) {
        int sensorValue = analogRead(sensorPins[i]);
        voltages[i] = sensorValue * (5.0 / 1023.0);  // Convert analog reading to voltage
      }
      
      // Send CSV-formatted response: temp,A0
      Serial.print(input);  // Echo back the received temperature
      for (int i = 0; i < numSensors; i++) {
        Serial.print(',');
        Serial.print(voltages[i]);  // Send the voltage reading
      }
      Serial.println();
    }
  }
  
  // Small delay to stabilize the connection
  delay(50);
}
