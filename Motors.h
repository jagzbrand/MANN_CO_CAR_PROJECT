#ifndef MOTORS_H
#define MOTORS_H

const int ENA = 5;
const int ENB = 6;
const int IN1 = 7;
const int IN2 = 8;
const int IN3 = 11;
const int IN4 = 12;

void initMotors();

void moveForward(int pwmLeft = 180, int pwmRight = 180);
void moveBackward(int pwmLeft = 180, int pwmRight = 180);
void turnLeft();
void turnRight();
void stopMotors();

void turnLeftForward();
void turnRightForward();
void turnLeftBackward();
void turnRightBackward();

#endif
