// Wire Master Writer
// by Nicholas Zambetti <http://www.zambetti.com>

// Demonstrates use of the Wire library
// Writes data to an I2C/TWI slave device
// Refer to the "Wire Slave Receiver" example for use with this

// Created 29 March 2006

// This example code is in the public domain.


#include <OneWire.h>
#include <Wire.h>

union u_tag {
  byte b[4];
  float expAverageTemperatureF;
} u;



#define ledPin 5
byte led_state = 0;

//Temperature chip i/o
#define DS18S20_PIN 12
OneWire ds(DS18S20_PIN);

void updateTemperature();

void setup() {
  Serial.begin(115200);
  Wire.begin(); // join i2c bus (address optional for master)
  // Initialize the LED pin
  pinMode(ledPin, OUTPUT);

  u.expAverageTemperatureF = 65.0;

}

void loop() {
  led_state = ~led_state;
  updateTemperature();
  digitalWrite(ledPin, led_state);
  Wire.beginTransmission(8); // transmit to device #8
  Wire.write(0xaa);
  for (int i = 0; i < 4; i++) {
    Wire.write(u.b[i]);
  }
  Wire.endTransmission();    // stop transmitting

  delay(1000);
}


void updateTemperature() {
  float degreesC, degreesF;
  const float alpha = 0.2;
  degreesC = getTemperature();
  degreesF = degreesC * (9.0 / 5.0) + 32.0;
  u.expAverageTemperatureF = degreesF * alpha + u.expAverageTemperatureF * ( 1.0 - alpha);
  //Blynk.virtualWrite(V1, degreesF);

  //Serial.println(timeClient.getFormattedTime());
  Serial.print(" deg C: ");
  Serial.print(degreesC);
  Serial.print(" deg F: ");
  Serial.print(degreesF);
  Serial.print(" avg F: ");
  Serial.println(u.expAverageTemperatureF);
}

/* ----------------------------------------------------------
  returns the temperature from one DS18S20 in DEG Celsius
  http://bildr.org/2011/07/ds18b20-arduino/
  ----------------------------------------------------------*/
float getTemperature()
{
  byte data[12];
  byte addr[8];

  ds.reset();
  if ( !ds.search(addr)) {
    //no more sensors on chain, reset search
    ds.reset_search();
    return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
    Serial.println("CRC is not valid");
    return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    Serial.println("Unknown device");
    return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1); // start conversion

  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad

  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }

  ds.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float temp = tempRead / 16.0;

  // Round to the nearest tenth, to avoid too many updates.
  // Subtract 2 from the temp since it seems to be a bit high.
  temp = round(temp * 10) / 10.0 - 2;

  return temp;

}


