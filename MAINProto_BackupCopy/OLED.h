#ifndef OLED_H
#define OLED_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Define the OLED width and height
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Extern declaration: no memory allocated here
extern Adafruit_SSD1306 display;

// Function to print centered text
void printCentered(const String &text, int y, int size);

#endif
