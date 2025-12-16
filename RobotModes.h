#ifndef ROBOT_MODES_H
#define ROBOT_MODES_H

#include <Arduino.h>

enum Mode : uint8_t {
  MODE_MANUAL = 0,
  MODE_LINE   = 1,
  MODE_PROX   = 2
};

void setMode(Mode m);
Mode getMode();

void handleCommand(char c, Stream &out);

void runManual();
void runLineFollow(int distance_cm, float linePos);
void runProximity(int distance_cm);

#endif
