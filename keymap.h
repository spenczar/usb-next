#include <Keyboard.h>

typedef unsigned short keycode;

/*
  Note to self (2021-11-19)

  These are USB HID scancodes. But the Arduino Keyboard library expects literal
  ascii characters to be sent. So everything here is wrong.

  The mapping table is correct, at least in the sense that I have entered all
  the values from the NeXT keyboard. But the defined constants that are in the
  big table are incorrect, at least for arduino's Keyboard library.

  One option is to try to get a lower-level library that allows sending the USB
  scancodes. Another option is to change the table to map NeXT scancodes to
  ASCII. The latter is easier, but it would unfortunately mean we need to drop
  support for the volume keys, and would make numpad no different than the top
  row, and so on.

 */

// Modifiers
#define KEY_LEFTCTRL   1<<0     // Keyboard Left Control
#define KEY_LEFTSHIFT  1<<1     // Keyboard Left Shift
#define KEY_LEFTALT    1<<2     // Keyboard Left Alt
#define KEY_LEFTMETA   1<<3     // Keyboard Left GUI
#define KEY_RIGHTCTRL  1<<4     // Keyboard Right Control
#define KEY_RIGHTSHIFT 1<<5     // Keyboard Right Shift
#define KEY_RIGHTALT   1<<6     // Keyboard Right Alt
#define KEY_RIGHTMETA  1<<7     // Keyboard Right GUI

#define KEY_NONE 0x00 // No key pressed
#define KEY_ERR_OVF 0x01 //  Keyboard Error Roll Over - used for all slots if too many keys are pressed ("Phantom key")
// 0x02 //  Keyboard POST Fail
// 0x03 //  Keyboard Error Undefined
#define KEY_A 0x04 // Keyboard a and A
#define KEY_B 0x05 // Keyboard b and B
#define KEY_C 0x06 // Keyboard c and C
#define KEY_D 0x07 // Keyboard d and D
#define KEY_E 0x08 // Keyboard e and E
#define KEY_F 0x09 // Keyboard f and F
#define KEY_G 0x0a // Keyboard g and G
#define KEY_H 0x0b // Keyboard h and H
#define KEY_I 0x0c // Keyboard i and I
#define KEY_J 0x0d // Keyboard j and J
#define KEY_K 0x0e // Keyboard k and K
#define KEY_L 0x0f // Keyboard l and L
#define KEY_M 0x10 // Keyboard m and M
#define KEY_N 0x11 // Keyboard n and N
#define KEY_O 0x12 // Keyboard o and O
#define KEY_P 0x13 // Keyboard p and P
#define KEY_Q 0x14 // Keyboard q and Q
#define KEY_R 0x15 // Keyboard r and R
#define KEY_S 0x16 // Keyboard s and S
#define KEY_T 0x17 // Keyboard t and T
#define KEY_U 0x18 // Keyboard u and U
#define KEY_V 0x19 // Keyboard v and V
#define KEY_W 0x1a // Keyboard w and W
#define KEY_X 0x1b // Keyboard x and X
#define KEY_Y 0x1c // Keyboard y and Y
#define KEY_Z 0x1d // Keyboard z and Z

#define KEY_1 0x1e // Keyboard 1 and !
#define KEY_2 0x1f // Keyboard 2 and @
#define KEY_3 0x20 // Keyboard 3 and #
#define KEY_4 0x21 // Keyboard 4 and $
#define KEY_5 0x22 // Keyboard 5 and %
#define KEY_6 0x23 // Keyboard 6 and ^
#define KEY_7 0x24 // Keyboard 7 and &
#define KEY_8 0x25 // Keyboard 8 and *
#define KEY_9 0x26 // Keyboard 9 and (
#define KEY_0 0x27 // Keyboard 0 and )

#define KEY_ENTER 0x28 // Keyboard Return (ENTER)
#define KEY_ESC 0x29 // Keyboard ESCAPE
#define KEY_BACKSPACE 0x2a // Keyboard DELETE (Backspace)
#define KEY_TAB 0x2b // Keyboard Tab
#define KEY_SPACE 0x2c // Keyboard Spacebar
#define KEY_MINUS 0x2d // Keyboard - and _
#define KEY_EQUAL 0x2e // Keyboard = and +
#define KEY_LEFTBRACE 0x2f // Keyboard [ and {
#define KEY_RIGHTBRACE 0x30 // Keyboard ] and }
#define KEY_BACKSLASH 0x31 // Keyboard \ and |
#define KEY_HASHTILDE 0x32 // Keyboard Non-US # and ~
#define KEY_SEMICOLON 0x33 // Keyboard ; and :
#define KEY_APOSTROPHE 0x34 // Keyboard ' and "
#define KEY_GRAVE 0x35 // Keyboard ` and ~
#define KEY_COMMA 0x36 // Keyboard , and <
#define KEY_DOT 0x37 // Keyboard . and >
#define KEY_SLASH 0x38 // Keyboard / and ?
#define KEY_CAPSLOCK 0x39 // Keyboard Caps Lock

