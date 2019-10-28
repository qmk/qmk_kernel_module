#pragma once

#ifndef __KERNEL__
    #include <stdint.h>
    #include <stdbool.h>
    #include <stddef.h>
#else
    #include <linux/types.h>
#endif

typedef uint8_t hid_keycode;
typedef uint16_t qmk_keycode;
typedef uint8_t qmk_layer;

typedef struct qmk_matrix_event {
    uint8_t row;
    uint8_t col;
    bool pressed;
} qmk_matrix_event;

// struct qmk_key_mapping {
//     qmk_keycode keycode;
//     uint8_t layer;
//     uint8_t row;
//     uint8_t col;
// };

typedef struct qmk_keyboard {
    uint8_t layers;
    uint8_t rows;
    uint8_t cols;
    qmk_keycode *keymap;
    uint16_t layer_state;
    uint16_t mod_tap_timeout;
} qmk_keyboard;
