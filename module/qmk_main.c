/*
 *  GPIO driven matrix keyboard driver
 *
 *  Copyright (c) 2008 Marek Vasut <marek.vasut@gmail.com>
 *                2019 Jack Humbert <jack.humb@gmail.com>
 *
 *  Based on matrix_keypad.c and gpio_keys_polled.c
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#include "qmk.h"
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/input-polldev.h>
#include <linux/input.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <qmk/types.h>

static void qmk_start(struct input_polled_dev *poll_dev)
{
	// struct qmk_module *keyboard = poll_dev->private;
	// const struct qmk_platform_data *pdata = keyboard->pdata;

	// if (pdata->enable)
	//     pdata->enable(keyboard->dev);
}

static void qmk_stop(struct input_polled_dev *poll_dev)
{
	// struct qmk_module *keyboard = poll_dev->private;
	// const struct qmk_platform_data *pdata = keyboard->pdata;

	// if (pdata->disable)
	//     pdata->disable(keyboard->dev);
}

#ifdef CONFIG_PM_SLEEP
static void qmk_enable_wakeup(struct qmk_module *keyboard)
{
	// const struct qmk_platform_data *pdata = keyboard->pdata;
	// unsigned int gpio;
	// int i;
}

static void qmk_disable_wakeup(struct qmk_module *keyboard)
{
	// const struct qmk_platform_data *pdata = keyboard->pdata;
	// unsigned int gpio;
	// int i;
}

static int qmk_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct qmk_module *module = platform_get_drvdata(pdev);

	qmk_stop(module->input_dev);

	if (device_may_wakeup(&pdev->dev))
		qmk_enable_wakeup(module);

	return 0;
}

static int qmk_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct qmk_module *module = platform_get_drvdata(pdev);

	if (device_may_wakeup(&pdev->dev))
		qmk_disable_wakeup(module);

	qmk_start(module->input_dev);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(qmk_pm_ops, qmk_suspend, qmk_resume);

#ifdef CONFIG_OF
static struct qmk_platform_data *qmk_parse_dt(struct device *dev,
					      struct qmk_keyboard *keyboard)
{
	struct qmk_platform_data *pdata;
	struct device_node *np = dev->of_node;
	unsigned int *gpios;
	int ret, i, nrow, ncol;

	if (!np) {
		dev_err(dev, "device lacks DT data\n");
		return ERR_PTR(-ENODEV);
	}

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		dev_err(dev, "could not allocate memory for platform data\n");
		return ERR_PTR(-ENOMEM);
	}

	of_property_read_string(np, "device-name", &pdata->name);
	of_property_read_u32(np, "keypad,num-layers", &keyboard->layers);
	if (keyboard->layers <= 0) {
		dev_err(dev, "number of keyboard layers not specified\n");
		return ERR_PTR(-EINVAL);
	}
	keyboard->rows = nrow = of_gpio_named_count(np, "row-gpios");
	if (nrow <= 0) {
		dev_err(dev, "number of keyboard rows not specified\n");
		return ERR_PTR(-EINVAL);
	}
	keyboard->cols = ncol = of_gpio_named_count(np, "col-gpios");
	if (ncol <= 0) {
		dev_err(dev, "number of keyboard columns not specified\n");
		return ERR_PTR(-EINVAL);
	}

	if (of_get_property(np, "linux,no-autorepeat", NULL))
		pdata->no_autorepeat = true;

	of_property_read_u32(np, "poll-interval", &pdata->poll_interval);

	pdata->wakeup = of_property_read_bool(np, "wakeup-source") ||
			of_property_read_bool(np, "linux,wakeup"); /* legacy */

	if (of_get_property(np, "gpio-activelow", NULL))
		pdata->active_low = true;

	pdata->drive_inactive_cols =
		of_property_read_bool(np, "drive-inactive-cols");

	of_property_read_u32(np, "debounce-delay-ms", &pdata->debounce_ms);
	of_property_read_u32(np, "col-scan-delay-us",
			     &pdata->col_scan_delay_us);

	gpios = devm_kcalloc(dev, keyboard->rows + keyboard->cols,
			     sizeof(unsigned int), GFP_KERNEL);
	if (!gpios) {
		dev_err(dev, "could not allocate memory for gpios\n");
		return ERR_PTR(-ENOMEM);
	}

	for (i = 0; i < nrow; i++) {
		ret = of_get_named_gpio(np, "row-gpios", i);
		if (ret < 0)
			return ERR_PTR(ret);
		gpios[i] = ret;
	}

	for (i = 0; i < ncol; i++) {
		ret = of_get_named_gpio(np, "col-gpios", i);
		if (ret < 0)
			return ERR_PTR(ret);
		gpios[nrow + i] = ret;
	}

	pdata->row_gpios = gpios;
	pdata->col_gpios = &gpios[keyboard->rows];

	return pdata;
}
#else
static inline struct qmk_platform_data *
qmk_parse_dt(struct device *dev, struct qmk_keyboard *keyboard)
{
	dev_err(dev, "no platform data defined\n");

	return ERR_PTR(-EINVAL);
}
#endif

