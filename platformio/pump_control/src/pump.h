/*
      Garage.h - Library for managing the garage door
        Created by Steven Sprouse, January 4, 2016.
          Released into the public domain.
*/
#ifndef Pump_h
#define Pump_h

#include "Arduino.h"
#include <assert.h>
#include "Simpletimer.h"

#include "config.h"

#define GARAGE_OPEN_TIMEOUT_MIN 10

extern void setLED(uint8_t state);
extern void set_event_time(uint8_t op);
extern void iosNotify(char *s);

class Pump
{
  private:
		enum _state_t {
			st_off,
			st_run_interval,
			st_wait_interval
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
    Pump(uint8_t sensor_pin);
    void run();
    uint8_t led_state();
    void fsm(uint8_t event);

  private:
    void pump_start();
    void pump_stop();
    void begin();
};

#endif