#define KEY_F1 0x3a // Keyboard F1
#define KEY_F2 0x3b // Keyboard F2
#define KEY_F3 0x3c // Keyboard F3
#define KEY_F4 0x3d // Keyboard F4
#define KEY_F5 0x3e // Keyboard F5
#define KEY_F6 0x3f // Keyboard F6
#define KEY_F7 0x40 // Keyboard F7
#define KEY_F8 0x41 // Keyboard F8
#define KEY_F9 0x42 // Keyboard F9
#define KEY_F10 0x43 // Keyboard F10
#define KEY_F11 0x44 // Keyboard F11
#define KEY_F12 0x45 // Keyboard F12

#define KEY_SYSRQ 0x46 // Keyboard Print Screen
#define KEY_SCROLLLOCK 0x47 // Keyboard Scroll Lock
#define KEY_PAUSE 0x48 // Keyboard Pause
#define KEY_INSERT 0x49 // Keyboard Insert
#define KEY_HOME 0x4a // Keyboard Home
#define KEY_PAGEUP 0x4b // Keyboard Page Up
#define KEY_DELETE 0x4c // Keyboard Delete Forward
#define KEY_END 0x4d // Keyboard End
#define KEY_PAGEDOWN 0x4e // Keyboard Page Down
#define KEY_RIGHT 0x4f // Keyboard Right Arrow
#define KEY_LEFT 0x50 // Keyboard Left Arrow
#define KEY_DOWN 0x51 // Keyboard Down Arrow
#define KEY_UP 0x52 // Keyboard Up Arrow

#define KEY_NUMLOCK 0x53 // Keyboard Num Lock and Clear
#define KEY_KPSLASH 0x54 // Keypad /
#define KEY_KPASTERISK 0x55 // Keypad *
#define KEY_KPMINUS 0x56 // Keypad -
#define KEY_KPPLUS 0x57 // Keypad +
#define KEY_KPENTER 0x58 // Keypad ENTER
#define KEY_KP1 0x59 // Keypad 1 and End
#define KEY_KP2 0x5a // Keypad 2 and Down Arrow
#define KEY_KP3 0x5b // Keypad 3 and PageDn
#define KEY_KP4 0x5c // Keypad 4 and Left Arrow
#define KEY_KP5 0x5d // Keypad 5
#define KEY_KP6 0x5e // Keypad 6 and Right Arrow
#define KEY_KP7 0x5f // Keypad 7 and Home
#define KEY_KP8 0x60 // Keypad 8 and Up Arrow
#define KEY_KP9 0x61 // Keypad 9 and Page Up
#define KEY_KP0 0x62 // Keypad 0 and Insert
#define KEY_KPDOT 0x63 // Keypad . and Delete

#define KEY_102ND 0x64 // Keyboard Non-US \ and |
#define KEY_COMPOSE 0x65 // Keyboard Application
#define KEY_POWER 0x66 // Keyboard Power
#define KEY_KPEQUAL 0x67 // Keypad =

#define KEY_F13 0x68 // Keyboard F13
#define KEY_F14 0x69 // Keyboard F14
#define KEY_F15 0x6a // Keyboard F15
#define KEY_F16 0x6b // Keyboard F16
#define KEY_F17 0x6c // Keyboard F17
#define KEY_F18 0x6d // Keyboard F18
#define KEY_F19 0x6e // Keyboard F19
#define KEY_F20 0x6f // Keyboard F20
#define KEY_F21 0x70 // Keyboard F21
#define KEY_F22 0x71 // Keyboard F22
#define KEY_F23 0x72 // Keyboard F23
#define KEY_F24 0x73 // Keyboard F24

#define KEY_OPEN 0x74 // Keyboard Execute
#define KEY_HELP 0x75 // Keyboard Help
#define KEY_PROPS 0x76 // Keyboard Menu
#define KEY_FRONT 0x77 // Keyboard Select
#define KEY_STOP 0x78 // Keyboard Stop
#define KEY_AGAIN 0x79 // Keyboard Again
#define KEY_UNDO 0x7a // Keyboard Undo
#define KEY_CUT 0x7b // Keyboard Cut
#define KEY_COPY 0x7c // Keyboard Copy
#define KEY_PASTE 0x7d // Keyboard Paste
#define KEY_FIND 0x7e // Keyboard Find
#define KEY_MUTE 0x7f // Keyboard Mute
#define KEY_VOLUMEUP 0x80 // Keyboard Volume Up
#define KEY_VOLUMEDOWN 0x81 // Keyboard Volume Down

