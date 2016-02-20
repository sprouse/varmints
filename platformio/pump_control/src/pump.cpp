#include "Arduino.h"
#include "Pump.h"
#include <Time.h>

extern void bprint(char *buf);
extern void set_pump_led(uint8_t state);
extern void set_run_button(uint8_t state);

char buf[256];

Pump::Pump(uint8_t relay_pin) {
  _state = st_off_and_locked_out;
  _relay_pin = relay_pin;
}

void Pump::set_pump(uint8_t state){
  digitalWrite(_relay_pin, state);
  set_pump_led(state);
}

void Pump::begin() {
  set_pump(_pump_off);
}

extern int button_state;
void Pump::run() {
}

void Pump::fsm(uint8_t event) {
  _state_t next_state;
  next_state = _state;

  switch (_state) {
    // Pump is off and is prevented from turning on for some interval.
    case st_off_and_locked_out:
      if (event == ev_timer_tick) {
          sprintf(buf, "lockout %d of %d", _tick_count, _pump_lockout_time);
          bprint(buf);
          _tick_count++;
      }
      if (_tick_count > _pump_lockout_time) {
        next_state = st_off;
        set_pump(_pump_off);
        _tick_count = 0;
      } 
      break;

    // Pump is off and is able to be turned on.
    case st_off:
      sprintf(buf, "pump off");
      bprint(buf);
      if (button_state == 1) {
        set_pump(_pump_on);
        next_state = st_run_interval;
        _tick_count = 0;
      }
      break;
    // Pump runs for some interval
    // TODO: Add temperature detection as another way to exit this state.
    case st_run_interval:
      if (event == ev_timer_tick) {
          sprintf(buf, "run: %d of %d", _tick_count, _pump_on_time);
          bprint(buf);
          _tick_count++;
      }
      if (_tick_count > _pump_on_time) {
        next_state = st_off_and_locked_out;
        set_pump(_pump_off);
        _tick_count = 0;
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
  // Clear the button state
  set_run_button(0);
}
