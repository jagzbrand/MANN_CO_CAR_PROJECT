#include <Arduino.h>
#include "Motors.h"

// Set to true if that motor's wiring causes it to run reversed for the current direction logic
// If true, the direction pins for that motor will be inverted.
bool leftInverted  = true;   // flip if left motor spins backward on "forward"
bool rightInverted = true;   // flip if right motor spins backward on "forward"

static void setLeftDirForward() {
  if(!leftInverted) { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); }
  else              { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); }
}
static void setLeftDirBackward() {
  if(!leftInverted) { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); }
  else              { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); }
}

static void setRightDirForward() {
  if(!rightInverted) { digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
  else               { digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); }
}
static void setRightDirBackward() {
  if(!rightInverted) { digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); }
  else               { digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
}

void moveForward(int pwmLeft, int pwmRight) {
  setLeftDirForward();
  setRightDirForward();
  analogWrite(ENA, pwmLeft);
  analogWrite(ENB, pwmRight);
}

void moveBackward(int pwmLeft, int pwmRight) {
  setLeftDirBackward();
  setRightDirBackward();
  analogWrite(ENA, pwmLeft);
  analogWrite(ENB, pwmRight);
}

void turnLeft() {
  // in-place turn (left wheel backward, right wheel forward)
  setLeftDirBackward();
  setRightDirForward();
  analogWrite(ENA, 180);
  analogWrite(ENB, 180);
}

void turnRight() {
  // in-place turn (left wheel forward, right wheel backward)
  setLeftDirForward();
  setRightDirBackward();
  analogWrite(ENA, 180);
  analogWrite(ENB, 180);
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// Curved forward — stronger curve
void turnLeftForward()  { moveForward(90, 255); }   // left wheel slow, right wheel fast
void turnRightForward() { moveForward(255, 90); }   // right wheel slow, left wheel fast

// Curved backward — stronger curve
void turnLeftBackward()  { moveBackward(90, 255); }
void turnRightBackward() { moveBackward(255, 90); }
