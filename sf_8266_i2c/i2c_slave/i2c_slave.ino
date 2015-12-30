// Wire Slave Receiver
// by Nicholas Zambetti <http://www.zambetti.com>

// Demonstrates use of the Wire library
// Receives data as an I2C/TWI slave device
// Refer to the "Wire Master Writer" example for use with this

// Created 29 March 2006

// This example code is in the public domain.

#include <Wire.h>

union u_tag {
   byte b[4];
   float expAverageTemperatureF;
} u;


void setup() {
  Wire.begin(8);                // join i2c bus with address #8
  Wire.onReceive(receiveEvent); // register event
  Serial.begin(115200);           // start serial for output
}

void loop() {
  delay(100);
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {

  // Frame the data. Always start with a 0xaa;
  while (Wire.available() && (Wire.read() != 0xaa));  // Burp non 0xaa characers;

  // Make sure there are at least 4 characters in the buffer
  if (Wire.available() < 4) {
    Serial.println("Waiting for 4");
    return;
  }  
  for (int i = 0; i < 4; i++) { // loop through all but the last
    u.b[i] = Wire.read(); // receive byte as a character
    Serial.print(u.b[i]);         // print the character
    Serial.print(", ");
  }   // receive byte as an integer
  Serial.println();
  Serial.println(u.expAverageTemperatureF);         // print the integer
}

