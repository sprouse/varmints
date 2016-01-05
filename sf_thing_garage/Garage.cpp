#include "Arduino.h"
#include "Garage.h"

Garage::Garage(int led_vpin, int relay_pin) {
  _state = CLOSED;
  _relay_pin = relay_pin;
  _led_vpin = led_vpin;

  _wdt_id = _timer.setTimeout(10 * 60 * 1000L, open_wdt);
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

void Garage::run() {
  int new_sensor = digitalRead(_relay_pin);
  if (new_sensor != _sensor_state) {
    if (new_sensor) {
      _event = OPEN;
    } else {
      _event = CLOSED;
    }
    _sensor_state = new_sensor;

    fsm(_event);
  }
}

void Garage::fsm(int event) {
  int next_state;
  next_state = _state;
  switch (_state) {
    case CLOSED:
      if (event == OPEN) {
        garage_closed();
        timer.disable(_wdt_id);
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
        timer.restartTimer(_wdt_id);  // reset and restart the timer
        timer.enable(_wdt_id);
      }
      break;
    default:
      //error(ERR_FSM);
      break;
  }
  _state = next_state;
}
