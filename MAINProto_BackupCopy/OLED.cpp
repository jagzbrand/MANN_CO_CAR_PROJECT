#include "OLED.h"
#include <Arduino.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

static bool oledInitialized = false;

// I2C scanner to find OLED address
static uint8_t scanI2C() {
  Serial.println("Scanning I2C bus...");
  uint8_t foundAddr = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
      if (addr == 0x3C || addr == 0x3D) {
        foundAddr = addr;
      }
    }
  }
  if (foundAddr == 0) {
    Serial.println("No OLED found at 0x3C or 0x3D");
  }
  return foundAddr;
}

void initOLED() {
  Serial.println("Initializing OLED...");
  Serial.flush(); // Ensure message is sent
  
  // Give I2C bus time to stabilize
  delay(100);
  
  // Scan I2C bus first
  uint8_t oledAddr = scanI2C();
  uint8_t usedAddr = 0;
  
  // Try initialization with detected address or default addresses
  bool success = false;
  
  if (oledAddr != 0) {
    // Try detected address first
    Serial.print("Attempting OLED init at detected address 0x");
    Serial.println(oledAddr, HEX);
    Serial.flush();
    success = display.begin(SSD1306_SWITCHCAPVCC, oledAddr);
    if (success) usedAddr = oledAddr;
  }
  
  if (!success) {
    // Try 0x3C
    Serial.println("Trying OLED at 0x3C...");
    Serial.flush();
    success = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    if (success) usedAddr = 0x3C;
  }
  
  if (!success) {
    // Try 0x3D
    Serial.println("Trying OLED at 0x3D...");
    Serial.flush();
    success = display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
    if (success) usedAddr = 0x3D;
  }
  
  if (!success) {
    oledInitialized = false;
    Serial.println("ERROR: OLED initialization FAILED - continuing without display");
    Serial.println("Check wiring: SDA->A4, SCL->A5, VCC->5V, GND->GND");
    Serial.flush();
    return;
  }
  
  oledInitialized = true;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Test display - show initialization message
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("OLED Ready!");
  display.setCursor(0, 10);
  display.print("Addr: 0x");
  display.println(usedAddr, HEX);
  display.display();
  
  delay(500); // Show test message briefly
  
  Serial.print("SUCCESS: OLED initialized at 0x");
  Serial.print(usedAddr, HEX);
  Serial.println(" and ready!");
  Serial.flush();
}

void updateOLED(uint8_t mode, const char* stateText, int distance_cm) {
  if (!oledInitialized) return; // Skip if OLED not initialized
  
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

bool isOLEDReady() {
  return oledInitialized;
}
