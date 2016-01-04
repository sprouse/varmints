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

// Forward declarations
void repeatMe();
long int getTime();
void digitalClockDisplay();


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

void garage_open(){
	led1.on();
	time_open = now();
}

void garage_closed(){
	led1.off();
	time_closed = now();
	//XXX make string with time closed and send to blynk display
}

#define OPEN 1
#define CLOSED 2
#define OPEN_WDT 4

#define ERR_FSM 1

int state, next_state;
void fsm(int event){
	next_state = state;
	switch (state){
	case CLOSED:
		if (event == OPEN){
			garage_open();
			next_state = OPEN;
		}
		break;
	case OPEN:
		if (event == CLOSED) {
			garage_open();
			next_state = CLOSED;
		} else if (event == OPEN_WDT) {
			long elapsed_time_open = now() - time_open;
			//update_elapsed_time_open(elapsed_time_open);
			//reset_open_wdt();
		}
		break;
	default:
		//error(ERR_FSM);
		break;
	}

	state = next_state;
}

void setup()
{
  Serial.begin(115200);
  Blynk.begin(auth, WIFI_SSID, WIFI_KEY);

  // Initialize the LED pin
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, 0);

  // Wait until connected to Blynk
  while (Blynk.connect() == false) {
    Serial.print(".");
    delay(500);
  }

  timer.setInterval(1000, repeatMe);
  timeClient.update();
  setSyncProvider(getTime);
}

void loop()
{
  Blynk.run();
  timer.run();
}

