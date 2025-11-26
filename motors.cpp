#include <Arduino.h>
#include "Motors.h"

void moveForward(int pwmLeft, int pwmRight) {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENA, pwmLeft); analogWrite(ENB, pwmRight);
}

void moveBackward(int pwmLeft, int pwmRight) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, pwmLeft); analogWrite(ENB, pwmRight);
}

void turnLeft() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  analogWrite(ENA, 180); analogWrite(ENB, 180);
}

void turnRight() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, 180); analogWrite(ENB, 180);
}

void stopMotors() {
  digitalWrite(ENA, LOW); digitalWrite(ENB, LOW);
}

// Curved forward — stronger curve
void turnLeftForward()  { moveForward(90, 255); }   // left wheel slow, right wheel fast
void turnRightForward() { moveForward(255, 90); }   // right wheel slow, left wheel fast

// Curved backward — stronger curve
void turnLeftBackward()  { moveBackward(90, 255); }
void turnRightBackward() { moveBackward(255, 90); }
