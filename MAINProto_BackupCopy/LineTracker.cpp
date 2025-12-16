#include "LineTracker.h"

LineTracker::LineTracker(const uint8_t sensorPins[NUM_SENSORS],
                         bool activeLow,
                         bool usePullups,
                         bool useAnalog,
                         int analogThreshold)
  : activeLow(activeLow),
    usePullups(usePullups),
    useAnalog(useAnalog),
    analogThreshold(analogThreshold) {
  for (uint8_t i = 0; i < NUM_SENSORS; i++) {
    pins[i] = sensorPins[i];
  }
  lastPos = (NUM_SENSORS - 1) / 2.0f;
}

void LineTracker::begin() {
  for (uint8_t i = 0; i < NUM_SENSORS; i++) {
    pinMode(pins[i], usePullups ? INPUT_PULLUP : INPUT);
  }
}

uint8_t LineTracker::readBits() {
  uint8_t bits = 0;
  for (uint8_t i = 0; i < NUM_SENSORS; i++) {
    int val = useAnalog ? analogRead(pins[i]) : digitalRead(pins[i]);
    bool onLine;
    if (useAnalog) {
      // Typical IR reflectance: black line -> lower value. Use threshold.
      onLine = (val < analogThreshold);
    } else {
      onLine = activeLow ? (val == LOW) : (val == HIGH);
    }
    if (onLine) bits |= (1 << i);
  }
  return bits;
}

float LineTracker::computePosition() {
  uint8_t bits = readBits();
  int sum = 0, count = 0;

  for (uint8_t i = 0; i < NUM_SENSORS; i++) {
    if (bits & (1 << i)) {
      sum += i;
      count++;
    }
  }

  if (count == 0) return lastPos;

  lastPos = float(sum) / float(count);
  return lastPos;
}
