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

#include <qmk/keycodes/basic.h>
#include <qmk/keycodes/process.h>
#include <qmk/keycodes/quantum.h>
#include <qmk/keymap.h>
#include <qmk/protocol.h>

bool process_basic(struct qmk_keyboard *keyboard, qmk_keycode_t *keycode,
		   bool pressed)
{
	if (*keycode < 0xFF) {
		protocol.send_keycode(keyboard, *keycode, pressed);
		return true;
	}
	return false;
}

bool process_keycode(struct qmk_keyboard *keyboard, struct qmk_matrix_event *me,
		     qmk_keycode_t *keycode)
{
	// bool handled = false;
	int i;
	uint8_t layer = 0;
	*keycode = KC_NO;
	static uint8_t layer_when_pressed[16][16] = { 0 };

	// protocol.printf("matrix event at (%d, %d, %s)\n", me->row, me->col,
	// 		(me->pressed ? "down" : "up"));

	if (me->row >= keyboard->rows)
		return QMK_ROW_OUT_OF_BOUNDS;
	if (me->col >= keyboard->cols)
		return QMK_COL_OUT_OF_BOUNDS;

	for (i = keyboard->layers - 1; i >= 0; i--) {
		if (((keyboard->layer_state >> i) & 1) &&
		    keycode_from_keymap(keyboard, i, me->row, me->col) !=
			    KC_TRNS) {
			layer = i;
			break;
		}
	}

	keyboard->active_layer = layer;

	// send key release to the layer it was pressed on
	if (me->pressed)
		layer_when_pressed[me->row][me->col] = layer;
	else
		layer = layer_when_pressed[me->row][me->col];

	*keycode = keycode_from_keymap(keyboard, layer, me->row, me->col);

	return process_layer(keyboard, keycode, me->pressed) ||
	       process_mods(keyboard, keycode, me->pressed) ||
	       process_basic(keyboard, keycode, me->pressed);
}