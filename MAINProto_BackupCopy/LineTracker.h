#ifndef LINETRACKER_H
#define LINETRACKER_H

#include <Arduino.h>

class LineTracker {
public:
  static const uint8_t NUM_SENSORS = 3;

  LineTracker(const uint8_t sensorPins[NUM_SENSORS],
              bool activeLow = true,
              bool usePullups = true,
              bool useAnalog = false,
              int analogThreshold = 700);

  void begin();
  uint8_t readBits();
  float computePosition();

private:
  uint8_t pins[NUM_SENSORS];
  bool activeLow;
  bool usePullups;
  bool useAnalog;
  int analogThreshold; // 0..1023 threshold for analog mode
  float lastPos;
};

#endif
