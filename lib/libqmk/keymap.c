#include <qmk/types.h>
#include "utils.h"
#include <qmk/keymap.h>

qmk_keycode_t keycode_from_keymap(struct qmk_keyboard *keyboard, uint8_t layer,
				  uint8_t row, uint8_t col)
{
	return keyboard->keymap[(layer << get_count_order(
					 keyboard->rows
					 << get_count_order(keyboard->cols))) +
				(row << get_count_order(keyboard->cols)) + col];
}

// bool key_state