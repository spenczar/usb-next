#define PAUSE_DURATION 100 // Time, in microseconds, between KB queries.
#define USB_SEND_INTERVAL 1000 // microseconds
//#define DEBUG
#undef DEBUG
#define KB_ENABLE

#include <Keyboard.h>
#include "keymap.h"

#ifdef DEBUG
#define PAUSE_DURATION 100
#define debug(MSG) Serial.print(MSG)
#define debugf(MSG, MODE) Serial.print(MSG, MODE)
#define debugln(MSG) Serial.println(MSG)
#else
#define debug(MSG)
#define debugf(MSG, MODE)
#define debugln(MSG)
#endif

#define OUT_PIN 3
#define IN_PIN 2
#define SAMPLE_INDICATOR_PIN 5
#define LED_PIN 13

#define SET_REGISTER_1(REGISTER, POSITION) REGISTER |= (1 << POSITION)
#define SET_REGISTER_0(REGISTER, POSITION) REGISTER &= ~(1 << POSITION)

// Macros and caches for reading data from the input pin
volatile uint8_t *input_pin_reg;
uint8_t input_pin_mask;
#define readbit() (PIND & 0b00000010)

volatile uint8_t *sample_pin_reg;
uint8_t sample_pin_mask;
#define enable_sample_pin() PORTC |= 0b01000000
#define disable_sample_pin() PORTC &= 0b10111111

// Sampling math
#define CLOCK_FREQ 16000000
// discovered with a logical analyzer: a 455 kHz clock powers signals that last
// for 24 clock cycles.
#define KB_DATA_FREQ (455000 / 24) // 18958
#define PULSE_WIDTH (1000000 / KB_DATA_FREQ)  // 52
#define PULSE_WIDTH_NANOS (1000000000 / KB_DATA_FREQ)  // 52748

#define KB_SAMPLE_COMP (CLOCK_FREQ / KB_DATA_FREQ) // 843
#define SAMPLE_INTERVAL_NANOS (1000000000 / CLOCK_FREQ) * KB_SAMPLE_COMP // 52687
#define ERROR_PER_SAMPLE (PULSE_WIDTH_NANOS - SAMPLE_INTERVAL_NANOS)
#define PULSE_SETTLE_TIME 5


#define RESPONSE_SIZE 20

const uint16_t t1_load = 0;
const uint16_t t1_comp = KB_SAMPLE_COMP;

enum state{
           ready,
           query_inflight,
           query_sent,
           query_response_timeout,
           query_response_ready,
           query_response_complete,
           pause,
};

// Global state
volatile enum state currentState = ready;
volatile uint8_t currentKeycode = 0;
volatile uint8_t currentModifier = 0;
volatile uint32_t currentData = 0;

volatile uint8_t currentResponseIndex = 0;

///////
// Protocol
///////

#define IDLE_SIGNAL 0x180600

struct KeyCommand {
  uint8_t keycode;
  uint8_t modifiers;
  bool pressed; // vs released
};

struct KeyCommand parseCurrentData() {
  struct KeyCommand cmd;
  cmd.keycode   = (currentData >> 1)  & 0xFF;
  cmd.modifiers = (currentData >> 11) & 0xFF;
  cmd.pressed   = !(cmd.keycode & 0x80);
  cmd.keycode   = cmd.keycode & 0x7F;
  return cmd;
}

void sendKBQuery() {
  digitalWrite(OUT_PIN, LOW);
  delayMicroseconds(PULSE_WIDTH *5);
  digitalWrite(OUT_PIN, HIGH);
  delayMicroseconds(PULSE_WIDTH );
  digitalWrite(OUT_PIN, LOW);
  delayMicroseconds(PULSE_WIDTH *3);
  digitalWrite(OUT_PIN, HIGH);
}

void sendKBReset() {
  // reset the keyboard
  digitalWrite(OUT_PIN, LOW);
  delayMicroseconds(PULSE_WIDTH);
  digitalWrite(OUT_PIN, HIGH);
  delayMicroseconds(PULSE_WIDTH*4);
  digitalWrite(OUT_PIN, LOW);
  delayMicroseconds(PULSE_WIDTH);
  digitalWrite(OUT_PIN, HIGH);
  delayMicroseconds(PULSE_WIDTH*6);
  digitalWrite(OUT_PIN, LOW);
  delayMicroseconds(PULSE_WIDTH*10);
  digitalWrite(OUT_PIN, HIGH);
}
//

