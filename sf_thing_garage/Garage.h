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

#define OPEN 1
#define CLOSED 2
#define OPEN_WDT 4
#define ERR_FSM 1

//#define OFF 0
//#define ON 1

// LCD VPins
//#define LCD_0 4
//#define LCD_1 5

#define GARAGE_OPEN_TIMEOUT_MIN 10

extern void setLED(uint8_t state);
extern void setTime(uint8_t vpin);
extern void iosNotify(char *s);

class Garage
{
  private:
    uint8_t _state;
    uint8_t _sensor_state;
    uint8_t _led_vpin;
    uint8_t _relay_pin;
    time_t _time_opened;
    time_t _time_closed;
    uint8_t _wdt_id;
    uint8_t _event;

    const uint8_t _OFF = 0;
    const uint8_t _ON = 1;
    const uint8_t LCD_0 = 4;
    const uint8_t LCD_1 = 5;

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
