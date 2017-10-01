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

// MQTT reconnect is non-blocking. We shorten
// the default MQTT timeout so that the WDT won't be tripped.
// Should be ok with the MQTT server on the local network.
#define MQTT_SOCKET_TIMEOUT 2

#include <ArduinoOTA.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <SimpleTimer.h>
#include <Time.h>
#include <Timezone.h>
#include <WifiUdp.h>

WiFiClient client;

#include "OTA_setup.h"


#define MQTT_PUMP_STATUS "home/pump/status"
#define MQTT_PUMP_CMD "home/pump/cmd"

#define DEBUG_TERM

// Private wifi settings and Blynk auth token are in this file
#include "wifi_settings.h"

// Forward declartions
void update_time_str();
void send_status_to_mqtt(const char* s);
void send_last_status_to_mqtt();
void mqtt_sub_callback(char* topic, byte* msg, unsigned int length);

#include "mqtt.h"

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
#define TIMEZONE 0
#define MSEC_PER_SEC 1000L
#define HOURS 3600L
#define MINUTES 60L

SimpleTimer timer;

#if 1
#define PUMP_RUN_TIME_S (4 * MINUTES)
#define PUMP_LOCKOUT_TIME_S (15 * MINUTES)
#else
#define PUMP_RUN_TIME_S (5)
#define PUMP_LOCKOUT_TIME_S (5)
#endif
unsigned int pump_countdown_timer = 0;
unsigned int pump_lockout_timer = PUMP_LOCKOUT_TIME_S;

#define PUMP_FSM_INTERVAL_S 1 * MSEC_PER_SEC

// NTP Client
// Set up Time Zone Rules
// US Pacific Time Zone (San Jose)
TimeChangeRule myDST = {"PDT", Second, Sun, Mar, 2, -7 * 60};
TimeChangeRule mySTD = {"PST", First, Sun, Nov, 2, -8 * 60};
Timezone myTZ(myDST, mySTD);
TimeChangeRule *tcr;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "clock.psu.edu", TIMEZONE*HOURS, 12*HOURS*MSEC_PER_SEC);

#define BUF_LEN 64
char time_buf[BUF_LEN];

// Blynk
// Virtual Pins
#define BLYNK_LED       V0
#define RUN_BUTTON      V1
#define LOCK_LED        V2
#define LOCAL_IP        V4
#define LOCAL_TCP_PORT  V5
#define MQTT_IP       V6
#define BLYNK_LOCKOUT   V7
#define LOCK_OVERRIDE   V8
#define TERMINAL        V10
/////////////////////// Pump Control ////////////////////////
void pump_lock(){
  Serial.println("Pump locked");
  Blynk.virtualWrite(LOCK_LED, 1023);
  pump_lockout_timer = PUMP_LOCKOUT_TIME_S + 2;
}

void pump_unlock(){
#ifdef DEBUG_TERM
  update_time_str();
  terminal.printf("%s: Pump Unlock\n", time_buf);
  terminal.flush();
#endif
  pump_countdown_timer = 0;
  pump_lockout_timer = 0;
  Serial.println("Pump unlocked");
  Blynk.virtualWrite(LOCK_LED, 0);
  send_status_to_mqtt("PUMP_UNLOCK");
}

void pump_on(){
static int on_count;
#ifdef DEBUG_TERM
  update_time_str();
  terminal.printf("=======================\n");
  terminal.printf("%s: Pump On = %d\n", time_buf, on_count++);
  terminal.flush();
#endif
  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELAYN_PIN, HIGH);
  Blynk.virtualWrite(BLYNK_LED, 1023);
  Serial.println("Pump starting");
  send_status_to_mqtt("PUMP_ON");
}