/////////
/// State transition handlers
////////
void handleReady() {
  // Disable interrupts while we configure things
  cli();

  // Set up interrupt for when we get a response back.
  enableResponseInterrupt();
  // Set up interrupt for when we timeout.
  enableTimeoutInterrupt();

  // Proceed to next state.
  currentState = query_sent;
  // Turn interrupts back on
  sei();
  // Send the query.
  sendKBQuery();
}

/////
// Start-of-response interrupt
/////
ISR(INT1_vect) {
  // Triggered when we get the falling edge of Pin INT1.
  // Turn off the response interrupt.
  disableResponseInterrupt();

  // We can turn off the timeout, since we have data.
  disableTimeoutInterrupt();

  delayMicroseconds(PULSE_SETTLE_TIME);
  // Turn on the ticker for reading data.
  enableReaderInterrupt();

  // Mark us as reading data.
  currentState = query_response_ready;
}

void enableResponseInterrupt() {
  // Enable interrupts for INT1.
  SET_REGISTER_1(EIMSK, INT1);
}

void configureResponseInterrupt() {
  // Enable interrupts on a low level of external interrupt pin 1
  SET_REGISTER_0(EICRA, ISC10);
  SET_REGISTER_0(EICRA, ISC11);
  // Clear any existing interrupts for pin 1
  SET_REGISTER_1(EIFR, INTF1);

  debug("EICRA: 0x");
  debugf(EICRA, HEX);
  debug("  EIFR: 0x");
  debugf(EIFR, HEX);
  debugln();
}

void disableResponseInterrupt() {
  // Enable the interrupt
  SET_REGISTER_0(EIMSK, INT1);
  // Clear any interrupts that have happened
  SET_REGISTER_1(EIFR, INTF1);
}

/////
// Timeout interrupt (TODO)
/////
void enableTimeoutInterrupt() {}
void configureTimeoutInterrupt() {}
void disableTimeoutInterrupt() {}

/////
// Reader interrupt
/////
ISR(TIMER1_COMPA_vect) {
  TCNT1 = 0;

  enable_sample_pin();
  if (readbit()) {
    currentData |= ((uint32_t)1 << currentResponseIndex);
  }
  disable_sample_pin();

  currentResponseIndex++;
  if (currentResponseIndex > RESPONSE_SIZE) {
    // Done reading.
    currentResponseIndex = 0;
    disableReaderInterrupt();
    currentState = query_response_complete;
  }
}

void enableReaderInterrupt() {
  // Reset to zero
  TCNT1 = 0;
  // Enable register A comparison interrupts
  SET_REGISTER_1(TIMSK1, OCIE1A);
  // Clear any existing interrupts for pin 1
  SET_REGISTER_1(EIFR, INTF1);
}

void configureReaderInterrupt() {
  // Set timer to defaults in case arduino has done anything funky
  TCCR1A = 0;

  // No prescaling
  TCCR1B &= ~(1 << CS12);
  TCCR1B &= ~(1 << CS11);
  TCCR1B |= (1 << CS10);

  // Set the trigger threshold
  OCR1A = t1_comp;
}

void disableReaderInterrupt() {
  // Disable interrupts
  TIMSK1 &= ~(1 << OCIE1A);
  // Clear any active interrupts
}


void handleQueryResponseTimeout() {
  // Send a reset
  sendKBReset();
  currentState = pause;
}

void handlePause() {
  delayMicroseconds(PAUSE_DURATION);
  currentState = ready;
}

void handleQueryResponse() {
  // Send current key over USB.
  debug("current data: ");
  if (currentData == IDLE_SIGNAL) {
    debugln("idle");
  } else {
    struct KeyCommand cmd = parseCurrentData();
    debug("mod: "); debugf(cmd.modifiers, HEX); debugln();
    debug("key: "); debugf(cmd.keycode, HEX); debugln();
    debug("prs: "); debug(cmd.pressed); debugln();

    if (cmd.pressed) {
      pressKey(cmd.keycode, cmd.modifiers);
    } else {
      releaseKey(cmd.keycode, cmd.modifiers);
    }
  }

  currentData = 0;
  currentState = pause;
}

volatile KeyReport report = {0};
volatile uint32_t last_send_time = micros();

