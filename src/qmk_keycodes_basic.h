/*
Copyright 2011,2012 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * Keycodes based on HID Keyboard/Keypad Usage Page (0x07) plus media keys from Generic Desktop Page (0x01) and Consumer Page (0x0C)
 *
 * See https://web.archive.org/web/20060218214400/http://www.usb.org/developers/devclass_docs/Hut1_12.pdf
 * or http://www.usb.org/developers/hidpage/Hut1_12v2.pdf (older)
 */
#ifndef KEYCODE_H
#define KEYCODE_H

/* FIXME: Add doxygen comments here */

#define IS_ERROR(code)           (KC_ROLL_OVER <= (code) && (code) <= KC_UNDEFINED)
#define IS_ANY(code)             (KC_A         <= (code) && (code) <= 0xFF)
#define IS_KEY(code)             (KC_A         <= (code) && (code) <= KC_EXSEL)
#define IS_MOD(code)             (KC_LCTRL     <= (code) && (code) <= KC_RGUI)

#define IS_SPECIAL(code)         ((0xA5 <= (code) && (code) <= 0xDF) || (0xE8 <= (code) && (code) <= 0xFF))
#define IS_SYSTEM(code)          (KC_PWR       <= (code) && (code) <= KC_WAKE)
#define IS_CONSUMER(code)        (KC_MUTE      <= (code) && (code) <= KC_BRID)

#define IS_FN(code)              (KC_FN0       <= (code) && (code) <= KC_FN31)

#define IS_MOUSEKEY(code)        (KC_MS_UP     <= (code) && (code) <= KC_MS_ACCEL2)
#define IS_MOUSEKEY_MOVE(code)   (KC_MS_UP     <= (code) && (code) <= KC_MS_RIGHT)
#define IS_MOUSEKEY_BUTTON(code) (KC_MS_BTN1   <= (code) && (code) <= KC_MS_BTN5)
#define IS_MOUSEKEY_WHEEL(code)  (KC_MS_WH_UP  <= (code) && (code) <= KC_MS_WH_RIGHT)
#define IS_MOUSEKEY_ACCEL(code)  (KC_MS_ACCEL0 <= (code) && (code) <= KC_MS_ACCEL2)

#define MOD_BIT(code)            (1 << MOD_INDEX(code))
#define MOD_INDEX(code)          ((code) & 0x07)

#define MOD_MASK_CTRL            (MOD_BIT(KC_LCTRL)  | MOD_BIT(KC_RCTRL))
#define MOD_MASK_SHIFT           (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_RSHIFT))
#define MOD_MASK_ALT             (MOD_BIT(KC_LALT)   | MOD_BIT(KC_RALT))
#define MOD_MASK_GUI             (MOD_BIT(KC_LGUI)   | MOD_BIT(KC_RGUI))
#define MOD_MASK_CS              (MOD_MASK_CTRL  | MOD_MASK_SHIFT)
#define MOD_MASK_CA              (MOD_MASK_CTRL  | MOD_MASK_ALT)
#define MOD_MASK_CG              (MOD_MASK_CTRL  | MOD_MASK_GUI)
#define MOD_MASK_SA              (MOD_MASK_SHIFT | MOD_MASK_ALT)
#define MOD_MASK_SG              (MOD_MASK_SHIFT | MOD_MASK_GUI)
#define MOD_MASK_AG              (MOD_MASK_ALT   | MOD_MASK_GUI)
#define MOD_MASK_CSA             (MOD_MASK_CTRL  | MOD_MASK_SHIFT | MOD_MASK_ALT)
#define MOD_MASK_CSG             (MOD_MASK_CTRL  | MOD_MASK_SHIFT | MOD_MASK_GUI)
#define MOD_MASK_CAG             (MOD_MASK_CTRL  | MOD_MASK_ALT   | MOD_MASK_GUI)
#define MOD_MASK_SAG             (MOD_MASK_SHIFT | MOD_MASK_ALT   | MOD_MASK_GUI)
#define MOD_MASK_CSAG            (MOD_MASK_CTRL  | MOD_MASK_SHIFT | MOD_MASK_ALT | MOD_MASK_GUI)

#define FN_BIT(code)             (1 << FN_INDEX(code))
#define FN_INDEX(code)           ((code) - KC_FN0)
#define FN_MIN                   KC_FN0
#define FN_MAX                   KC_FN31

