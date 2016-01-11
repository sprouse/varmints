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
#include "SimpleTimer.h"
#include <TimeLib.h>

#include "Garage.h"


// Private wifi settings and Blynk auth token are in this file
#include "wifi_settings.h"

WidgetLED led1(1);

// Physical Pins
#define ledPin 5
#define RELAY_PIN 13

SimpleTimer timer;

// Time Configurtion
#define TIMEZONE -8
#define MSECS 1000L
#define HOURS 3600L
#define MINUTES 60L

NTPClient timeClient("clock.psu.edu", TIMEZONE*HOURS, 12 * HOURS * MSECS);


Garage garage(RELAY_PIN);

// Forward declarations
void repeat_me();
long int get_time();
void digitalClockDisplay();

int dummy_sensor;

// Virtual Pins
#define STATUS_LED 0
#define DUMMY_BUTTON V1
#define NOTIFY_LED 2
#define NOTIFY_BUTTON V3
#define BLYNK_LCD_0 V4
#define BLYNK_LCD_1 V5
#define BLYNK_LED V0

/////////////////////// Blynk Routines ////////////////////////

// Blynk Button simulates garage open or closed.
BLYNK_WRITE(DUMMY_BUTTON) //Button Widget is writing to pin V2
{
  dummy_sensor = param.asInt();
  Serial.println("=======");
  Serial.printf("button V1 = %u\n", dummy_sensor);
}

// Blynk LED Control
BLYNK_READ(BLYNK_LED){
  setLED(garage.led_state());
}

void setLED(int state) {
  Blynk.virtualWrite(STATUS_LED, state);
  digitalWrite(ledPin, state);
}

char lcd_buf[2][64];
void setLCD(int vpin, char *buf) {
  Blynk.virtualWrite(vpin, buf);
  strncpy(lcd_buf[vpin], buf, 64);
}

BLYNK_READ(BLYNK_LCD_0) // Serve data to Temperature Gauge
{
  Blynk.virtualWrite(BLYNK_LCD_0, lcd_buf[0]);
}

BLYNK_READ(BLYNK_LCD_1) // Serve data to Temperature Gauge
{
  Blynk.virtualWrite(BLYNK_LCD_1, lcd_buf[1]);
}

void iosNotify(char *s) {
  Blynk.notify(s);
  Blynk.email("sprouses@gmail.com", "Subject: Garage Open", s);
}

/////////////////////// Timer Routines ////////////////////////
void setTime(int vpin) {
  char buf[64];
  snprintf(buf, 64, "%d.%d %02d:%02d:%02d", month(), day(), hour(), minute(), second());
  setLCD(vpin, buf);
}

void gate_open_wdt_expired() {
  Serial.printf("WDT!\n");
  garage.fsm(OPEN_WDT);
}


void repeat_me() {
  //timeClient.update();
  digitalClockDisplay();
}

long int get_time() {
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

  // Initialize the relay input pin
  pinMode(RELAY_PIN, INPUT_PULLUP);

  // Wait until connected to Blynk
  while (Blynk.connect() == false) {
    Serial.print(".");
    delay(500);
  }

  //timer.setInterval(1000, repeat_me);
  timeClient.update();
  setSyncProvider(get_time);
}

void loop()
{
  Blynk.run();
  timer.run();
  garage.run();
}