void pressKey(uint8_t keycode, uint8_t modifier) {
  #ifdef KB_ENABLE
  report.modifiers = mapModifiers(modifier);
  uint8_t mapped_code = keymap[keycode];
  if (mapped_code != 0) {
    // Only set if it's not present
    if (report.keys[0] != mapped_code && report.keys[1] != mapped_code &&
        report.keys[2] != mapped_code && report.keys[3] != mapped_code &&
        report.keys[4] != mapped_code && report.keys[5] != mapped_code) {
      for (int i = 0; i < 6; i ++) {
        if (report.keys[i] == 0) {
          report.keys[i] = mapped_code;
          break;
        }
      }
    }
  }
  sendUSBReport();
  #endif

  debug("press key code=");
  debugf(keycode, HEX);
  debug(" mod=");
  debugf(modifier, HEX);
  debug(" mapped-code=");
  debugf(mapped_code, HEX);
  debugln();
}

void releaseKey(uint8_t keycode, uint8_t modifier) {
  #ifdef KB_ENABLE
  uint8_t mapped_code = keymap[keycode];
  if (mapped_code != 0) {
    for (int i = 0; i < 6; i ++) {
      if (report.keys[i] == mapped_code) {
        report.keys[i] = 0;
        break;
      }
    }
  }
  if (modifier != 0) {
    report.modifiers = mapModifiers(modifier);
  }
  sendUSBReport();
  #endif

  debug("release key code=");
  debugf(keycode, HEX);
  debug(" mod=");
  debugf(modifier, HEX);
  debug(" mapped-code=");
  debugf(mapped_code, HEX);
  debugln();
}

void sendUSBReport() {
  uint32_t now = micros();
  if ((now - last_send_time) < USB_SEND_INTERVAL) {
    return;
  }
  HID().SendReport(2, &report, sizeof(KeyReport));
  debug("elapsed: "); debugf(now - last_send_time, DEC);
  last_send_time = now;

  debug(" report[0]="); debugf(report.keys[0], HEX);
  debug(" report[1]="); debugf(report.keys[1], HEX);
  debug(" report[2]="); debugf(report.keys[2], HEX);
  debug(" report[3]="); debugf(report.keys[3], HEX);
  debug(" report[4]="); debugf(report.keys[4], HEX);
  debug(" report[5]="); debugf(report.keys[5], HEX);
  debugln();
}

uint8_t mapModifiers(uint8_t nextMod) {
  uint8_t result = 0;
  if (nextMod & 0x2) {
    result |= KEY_LEFTCTRL;
  }
  if (nextMod & 0x4) {
    result |= KEY_LEFTSHIFT;
  }
  if (nextMod & 0x8) {
    result |= KEY_RIGHTSHIFT;
  }
  if (nextMod & 0x10) {
    result |= KEY_LEFTMETA;
  }
  if (nextMod & 0x20) {
    result |= KEY_RIGHTMETA;
  }
  if (nextMod & 0x40) {
    result |= KEY_LEFTALT;
  }
  return result;
}

void loop() {
  switch (currentState) {
  case ready:
    handleReady();
    break;
  case query_sent:
    // No-op. Waiting for a timeout or ready state.
    break;
  case query_response_timeout:
    debugln("timeout");
    handleQueryResponseTimeout();
    break;
  case query_response_ready:
    // No-op. Wait for response complete.
    break;
  case query_response_complete:
    handleQueryResponse();
    break;
  case pause:
    handlePause();
    break;
  }
}
void setup() {
  pinMode(OUT_PIN, OUTPUT);
  pinMode(SAMPLE_INDICATOR_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(IN_PIN, INPUT);

  digitalWrite(LED_PIN, HIGH);
  input_pin_mask = digitalPinToBitMask(IN_PIN);

  sample_pin_reg = portInputRegister(digitalPinToPort(SAMPLE_INDICATOR_PIN));
  sample_pin_mask = digitalPinToBitMask(SAMPLE_INDICATOR_PIN);

  int start = millis();
  while ((!Serial) & (millis() - start < 2000)) {
    Serial.begin(57600);
   }
  digitalWrite(LED_PIN, LOW);

  configureResponseInterrupt();
  configureReaderInterrupt();

  debugln("--== NeXT Keyboard Initialization ==--");
  debug("Pulse width: "); debugln(PULSE_WIDTH);
  debug("Sample Frequency: "); debugln(KB_SAMPLE_COMP);

  delay(200);
  sendKBQuery();
  delay(5);
  sendKBReset();
  delay(8);
  sendKBQuery();
  delay(5);
  sendKBReset();
  delay(8);
  Keyboard.begin();
  debugln("keyboard on!");
  digitalWrite(LED_PIN, LOW);
}
