#ifndef OLED_H
#define OLED_H

#include <Adafruit_SSD1306.h>

// Global display object
extern Adafruit_SSD1306 display;

// Init
void initOLED();

// Update display with system status
void updateOLED(
  uint8_t mode,
  const char* stateText,
  int distance_cm
);

#endif

