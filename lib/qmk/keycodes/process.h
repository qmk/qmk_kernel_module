#pragma once

#include <qmk/types.h>

// Types of QMK errors

#define QMK_UNHANDLED_KEYCODE 	1
#define QMK_ROW_OUT_OF_BOUNDS	2
#define QMK_COL_OUT_OF_BOUNDS	3

int process_keycode(struct qmk_interface *qi, 
                    struct qmk_matrix_event ke, 
                    qmk_keycode *keycode);