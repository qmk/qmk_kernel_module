#pragma once

#include <qmk/types.h>

qmk_keycode_t keycode_from_keymap(struct qmk_keyboard *keyboard, uint8_t layer,
				  uint8_t row, uint8_t col);