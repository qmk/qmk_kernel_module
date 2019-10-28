#pragma once

#include <qmk/types.h>

qmk_keycode keycode_from_keymap(qmk_keyboard *keyboard, uint8_t layer, 
                                uint8_t row, uint8_t col);