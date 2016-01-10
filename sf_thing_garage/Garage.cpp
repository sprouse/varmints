#include "Arduino.h"
#include "Garage.h"
#include "SimpleTimer.h"

extern SimpleTimer timer;
extern void gateOpenWDT();

Garage::Garage(int relay_pin) {
  _state = CLOSED;
  //setLED(OFF);
  _relay_pin = relay_pin;
  _wdt_id = -1;
}

void Garage::garage_open() {
  setLED(ON);
  _time_opened = now();
  Serial.printf("Set WDT\n");
  _wdt_id = timer.setTimer(GARAGE_OPEN_TIMEOUT_MIN * 1000L, gateOpenWDT, 3); //Give three notifications
  setTime(LCD_0);
  timer.pprev(2);
}

void Garage::garage_closed() {
  setLED(OFF);
  _time_closed = now();
  timer.deleteTimer(_wdt_id);
  _wdt_id = -1;
  setTime(LCD_1);
  //XXX make string with time closed and send to blynk display
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
      _event = OPEN;
      Serial.println("Sensor opened");
    } else {
      _event = CLOSED;
      Serial.println("Sensor closed");
    }
    _sensor_state = new_sensor;

    fsm(_event);
  }
}

void Garage::fsm(int event) {
  int next_state;
  next_state = _state;
  timer.pprev(1);

  switch (_state) {
    case CLOSED:
      if (event == OPEN) {
        garage_open();
        next_state = OPEN;
        timer.pprev(3);

      }
      break;
    case OPEN:
      if (event == CLOSED) {
        garage_closed();
        next_state = CLOSED;
      } else if (event == OPEN_WDT) {
        char buf[256];
        long elapsed_time_open = now() - _time_opened;
        Serial.printf("duration: %ld\n", elapsed_time_open);
        sprintf(buf, "Garage door is open %d secs", elapsed_time_open);
        iosNotify(buf);
      }
      break;
    default:
      //error(ERR_FSM);
      break;
  }
  timer.pprev(4);

  if (_state != next_state) {
    Serial.printf("State: %d => %d\n", _state, next_state);
  }
  _state = next_state;
  timer.pprev(5);
}
