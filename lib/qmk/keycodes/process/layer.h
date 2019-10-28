#pragma once

#include <qmk/types.h>

bool process_layer(struct qmk_keyboard *keyboard, qmk_keycode_t *keycode,
		   bool pressed);
