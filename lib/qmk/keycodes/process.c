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

#include <qmk/keycodes/process.h>
#include <qmk/keycodes/basic.h>
#include <qmk/keycodes/quantum.h>
#include <qmk/keymap.h>

#include <qmk/keycodes/process/layer.h>

bool process_basic(qmk_keyboard *keyboard, qmk_keycode *keycode, bool pressed)
{
    if (*keycode < 0xFF) {
        return true;
    }
    return false;
}

int process_keycode(qmk_keyboard *keyboard, qmk_matrix_event ke, 
                    qmk_keycode *keycode)
{
    bool handled = false;
    uint8_t i;
    uint8_t layer = 0;
    *keycode = KC_NO;

    if (ke.row >= keyboard->rows)
        return QMK_ROW_OUT_OF_BOUNDS;
    if (ke.col >= keyboard->cols)
        return QMK_COL_OUT_OF_BOUNDS;

    for (i = keyboard->layers - 1; i >= 0; i--) {
        if (((keyboard->layer_state >> i) & 1) &&
            keycode_from_keymap(keyboard, i, ke.row, ke.col) != KC_TRNS) {
            layer = i;
            break;
        }
    }

    *keycode = keycode_from_keymap(keyboard, layer, ke.row, ke.col);

    handled = process_layer(keyboard, keycode, ke.pressed) ||
        process_basic(keyboard, keycode, ke.pressed);

    if (handled) {
        return 0;
    } else {
        return QMK_UNHANDLED_KEYCODE;
    }

}