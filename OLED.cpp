#include "OLED.h"
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void initOLED() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
}

void updateOLED(uint8_t mode, const char* stateText, int distance_cm) {
  display.clearDisplay();

  // ---------- MODE ----------
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("MODE: ");

  switch (mode) {
    case 0: display.print("MANUAL"); break;
    case 1: display.print("LINE");   break;
    case 2: display.print("PROX");   break;
    default: display.print("UNK");   break;
  }

  // ---------- STATE ----------
  display.setCursor(0, 14);
  display.print("STATE: ");
  display.print(stateText);

  // ---------- DISTANCE ----------
  display.setCursor(0, 36);
  display.print("DIST: ");

  if (distance_cm < 0) {
    display.print("--");
  } else {
    display.print(distance_cm);
  }
  display.print(" cm");

  display.display();
}

