#define PAUSE_DURATION 400 // Time, in milliseconds, between KB queries.
#define DEBUG

#ifdef DEBUG
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
#define PULSE_WIDTH_NANOS (1000000000 / KB_DATA_FREQ  // 52748
#define KB_SAMPLE_COMP (CLOCK_FREQ / KB_DATA_FREQ) // 843
#define SAMPLE_INTERVAL_NANOS (1000000000 / CLOCK_FREQ) * KB_SAMPLE_COMP // 52687
#define ERROR_PER_SAMPLE PULSE_WIDTH_NANOS - SAMPLE_INTERVAL_NANOS
#define PULSE_SETTLE_TIME 5

#define RESPONSE_SIZE 24

const uint16_t t1_load = 0;
const uint16_t t1_comp = KB_SAMPLE_COMP;

//#define KB_ENABLE

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
  currentData |= (readbit() << currentResponseIndex);
  disable_sample_pin();

  currentResponseIndex++;

  if (currentResponseIndex >= RESPONSE_SIZE) {
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
  delay(PAUSE_DURATION);
  currentState = ready;
}

void handleQueryResponse() {
  // Send current key over USB.
  debug("current data: ");
  for (uint8_t i = 0; i <= RESPONSE_SIZE; i++) {
    debug((currentData >> i) & 1);
    debug("");
    if (i % 4 == 3) {
      debug(" ");
    }
    if (i % 8 == 7) {
      debug(" ");
    }
  }
  currentData = 0;
  debugln();
  // sendKey(currentKeycode, currentModifier);
  currentState = pause;
}

void sendKey(uint8_t keycode, uint8_t modifier) {
  #ifdef KB_ENABLE
  #endif

  debug("sending key code=");
  debugf(keycode, HEX);
  debug(" mod=");
  debugf(modifier, HEX);
  debugln();
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
  pinMode(IN_PIN, INPUT);

  input_pin_reg = portInputRegister(digitalPinToPort(IN_PIN));
  input_pin_mask = digitalPinToBitMask(IN_PIN);

  sample_pin_reg = portInputRegister(digitalPinToPort(SAMPLE_INDICATOR_PIN));
  sample_pin_mask = digitalPinToBitMask(SAMPLE_INDICATOR_PIN);

#ifdef DEBUG
  while (!Serial)
  Serial.begin(57600);
#endif
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
}
