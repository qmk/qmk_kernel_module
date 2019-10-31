#pragma once

#include <qmk/types.h>

// Types of QMK errors

#define QMK_UNHANDLED_KEYCODE 1
#define QMK_ROW_OUT_OF_BOUNDS 2
#define QMK_COL_OUT_OF_BOUNDS 3

bool process_keycode(struct qmk_keyboard *keyboard, struct qmk_matrix_event *me,
		     qmk_keycode_t *keycode);
bool process_basic(struct qmk_keyboard *keyboard, qmk_keycode_t *keycode,
		   bool pressed);
bool process_mods(struct qmk_keyboard *keyboard, qmk_keycode_t *keycode,
		  bool pressed);
bool process_layer(struct qmk_keyboard *keyboard, qmk_keycode_t *keycode,
		   bool pressed);