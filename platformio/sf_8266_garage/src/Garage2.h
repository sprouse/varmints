#define GARAGE_OPEN_TIMEOUT_MIN 10

class Garage
{
  private:
		enum _state_t {
			st_closed,
			st_open
		};

		_state_t _state;
    int _sensor_state;
    int _led_vpin;
    int _relay_pin;
    time_t _time_opened;
    time_t _time_closed;
    int _wdt_id;
    int _led_state;
    int _event;

  public:
    Garage(uint8_t sensor_pin);
    void run();
    byte led_state();
    void fsm(uint8_t event);

  private:
    void garage_open();
    void garage_closed();
    void garage_led(int state);
    void begin();
};

Garage::Garage(uint8_t relay_pin) {
  _state = st_closed;
  _relay_pin = relay_pin;
  _wdt_id = -1;
  _led_state = UNKNOWN;
}

void Garage::garage_open() {
  setLED(ON);
  _time_opened = now();
  Serial.printf("Set WDT\n");
  term_msg("Set WDT");

  _wdt_id = timer.setTimer(GARAGE_OPEN_TIMEOUT_MIN * 60 * 1000L,
                           gate_open_wdt_expired, 3); //Give three notifications

  set_event_time(EV_OPENED);
}

void Garage::garage_closed() {
  setLED(OFF);
  _time_closed = now();
  timer.deleteTimer(_wdt_id);
  _wdt_id = -1;
  set_event_time(EV_CLOSED);
  Serial.printf("Deleted WDT\n");
  term_msg("Deleted WDT");
}

void Garage::begin() {
  _time_opened = now();
  _time_closed = now();
  pinMode(_relay_pin, INPUT);
}

void Garage::run() {
  int new_sensor = digitalRead(_relay_pin);
  setLED(new_sensor);
  // If the RTC has not yet sync'd only update the LED state.
  if (year() == 1970){
    return;
  }

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
        send_status_to_indigo("GARAGE_OPEN");
        next_state = st_open;

      }
      break;
    case st_open:
      if (event == EV_CLOSED) {
        garage_closed();
        send_status_to_indigo("GARAGE_CLOSED");
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

    String s = String(event);
    s += ":";
    s += String(_state);;
    s += "->";
    s += String(next_state);;
    term_msg(s);
  }
  _state = next_state;
}
