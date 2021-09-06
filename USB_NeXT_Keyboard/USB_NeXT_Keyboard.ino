// NeXT non-ADB Keyboard to USB converter
// This will take an older NeXT keyboard, talk to it, and turn the keycodes into a USB keyboard
// Requires an Arduino Micro for the USB portion - but could be ported to another micro fairly easily
// Written by Limor Fried / Adafruit Industries
// Released under BSD license - thanks NetBSD! :)
//
// Timing reference thanks to http://m0115.web.fc2.com/
// Pinouts thanks to http://www.68k.org/~degs/nextkeyboard.html
// Keycodes from http://ftp.netbsd.org/pub/NetBSD/NetBSD-release-6/src/sys/arch/next68k/dev/

#include "wsksymdef.h"
#include "nextkeyboard.h"
#include <Keyboard.h>

// the timing per bit, 50microseconds
#define TIMING 51

// pick which pins you want to use
#define KEYBOARDOUT 3
#define KEYBOARDIN 2

// comment to speed things up, uncomment for help!
#define DEBUG

// comment to stop sending key presses
//#define SEND_OUTPUTS

// speed up reads by caching the 'raw' pin ports
volatile uint8_t *misoportreg;
uint8_t misopin;
// our little macro
#define readkbd() ((*misoportreg) & misopin)

// debugging/activity LED
#define LED 13

#define NEXT_KMBUS_IDLE 0x200600

// NeXT Keyboard Defines
// modifiers
#define NEXT_KB_CONTROL 0x1000
#define NEXT_KB_ALTERNATE_LEFT 0x20000
#define NEXT_KB_ALTERNATE_RIGHT 0x40000
#define NEXT_KB_COMMAND_LEFT 0x8000
#define NEXT_KB_COMMAND_RIGHT 0x10000
#define NEXT_KB_SHIFT_LEFT 0x2000
#define NEXT_KB_SHIFT_RIGHT 0x4000

// special command for setting LEDs
void setLEDs(bool leftLED, bool rightLED) {
  digitalWrite(KEYBOARDOUT, LOW);
  delayMicroseconds(TIMING *9);
  digitalWrite(KEYBOARDOUT, HIGH);
  delayMicroseconds(TIMING *3);
  digitalWrite(KEYBOARDOUT, LOW);
  delayMicroseconds(TIMING);

  if (leftLED)
      digitalWrite(KEYBOARDOUT, HIGH);
  else
      digitalWrite(KEYBOARDOUT, LOW);
  delayMicroseconds(TIMING);

  if (rightLED)
      digitalWrite(KEYBOARDOUT, HIGH);
  else
      digitalWrite(KEYBOARDOUT, LOW);
  delayMicroseconds(TIMING);
  digitalWrite(KEYBOARDOUT, LOW);
  delayMicroseconds(TIMING *7);
  digitalWrite(KEYBOARDOUT, HIGH);
}

void query() {
  // query the keyboard for data
  digitalWrite(KEYBOARDOUT, LOW);
  delayMicroseconds(TIMING *5);
  digitalWrite(KEYBOARDOUT, HIGH);
  delayMicroseconds(TIMING );
  digitalWrite(KEYBOARDOUT, LOW);
  delayMicroseconds(TIMING *3);
  digitalWrite(KEYBOARDOUT, HIGH);
}

void nextreset() {
  // reset the keyboard
  digitalWrite(KEYBOARDOUT, LOW);
  delayMicroseconds(TIMING);
  digitalWrite(KEYBOARDOUT, HIGH);
  delayMicroseconds(TIMING*4);
  digitalWrite(KEYBOARDOUT, LOW);
  delayMicroseconds(TIMING);
  digitalWrite(KEYBOARDOUT, HIGH);
  delayMicroseconds(TIMING*6);
  digitalWrite(KEYBOARDOUT, LOW);
  delayMicroseconds(TIMING*10);
  digitalWrite(KEYBOARDOUT, HIGH);
}


void setup() {
  // set up pin directions
  pinMode(KEYBOARDOUT, OUTPUT);
  pinMode(KEYBOARDIN, INPUT);
  pinMode(LED, OUTPUT);

  misoportreg = portInputRegister(digitalPinToPort(KEYBOARDIN));
  misopin = digitalPinToBitMask(KEYBOARDIN);

  // according to http://cfile7.uf.tistory.com/image/14448E464F410BF22380BB
  query();
  delay(5);
  nextreset();
  delay(8);

  query();
  delay(5);
  nextreset();
  delay(8);

  kbbegin();

#ifdef DEBUG
  while (!Serial)
  Serial.begin(57600);
  Serial.println("NeXT keyboard initialized!");
#endif
}

uint32_t getresponse() {
  // bitbang timing, read 22 bits 50 microseconds apart
  cli();
  uint32_t n = 0;
  while (n < 1000) {
    if (!readkbd()) {
      break;
    }
    n++;
  }
  delayMicroseconds(TIMING/2);
  uint32_t data = 0;
  for (uint8_t i=0; i < 22; i++) {
      if (readkbd())
        data |= ((uint32_t)1 << i);
      delayMicroseconds(TIMING);
  }
  sei();
  return data;

}

void kbpress(char key) {
#ifdef DEBUG
  Serial.print("sending key "); Serial.print(key, HEX); Serial.println();
#endif
#ifdef SEND_OUTPUT
  Keyboard.press(key);
#endif
}