static int qmk_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct qmk_platform_data *pdata;
	struct qmk_module *module;
	struct qmk_keyboard *keyboard;
	struct input_polled_dev *poll_dev;
	struct input_dev *input;
	size_t size;
	int err;

	size = sizeof(struct qmk_keyboard);
	keyboard = devm_kzalloc(dev, size, GFP_KERNEL);
	if (!keyboard) {
		dev_err(dev, "no memory for qmk keyboard\n");
		err = -ENOMEM;
		goto err_no_free;
	}

	pdata = dev_get_platdata(dev);
	if (!pdata) {
		pdata = qmk_parse_dt(dev, keyboard);
		if (IS_ERR(pdata)) {
			err = PTR_ERR(pdata);
			goto err_free_keyboard;
		}
	} else if (!pdata->keymap_data) {
		dev_err(dev, "no keymap data defined\n");
		err = -EINVAL;
		goto err_free_keyboard;
	}

	size = sizeof(struct qmk_module);
	module = devm_kzalloc(dev, size, GFP_KERNEL);
	if (!module) {
		dev_err(dev, "no memory for module data\n");
		err = -ENOMEM;
		goto err_free_module;
	}

	module->keyboard = keyboard;
	keyboard->parent = module;

	poll_dev = input_allocate_polled_device();
	if (!poll_dev) {
		dev_err(dev, "no memory for polled device\n");
		err = -ENOMEM;
		goto err_free_device;
	}

	input = poll_dev->input;
	input->name = pdata->name;
	input->phys = "qmk/input0";
	input->id.bustype = BUS_HOST;
	// input->id.vendor        = 0x0001;
	// input->id.product       = 0x0001;
	// input->id.version       = 0x0100;
	input->id.vendor = 0x03A8;
	input->id.product = 0x0068;
	input->id.version = 0x0001;
	input->dev.parent = dev;
	// input->open             = qmk_start;
	// input->close            = qmk_stop;

	err = qmk_build_keymap(pdata->keymap_data, "qmk,keymap",
			       keyboard->layers, keyboard->rows, keyboard->cols,
			       NULL, input);
	if (err) {
		dev_err(dev, "failed to build keymap\n");
		goto err_free_device;
	}

	poll_dev->private = module;
	poll_dev->poll = qmk_scan;
	poll_dev->poll_interval = pdata->poll_interval;
	poll_dev->open = qmk_start;
	poll_dev->close = qmk_stop;

	keyboard->keymap = input->keycode;
	keyboard->layer_state = 1;

	if (!pdata->no_autorepeat)
		__set_bit(EV_REP, input->evbit);
	input_set_capability(input, EV_MSC, MSC_SCAN);
	input_set_drvdata(input, module);

	module->poll_dev = poll_dev;
	module->input_dev = input;
	module->pdata = pdata;
	module->dev = dev;
	module->row_shift = get_count_order(keyboard->cols);
	module->layer_shift =
		get_count_order(keyboard->rows << module->row_shift);
	module->stopped = true;

	err = qmk_init_gpio(pdev, module);
	if (err) {
		dev_err(dev, "unable to init gpio, err=%d\n", err);
		goto err_free_device;
	}

	err = sysfs_create_group(&pdev->dev.kobj, get_qmk_group());
	if (err) {
		dev_err(dev, "sysfs creation failed\n");
		goto err_free_gpio;
	}

	err = input_register_polled_device(poll_dev);
	if (err) {
		dev_err(dev, "unable to register polled device, err=%d\n", err);
		goto err_free_sysfs;
	}

	device_init_wakeup(dev, pdata->wakeup);
	platform_set_drvdata(pdev, module);

	return 0;

err_free_sysfs:
	sysfs_remove_group(&pdev->dev.kobj, get_qmk_group());
err_free_gpio:
	qmk_free_gpio(module);
err_free_device:
	input_free_polled_device(poll_dev);
err_free_module:
	devm_kfree(dev, module);
err_free_keyboard:
	devm_kfree(dev, keyboard);
err_no_free:
	return err;
}

static int qmk_remove(struct platform_device *pdev)
{
	struct qmk_module *module = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;

	qmk_free_gpio(module);
	input_unregister_polled_device(module->poll_dev);
	devm_kfree(dev, module);

	sysfs_remove_group(&pdev->dev.kobj, get_qmk_group());

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id qmk_dt_match[] = { { .compatible = "qmk" },
						    {} };
MODULE_DEVICE_TABLE(of, qmk_dt_match);
#endif

struct platform_driver qmk_driver = {
	.probe = qmk_probe,
	.remove = qmk_remove,
	.driver =
		{
			.name = "qmk",
			.pm = &qmk_pm_ops,
			.of_match_table = of_match_ptr(qmk_dt_match),
		},
};

// module_platform_driver(qmk_driver);
static int __init qmk_driver_init(void)
{
	int status;
	status = gadget_init();

	status = platform_driver_register(&qmk_driver);
    if (status)
       goto err_free_gadget;

    return status;

err_free_gadget:
	gadget_exit();

	return status;
}

static void __exit qmk_driver_exit(void)
{
	platform_driver_unregister(&qmk_driver);
	gadget_exit();
}

module_init(qmk_driver_init);
module_exit(qmk_driver_exit);

MODULE_AUTHOR("Jack Humbert <jack.humb@gmail.com>");
MODULE_DESCRIPTION("QMK Feature Support For GPIO Driven Keyboards");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:qmk");
MODULE_SOFTDEP("pre: input-polldev");