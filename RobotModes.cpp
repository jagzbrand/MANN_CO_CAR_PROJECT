#include "RobotModes.h"
#include "Motors.h"
#include "LineTracker.h"

// ---------- State ----------
static Mode currentMode = MODE_MANUAL;

// Manual control flags
static bool fwd=false, back=false, left=false, right=false;

// Line follow tuning
static int   LF_basePWM = 140;
static float LF_kP      = 35.0f;
static int   LF_minPWM  = 90;
static int   LF_maxPWM  = 255;

// Proximity state machine
static uint8_t proxState = 0;
static unsigned long stateUntil = 0;

// ---------- Mode control ----------
Mode getMode() {
  return currentMode;
}

void setMode(Mode m) {
  currentMode = m;
  fwd = back = left = right = false;
  proxState = 0;
  stopMotors();
}

// ---------- Command handling ----------
void handleCommand(char c, Stream &out) {
  switch (c) {
    case 'm': case 'M':
      setMode(MODE_MANUAL);
      out.println("MODE: MANUAL");
      break;

    case 'v': case 'V':
      setMode(MODE_LINE);
      out.println("MODE: LINE");
      break;

    case 'p': case 'P':
      setMode(MODE_PROX);
      out.println("MODE: PROX");
      break;

    case 'f': case 'F': fwd = !fwd; back=false; break;
    case 'b': case 'B': back = !back; fwd=false; break;
    case 'l': case 'L': left = !left; right=false; break;
    case 'r': case 'R': right = !right; left=false; break;
    case 'x': case 'X':
      fwd = back = left = right = false;
      stopMotors();
      break;
  }
}

// ---------- Mode logic ----------
void runManual() {
  if (fwd) {
    if (left)      turnLeftForward();
    else if (right) turnRightForward();
    else           moveForward();
  }
  else if (back) {
    if (left)      turnLeftBackward();
    else if (right) turnRightBackward();
    else           moveBackward();
  }
  else {
    if (left)      turnLeft();
    else if (right) turnRight();
    else           stopMotors();
  }
}

void runLineFollow(int distance_cm, float linePos) {
  if (distance_cm > 0 && distance_cm < 25) {
    stopMotors();
    return;
  }

  float center = (LineTracker::NUM_SENSORS - 1) / 2.0f;
  float error  = linePos - center;
  int turn     = (int)(LF_kP * error);

  int leftPWM  = constrain(LF_basePWM - turn, LF_minPWM, LF_maxPWM);
  int rightPWM = constrain(LF_basePWM + turn, LF_minPWM, LF_maxPWM);

  moveForward(leftPWM, rightPWM);
}

void runProximity(int distance_cm) {
  unsigned long now = millis();

  if (proxState == 0) {
    if (distance_cm > 0 && distance_cm < 20) {
      proxState = 1;
      stateUntil = now + 400;
      moveBackward();
    } else {
      moveForward();
    }
  }
  else if (proxState == 1 && now >= stateUntil) {
    proxState = 2;
    stateUntil = now + 300;
    turnLeft();
  }
  else if (proxState == 2 && now >= stateUntil) {
    proxState = 0;
  }
}
