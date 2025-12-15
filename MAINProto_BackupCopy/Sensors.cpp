#include <Arduino.h>
#include "Sensors.h"

// Define pins
const int trigPin = 3;
const int echoPin = 4;

// Function to read distance in cm
int readDistanceCM() {
  // Trigger pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read echo
  unsigned long duration = pulseIn(echoPin, HIGH, 50000);

  // Calculate distance in cm
  int distance_cm = duration * 0.034 / 2;

  return distance_cm; // returns 0 if no reading
}
