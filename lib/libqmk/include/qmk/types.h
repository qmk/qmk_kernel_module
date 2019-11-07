#pragma once

#ifndef __KERNEL__
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#else
#include <linux/types.h>
#endif

typedef uint8_t hid_keycode_t;
typedef uint16_t qmk_keycode_t;
typedef uint8_t qmk_layer;
typedef uint8_t io_t;

struct qmk_matrix_event {
	uint8_t row;
	uint8_t col;
	bool pressed;
};

// struct qmk_key_mapping {
//     qmk_keycode_t keycode;
//     uint8_t layer;
//     uint8_t row;
//     uint8_t col;
// };

struct qmk_keyboard {
	void *parent;
	uint32_t layers;
	uint8_t rows;
	uint8_t cols;
	qmk_keycode_t *keymap;
	bool *key_state;
	uint16_t layer_state;
	uint8_t active_layer;
	uint16_t mod_tap_timeout;
	uint8_t encoder_ab_count;
	uint8_t encoder_c_count;
	io_t *encoder_a;
	io_t *encoder_b;
	io_t *encoder_c;
};
