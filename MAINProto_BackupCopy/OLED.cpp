#include "OLED.h"

// Define the display object here (only in OLED.cpp)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void printCentered(const String &text, int y, int size) {
    display.setTextSize(size);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((display.width() - w) / 2, y);
    display.print(text);
}
