#include <Arduino.h>
#include "Sensors.h"

const int trigPin = 3;
const int echoPin = 4;

int readDistanceCM() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return -1;

  return duration * 0.034 / 2;
}
