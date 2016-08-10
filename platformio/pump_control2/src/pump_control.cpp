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
#include "OTA_setup.h"
#include <SimpleTimer.h>
#include <Time.h>

#define DEBUG_TERM

// Private wifi settings and Blynk auth token are in this file
#include "wifi_settings.h"

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
#define RELAYN_PIN 0

#endif

// Time Configurtion
#define TIMEZONE -8
#define MSECS 1000L
#define HOURS 3600L
#define MINUTES 60L

SimpleTimer timer;

#define PUMP_RUN_TIME_S 3
#define PUMP_LOCKOUT_TIME_S 3
unsigned int pump_countdown_timer = 0;
unsigned int pump_lockout_timer = PUMP_LOCKOUT_TIME_S;

#define PUMP_FSM_INTERVAL_S 1 * MSECS

// Blynk
int run_button_state = 0;
// Virtual Pins
#define BLYNK_LED   V0
#define RUN_BUTTON  V1
#define LOCK_LED     V2
#define VAC_BUTTON  V3
#define TERMINAL    V10

/////////////////////// Pump Control ////////////////////////
void pump_lock(){
  Serial.println("Pump locked");
  Blynk.virtualWrite(LOCK_LED, 1023);
  pump_lockout_timer = PUMP_LOCKOUT_TIME_S;
}

void pump_unlock(){
  Serial.println("Pump unlocked");
  Blynk.virtualWrite(LOCK_LED, 0);
}

void pump_on(){
static int on_count;
#ifdef DEBUG_TERM
  terminal.printf("Pump On = %d\n", on_count++);
  terminal.flush();
#endif
  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELAYN_PIN, LOW);
  Blynk.virtualWrite(BLYNK_LED, 1023);
  Serial.println("Pump starting");
}

void pump_off(){
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(RELAYN_PIN, HIGH);
  Blynk.virtualWrite(BLYNK_LED, 0);
  Serial.println("Pump off");
  pump_lock();
}
/////////////////////// Timer Routines ////////////////////////
void pump_start() {
  if (pump_lockout_timer != 0) {
    Serial.println("Pump locked out");
    return;
  }
  if (pump_countdown_timer != 0) {
    Serial.println("Pump already running");
    return;
  }

  pump_on();
  pump_countdown_timer = PUMP_RUN_TIME_S;
}

void pump_countdown(){
  if (pump_lockout_timer > 0) {
    pump_lockout_timer --;
    Serial.print("Lockout");
    Serial.println(pump_lockout_timer);

    if (pump_lockout_timer == 0) {
      pump_unlock();
    }
  }

  if (pump_countdown_timer == 0){
    return;
  }

  pump_countdown_timer--;
  Serial.print("Countdown ");
  Serial.println(pump_countdown_timer);
  if (pump_countdown_timer == 0){
    pump_off();
  }
}

/////////////////  Network UDP ///////////////////
#if 1
String cmd_parse(String packetBuffer){
    Serial.println("Contents:");
    Serial.print("\"");
    Serial.print(packetBuffer);
    Serial.println("\"");

    if (strncmp("PUMP_ON", packetBuffer.c_str(), 7) == 0) {
      pump_start();
      return String("Pump On");
    }

    return String("Unknown Cmd");
}
#endif

///////////////// Network TCP /////////////////////////////////
WiFiServer server(2390);
WiFiClient client;
#define CLIENT_TIMEOUT (10 * MSECS)
long client_wdt;

void tcp_run() {
  if (!client.connected()) {
    client = server.available();
    client_wdt = CLIENT_TIMEOUT;
  }

  if (!client) {
    return;
  }

  if (!client.available()) {
    if (client_wdt == 0) {
      client.stop();
    } else {
      client_wdt--;
      delay(1); // Delay 1ms
    }
    return;
  } 

  Serial.println("new client");

  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  String s = cmd_parse(req);

  client.print(s);
  client.stop(); // Terminate the client
}

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
  if (run_button_state == 1) {
    pump_start();
  }
}

void iosNotify(char *s) {
  Blynk.notify(s);
  Blynk.email("sprouses@gmail.com", "Subject: Garage Open", s);
}

void setup()
{
  Serial.begin(115200);
  Serial.print("\nStarting\n");
  // Initialize the LED and Relay pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAYN_PIN, OUTPUT);
  pump_off();

  Blynk.begin(auth, WIFI_SSID, WIFI_KEY);


  // Wait until connected to Blynk
  while (Blynk.connect() == false) {
    Serial.print(".");
    delay(1000);
  }

  // Set lock after Blynk is online
  pump_lock();

  terminal.printf("\nBlynk Ready\n");
  terminal.flush();

  OTA_setup();

  // Set a 1 sec timer to call the pump fsm.
  Serial.printf("Set 1 sec fsm interval\n");
  timer.setInterval(PUMP_FSM_INTERVAL_S, pump_countdown); 

  // Udp.begin(localPort);
  server.begin();
  Serial.println("Server started");
}

void loop()
{
  tcp_run();
  Blynk.run();
  timer.run();
  ArduinoOTA.handle();
  //pump_run();
}

