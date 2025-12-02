#ifndef LINETRACKER_H
#define LINETRACKER_H

#include <Arduino.h>

class LineTracker {
public:
    static const uint8_t NUM_SENSORS = 4;

    LineTracker(const uint8_t sensorPins[NUM_SENSORS],
                bool activeLow = true,
                bool usePullups = true);

    void begin();
    uint8_t readBits();
    float computePosition();      // updates lastPos and returns new position
    float getLastPosition() const { return lastPos; }

private:
    uint8_t pins[NUM_SENSORS];
    bool activeLow;
    bool usePullups;
    float lastPos;
};

#endif