/*
 * Short names for ease of definition of keymap
 */
/* Transparent */
#define KC_TRANSPARENT 0x01
#define KC_TRNS        KC_TRANSPARENT

/* Punctuation */
#define KC_ENT  KC_ENTER
#define KC_ESC  KC_ESCAPE
#define KC_BSPC KC_BSPACE
#define KC_SPC  KC_SPACE
#define KC_MINS KC_MINUS
#define KC_EQL  KC_EQUAL
#define KC_LBRC KC_LBRACKET
#define KC_RBRC KC_RBRACKET
#define KC_BSLS KC_BSLASH
#define KC_NUHS KC_NONUS_HASH
#define KC_SCLN KC_SCOLON
#define KC_QUOT KC_QUOTE
#define KC_GRV  KC_GRAVE
#define KC_COMM KC_COMMA
#define KC_SLSH KC_SLASH
#define KC_NUBS KC_NONUS_BSLASH

/* Lock Keys */
#define KC_CLCK KC_CAPSLOCK
#define KC_CAPS KC_CAPSLOCK
#define KC_SLCK KC_SCROLLLOCK
#define KC_NLCK KC_NUMLOCK
#define KC_LCAP KC_LOCKING_CAPS
#define KC_LNUM KC_LOCKING_NUM
#define KC_LSCR KC_LOCKING_SCROLL

/* Commands */
#define KC_PSCR KC_PSCREEN
#define KC_PAUS KC_PAUSE
#define KC_BRK  KC_PAUSE
#define KC_INS  KC_INSERT
#define KC_DEL  KC_DELETE
#define KC_PGDN KC_PGDOWN
#define KC_RGHT KC_RIGHT
#define KC_APP  KC_APPLICATION
#define KC_EXEC KC_EXECUTE
#define KC_SLCT KC_SELECT
#define KC_AGIN KC_AGAIN
#define KC_PSTE KC_PASTE
#define KC_ERAS KC_ALT_ERASE
#define KC_CLR  KC_CLEAR

/* Keypad */
#define KC_PSLS KC_KP_SLASH
#define KC_PAST KC_KP_ASTERISK
#define KC_PMNS KC_KP_MINUS
#define KC_PPLS KC_KP_PLUS
#define KC_PENT KC_KP_ENTER
#define KC_P1   KC_KP_1
#define KC_P2   KC_KP_2
#define KC_P3   KC_KP_3
#define KC_P4   KC_KP_4
#define KC_P5   KC_KP_5
#define KC_P6   KC_KP_6
#define KC_P7   KC_KP_7
#define KC_P8   KC_KP_8
#define KC_P9   KC_KP_9
#define KC_P0   KC_KP_0
#define KC_PDOT KC_KP_DOT
#define KC_PEQL KC_KP_EQUAL
#define KC_PCMM KC_KP_COMMA

/* Japanese specific */
#define KC_ZKHK KC_GRAVE
#define KC_RO   KC_INT1
#define KC_KANA KC_INT2
#define KC_JYEN KC_INT3
#define KC_HENK KC_INT4
#define KC_MHEN KC_INT5

/* Korean specific */
#define KC_HAEN KC_LANG1
#define KC_HANJ KC_LANG2

/* Modifiers */
#define KC_LCTL KC_LCTRL
#define KC_LSFT KC_LSHIFT
#define KC_LCMD KC_LGUI
#define KC_LWIN KC_LGUI
#define KC_RCTL KC_RCTRL
#define KC_RSFT KC_RSHIFT
#define KC_ALGR KC_RALT
#define KC_RCMD KC_RGUI
#define KC_RWIN KC_RGUI

/* Generic Desktop Page (0x01) */
#define KC_PWR  KC_SYSTEM_POWER
#define KC_SLEP KC_SYSTEM_SLEEP
#define KC_WAKE KC_SYSTEM_WAKE

