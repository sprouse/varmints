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
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266mDNS.h>
#include <WidgetRTC.h>
#include "OTA_setup.h"
#include "PubSubClient.h"
#include <SimpleTimer.h>
#include <Time.h>

// Private wifi settings and Blynk auth token are in this file
#include "wifi_settings.h"

#include "config.h"
#include "indigo.h"
#include "mqtt.h"

// Globals
SimpleTimer timer;

// Forward Decls

void gate_open_wdt_expired();
void term_msg(String s);
void setLED(uint8_t state);
void set_event_time(uint8_t op);
void iosNotify(char *s);

#include "Garage2.h"

#undef DEBUG

WidgetLED led0(0);

// Physical Pins
#ifndef NODE_MCU

#define ledPin 5
#define RELAY_PIN 13

#else

#define ledPin 16
#define RELAY_PIN 4

#endif


WidgetRTC rtc;
WidgetTerminal terminal(V10);

Garage garage(RELAY_PIN);

// Forward declarations
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

String now_str(){
    String s;
    s = String(month()) + "/";
    s += String(day()) + " ";
    s += String(hour()) + ":";
    int m = minute();
    if (m < 10) 
        s += "0";
    s += String(m);
    s += "- ";
    return s;
}

void term_msg(String s){
    terminal.print(now_str());
    terminal.println(s);
    terminal.flush();
}

/////////////////////// Blynk Routines ////////////////////////
// Blynk Button simulates garage open or closed.
BLYNK_WRITE(DUMMY_BUTTON) //Button Widget is writing to pin V2
{
  dummy_sensor = param.asInt();
  Serial.println("=======");
  Serial.printf("Button V1 = %u\n", dummy_sensor);
}

uint8_t last_led_state = UNKNOWN;
void setLED(uint8_t state) {
    if (!Blynk.connected())
        return;
    if (state == last_led_state)
        return;
    Serial.printf("set led=%d\n", state);
    terminal.print(now_str());
    terminal.printf("set led=%d\n", state);
    terminal.flush();
    if (state == ON){
        led0.on();
    } else {
        led0.off();
    }
    digitalWrite(ledPin, state);
    last_led_state = state;
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
}

BLYNK_READ(BLYNK_LCD_1) // Serve data to Temperature Gauge
{
	Serial.printf("lcd1 read\n");
  Blynk.virtualWrite(BLYNK_LCD_1, lcd_buf[1]);
}

void iosNotify(char *s) {
  Blynk.notify(s);
  Blynk.email("sprouses@gmail.com", "Subject: Garage Open", s);
}

/////////////////////// Timer Routines ////////////////////////
void set_event_time(uint8_t op) {
  char buf[64];
  snprintf(buf, 64, "%d.%d %02d:%02d:%02d", month(),
                day(), hour(), minute(), second());
  setLCD(op, buf);
}

void gate_open_wdt_expired() {
  Serial.printf("WDT Expired!\n");
  Serial.flush();
  terminal.print(now_str());
  terminal.println("WDT");
  terminal.flush();
  garage.fsm(EV_OPEN_WDT);
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


BLYNK_CONNECTED() {
    // Synchronize time on connection
    rtc.begin();
    Blynk.run();
    delay(100);
    Blynk.run();
    terminal.print(now_str());
    terminal.println("Booting...");
    terminal.flush();
    Blynk.syncAll();
}

int timer_rtc_id;
void check_rtc(){
    terminal.println(" RTC check.");
    terminal.flush();
    if (year() == 1970)
        return;

    terminal.print(now_str());
    terminal.println(" RTC sync.");
    terminal.flush();

    timer.disable(timer_rtc_id);
}

int timer_mon_id;
void monitor_timer(){
    Serial.printf("Num timers=%d\n",timer.getNumTimers());
    Serial.flush();
}

////////////// Setup /////////////////////
void setup()
{
    Serial.begin(9600);
    Serial.print("\nStarting\n");
    Blynk.begin(auth, WIFI_SSID, WIFI_KEY);

    // Initialize the LED pin
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, 0);
    
    // Initialize the relay input pin
    pinMode(RELAY_PIN, INPUT_PULLUP);
    
    OTA_setup();

    //timer_rtc_id = timer.setInterval(1000, check_rtc);
    //timer_mon_id = timer.setInterval(5000, monitor_timer);
    mqtt_setup();
}

void loop()
{
  Blynk.run();
  timer.run();
  garage.run();
  ArduinoOTA.handle();
  mqtt_run();
}

