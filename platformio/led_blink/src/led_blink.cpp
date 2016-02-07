#include <ESP8266WiFi.h>
/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the Uno and
  Leonardo, it is attached to digital pin 13. If you're unsure what
  pin the on-board LED is connected to on your Arduino model, check
  the documentation at http://www.arduino.cc

  This example code is in the public domain.

  modified 8 May 2014
  by Scott Fitzgerald
 */

#define LED 16

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital LED pin as an output.
  pinMode(LED, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(5000);              // wait for a second
  digitalWrite(LED, LOW);    // turn the LED off by making the voltage LOW
  delay(5000);              // wait for a second
}
