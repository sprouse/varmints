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

// Forward declarations
void bprint(char *buf);
void show_time();
void show_time(time_t t);

WidgetLED run_led(V0);
WidgetLED vac_led(V2);
WidgetTerminal terminal(10);

// Physical Pins
#ifndef NODE_MCU

#define LED_PIN 5
// Connect relay pin to "-" port. Connect the "+" port to Vcc.
#define RELAYN_PIN 4

#else

#define LED_PIN 16
// Connect relay pin to "-" port. Connect the "+" port to Vcc.
#define RELAYN_PIN 15

#endif

SimpleTimer timer;

// Time Configurtion
#define TIMEZONE -8
#define MSECS 1000L
#define HOURS 3600L
#define MINUTES 60L

#define PUMP_FSM_INTERVAL_S 1 * MSECS
#define PUMP_SCHED_INTERVAL_S 1 * MSECS

NTPClient timeClient("clock.psu.edu", TIMEZONE*HOURS, 12 * HOURS * MSECS);

Pump pump;

// Forward declarations
long int get_time();
void digitalClockDisplay();

int run_button_state;
int vacation_button_state;

// Virtual Pins
#define BLYNK_LED   V0
#define RUN_BUTTON  V1
#define VAC_LED     V2
#define VAC_BUTTON  V3
#define TERMINAL    V10

/////////////////////// Blynk Routines ////////////////////////
// Blynk button to run the pump
BLYNK_WRITE(RUN_BUTTON) 
{
  run_button_state = param.asInt();
  Serial.println("=======");
  Serial.printf("Button V1 = %u\n", run_button_state);
#ifdef DEBUG_TERM
  terminal.printf("Button V1 = %u\n", run_button_state);
  terminal.flush();
#endif
  pump.fsm(Pump::ev_none);
}

// TODO: The Vacation button state does not persist across power cycles on the board
// or across app starts on the phone. :(
BLYNK_WRITE(VAC_BUTTON) 
{
  vacation_button_state = param.asInt();
  Serial.println("=======");
  Serial.printf("Button V3 = %u\n", vacation_button_state);
#ifdef DEBUG_TERM
  terminal.printf("Button V3 = %u\n", vacation_button_state);
  terminal.flush();
#endif
  // Remember that LED's have a "brightness" in the range 0 to 1023
  Blynk.virtualWrite(V2, vacation_button_state * 1023);
}

void set_pump_state(uint8_t state) {
  Serial.printf("set led=%d\n", state);

  if (state == 1){
    run_led.on();
  } else {
    run_led.off();
  }
  digitalWrite(RELAYN_PIN, !state);
#ifdef NODE_MCU
  state != state;
#endif
  digitalWrite(LED_PIN, !state);
}

void iosNotify(char *s) {
  Blynk.notify(s);
  Blynk.email("sprouses@gmail.com", "Subject: Garage Open", s);
}

/////////////////////// Timer Routines ////////////////////////
void pump_fsm_timer() {
  pump.fsm(Pump::ev_timer_tick);
}

void pump_sched_timer() {
  char buf[64];

  // Sample these immediately.

  time_t t = now();

#undef DEBUG_TERM
#ifdef DEBUG_TERM
  snprintf(buf, 64, "Sched timer %02d:%02d", minute(t), second(t));
  bprint(buf);
#endif
  switch(minute(t) % 2) {
    case 0:
    case 1:
      if (second(t) % 15 == 0){
        run_button_state = 1;
        show_time(t);
        snprintf(buf, 64, "Sched trigger");
        bprint(buf);
        pump.fsm(Pump::ev_none);
      }
      break;
    default:
      break;
  }
}
    
void show_time() {
  time_t t = now();
  show_time(t);
}

void show_time(time_t t) {
  char buf[64];
  snprintf(buf, 64, "%d.%d %02d:%02d:%02d", month(t), day(t), hour(t), minute(t), second(t));
  bprint(buf);
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

////////////////// Pump support routines
void bprint(char *buf) {
  terminal.println(buf);
  terminal.flush();
}

void set_run_button(uint8_t state) {
  Blynk.virtualWrite(V1, state);
  run_button_state = state;
}

void setup()
{
  Serial.begin(115200);
  Serial.print("\nStarting\n");
  Blynk.begin(auth, WIFI_SSID, WIFI_KEY);

  // Initialize the LED and Relay pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAYN_PIN, OUTPUT);
  set_pump_state(0);

  // Wait until connected to Blynk
  while (Blynk.connect() == false) {
    Serial.print(".");
    delay(1000);
  }

  terminal.printf("\nBlynk Ready\n");
  terminal.flush();

  OTA_setup();

  timeClient.update();
  setSyncProvider(get_time);
  show_time();

  // Set a 1 sec timer to call the pump fsm.
  Serial.printf("Set 5 sec fsm interval\n");
  timer.setInterval(PUMP_FSM_INTERVAL_S, pump_fsm_timer); 

  Serial.printf("Set 1 minute pump interval\n");
  timer.setInterval(PUMP_SCHED_INTERVAL_S, pump_sched_timer); 

  pump.begin();
}

void loop()
{
  Blynk.run();
  timer.run();
  ArduinoOTA.handle();
  pump.run();
}

