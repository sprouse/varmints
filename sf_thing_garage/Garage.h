/*
      Garage.h - Library for managing the garage door
        Created by Steven Sprouse, January 4, 2016.
          Released into the public domain.
          */
#ifndef Garage_h
#define Garage_h

#include "Arduino.h"
#include <TimeLib.h>

#define OPEN 1
#define CLOSED 2
#define OPEN_WDT 4
#define ERR_FSM 1

#define OFF 0
#define ON 1

extern void setLED(int state);
        
class Garage
{
    private:
        int _state;
        int _led_vpin;
        int _relay_pin;
        time_t _time_opened;
        time_t _time_closed;
    public:
        Garage(int led_pin, int sensor_pin);
        void run();
        
    private:
        void garage_open();
        void garage_closed();
        void begin();
        void fsm(int event);

};

#endif