// Indexed by the NeXT scancode, values are USB HID scancodes.
static const uint8_t keymap[0x80] =
  {
   KEY_NONE,       // 00
   KEY_F5,         // 01
   KEY_NONE,       // 02
   KEY_BACKSLASH,  // 03
   KEY_RIGHTBRACE, // 04
   KEY_LEFTBRACE,  // 05
   KEY_I,          // 06
   KEY_O,          // 07
   KEY_P,          // 08
   KEY_LEFT,       // 09
   KEY_NONE,       // 0A
   KEY_KP0,        // 0B
   KEY_KPDOT,      // 0C
   KEY_KPENTER,    // 0D
   KEY_NONE,       // 0E
   KEY_DOWN,       // 0F
   KEY_RIGHT,      // 10
   KEY_KP1,        // 11
   KEY_KP4,        // 12
   KEY_KP6,        // 13
   KEY_KP3,        // 14
   KEY_KPPLUS,     // 15
   KEY_UP,         // 16
   KEY_KP2,        // 17
   KEY_KP5,        // 18
   KEY_F6,         // 19
   KEY_VOLUMEUP,   // 1A
   KEY_BACKSPACE,  // 1B
   KEY_EQUAL,      // 1C
   KEY_MINUS,      // 1D
   KEY_8,          // 1E
   KEY_9,          // 1F
   KEY_0,          // 20
   KEY_KP7,        // 21
   KEY_KP8,        // 22
   KEY_KP9,        // 23
   KEY_KPMINUS,    // 24
   KEY_KPASTERISK, // 25
   KEY_GRAVE,      // 26
   KEY_KPEQUAL,    // 27
   KEY_KPSLASH,    // 28
   KEY_NONE,       // 29
   KEY_ENTER,      // 2A
   KEY_APOSTROPHE, // 2B
   KEY_SEMICOLON,  // 2C
   KEY_L,          // 2D
   KEY_COMMA,      // 2E
   KEY_DOT,        // 2F
   KEY_SLASH,      // 30
   KEY_Z,          // 31
   KEY_X,          // 32
   KEY_C,          // 33
   KEY_V,          // 34
   KEY_B,          // 35
   KEY_M,          // 36
   KEY_N,          // 37
   KEY_SPACE,      // 38
   KEY_A,          // 39
   KEY_S,          // 3A
   KEY_D,          // 3B
   KEY_F,          // 3C
   KEY_G,          // 3D
   KEY_K,          // 3E
   KEY_J,          // 3F
   KEY_H,          // 40
   KEY_TAB,        // 41
   KEY_Q,          // 42
   KEY_W,          // 43
   KEY_E,          // 44
   KEY_R,          // 45
   KEY_U,          // 46
   KEY_Y,          // 47
   KEY_T,          // 48
   KEY_ESC,        // 49
   KEY_1,          // 4A
   KEY_2,          // 4B
   KEY_3,          // 4C
   KEY_4,          // 4D
   KEY_7,          // 4E
   KEY_6,          // 4F
   KEY_5,          // 50
   KEY_NONE,       // 51
   KEY_NONE,       // 52
   KEY_NONE,       // 53
   KEY_NONE,       // 54
   KEY_NONE,       // 55
   KEY_NONE,       // 56
   KEY_NONE,       // 57
   KEY_NONE,       // 58
   KEY_NONE,       // 59
   KEY_NONE,       // 5A
   KEY_NONE,       // 5B
   KEY_NONE,       // 5C
   KEY_NONE,       // 5D
   KEY_NONE,       // 5E
   KEY_NONE,       // 5F
   KEY_NONE,       // 60
   KEY_NONE,       // 61
   KEY_NONE,       // 62
   KEY_NONE,       // 63
   KEY_NONE,       // 64
   KEY_NONE,       // 65
   KEY_NONE,       // 66
   KEY_NONE,       // 67
   KEY_NONE,       // 68
   KEY_NONE,       // 69
   KEY_NONE,       // 6A
   KEY_NONE,       // 6B
   KEY_NONE,       // 6C
   KEY_NONE,       // 6D
   KEY_NONE,       // 6E
   KEY_NONE,       // 6F
   KEY_NONE,       // 70
   KEY_NONE,       // 71
   KEY_NONE,       // 72
   KEY_NONE,       // 73
   KEY_NONE,       // 74
   KEY_NONE,       // 75
   KEY_NONE,       // 76
   KEY_NONE,       // 77
   KEY_NONE,       // 78
   KEY_NONE,       // 79
   KEY_NONE,       // 7A
   KEY_NONE,       // 7B
   KEY_NONE,       // 7C
   KEY_NONE,       // 7D
   KEY_NONE,       // 7E
   KEY_NONE,       // 7F
  };
