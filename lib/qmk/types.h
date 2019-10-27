#pragma once

#ifndef __KERNEL__
    #include <stdint.h>
    #include <stdbool.h>
#else
    #include <linux/types.h>
#endif

#ifndef hid_keycode
    #define hid_keycode uint8_t
#endif

#ifndef qmk_keycode
    #define qmk_keycode uint16_t
#endif

struct qmk_matrix_event {
    uint8_t row;
    uint8_t col;
    bool pressed;
};

// struct qmk_key_mapping {
//     qmk_keycode keycode;
//     uint8_t layer;
//     uint8_t row;
//     uint8_t col;
// };

struct qmk_interface {
    uint8_t layers;
    uint8_t rows;
    uint8_t cols;
    qmk_keycode *keymap;
    uint16_t layer_state;
};