/* Consumer Page (0x0C) */
#define KC_MUTE KC_AUDIO_MUTE
#define KC_VOLU KC_AUDIO_VOL_UP
#define KC_VOLD KC_AUDIO_VOL_DOWN
#define KC_MNXT KC_MEDIA_NEXT_TRACK
#define KC_MPRV KC_MEDIA_PREV_TRACK
#define KC_MSTP KC_MEDIA_STOP
#define KC_MPLY KC_MEDIA_PLAY_PAUSE
#define KC_MSEL KC_MEDIA_SELECT
#define KC_EJCT KC_MEDIA_EJECT
#define KC_CALC KC_CALCULATOR
#define KC_MYCM KC_MY_COMPUTER
#define KC_WSCH KC_WWW_SEARCH
#define KC_WHOM KC_WWW_HOME
#define KC_WBAK KC_WWW_BACK
#define KC_WFWD KC_WWW_FORWARD
#define KC_WSTP KC_WWW_STOP
#define KC_WREF KC_WWW_REFRESH
#define KC_WFAV KC_WWW_FAVORITES
#define KC_MFFD KC_MEDIA_FAST_FORWARD
#define KC_MRWD KC_MEDIA_REWIND
#define KC_BRIU KC_BRIGHTNESS_UP
#define KC_BRID KC_BRIGHTNESS_DOWN

/* System Specific */
#define KC_BRMU KC_PAUSE
#define KC_BRMD KC_SCROLLLOCK

/* Mouse Keys */
#define KC_MS_U KC_MS_UP
#define KC_MS_D KC_MS_DOWN
#define KC_MS_L KC_MS_LEFT
#define KC_MS_R KC_MS_RIGHT
#define KC_BTN1 KC_MS_BTN1
#define KC_BTN2 KC_MS_BTN2
#define KC_BTN3 KC_MS_BTN3
#define KC_BTN4 KC_MS_BTN4
#define KC_BTN5 KC_MS_BTN5
#define KC_WH_U KC_MS_WH_UP
#define KC_WH_D KC_MS_WH_DOWN
#define KC_WH_L KC_MS_WH_LEFT
#define KC_WH_R KC_MS_WH_RIGHT
#define KC_ACL0 KC_MS_ACCEL0
#define KC_ACL1 KC_MS_ACCEL1
#define KC_ACL2 KC_MS_ACCEL2

/* Keyboard/Keypad Page (0x07) */
// hid_keyboard_keypad_usage
#define KC_NO  0
#define KC_ROLL_OVER  1
#define KC_POST_FAIL  2
#define KC_UNDEFINED  3
#define KC_A  4
#define KC_B  5
#define KC_C  6
#define KC_D  7
#define KC_E  8
#define KC_F  9
#define KC_G  10
#define KC_H  11
#define KC_I  12
#define KC_J  13
#define KC_K  14
#define KC_L  15
#define KC_M  16                   //0x10
#define KC_N  17
#define KC_O  18
#define KC_P  19
#define KC_Q  20
#define KC_R  21
#define KC_S  22
#define KC_T  23
#define KC_U  24
#define KC_V  25
#define KC_W  26
#define KC_X  27
#define KC_Y  28
#define KC_Z  29
#define KC_1  30
#define KC_2  31
#define KC_3  32                   //0x20
#define KC_4  33
#define KC_5  34
#define KC_6  35
#define KC_7  36
#define KC_8  37
#define KC_9  38
#define KC_0  39
#define KC_ENTER  40
#define KC_ESCAPE 41
#define KC_BSPACE 42
#define KC_TAB  43
#define KC_SPACE  44
#define KC_MINUS  45
#define KC_EQUAL  46
#define KC_LBRACKET 47
#define KC_RBRACKET 48            //0x30
#define KC_BSLASH 49
#define KC_NONUS_HASH 50
#define KC_SCOLON 51
#define KC_QUOTE  52
#define KC_GRAVE  53
#define KC_COMMA  54
#define KC_DOT  55
#define KC_SLASH  56
#define KC_CAPSLOCK 57
#define KC_F1 58
#define KC_F2 59
#define KC_F3 60
#define KC_F4 61
#define KC_F5 62
#define KC_F6 63
#define KC_F7 64                  //0x40
#define KC_F8 65
#define KC_F9 66
#define KC_F10  67
#define KC_F11  68
#define KC_F12  69
#define KC_PSCREEN  70
#define KC_SCROLLLOCK 71
#define KC_PAUSE  72
#define KC_INSERT 73
#define KC_HOME 74
#define KC_PGUP 75
#define KC_DELETE 76
#define KC_END  77
#define KC_PGDOWN 78
#define KC_RIGHT  79
#define KC_LEFT 80                //0x50
#define KC_DOWN 81
#define KC_UP 82
#define KC_NUMLOCK  83
#define KC_KP_SLASH 84
#define KC_KP_ASTERISK  85
#define KC_KP_MINUS 86
#define KC_KP_PLUS  87
#define KC_KP_ENTER 88
#define KC_KP_1 89
#define KC_KP_2 90
#define KC_KP_3 91
#define KC_KP_4 92
#define KC_KP_5 93
#define KC_KP_6 94
#define KC_KP_7 95
#define KC_KP_8 96                //0x60
#define KC_KP_9 97
#define KC_KP_0 98
#define KC_KP_DOT 99
#define KC_NONUS_BSLASH 100
#define KC_APPLICATION  101
#define KC_POWER  102
#define KC_KP_EQUAL 103
#define KC_F13  104
#define KC_F14  105
#define KC_F15  106
#define KC_F16  107
#define KC_F17  108
#define KC_F18  109
#define KC_F19  110
#define KC_F20  111
#define KC_F21  112                 //0x70
#define KC_F22  113
#define KC_F23  114
#define KC_F24  115
#define KC_EXECUTE  116
#define KC_HELP 117
#define KC_MENU 118
#define KC_SELECT 119
#define KC_STOP 120
#define KC_AGAIN  121
#define KC_UNDO 122
#define KC_CUT  123
#define KC_COPY 124
#define KC_PASTE  125
#define KC_FIND 126
#define KC__MUTE  127
#define KC__VOLUP 128              //0x80
#define KC__VOLDOWN 129
#define KC_LOCKING_CAPS 130
#define KC_LOCKING_NUM  131
#define KC_LOCKING_SCROLL 132
#define KC_KP_COMMA 133
#define KC_KP_EQUAL_AS400 134
#define KC_INT1 135
#define KC_INT2 136
#define KC_INT3 137
#define KC_INT4 138
#define KC_INT5 139
#define KC_INT6 140
#define KC_INT7 141
#define KC_INT8 142
#define KC_INT9 143
#define KC_LANG1  144               //0x90
#define KC_LANG2  145
#define KC_LANG3  146
#define KC_LANG4  147
#define KC_LANG5  148
#define KC_LANG6  149
#define KC_LANG7  150
#define KC_LANG8  151
#define KC_LANG9  152
#define KC_ALT_ERASE  153
#define KC_SYSREQ 154
#define KC_CANCEL 155
#define KC_CLEAR  156
#define KC_PRIOR  157
#define KC_RETURN 158
#define KC_SEPARATOR  159
#define KC_OUT  160                 //0xA0
#define KC_OPER 161
#define KC_CLEAR_AGAIN  162
#define KC_CRSEL  163
#define KC_EXSEL  164

