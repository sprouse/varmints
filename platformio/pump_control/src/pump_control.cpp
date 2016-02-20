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
#include <ArduinoOTA.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include "OTA_setup.h"
#include <SimpleTimer.h>
#include <Time.h>
#include <WifiUdp.h>

#include "Pump.h"

// Private wifi settings and Blynk auth token are in this file
#include "wifi_settings.h"

WidgetLED led0(0);

// Physical Pins
#ifndef NODE_MCU

#define ledPin 5
#define RELAY_PIN 13

#else

#define ledPin 16
#define RELAY_PIN 14

#endif

SimpleTimer timer;

// Time Configurtion
#define TIMEZONE -8
#define MSECS 1000L
#define HOURS 3600L
#define MINUTES 60L

NTPClient timeClient("clock.psu.edu", TIMEZONE*HOURS, 12 * HOURS * MSECS);


Pump pump(RELAY_PIN);

// Forward declarations
long int get_time();
void digitalClockDisplay();

int dummy_sensor;

// Virtual Pins
#define STATUS_LED 0

#define BLYNK_LED V0
#define DUMMY_BUTTON V1
#define BLYNK_LCD_0 V4
#define BLYNK_LCD_1 V5

#define BLYNK_LCD_ROW0 4
#define BLYNK_LCD_ROW1 5

/////////////////////// Blynk Routines ////////////////////////
// Blynk Button simulates garage open or closed.
BLYNK_WRITE(DUMMY_BUTTON) //Button Widget is writing to pin V2
{
  dummy_sensor = param.asInt();
  Serial.println("=======");
  Serial.printf("Button V1 = %u\n", dummy_sensor);
}

void setLED(uint8_t state) {
  Serial.printf("set led=%d\n", state);
  if (state == 1){
    led0.on();
  } else {
    led0.off();
  }
  digitalWrite(ledPin, state);
}

void iosNotify(char *s) {
  Blynk.notify(s);
  Blynk.email("sprouses@gmail.com", "Subject: Garage Open", s);
}

/////////////////////// Timer Routines ////////////////////////
void pump_timer_event() {
  pump.fsm(Pump::ev_timer_tick);
}


void set_event_time(uint8_t op) {
  char buf[64];
  snprintf(buf, 64, "%d.%d %02d:%02d:%02d", month(), day(), hour(), minute(), second());
}

void gate_open_wdt_expired() {
  Serial.printf("WDT!\n");
  pump.fsm(EV_OPEN_WDT);
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

  OTA_setup();

  timeClient.update();
  setSyncProvider(get_time);

	// Initialize the LCD
	set_event_time(0);

  // Set a 1 sec timer to call the pump fsm.
  Serial.printf("Set 1 sec pump timer\n");
  timer.setInterval(1000L, pump_timer_event); //Give three notifications
}

void loop()
{
  Blynk.run();
  timer.run();
  pump.run();
  ArduinoOTA.handle();
}

