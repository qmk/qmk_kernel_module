/*
 * Helpers for matrix keyboard bindings
 *
 * Copyright (C) 2012 Google, Inc
 *               2019 Jack Humbert <jack.humb@gmail.com>
 *
 * Author:
 *  Olof Johansson <olof@lixom.net>
 *  Jack Humbert <jack.humb@gmail.com>
 *  Based on matrix-keymap.c
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

#include <linux/device.h>
#include <linux/export.h>
#include <linux/gfp.h>
#include <linux/input.h>
#include "qmk.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/property.h>
#include <linux/slab.h>
#include <linux/types.h>
#include "qmk_scancodes.h"

static bool qmk_map_key(struct input_dev *input_dev, unsigned int layers,
			unsigned int layer_shift, unsigned int rows,
			unsigned int cols, unsigned int row_shift,
			unsigned int key)
{
	unsigned short *keymap = input_dev->keycode;
	unsigned int layer = KEY_LAYER(key);
	unsigned int row = KEY_ROW(key);
	unsigned int col = KEY_COL(key);
	unsigned short code = KEY_VAL(key);

	if (layer >= layers || row >= rows || col >= cols) {
		dev_err(input_dev->dev.parent,
			"%s: invalid keymap entry 0x%x (layer: %d, row: %d, col: %d, layers: %d, rows: %d, cols: %d)\n",
			__func__, key, layer, row, col, layers, rows, cols);
		return false;
	}

	keymap[QMK_MATRIX_SCAN_CODE(layer, row, col, layer_shift, row_shift)] =
		code;
	if (code < 0xFF) {
		__set_bit(keycode_to_scancode[code], input_dev->keybit);
	}

	return true;
}

/**
 * qmk_parse_properties() - Read properties of matrix keyboard
 *
 * @dev: Device containing properties
 * @layers: Returns number of matrix layers
 * @rows: Returns number of matrix rows
 * @cols: Returns number of matrix columns
 * @return 0 if OK, <0 on error
 */
int qmk_parse_properties(struct device *dev, unsigned int *layers,
			 unsigned int *rows, unsigned int *cols)
{
	*layers = *rows = *cols = 0;

	device_property_read_u32(dev, "keypad,num-layers", layers);
	device_property_read_u32(dev, "keypad,num-rows", rows);
	device_property_read_u32(dev, "keypad,num-columns", cols);

	if (!*layers || !*rows || !*cols) {
		dev_err(dev,
			"number of keyboard layers/rows/columns not specified\n");
		return -EINVAL;
	}

	return 0;
}

static int qmk_parse_keymap(const char *propname, unsigned int layers,
			    unsigned int rows, unsigned int cols,
			    struct input_dev *input_dev)
{
	struct device *dev = input_dev->dev.parent;
	unsigned int row_shift = get_count_order(cols);
	unsigned int layer_shift = get_count_order(rows << row_shift);
	unsigned int max_keys = layers << layer_shift;
	u32 *keys;
	int i;
	int size;
	int retval;

	if (!propname)
		propname = "qmk,keymap";

	size = device_property_read_u32_array(dev, propname, NULL, 0);
	if (size <= 0) {
		dev_err(dev, "missing or malformed property %s: %d\n", propname,
			size);
		return size < 0 ? size : -EINVAL;
	}

	if (size > max_keys) {
		dev_err(dev, "%s size overflow (%d vs max %u)\n", propname,
			size, max_keys);
		return -EINVAL;
	}

	keys = kmalloc_array(size, sizeof(u32), GFP_KERNEL);
	if (!keys)
		return -ENOMEM;

	retval = device_property_read_u32_array(dev, propname, keys, size);
	if (retval) {
		dev_err(dev, "failed to read %s property: %d\n", propname,
			retval);
		goto out;
	}

	for (i = 0; i < size; i++) {
		if (!qmk_map_key(input_dev, layers, layer_shift, rows, cols,
				 row_shift, keys[i])) {
			retval = -EINVAL;
			goto out;
		}
	}

	retval = 0;

out:
	kfree(keys);
	return retval;
}

/**
 * qmk_build_keymap - convert platform keymap into matrix keymap
 * @keymap_data: keymap supplied by the platform code
 * @keymap_name: name of device tree property containing keymap (if device
 *  tree support is enabled).
 * @layers: number of layers in target keymap array
 * @rows: number of rows in target keymap array
 * @cols: number of cols in target keymap array
 * @keymap: expanded version of keymap that is suitable for use by
 * matrix keyboard driver
 * @input_dev: input devices for which we are setting up the keymap
 *
 * This function converts platform keymap (encoded with KEY() macro) into
 * an array of keycodes that is suitable for using in a standard matrix
 * keyboard driver that uses row and col as indices.
 *
 * If @keymap_data is not supplied and device tree support is enabled
 * it will attempt load the keymap from property specified by @keymap_name
 * argument (or "linux,keymap" if @keymap_name is %NULL).
 *
 * If @keymap is %NULL the function will automatically allocate managed
 * block of memory to store the keymap. This memory will be associated with
 * the parent device and automatically freed when device unbinds from the
 * driver.
 *
 * Callers are expected to set up input_dev->dev.parent before calling this
 * function.
 */
int qmk_build_keymap(const struct matrix_keymap_data *keymap_data,
		     const char *keymap_name, unsigned int layers,
		     unsigned int rows, unsigned int cols,
		     unsigned short *keymap, struct input_dev *input_dev)
{
	unsigned int row_shift = get_count_order(cols);
	unsigned int layer_shift = get_count_order(rows << row_shift);
	size_t max_keys = layers << layer_shift;
	int i;
	int error;

	if (WARN_ON(!input_dev->dev.parent))
		return -EINVAL;

	if (!keymap) {
		keymap = devm_kcalloc(input_dev->dev.parent, max_keys,
				      sizeof(*keymap), GFP_KERNEL);
		if (!keymap) {
			dev_err(input_dev->dev.parent,
				"Unable to allocate memory for keymap");
			return -ENOMEM;
		}
	}

	input_dev->keycode = keymap;
	input_dev->keycodesize = sizeof(*keymap);
	input_dev->keycodemax = max_keys;

	__set_bit(EV_KEY, input_dev->evbit);

	if (keymap_data) {
		for (i = 0; i < keymap_data->keymap_size; i++) {
			unsigned int key = keymap_data->keymap[i];

			if (!qmk_map_key(input_dev, layers, layer_shift, rows,
					 cols, row_shift, key))
				return -EINVAL;
		}
	} else {
		error = qmk_parse_keymap(keymap_name, layers, rows, cols,
					 input_dev);
		if (error)
			return error;
	}

	__clear_bit(KEY_RESERVED, input_dev->keybit);

	return 0;
}

MODULE_LICENSE("GPL");
