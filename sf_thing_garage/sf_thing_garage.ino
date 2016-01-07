/**************************************************************
  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

  Downloads, docs, tutorials: http://www.blynk.cc
  Blynk community: http://community.blynk.cc
  Social networks: http://www.fb.com/blynkapp
  http://twitter.com/blynk_app
  http://community.blynk.cc/t/continuing-loop-while-reconnecting/620/4
  https://learn.sparkfun.com/tutorials/esp8266-thing-hookup-guide/hardware-overview

  Change WiFi ssid, pass, and Blynk auth token to run :)

**************************************************************/

#define BLYNK_PRINT Serial // Comment this out to disable prints and save space
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <NTPClient.h>
#include <SimpleTimer.h>
#include <TimeLib.h>

#include "Garage.h"


// Private wifi settings and Blynk auth token are in this file
#include "wifi_settings.h"

WidgetLED led1(1);

#define ledPin 5

SimpleTimer timer;
float expAverageTemperatureF = 65.0;

#define TIMEZONE -7
#define MSECS 1000L
#define HOURS 3600
#define MINUTES 60
//NTPClient timeClient(TIMEZONE * 3600);
NTPClient timeClient("clock.psu.edu", TIMEZONE*HOURS, 1 * MINUTES*MSECS);

Garage garage(1, 4);

// Forward declarations
void repeatMe();
long int getTime();
void digitalClockDisplay();

int dummy_sensor;

#define STATUS_LED 0
#define DUMMY_BUTTON V1
#define NOTIFY_LED 2
#define NOTIFY_BUTTON V3


// Blynk Button simulates garage open or closed.
BLYNK_WRITE(DUMMY_BUTTON) //Button Widget is writing to pin V2
{
  dummy_sensor = param.asInt();
  Serial.println("=======");
  Serial.printf("button V1 = %u\n", dummy_sensor);
}

// Blynk LED Control
void setLED(int state) {
  Serial.printf("Led = %d\n", state);
  Blynk.virtualWrite(STATUS_LED, state);
}

void setLCD(int vpin, char *buf) {
  static char lcd_buf[2][64];
  strcpy(lcd_buf[vpin], buf);
  Blynk.virtualWrite(vpin, lcd_buf[vpin]);
}

void setTime(int vpin) {
  char buf[64];
  sprintf(buf, "%d.%d %02d:%02d:%02d", month(), day(), hour(), minute(), second());
  Serial.printf("LCD pin %s\n", buf);
  setLCD(vpin, buf);
}

void gateOpenWDT() {
  Serial.printf("WDT!\n");
  garage.fsm(OPEN_WDT);
}

void iosNotify(char *s) {
  Blynk.notify(s);
}

void repeatMe() {
  //timeClient.update();
  digitalClockDisplay();
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
}

time_t time_open;
time_t time_closed;

void setup()
{
  Serial.begin(115200);
  Serial.print("\nStarting\n");
  Blynk.begin(auth, WIFI_SSID, WIFI_KEY);

  // Initialize the LED pin
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, 0);

  // Wait until connected to Blynk
  while (Blynk.connect() == false) {
    Serial.print(".");
    delay(500);
  }

  //timer.setInterval(1000, repeatMe);
  timeClient.update();
  setSyncProvider(getTime);
}

void loop()
{
  Blynk.run();
  timer.run();
  garage.run();
}

