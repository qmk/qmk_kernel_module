#pragma once

#include <stdlib.h>
#include <usbg/usbg.h>

#define REPORT_ID_KEYBOARD 0x01
#define REPORT_ID_SYSTEM   0x03
#define REPORT_ID_CONSUMER 0x04
#define REPORT_ID_MAX      0x04

struct cfg_attr {
    char *str;
    uint16_t id;
};

struct qmk_gadget_cfg {
    struct cfg_attr vendor;
    struct cfg_attr product;
    char *serial;
};

int gadget_write_u8(uint8_t *buf);
int gadget_write_u16(uint16_t *buf);
int gadget_close(char *name);
int gadget_open(char *name, struct qmk_gadget_cfg *cfg);

// from tmk_core/common/report.h

/* Consumer Page(0x0C)
 * following are supported by Windows: http://msdn.microsoft.com/en-us/windows/hardware/gg463372.aspx
 * see also https://docs.microsoft.com/en-us/windows-hardware/drivers/hid/display-brightness-control
 */
#define AUDIO_MUTE 0x00E2
#define AUDIO_VOL_UP 0x00E9
#define AUDIO_VOL_DOWN 0x00EA
#define TRANSPORT_NEXT_TRACK 0x00B5
#define TRANSPORT_PREV_TRACK 0x00B6
#define TRANSPORT_STOP 0x00B7
#define TRANSPORT_STOP_EJECT 0x00CC
#define TRANSPORT_PLAY_PAUSE 0x00CD
#define BRIGHTNESS_UP 0x006F
#define BRIGHTNESS_DOWN 0x0070
/* application launch */
#define AL_CC_CONFIG 0x0183
#define AL_EMAIL 0x018A
#define AL_CALCULATOR 0x0192
#define AL_LOCAL_BROWSER 0x0194
/* application control */
#define AC_SEARCH 0x0221
#define AC_HOME 0x0223
#define AC_BACK 0x0224
#define AC_FORWARD 0x0225
#define AC_STOP 0x0226
#define AC_REFRESH 0x0227
#define AC_BOOKMARKS 0x022A
/* supplement for Bluegiga iWRAP HID(not supported by Windows?) */
#define AL_LOCK 0x019E
#define TRANSPORT_RECORD 0x00B2
#define TRANSPORT_FAST_FORWARD 0x00B3
#define TRANSPORT_REWIND 0x00B4
#define TRANSPORT_EJECT 0x00B8
#define AC_MINIMIZE 0x0206

/* Generic Desktop Page(0x01) - system power control */
#define SYSTEM_POWER_DOWN 0x0081
#define SYSTEM_SLEEP 0x0082
#define SYSTEM_WAKE_UP 0x0083

/* keycode to system usage */
#define KEYCODE2SYSTEM(key) (key == KC_SYSTEM_POWER ? SYSTEM_POWER_DOWN : (key == KC_SYSTEM_SLEEP ? SYSTEM_SLEEP : (key == KC_SYSTEM_WAKE ? SYSTEM_WAKE_UP : 0)))

/* keycode to consumer usage */
#define KEYCODE2CONSUMER(key) \
    (key == KC_AUDIO_MUTE ? AUDIO_MUTE : (key == KC_AUDIO_VOL_UP ? AUDIO_VOL_UP : (key == KC_AUDIO_VOL_DOWN ? AUDIO_VOL_DOWN : (key == KC_MEDIA_NEXT_TRACK ? TRANSPORT_NEXT_TRACK : (key == KC_MEDIA_PREV_TRACK ? TRANSPORT_PREV_TRACK : (key == KC_MEDIA_FAST_FORWARD ? TRANSPORT_FAST_FORWARD : (key == KC_MEDIA_REWIND ? TRANSPORT_REWIND : (key == KC_MEDIA_STOP ? TRANSPORT_STOP : (key == KC_MEDIA_EJECT ? TRANSPORT_STOP_EJECT : (key == KC_MEDIA_PLAY_PAUSE ? TRANSPORT_PLAY_PAUSE : (key == KC_MEDIA_SELECT ? AL_CC_CONFIG : (key == KC_MAIL ? AL_EMAIL : (key == KC_CALCULATOR ? AL_CALCULATOR : (key == KC_MY_COMPUTER ? AL_LOCAL_BROWSER : (key == KC_WWW_SEARCH ? AC_SEARCH : (key == KC_WWW_HOME ? AC_HOME : (key == KC_WWW_BACK ? AC_BACK : (key == KC_WWW_FORWARD ? AC_FORWARD : (key == KC_WWW_STOP ? AC_STOP : (key == KC_WWW_REFRESH ? AC_REFRESH : (key == KC_BRIGHTNESS_UP ? BRIGHTNESS_UP : (key == KC_BRIGHTNESS_DOWN ? BRIGHTNESS_DOWN : (key == KC_WWW_FAVORITES ? AC_BOOKMARKS : 0)))))))))))))))))))))))
