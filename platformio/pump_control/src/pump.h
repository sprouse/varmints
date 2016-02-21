/*
      Garage.h - Library for managing the garage door
        Created by Steven Sprouse, January 4, 2016.
          Released into the public domain.
*/
#ifndef Pump_h
#define Pump_h

#include "Arduino.h"
#include <assert.h>

#include "config.h"

#define GARAGE_OPEN_TIMEOUT_MIN 10

extern void setLED(uint8_t state);
extern void set_event_time(uint8_t op);
extern void iosNotify(char *s);

class Pump
{
  public:
		enum _event_t {
			ev_none,
			ev_timer_tick
		};

  private:
		enum _state_t {
			st_off_and_locked_out,
			st_off,
			st_run_interval,
		} _state;

    const uint8_t _pump_off = 0;
    const uint8_t _pump_on = 1;
    const uint8_t _pump_lockout_time = 3;
    const uint8_t _pump_on_time = 3;

    uint8_t _relay_pin;
    uint8_t _event;
    uint32_t _tick_count;

#define BUF_LEN 128
    char buf[BUF_LEN];

  public:
    Pump(uint8_t sensor_pin);
    void run();
    uint8_t led_state();
    void fsm(uint8_t event);
    void begin();

  private:
    void set_pump(uint8_t state);
};

#endif
