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
#include <Time.h>

#include "Garage.h"

// Private wifi settings and Blynk auth token are in this file
#include "wifi_settings.h"

WidgetLED led0(0);

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

char lcd_buf[2][64];

void refreshLCD(uint8_t op) {
	uint8_t vpin = BLYNK_LCD_ROW0;
  char *buf = lcd_buf[0];
	if (op == EV_CLOSED) {
		vpin = BLYNK_LCD_ROW1;
    buf = lcd_buf[1];
	} 
  //Serial.printf("refresh lcd: %d\n", vpin);
  Blynk.virtualWrite(vpin, buf);
  setLED(garage.led_state());
}

void setLCD(uint8_t op, char const *buf) {
  Serial.printf("setLCD %d: %s\n", op, buf);
  char *bufp = lcd_buf[0];
  if (op == EV_CLOSED) {
    bufp = lcd_buf[1];
  }

  strncpy(bufp, buf, 64);
	refreshLCD(op);
}

BLYNK_READ(BLYNK_LCD_0) // Serve data to Temperature Gauge
{
	Serial.printf("lcd0 read\n");
  Blynk.virtualWrite(BLYNK_LCD_0, lcd_buf[0]);
  setLED(garage.led_state());
}

BLYNK_READ(BLYNK_LCD_1) // Serve data to Temperature Gauge
{
	Serial.printf("lcd1 read\n");
  setLED(garage.led_state());
  Blynk.virtualWrite(BLYNK_LCD_1, lcd_buf[1]);
}

void iosNotify(char *s) {
  Blynk.notify(s);
  Blynk.email("sprouses@gmail.com", "Subject: Garage Open", s);
}

/////////////////////// Timer Routines ////////////////////////
void set_event_time(uint8_t op) {
  char buf[64];
  snprintf(buf, 64, "%d.%d %02d:%02d:%02d", month(), day(), hour(), minute(), second());
  setLCD(op, buf);
}

void gate_open_wdt_expired() {
  Serial.printf("WDT!\n");
  garage.fsm(EV_OPEN_WDT);
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

  timeClient.update();
  setSyncProvider(get_time);

	// Initialize the LCD
	set_event_time(0);
	setLCD(EV_CLOSED, "NA");
}

void loop()
{
  Blynk.run();
  timer.run();
  garage.run();
}

