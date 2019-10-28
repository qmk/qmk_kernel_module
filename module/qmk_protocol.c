#include <linux/timer.h>
#include <qmk/protocol.h>
#include <linux/input.h>
#include "qmk_scancodes.h"
#include "qmk.h"

void *timer_init(void)
{
	return NULL;
}

uint8_t timer_elapsed(void *timer)
{
	return 0;
}

void send_keycode(struct qmk_keyboard *keyboard, hid_keycode_t keycode,
		  bool pressed)
{
	uint8_t scancode;
	struct qmk_module *module = keyboard->parent;
	struct input_dev *input = module->input_dev;

	scancode = keycode_to_scancode[keycode];
	input_report_key(input, scancode, pressed);
	input_event(input, EV_MSC, MSC_SCAN, scancode);
}

struct qmk_protocol protocol = {
	.timer_init = &timer_init,
	.timer_elapsed = &timer_elapsed,
	.send_keycode = &send_keycode,
};
