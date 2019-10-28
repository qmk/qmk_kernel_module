/*
 * Attribute definitions for sysft configuration
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

#include "qmk.h"
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/platform_device.h>

static ssize_t qmk_keymap_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct qmk_module *module = platform_get_drvdata(pdev);
	struct input_dev *input_dev = module->input_dev;
	const unsigned short *keymap = input_dev->keycode;
	int len;

	len = sprintf(buf, "%d\n",
		      keymap[QMK_MATRIX_SCAN_CODE(0, 0, 7, module->layer_shift,
						  module->row_shift)]);
	if (len <= 0)
		dev_err(dev, "qmk: Invalid sprintf len: %d\n", len);

	return len;
}

static ssize_t qmk_keymap_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	return count;
}

static DEVICE_ATTR(keymap, S_IRUGO | S_IWUSR, qmk_keymap_show,
		   qmk_keymap_store);

static ssize_t qmk_layer_state_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct qmk_module *module = platform_get_drvdata(pdev);
	int len;

	len = sprintf(buf, "%ld\n", module->layer_state);
	if (len <= 0)
		dev_err(dev, "qmk: Invalid sprintf len: %d\n", len);

	return len;
}

static ssize_t qmk_layer_state_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct qmk_module *module = platform_get_drvdata(pdev);
	int err;

	err = kstrtoul(buf, 10, &module->layer_state);
	if (err)
		dev_err(dev, "qmk: kstrtouint error: %d\n", err);

	return count;
}

static DEVICE_ATTR(layer_state, S_IRUGO | S_IWUSR, qmk_layer_state_show,
		   qmk_layer_state_store);

static struct attribute *qmk_attrs[] = { &dev_attr_keymap.attr,
					 &dev_attr_layer_state.attr, NULL };

static struct attribute_group qmk_group = {
	.attrs = qmk_attrs,
};

struct attribute_group *get_qmk_group(void)
{
	return &qmk_group;
}

// static struct attribute_group *qmk_groups[] = {
//     &qmk_group,
//     NULL
// };

MODULE_LICENSE("GPL");