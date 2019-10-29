#pragma once

#include <qmk/types.h>

struct qmk_protocol {
	void *(*timer_init)(void);
	uint8_t (*timer_elapsed)(void *);
	void (*send_keycode)(struct qmk_keyboard *, hid_keycode_t, bool);
	int (*printf)(const char *, ...);
};

extern const struct qmk_protocol protocol;