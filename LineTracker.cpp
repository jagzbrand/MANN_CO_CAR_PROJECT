#include "LineTracker.h"

LineTracker::LineTracker(const uint8_t sensorPins[NUM_SENSORS],
                         bool activeLow,
                         bool usePullups)
    : activeLow(activeLow), usePullups(usePullups)
{
    for (uint8_t i = 0; i < NUM_SENSORS; i++) {
        pins[i] = sensorPins[i];
    }

    lastPos = (NUM_SENSORS - 1) / 2.0f;  // default center
}

void LineTracker::begin() {
    for (uint8_t i = 0; i < NUM_SENSORS; i++) {
        pinMode(pins[i], usePullups ? INPUT_PULLUP : INPUT);
    }
}

uint8_t LineTracker::readBits() {
    uint8_t bits = 0;

    for (uint8_t i = 0; i < NUM_SENSORS; i++) {
        int val = digitalRead(pins[i]);
        bool onLine = activeLow ? (val == LOW) : (val == HIGH);
        if (onLine) bits |= (1u << i);
    }

    return bits;
}

float LineTracker::computePosition() {
    uint8_t bits = readBits();

    int weightSum = 0;
    int count = 0;

    for (uint8_t i = 0; i < NUM_SENSORS; i++) {
        if (bits & (1u << i)) {
            weightSum += i;
            count++;
        }
    }

    if (count == 0) {
        // no sensors hit → return last known position
        return lastPos;
    }

    lastPos = float(weightSum) / float(count);
    return lastPos;
}

