#include "Arduino.h"
#include "Pump.h"
#include <Time.h>

extern SimpleTimer timer;
extern void gate_open_wdt_expired();

Pump::Pump(uint8_t relay_pin) {
  _state = st_off;
  _relay_pin = relay_pin;
  _wdt_id = -1;
}

void Pump::set_pump(int state){
  digitalWrite(relay_pin, state);
}

void Pump::start_timer() {
  Serial.printf("Set WDT\n");
  _wdt_id = timer.setTimerInterval(1000L, pump_timer_event); //Give three notifications
}

void Pump::stop_timer() {
  timer.deleteTimer(_wdt_id);
  _wdt_id = -1;
  Serial.printf("Deleted WDT\n");
}

void Pump::begin() {
  _time_opened = now();
  _time_closed = now();
  pinMode(_relay_pin, INPUT);
}

extern int dummy_sensor;
void Pump::run() {
  int new_sensor = dummy_sensor;
  if (new_sensor != _sensor_state) {
    if (new_sensor) {
      _event = EV_START_PUMP;
      Serial.println("Sensor opened");
    } else {
      _event = EV_STOP_PUMP;
      Serial.println("Sensor closed");
    }
    _sensor_state = new_sensor;

    fsm(_event);
  }
}

uint8_t Pump::led_state(){
  return _sensor_state;
}

#define PUMP_ON 1
#define PUMP_OFF 0
void Pump::fsm(uint8_t event) {
  _state_t next_state;
  next_state = _state;

  switch (_state) {
    case st_off:
      if (event == EV_START_PUMP) {
        set_pump(PUMP_ON);
        start_timer();
        next_state = st_run_interval;
        tick_count = 0;
      }
      break;
    case st_run_interval:
      if (event == PUMP_TICK) {
          tick_count++;
      }
      if (tick_count > PUMP_ON_TIME) {
        next_state = st_wait_interval;
        set_pump(PUMP_OFF);
        tick_count = 0;
      } 
      break;
    case st_wait_interval:
      if (event == PUMP_TICK) {
          tick_count++;
      }
      if (tick_count > PUMP_WAIT_TIME) {
        next_state = st_off;
        tick_count = 0;
        stop_timer();
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
