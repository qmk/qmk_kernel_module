#pragma once

#include <qmk/types.h>

// Types of QMK errors

#define QMK_UNHANDLED_KEYCODE 	1
#define QMK_ROW_OUT_OF_BOUNDS	2
#define QMK_COL_OUT_OF_BOUNDS	3

int process_keycode(qmk_keyboard *keyboard, 
                    qmk_matrix_event ke, 
                    qmk_keycode *keycode);