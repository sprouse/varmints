/**************************************************************
  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

  Downloads, docs, tutorials: http://www.blynk.cc
  Blynk community: http://community.blynk.cc
  Social networks: http://www.fb.com/blynkapp
  http://twitter.com/blynk_app

  Change WiFi ssid, pass, and Blynk auth token to run :)

**************************************************************/

#define BLYNK_PRINT Serial // Comment this out to disable prints and save space
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <NTPClient.h>
#include <OneWire.h>
#include <Servo.h>
#include <SimpleTimer.h>
#include <TimeLib.h>


//Temperature chip i/o
#define DS18S20_PIN 12
OneWire ds(DS18S20_PIN);

// Private wifi settings and Blynk auth token are in this file
#include "wifi_settings.h"

const int temperaturePin = A0;
#define ledPin 5
#define speakerPin 4

int sliderData = 0;
int oldSlider = 0;

SimpleTimer timer;
float expAverageTemperatureF = 65.0;

#define TIMEZONE -7
#define MSECS 1000L
#define HOURS 3600
#define MINUTES 60
//NTPClient timeClient(TIMEZONE * 3600);
NTPClient timeClient("clock.psu.edu", TIMEZONE*HOURS, 1 * MINUTES*MSECS);

// Servo
#define SERVO_PIN 0
Servo myservo;  // create servo object to control a servo


// Forward declarations
void repeatMe();
void playTone(int tone, int duration);
long int getTime();
void digitalClockDisplay();
float retrieveTemperature();

void setup()
{
  Serial.begin(115200);
  Blynk.begin(auth, WIFI_SSID, WIFI_KEY);

  // Initialize the LED pin
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, 0);

  // Initialize the speaker pin
  pinMode(speakerPin, OUTPUT);

  // Wait until connected to Blynk
  while (Blynk.connect() == false) {
    Serial.print(".");
    delay(500);
  }

  timer.setInterval(1000, repeatMe);
  timeClient.update();
  setSyncProvider(getTime);
  myservo.attach(SERVO_PIN);  // attaches the servo on pin SERVO_PIN to the servo object
}

void loop()
{
  Blynk.run();
  timer.run();
}

BLYNK_WRITE(V0) //Slider Widget is writing to pin V0
{
  sliderData = param.asInt();
}

BLYNK_WRITE(V3) //Button Widget is writing to pin V3
{
  if (param.asInt() == 1) {
    playTone(1915, 200);
  }
}

BLYNK_WRITE(V5) //Joystick Widget is writing to pin V3
{
  int mytone = param.asInt();
  playTone(mytone + 100, 20);
}

BLYNK_WRITE(V2) //Button Widget is writing to pin V2
{
  int button = param.asInt();
  Serial.printf("button V2 = %u\n", button);
  if (button == 1) {
    Blynk.tweet("Hello, World!");
  }
}

BLYNK_READ(V1) // Serve data to Temperature Gauge
{
  Blynk.virtualWrite(V1, expAverageTemperatureF);
}

// Blynk Servo Control
BLYNK_WRITE(V4) //Slide Widget is writing to pin V4
{
  int val = param.asInt();
  Serial.printf("slider V4 = %u\n", val);
  myservo.write(val);                  // sets the servo position according to the scaled value
}

#define T_OFFSET_C 5.3
void updateTemperature() {
  float voltage, degreesC, degreesF;
  const float alpha = 0.2;
#define ONEWIRE
#ifdef ONEWIRE
  degreesC = retrieveTemperature();
#else
  voltage = analogRead(temperaturePin) / 1023.0;
  degreesC = (voltage - 0.5) * 100.0 + 5.0 / 9.0 * T_OFFSET_C;
#endif
  degreesF = degreesC * (9.0 / 5.0) + 32.0;
  expAverageTemperatureF = degreesF * alpha + expAverageTemperatureF * ( 1.0 - alpha);
  //Blynk.virtualWrite(V1, degreesF);

  //Serial.println(timeClient.getFormattedTime());
  Serial.print("voltage: ");
  Serial.print(voltage);
  Serial.print(" deg C: ");
  Serial.print(degreesC);
  Serial.print(" deg F: ");
  Serial.print(degreesF);
  Serial.print(" avg F: ");
  Serial.println(expAverageTemperatureF);
}

void repeatMe() {
  updateTemperature();
  //timeClient.update();
  digitalClockDisplay();

  if (oldSlider != sliderData) {
    oldSlider = sliderData;
    analogWrite(ledPin, sliderData);
    Serial.printf("got new value %u\n", sliderData);
  }
}

void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(speakerPin, LOW);
    delayMicroseconds(tone);
  }
}

long int getTime() {
  return timeClient.getRawTime();
}


void printDigits(int digits) {
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");

  Serial.print(month());
  Serial.print("/");
  Serial.print(day());
  Serial.print("/");
  Serial.print(year());
  Serial.println();

  if (second() == 58) {
    playTone(2000, 500);
  }
}




/* ----------------------------------------------------------
  returns the temperature from one DS18S20 in DEG Celsius
  http://bildr.org/2011/07/ds18b20-arduino/
  ----------------------------------------------------------*/
float retrieveTemperature()
{
  byte data[12];
  byte addr[8];

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




