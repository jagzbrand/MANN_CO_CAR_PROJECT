#include <Servo.h>
#include <SoftwareSerial.h>

#include "Motors.h"
#include "Sensors.h"
#include "LineTracker.h"
#include "RobotModes.h"
#include "OLED.h"

// ---------- Bluetooth ----------
SoftwareSerial BT(9, 10);   // RX, TX

// ---------- Servo ----------
Servo scanServo;
const int SERVO_PIN = 2;

// ---------- Line sensors ----------
static const uint8_t LTpins[3] = { A0, A2, A3 };
LineTracker lt(LTpins, true, true);

// ---------- Timing ----------
unsigned long lastTick = 0;
const unsigned long LOOP_INTERVAL = 20;

// ---------- OLED STATE TEXT ----------
// 🔧 UPDATED: global text describing what the robot is doing
const char* currentStateText = "IDLE";

void setup() {
  Serial.begin(9600);
  BT.begin(9600);

  // Ultrasonic
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Line sensors
  lt.begin();

  // Servo
  scanServo.attach(SERVO_PIN);
  scanServo.write(90);

  // Motors
  initMotors();

  // OLED
  initOLED();

  Serial.println("Robot ready");
  BT.println("Robot ready");
}

void loop() {
  if (millis() - lastTick < LOOP_INTERVAL) return;
  lastTick = millis();

  // ---------- Read sensors ----------
  int distance_cm = readDistanceCM();
  float linePos   = lt.computePosition();

  // ---------- Read commands (USB) ----------
  while (Serial.available()) {
    handleCommand(Serial.read(), Serial);
  }

  // ---------- Read commands (Bluetooth) ----------
  while (BT.available()) {
    handleCommand(BT.read(), BT);
  }

  // ---------- Run active mode ----------
  switch (getMode()) {
    case MODE_MANUAL:
      runManual();
      break;

    case MODE_LINE:
      runLineFollow(distance_cm, linePos);
      break;

    case MODE_PROX:
      runProximity(distance_cm);
      break;
  }

  // =====================================================
  // 🔧 UPDATED: DECIDE WHAT THE OLED SHOULD SAY
  // =====================================================
  if (getMode() == MODE_MANUAL) {
    currentStateText = "MANUAL CTRL";
  }
  else if (getMode() == MODE_LINE) {
    currentStateText = "TRACKING";
  }
  else if (getMode() == MODE_PROX) {
    if (distance_cm > 0 && distance_cm < 25)
      currentStateText = "AVOIDING";
    else
      currentStateText = "SCANNING";
  }

  // =====================================================
  // 🔧 UPDATED: UPDATE OLED WITH MODE + STATE + DISTANCE
  // =====================================================
  updateOLED(getMode(), currentStateText, distance_cm);
}
