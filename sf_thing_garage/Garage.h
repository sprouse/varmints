/*
      Garage.h - Library for managing the garage door
        Created by Steven Sprouse, January 4, 2016.
          Released into the public domain.
*/
#ifndef Garage_h
#define Garage_h

#include "Arduino.h"
#include <TimeLib.h>
#include <assert.h>
#include "Simpletimer.h"

#include "config.h"

#define GARAGE_OPEN_TIMEOUT_MIN 10

extern void setLED(uint8_t state);
extern void set_event_time(uint8_t op);
extern void iosNotify(char *s);

class Garage
{
  private:
		enum _state_t {
			st_closed,
			st_open
		};

		_state_t _state;
    uint8_t _sensor_state;
    uint8_t _led_vpin;
    uint8_t _relay_pin;
    time_t _time_opened;
    time_t _time_closed;
    uint8_t _wdt_id;
    uint8_t _event;

  public:
    Garage(uint8_t sensor_pin);
    void run();
    uint8_t led_state();
    void fsm(uint8_t event);

  private:
    void garage_open();
    void garage_closed();
    void begin();
};

#endif
