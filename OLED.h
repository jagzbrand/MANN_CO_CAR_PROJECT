#ifndef OLED_H
#define OLED_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Extern declaration for the display object
extern Adafruit_SSD1306 display;

// Function to print centered text
void printCentered(String text, int y, int size);

#endif
