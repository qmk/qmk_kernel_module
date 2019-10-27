#pragma once

#include <qmk/types.h>

qmk_keycode keycode_from_keymap(struct qmk_interface *qi, uint8_t layer, 
                                uint8_t row, uint8_t col);