#if 0
  // ***************************************************************
  // These keycodes are present in the HID spec, but are           *
  // nonfunctional on modern OSes. QMK uses this range (0xA5-0xDF) *
  // for the media and function keys instead - see below.          *
  // ***************************************************************

  KC_KP_00                = 0xB0,
  KC_KP_000,
  KC_THOUSANDS_SEPARATOR,
  KC_DECIMAL_SEPARATOR,
  KC_CURRENCY_UNIT,
  KC_CURRENCY_SUB_UNIT,
  KC_KP_LPAREN,
  KC_KP_RPAREN,
  KC_KP_LCBRACKET,
  KC_KP_RCBRACKET,
  KC_KP_TAB,
  KC_KP_BSPACE,
  KC_KP_A,
  KC_KP_B,
  KC_KP_C,
  KC_KP_D,
  KC_KP_E,                //0xC0
  KC_KP_F,
  KC_KP_XOR,
  KC_KP_HAT,
  KC_KP_PERC,
  KC_KP_LT,
  KC_KP_GT,
  KC_KP_AND,
  KC_KP_LAZYAND,
  KC_KP_OR,
  KC_KP_LAZYOR,
  KC_KP_COLON,
  KC_KP_HASH,
  KC_KP_SPACE,
  KC_KP_ATMARK,
  KC_KP_EXCLAMATION,
  KC_KP_MEM_STORE,        //0xD0
  KC_KP_MEM_RECALL,
  KC_KP_MEM_CLEAR,
  KC_KP_MEM_ADD,
  KC_KP_MEM_SUB,
  KC_KP_MEM_MUL,
  KC_KP_MEM_DIV,
  KC_KP_PLUS_MINUS,
  KC_KP_CLEAR,
  KC_KP_CLEAR_ENTRY,
  KC_KP_BINARY,
  KC_KP_OCTAL,
  KC_KP_DECIMAL,
  KC_KP_HEXADECIMAL,
