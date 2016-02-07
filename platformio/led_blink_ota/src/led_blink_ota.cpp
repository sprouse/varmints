#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SimpleTimer.h>

#include "wifi_settings.h"

SimpleTimer timer;
SimpleTimer timer2;
SimpleTimer timer3;

#define LED_PIN 16
bool led_state = 0;
void timer_handler() {
  if (led_state != 1) 
     led_state = 1;
  else 
    led_state = 0;

  Serial.printf("Led = %d\n", led_state);
  digitalWrite(LED_PIN, led_state);
}

void led_setup(){
  pinMode(LED_PIN, OUTPUT);
  timer.setInterval(3000, timer_handler);
  timer2.setInterval(5000, timer_handler);
  timer3.setInterval(7000, timer_handler);
}

void WiFi_setup() {
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
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
  timer2.run();
  timer3.run();
}
