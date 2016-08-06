#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SimpleTimer.h>

#include "wifi_settings.h"

SimpleTimer timer;

#define PERIOD_MS  2000

#ifndef NODE_MCU
#define LED_PIN 5
#else
#define LED_PIN 16
// Relay
//#define LED_PIN 12
// LED

//#define LED_PIN 13

//#define RELAY_ENABLED
#ifdef RELAY_ENABLED
#define RELAY_PIN 12
#endif
#endif

bool led_state = 0;
void timer_handler() {
  led_state = !led_state;

  Serial.printf("Led = %d %d\n", led_state, LED_PIN);
  digitalWrite(LED_PIN, led_state);
#ifdef RELAY_ENABLED
  digitalWrite(RELAY_PIN, !led_state);
#endif
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void led_setup(){
  pinMode(LED_PIN, OUTPUT);
#ifdef RELAY_ENABLED
  pinMode(RELAY_PIN, OUTPUT);
#endif
  timer.setInterval(PERIOD_MS, timer_handler);
}

void WiFi_setup() {
  Serial.println("Booting");

  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  for (uint8_t i=0; i<6; i++){
    Serial.print(mac[i],HEX);
    Serial.print(":");
  }
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println(F("Connection Failed! Rebooting..."));
    delay(5000);
    ESP.restart();
  }
}

void OTA_setup() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("LED pin is%d\n", LED_PIN);
#ifdef RELAY_ENABLED
  Serial.printf("Relay pin is%d\n", RELAY_PIN);
#endif
}

void setup() {
  Serial.begin(115200);

  WiFi_setup();
  OTA_setup();
  led_setup();
}

void loop() {
  ArduinoOTA.handle();
  timer.run();
}
