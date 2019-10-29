#pragma once

#include <qmk/types.h>

// Types of QMK errors

#define QMK_UNHANDLED_KEYCODE 1
#define QMK_ROW_OUT_OF_BOUNDS 2
#define QMK_COL_OUT_OF_BOUNDS 3

int process_keycode(struct qmk_keyboard *keyboard, struct qmk_matrix_event me,
		    qmk_keycode_t *keycode);