void kbbegin() {
#ifdef SEND_OUTPUT
  Keyboard.begin();
#endif
}

void kbrelease(char key) {
#ifdef DEBUG
  if (key & 0xFFFFFF00) {
    // releasing a modifier - no op
  } else {
    Serial.print("releasing key "); Serial.print(key, HEX); Serial.println();
  }
#endif
#ifdef SEND_OUTPUT
  Keyboard.release(key);
#endif
}

void loop() {
  digitalWrite(LED, LOW);
  delay(20);
  uint32_t resp;
  query();
  resp = getresponse();

  // check for a 'idle' response, we'll do nothing
  if (resp == NEXT_KMBUS_IDLE) return;

  // turn on the LED when we get real resposes!
  digitalWrite(LED, HIGH);

  // keycode is the lower 7 bits
  uint8_t keycode = resp & 0xFF;
  keycode /= 2;

#ifdef DEBUG
  Serial.print('['); Serial.print(resp, HEX);  Serial.print("] ");
  Serial.print("keycode: "); Serial.print(keycode);
#endif

  // modifiers! you can remap these here,
  // but I suggest doing it in the OS instead
  if (resp & NEXT_KB_CONTROL)
    kbpress(KEY_LEFT_CTRL);
  else
    kbrelease(KEY_LEFT_CTRL);

  if (resp & NEXT_KB_SHIFT_LEFT) {
    kbpress(KEY_LEFT_SHIFT);
  } else {
    kbrelease(KEY_LEFT_SHIFT);
  }
  if (resp & NEXT_KB_SHIFT_RIGHT) {
    kbpress(KEY_RIGHT_SHIFT);
  } else {
    kbrelease(KEY_RIGHT_SHIFT);
  }

  boolean shiftPressed = (resp & (NEXT_KB_SHIFT_LEFT|NEXT_KB_SHIFT_RIGHT)) != 0;

  // turn on shift LEDs if shift is held down
  if (shiftPressed)
    setLEDs(true, true);
  else
    setLEDs(false, false);

  if (resp & NEXT_KB_COMMAND_LEFT)
    kbpress(KEY_LEFT_GUI);
  else
    kbrelease(KEY_LEFT_GUI);

  if (resp & NEXT_KB_COMMAND_RIGHT)
    kbpress(KEY_RIGHT_GUI);
  else
    kbrelease(KEY_RIGHT_GUI);

  if (resp & NEXT_KB_ALTERNATE_LEFT)
    kbpress(KEY_LEFT_ALT);
  else
    kbrelease(KEY_LEFT_ALT);
  if (resp & NEXT_KB_ALTERNATE_RIGHT)
    kbpress(KEY_RIGHT_ALT);
  else
    kbrelease(KEY_RIGHT_ALT);

  if (keycode == 0) return;

  for (int i = 0; i< 100; i++) {
    if (nextkbd_keydesc_us[i*3] == keycode) {
      keysym_t keydesc = nextkbd_keydesc_us[i*3+1];
      char ascii = (char) keydesc;

#ifdef DEBUG
      Serial.print("--> ");      Serial.print(ascii); Serial.print(" / "); Serial.print(keydesc, HEX);
#endif

      int code;
      switch (keydesc) {
        case KS_KP_Enter:
        case KS_Return:    code = KEY_RETURN; break;
        case KS_Escape:    code = KEY_ESC; break;
        case KS_BackSpace: code = KEY_BACKSPACE; break;
        case KS_Up:        code = KEY_UP_ARROW; break;
        case KS_Down:      code = KEY_DOWN_ARROW; break;
        case KS_Left:      code = KEY_LEFT_ARROW; break;
        case KS_Right:     code = KEY_RIGHT_ARROW; break;

        // hacks for two tricky numpad keys
        case KS_KP_Equal:  code = (shiftPressed ? KS_bar : ascii); break;
        case KS_KP_Divide:
          if (shiftPressed) {
            kbrelease(KEY_RIGHT_SHIFT);
            kbrelease(KEY_LEFT_SHIFT);

            code = KS_backslash;
          } else {
            code = ascii;
          }
          break;

        // remap the other special keys because the KeyboardMouse can't send proper vol/brightness anyway
        case KS_AudioLower:  code = KEY_INSERT; break;
        case KS_AudioRaise:  code = KEY_DELETE; break;
        case KS_Cmd_BrightnessUp:    code = KEY_PAGE_UP; break;
        case KS_Cmd_BrightnessDown:  code = KEY_PAGE_DOWN; break;

        case 0:
        default: code = ascii;
      }

      if ((resp & 0xF00) == 0x400) {  // down press
#ifdef DEBUG
        Serial.println(" v ");
#endif
        kbpress(code);
        break;
      }
      if ((resp & 0xF00) == 0x500) {
        kbrelease(code);
#ifdef DEBUG
        Serial.println(" ^ ");
#endif
        break;
      }

      // re-press shift if need be
      if (keydesc == KS_KP_Divide && shiftPressed) {
          if (resp & NEXT_KB_SHIFT_LEFT)
            kbpress(KEY_LEFT_SHIFT);
          if (resp & NEXT_KB_SHIFT_RIGHT)
            kbpress(KEY_RIGHT_SHIFT);
      }
    }
  }
}
