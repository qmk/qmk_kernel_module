/*
 * Processes the keycodes from the keymap and sends the events
 *
 * Copyright (C) 2019 Jack Humbert <jack.humb@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include "qmk.h"
#include <qmk/keycodes/basic.h>
#include <qmk/keycodes/quantum.h>
#include "qmk_scancodes.h"

#define KEY_INDEX(layer)                                                       \
	QMK_MATRIX_SCAN_CODE(layer, row, col, keyboard->layer_shift,           \
			     keyboard->row_shift)

bool qmk_process_layer(unsigned int keycode, bool pressed,
		       unsigned long *layer_state)
{
	unsigned int layer;
	switch (keycode) {
	case QK_TO ... QK_TO_MAX:;
		// Layer set "GOTO"
		// when = (keycode >> 0x4) & 0x3;
		// action_layer = keycode & 0xF;
		// action.code = ACTION_LAYER_SET(action_layer, when);
		return true;
		break;
	case QK_MOMENTARY ... QK_MOMENTARY_MAX:;
		// Momentary action_layer
		layer = keycode & 0xFF;
		if (pressed)
			(*layer_state) |= (1UL << layer);
		else
			(*layer_state) &= ~(1UL << layer);
		return true;
		break;
	default:
		return false;
		break;
	}
}

bool qmk_process_basic(unsigned int keycode, bool pressed,
		       unsigned int *scancode)
{
	// We can easily convert this if it's in the basic keycodes
	if (keycode < 0xFF) {
		*scancode = keycode_to_scancode[keycode];
		return true;
	}
	return false;
}

void qmk_process_keycode(struct qmk *keyboard, unsigned int row,
			 unsigned int col, bool pressed)
{
	unsigned int keycode, i;
	unsigned int layer = 0;
	bool handled;
	unsigned int scancode = KEY_RESERVED;

	struct input_dev *input = keyboard->input_dev;
	const unsigned short *keymap = input->keycode;

	for (i = MATRIX_MAX_LAYERS; i > 0; i--) {
		if (((keyboard->layer_state >> i) & 0b1) &&
		    keymap[KEY_INDEX(layer)] != KC_TRNS) {
			layer = i;
			break;
		}
	}
	keycode = keymap[KEY_INDEX(layer)];

	handled = qmk_process_layer(keycode, pressed, &keyboard->layer_state) ||
		  qmk_process_basic(keycode, pressed, &scancode);

	if (!handled)
		dev_warn(&input->dev, "unhandled keycode: 0x%x", keycode);

	// using the key index for this event is what's done in matrix_keypad
	input_report_key(input, scancode, pressed);
	// input_event(input, EV_MSC, MSC_SCAN, KEY_INDEX(0));
	input_event(input, EV_MSC, MSC_SCAN, scancode);
}

MODULE_LICENSE("GPL");