#endif

  /* Modifiers */
#define KC_LCTRL  0xE0
#define KC_LSHIFT 0xE1
#define KC_LALT   0xE2
#define KC_LGUI   0xE3
#define KC_RCTRL  0xE4
#define KC_RSHIFT 0xE5
#define KC_RALT   0xE6
#define KC_RGUI   0xE7

  // **********************************************
  // * 0xF0-0xFF are unallocated in the HID spec. *
  // * QMK uses these for Mouse Keys - see below. *
  // **********************************************

/* Media and Function keys */
// internal_special_keycodes
  /* Generic Desktop Page (0x01) */
  #define KC_SYSTEM_POWER     0xA5
  #define KC_SYSTEM_SLEEP     0xA6
  #define KC_SYSTEM_WAKE      0xA7

  /* Consumer Page (0x0C) */
  #define KC_AUDIO_MUTE       0xA8
  #define KC_AUDIO_VOL_UP     0xA9
  #define KC_AUDIO_VOL_DOWN   0xAA
  #define KC_MEDIA_NEXT_TRACK 0xAB
  #define KC_MEDIA_PREV_TRACK 0xAC
  #define KC_MEDIA_STOP       0xAD
  #define KC_MEDIA_PLAY_PAUSE 0xAE
  #define KC_MEDIA_SELECT     0xAF
  #define KC_MEDIA_EJECT      0xB0
  #define KC_MAIL             0xB1
  #define KC_CALCULATOR       0xB2
  #define KC_MY_COMPUTER      0xB3
  #define KC_WWW_SEARCH       0xB4
  #define KC_WWW_HOME         0xB5
  #define KC_WWW_BACK         0xB6
  #define KC_WWW_FORWARD      0xB7
  #define KC_WWW_STOP         0xB8
  #define KC_WWW_REFRESH      0xB9
  #define KC_WWW_FAVORITES    0xBA
  #define KC_MEDIA_FAST_FORWARD 0xBB
  #define KC_MEDIA_REWIND     0xBC
  #define KC_BRIGHTNESS_UP    0xBD
  #define KC_BRIGHTNESS_DOWN  0xBE

  /* Fn keys */
  #define KC_FN0   0xC0
  #define KC_FN1   0xC1
  #define KC_FN2   0xC2
  #define KC_FN3   0xC3
  #define KC_FN4   0xC4
  #define KC_FN5   0xC5
  #define KC_FN6   0xC6
  #define KC_FN7   0xC7
  #define KC_FN8   0xC8
  #define KC_FN9   0xC9
  #define KC_FN10  0xCA
  #define KC_FN11  0xCB
  #define KC_FN12  0xCC
  #define KC_FN13  0xCD
  #define KC_FN14  0xCE
  #define KC_FN15  0xCF
  #define KC_FN16  0xD0
  #define KC_FN17  0xD1
  #define KC_FN18  0xD2
  #define KC_FN19  0xD3
  #define KC_FN20  0xD4
  #define KC_FN21  0xD5
  #define KC_FN22  0xD6
  #define KC_FN23  0xD7
  #define KC_FN24  0xD8
  #define KC_FN25  0xD9
  #define KC_FN26  0xDA
  #define KC_FN27  0xDB
  #define KC_FN28  0xDC
  #define KC_FN29  0xDD
  #define KC_FN30  0xDE
  #define KC_FN31  0xDF

// mouse_keys
  /* Mouse Buttons */
  #define KC_MS_UP    0xF0
  #define KC_MS_DOWN  0xF1
  #define KC_MS_LEFT  0xF2
  #define KC_MS_RIGHT 0xF3
  #define KC_MS_BTN1  0xF4
  #define KC_MS_BTN2  0xF5
  #define KC_MS_BTN3  0xF6
  #define KC_MS_BTN4  0xF7
  #define KC_MS_BTN5  0xF8

  /* Mouse Wheel */
  #define KC_MS_WH_UP 0xF9
  #define KC_MS_WH_DOWN 0xFA
  #define KC_MS_WH_LEFT 0xFB
  #define KC_MS_WH_RIGHT 0xFC

  /* Acceleration */
  #define KC_MS_ACCEL0 0xFD
  #define KC_MS_ACCEL1 0xFE
  #define KC_MS_ACCEL2 0xFF

#endif
