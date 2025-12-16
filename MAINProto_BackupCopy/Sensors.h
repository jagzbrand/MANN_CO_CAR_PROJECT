#ifndef SENSORS_H
#define SENSORS_H

// Ultrasonic sensor pins
extern const int trigPin;
extern const int echoPin;

// Function to read distance in cm
int readDistanceCM();

#endif
