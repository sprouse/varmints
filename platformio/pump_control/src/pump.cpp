#include "Arduino.h"
#include "Pump.h"
#include <Time.h>

extern void bprint(char *buf);
extern void set_pump_state(uint8_t state);
extern void set_run_button(uint8_t state);
extern void show_time();

#define X(a, b) b,  
static char *state_strings[] = { PUMP_STATES };
#undef X

Pump::Pump() {
  _state = st_off_and_locked_out;
}

void Pump::begin() {
  set_pump_state(_pump_off);
}

extern int run_button_state;
extern int vacation_button_state;
void Pump::run() {
}

void Pump::fsm(uint8_t event) {
  _state_t next_state = _state;

  switch (_state) {
    // Pump is off and is prevented from turning on for some interval.
    case st_off_and_locked_out:
      if (event == ev_timer_tick) {
#ifdef DEBUG_TERM
          snprintf(buf, BUF_LEN, "lockout: %d of %d", _tick_count, _pump_lockout_time);
          bprint(buf);
#endif
          _tick_count++;
      }
      if (_tick_count >= _pump_lockout_time) {
        next_state = st_off;
        set_pump_state(_pump_off);
        _tick_count = 0;
      } 
      break;

    // Pump is off and is able to be turned on.
    case st_off:
      if (!vacation_button_state && run_button_state == 1) {
        set_pump_state(_pump_on);
        next_state = st_run_interval;
        _tick_count = 0;
      }
      break;
    // Pump runs for some interval
    // TODO: Add temperature detection as another way to exit this state.
    case st_run_interval:
      if (event == ev_timer_tick) {
#ifdef DEBUG_TERM
          snprintf(buf, BUF_LEN, "run: %d of %d", _tick_count, _pump_on_time);
          bprint(buf);
#endif
          _tick_count++;
      }
      if (_tick_count >= _pump_on_time) {
        next_state = st_off_and_locked_out;
        set_pump_state(_pump_off);
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
  if (next_state != _state) {
          show_time();
          snprintf(buf, BUF_LEN, "fsm: %s -> %s", state_strings[_state], state_strings[next_state]);
          bprint(buf);
  }
  _state = next_state;
  // Clear the button state
  set_run_button(0);
}