void pump_off(){
#ifdef DEBUG_TERM
  update_time_str();
  terminal.printf("%s: Pump Off/Locked\n", time_buf);
  terminal.flush();
#endif
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(RELAYN_PIN, LOW);
  Blynk.virtualWrite(BLYNK_LED, 0);
  Serial.println("Pump off");
  pump_lock();
  send_status_to_mqtt("PUMP_OFF");
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
  pump_countdown_timer = PUMP_RUN_TIME_S + 2;
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
void mqtt_sub_callback(char* topic, byte* msg, unsigned int length) {
    char cmsg[32];
    strncpy(cmsg, reinterpret_cast<char*>(msg), min(32, length));
    Serial.println("topic: ");
    Serial.print(topic);
    Serial.print(", value: ");
    Serial.println(cmsg);

    if (strcmp(MQTT_PUMP_CMD, topic) == 0) {
        if (strncmp("RUN", cmsg, length) == 0) {
            pump_start();
        }
    }
}
#endif

///////////////// Network TCP Server/////////////////////////////////
#define CLIENT_TIMEOUT (10 * MSEC_PER_SEC)
long client_wdt;


///////////////// MQTT Client/////////////////////////////////
// Used to send status to the Indigo Server
char last_status[32];
void send_status_to_mqtt(const char *s) {
  strncpy(last_status, s, 32);
  mqtt_publish_topic(MQTT_PUMP_STATUS, s);
}
void send_last_status_to_mqtt() {
  mqtt_publish_topic(MQTT_PUMP_STATUS, last_status);
}

/////////////////////// Blynk Routines ////////////////////////
// Blynk button to run the pump
BLYNK_WRITE(RUN_BUTTON) 
{
  int run_button_state = param.asInt();
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

// Blynk button to run the pump
BLYNK_WRITE(LOCK_OVERRIDE) 
{
  int override_button_state = param.asInt();
  Serial.println("=======");
  Serial.printf("Button Lock Override = %u\n", override_button_state);
#ifdef DEBUG_TERM
  terminal.printf("Button Lock Override = %u\n", override_button_state);
  terminal.flush();
#endif
  if (override_button_state == 1) {
    pump_off();
    pump_unlock();
  }
}

void iosNotify(char *s) {
  Blynk.notify(s);
  Blynk.email("sprouses@gmail.com", "Subject: Garage Open", s);
}


#if 0
BLYNK_READ(LOCAL_IP) //Function to display the local IP address
{
    char myIpString[24];
    IPAddress myIp = WiFi.localIP();
    sprintf(myIpString, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], myIp[3]);
    Blynk.virtualWrite(LOCAL_IP, myIpString);
}

BLYNK_READ(MQTT_IP) //Function to display the local IP address
{
    char myIpString[24];
    IPAddress myIp = mqtt_server;
    sprintf(myIpString, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], myIp[3]);
    Blynk.virtualWrite(MQTT_IP, myIpString);
}

BLYNK_READ(LOCAL_TCP_PORT) //Function to display the local IP address
{
    Blynk.virtualWrite(LOCAL_TCP_PORT, INDIGO_PORT);
}
#endif

BLYNK_READ(BLYNK_LOCKOUT) //Function to display the local IP address
{
    char myString[24];
    int m = pump_lockout_timer / 60;
    int s = pump_lockout_timer % 60;
    sprintf(myString, "% 2dm %2ds", m, s);
    Blynk.virtualWrite(BLYNK_LOCKOUT, myString);
}

//////////////////////// NTP //////////////////////
long get_time() {
  long ltime = timeClient.getEpochTime();
  return ltime;
}

void update_time_str() {
  time_t utc = now();
  time_t t = myTZ.toLocal(utc, &tcr);
  snprintf(time_buf, BUF_LEN, "%d.%d %02d:%02d:%02d %s",
    month(t), day(t), hour(t), minute(t), second(), tcr);
}

void setup()
{
  Serial.begin(115200);
  Serial.print("\nStarting\n");
  // Initialize the LED and Relay pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAYN_PIN, OUTPUT);

  Blynk.begin(auth, WIFI_SSID, WIFI_KEY);


  // Wait until connected to Blynk
  while (Blynk.connect() == false) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("Blynk connected");
  Serial.print("MQTT TIMEOUT: ");
  Serial.println(MQTT_SOCKET_TIMEOUT);

  char myIpString[24];
  IPAddress myIp = WiFi.localIP();
  sprintf(myIpString, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], myIp[3]);
  Blynk.virtualWrite(LOCAL_IP, myIpString);

  terminal.printf("\nBlynk Ready\n");
  terminal.flush();

  OTA_setup();

  // Initialzie the NTP Client
  timeClient.update();
  setSyncProvider(get_time);
  terminal.printf("NTP Ready\n");
  terminal.flush();

  // Set a 1 sec timer to call the pump fsm.
  Serial.printf("Set 1 sec fsm interval\n");
  timer.setInterval(PUMP_FSM_INTERVAL_S, pump_countdown); 

  //mqtt setup
  mqtt_setup();

  // Set lock after Blynk is online
  pump_off();
  pump_lock();
}

void loop()
{
  Blynk.run();
  timer.run();
  ArduinoOTA.handle();
  //pump_run();
  mqtt_run();
}

