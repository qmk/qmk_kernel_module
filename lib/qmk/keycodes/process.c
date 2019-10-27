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

bool process_layer(qmk_keycode *keycode, bool pressed, uint16_t *layer_state)
{
    unsigned int layer;
    switch (*keycode) {
        case QK_TO ... QK_TO_MAX: ;
            // Layer set "GOTO"
            // when = (keycode >> 0x4) & 0x3;
            // action_layer = keycode & 0xF;
            // action.code = ACTION_LAYER_SET(action_layer, when);
            return true;
            break;
        case QK_MOMENTARY ... QK_MOMENTARY_MAX: ;
            // Momentary action_layer
            layer = *keycode & 0xFF;
            if (pressed)
                (*layer_state) |= (1UL << layer);
            else
                (*layer_state) &= ~(1UL << layer);
            return true;
            break;
        default:
            return false;
            break;
    }
}

bool process_basic(qmk_keycode *keycode, bool pressed)
{
    if (*keycode < 0xFF) {
        return true;
    }
    return false;
}

int process_keycode(struct qmk_interface *qi, 
                    struct qmk_matrix_event ke, 
                    qmk_keycode *keycode)
{
    bool handled = false;
    uint8_t i;
    uint8_t layer = 0;
    *keycode = KC_NO;

    if (ke.row >= qi->rows)
        return QMK_ROW_OUT_OF_BOUNDS;
    if (ke.col >= qi->cols)
        return QMK_COL_OUT_OF_BOUNDS;

    for (i = qi->layers - 1; i >= 0; i--) {
        if (((qi->layer_state >> i) & 1) &&
            keycode_from_keymap(qi, i, ke.row, ke.col) != KC_TRNS) {
            layer = i;
            break;
        }
    }

    *keycode = keycode_from_keymap(qi, layer, ke.row, ke.col);

    handled = process_layer(keycode, ke.pressed, &qi->layer_state) ||
        process_basic(keycode, ke.pressed);

    if (handled) {
        return 0;
    } else {
        return QMK_UNHANDLED_KEYCODE;
    }

}