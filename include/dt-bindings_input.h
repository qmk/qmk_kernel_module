/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This header provides constants for most input bindings.
 *
 * Most input bindings include key code, matrix key code format.
 * In most cases, key code and matrix key code format uses
 * the standard values/macro defined in this header.
 */

#ifndef _QMK_DT_BINDINGS_INPUT_H
#define _QMK_DT_BINDINGS_INPUT_H

#include <qmk/keycodes/basic.h>
#include <qmk/keycodes/quantum.h>

#define LAYER_MATRIX_KEY(layer, row, col, code)                                \
	((((layer)&0xF) << 26) | (((row)&0x1F) << 21) | (((col)&0x1F) << 16) | \
	 ((code)&0xFFFF))

#endif /* _QMK_DT_BINDINGS_INPUT_H */
