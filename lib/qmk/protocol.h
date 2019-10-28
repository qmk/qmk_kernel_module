#pragma once

#include <qmk/types.h>

typedef struct qmk_protocol {
	void *(*timer_init)(void);
	uint8_t (*timer_elapsed)(void *);
	void (*send_keycode)(struct qmk_keyboard *, hid_keycode_t, bool);
} qmk_protocol;

qmk_protocol protocol;