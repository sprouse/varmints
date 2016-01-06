/*
      Garage.h - Library for managing the garage door
        Created by Steven Sprouse, January 4, 2016.
          Released into the public domain.
*/
#ifndef Garage_h
#define Garage_h

#include "Arduino.h"
#include <SimpleTimer.h>
#include <TimeLib.h>

#define OPEN 1
#define CLOSED 2
#define OPEN_WDT 4
#define ERR_FSM 1

#define OFF 0
#define ON 1

#define LCD_0 4
#define LCD_1 5

extern void setLED(int state);
extern void setTime(int vpin);
extern SimpleTimer timer;
extern void gateOpenWDT();
extern void iosNotify(char *s);

class Garage
{
  private:
    int _state;
    int _sensor_state;
    int _led_vpin;
    int _relay_pin;
    time_t _time_opened;
    time_t _time_closed;
    int _wdt_id;
    int _event;

  public:
    Garage(int led_pin, int sensor_pin);
    void run();
    void fsm(int event);

  private:
    void garage_open();
    void garage_closed();
    void begin();
};

#endif
