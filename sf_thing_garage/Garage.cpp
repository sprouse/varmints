#include "Arduino.h"
#include "Garage.h"

Garage::Garage(int led_vpin, int relay_pin) {
  _state = CLOSED;
  _relay_pin = relay_pin;
  _led_vpin = led_vpin;
}

void Garage::garage_open() {
  setLED(ON);
  _time_opened = now();
}

void Garage::garage_closed() {
  setLED(OFF);
  _time_closed = now();
  //XXX make string with time closed and send to blynk display
}

void Garage::begin() {
  _time_opened = now();
  _time_closed = now();
  pinMode(_relay_pin, INPUT);
}

void Garage::run(){
}

void Garage::fsm(int event) {
  int next_state;
  next_state = _state;
  switch (_state) {
    case CLOSED:
      if (event == OPEN) {
        garage_open();
        next_state = OPEN;
      }
      break;
    case OPEN:
      if (event == CLOSED) {
        garage_open();
        next_state = CLOSED;
      } else if (event == OPEN_WDT) {
        long elapsed_time_open = now() - _time_opened;
        //update_elapsed_time_open(elapsed_time_open);
        //reset_open_wdt();
      }
      break;
    default:
      //error(ERR_FSM);
      break;
  }

  _state = next_state;
}
