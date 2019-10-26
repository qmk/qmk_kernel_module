#ifndef _QMK_SCANCODES_H
#define _QMK_SCANCODES_H

#include <uapi/linux/input-event-codes.h>

static unsigned int keycode_to_scancode[256] = {
  0, //KC_NO
  0, //KC_ROLL_OVER
  0, //KC_POST_FAIL
  0, //KC_UNDEFINED
  KEY_A,
  KEY_B,
  KEY_C,
  KEY_D,
  KEY_E,
  KEY_F,
  KEY_G,
  KEY_H,
  KEY_I,
  KEY_J,
  KEY_K,
  KEY_L,
  KEY_M,                   //0x10
  KEY_N,
  KEY_O,
  KEY_P,
  KEY_Q,
  KEY_R,
  KEY_S,
  KEY_T,
  KEY_U,
  KEY_V,
  KEY_W,
  KEY_X,
  KEY_Y,
  KEY_Z,
  KEY_1,
  KEY_2,
  KEY_3,                   //0x20
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,
  KEY_0,
  KEY_ENTER,
  KEY_ESC,
  KEY_BACKSPACE,
  KEY_TAB,
  KEY_SPACE,
  KEY_MINUS,
  KEY_EQUAL,
  KEY_LEFTBRACE,
  KEY_RIGHTBRACE,            //0x30
  KEY_BACKSLASH,
  0, //KC_NONUS_HASH
  KEY_SEMICOLON,
  KEY_APOSTROPHE,
  KEY_GRAVE,
  KEY_COMMA,
  KEY_DOT,
  KEY_SLASH,
  KEY_CAPSLOCK,
  KEY_F1,
  KEY_F2,
  KEY_F3,
  KEY_F4,
  KEY_F5,
  KEY_F6,
  KEY_F7,                  //0x40
  KEY_F8,
  KEY_F9,
  KEY_F10,
  KEY_F11,
  KEY_F12,
  0, //KC_PSCREEN
  KEY_SCROLLLOCK,
  KEY_PAUSE,
  KEY_INSERT,
  KEY_HOME,
  KEY_PAGEUP,
  KEY_DELETE,
  KEY_END,
  KEY_PAGEDOWN,
  KEY_RIGHT,
  KEY_LEFT,                //0x50
  KEY_DOWN,
  KEY_UP,
  KEY_NUMLOCK,
  KEY_KPSLASH,
  KEY_KPASTERISK,
  KEY_KPMINUS,
  KEY_KPPLUS,
  KEY_KPENTER,
  KEY_KP1,
  KEY_KP2,
  KEY_KP3,
  KEY_KP4,
  KEY_KP5,
  KEY_KP6,
  KEY_KP7,
  KEY_KP8,                //0x60
  KEY_KP9,
  KEY_KP0,
  KEY_KPDOT,
  0, //KC_NONUS_BSLASH
  0, //KC_APPLICATION
  KEY_POWER,
  KEY_KPEQUAL,
  KEY_F13,
  KEY_F14,
  KEY_F15,
  KEY_F16,
  KEY_F17,
  KEY_F18,
  KEY_F19,
  KEY_F20,
  KEY_F21,                 //0x70
  KEY_F22,
  KEY_F23,
  KEY_F24,
  0, //KC_EXECUTE
  KEY_HELP,
  KEY_MENU,
  KEY_SELECT,
  KEY_STOP,
  KEY_AGAIN,
  KEY_UNDO,
  KEY_CUT,
  KEY_COPY,
  KEY_PASTE,
  KEY_FIND,
  KEY_MUTE,
  KEY_VOLUMEUP,              //0x80
  KEY_VOLUMEDOWN,
  0, //KC_LOCKING_CAPS
  0, //KC_LOCKING_NUM
  0, //KC_LOCKING_SCROLL
  KEY_KPCOMMA,
  0, //KC_KP_EQUAL_AS400
  0, //KC_INT1
  0, //KC_INT2
  0, //KC_INT3
  0, //KC_INT4
  0, //KC_INT5
  0, //KC_INT6
  0, //KC_INT7
  0, //KC_INT8
  0, //KC_INT9
  0, //KC_LANG1              // 0x90
  0, //KC_LANG2
  0, //KC_LANG3
  0, //KC_LANG4
  0, //KC_LANG5
  0, //KC_LANG6
  0, //KC_LANG7
  0, //KC_LANG8
  0, //KC_LANG9
  KEY_ALTERASE,
  KEY_SYSRQ,
  KEY_CANCEL,
  KEY_CLEAR,
  0, //KC_PRIOR
  0, //KC_RETURN
  0, //KC_SEPARATOR
  0, //KC_OUT                // 0xA0
  0, //KC_OPER
  0, //KC_CLEAR_AGAIN
  0, //KC_CRSEL
  0, //KC_EXSEL  

  /* Generic Desktop Page (0x01) */
  KEY_POWER,         // 0xA5
  KEY_SLEEP,
  KEY_WAKEUP,

  /* Consumer Page (0x0C) */
  KEY_MUTE, // 0xA8
  KEY_VOLUMEUP,
  KEY_VOLUMEDOWN,
  KEY_NEXTSONG,
  KEY_PREVIOUSSONG,
  KEY_STOPCD,
  KEY_PLAYPAUSE,
  KEY_MEDIA,
  KEY_EJECTCD,         // 0xB0
  KEY_EMAIL,
  KEY_CALENDAR,
  KEY_COMPUTER,
  KEY_SEARCH,
  KEY_HOMEPAGE,
  KEY_BACK,
  KEY_FORWARD,
  KEY_EXIT,
  KEY_REFRESH,
  KEY_FAVORITES,
  KEY_FASTFORWARD,
  KEY_REWIND,
  KEY_BRIGHTNESSUP,
  KEY_BRIGHTNESSDOWN,
  0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0xC0
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0xD0

  /* Modifiers */
  KEY_LEFTCTRL,            // 0xE0
  KEY_LEFTSHIFT,
  KEY_LEFTALT,
  KEY_LEFTMETA,
  KEY_RIGHTCTRL,
  KEY_RIGHTSHIFT,
  KEY_RIGHTALT,
  KEY_RIGHTMETA

};

#endif