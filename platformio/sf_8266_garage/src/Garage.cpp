#include "Arduino.h"
#include "Garage.h"
#include <Time.h>

extern SimpleTimer timer;
extern void gate_open_wdt_expired();

Garage::Garage(uint8_t relay_pin) {
  _state = st_closed;
  //setLED(OFF);
  _relay_pin = relay_pin;
  _wdt_id = -1;
}

void Garage::garage_open() {
  setLED(ON);
  _time_opened = now();
  Serial.printf("Set WDT\n");

  _wdt_id = timer.setTimer(GARAGE_OPEN_TIMEOUT_MIN * 60 * 1000L, gate_open_wdt_expired, 3); //Give three notifications

  set_event_time(EV_OPENED);
}

void Garage::garage_closed() {
  setLED(OFF);
  _time_closed = now();
  timer.deleteTimer(_wdt_id);
  _wdt_id = -1;
  set_event_time(EV_CLOSED);
  Serial.printf("Deleted WDT\n");
}

void Garage::begin() {
  _time_opened = now();
  _time_closed = now();
  pinMode(_relay_pin, INPUT);
}

extern int dummy_sensor;
void Garage::run() {
  int new_sensor = digitalRead(_relay_pin);
  //int new_sensor = dummy_sensor;
  if (new_sensor != _sensor_state) {
    if (new_sensor) {
      _event = EV_OPENED;
      Serial.println("Sensor opened");
    } else {
      _event = EV_CLOSED;
      Serial.println("Sensor closed");
    }
    _sensor_state = new_sensor;

    fsm(_event);
  }
}

uint8_t Garage::led_state(){
  return _sensor_state;
}

void Garage::fsm(uint8_t event) {
  _state_t next_state;
  next_state = _state;

  switch (_state) {
    case st_closed:
      if (event == EV_OPENED) {
        garage_open();
        next_state = st_open;

      }
      break;
    case st_open:
      if (event == EV_CLOSED) {
        garage_closed();
        next_state = st_closed;
      } else if (event == EV_OPEN_WDT) {
        char buf[256];
        long elapsed_time_open = now() - _time_opened;
        Serial.printf("duration: %ld\n", elapsed_time_open);
        sprintf(buf, "Garage door is open %d secs", elapsed_time_open);
        iosNotify(buf);
        //_wdt_id = timer.setTimeout(1 * 60L * 1000L, gate_open_wdt_expired); // reset and restart the timer
      }
      break;
    default:
      //error(ERR_FSM);
      break;
  }

  if (_state != next_state) {
    Serial.printf("State: %d => %d\n", _state, next_state);
  }
  _state = next_state;
}
