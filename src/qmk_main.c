/*
 *  GPIO driven matrix keyboard driver
 *
 *  Copyright (c) 2008 Marek Vasut <marek.vasut@gmail.com>
 *                2019 Jack Humbert <jack.humb@gmail.com>
 *
 *  Based on matrix_keypad.c
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include "qmk.h"
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/moduleparam.h>

#include <linux/sysfs.h>


/*
 * Attribute definitions for sysfs
 */

static ssize_t qmk_layers_show(struct device *dev, 
	                           struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct qmk *keyboard = platform_get_drvdata(pdev);
	int len;

	len = sprintf(buf, "%d\n", keyboard->pdata->num_layers);
	if (len <= 0)
		dev_err(dev, "qmk: Invalid sprintf len: %d\n", len);

	return len;
}

static ssize_t qmk_layers_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	const struct qmk *keyboard = platform_get_drvdata(pdev);
	int err;

    err = kstrtouint(buf, 10, &keyboard->pdata->num_layers);
    if (err)
    	dev_err(dev, "qmk: kstrtouint error: %d\n", err);

    return count;
}

static DEVICE_ATTR(layers, S_IRUGO | S_IWUSR, qmk_layers_show,
	qmk_layers_store);

static ssize_t qmk_layer_state_show(struct device *dev, 
	                           struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct qmk *keyboard = platform_get_drvdata(pdev);
	int len;

	len = sprintf(buf, "%ld\n", keyboard->layer_state);
	if (len <= 0)
		dev_err(dev, "qmk: Invalid sprintf len: %d\n", len);

	return len;
}

static ssize_t qmk_layer_state_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	const struct qmk *keyboard = platform_get_drvdata(pdev);
	int err;

    err = kstrtoul(buf, 10, &keyboard->layer_state);
    if (err)
    	dev_err(dev, "qmk: kstrtouint error: %d\n", err);

    return count;
}

static DEVICE_ATTR(layer_state, S_IRUGO | S_IWUSR, qmk_layer_state_show,
	qmk_layer_state_store);

static struct attribute *qmk_attrs[] = {
    &dev_attr_layers.attr,
    &dev_attr_layer_state.attr,
    NULL
};

static struct attribute_group qmk_group = {
    .attrs = qmk_attrs,
};

// static struct attribute_group *qmk_groups[] = {
//     &qmk_group,
//     NULL
// };

/*
 * NOTE: If drive_inactive_cols is false, then the GPIO has to be put into
 * HiZ when de-activated to cause minmal side effect when scanning other
 * columns. In that case it is configured here to be input, otherwise it is
 * driven with the inactive value.
 */
static void __activate_col(const struct qmk_platform_data *pdata,
			   int col, bool on)
{
	bool level_on = !pdata->active_low;

	if (on) {
		gpio_direction_output(pdata->col_gpios[col], level_on);
	} else {
		gpio_set_value_cansleep(pdata->col_gpios[col], !level_on);
		if (!pdata->drive_inactive_cols)
			gpio_direction_input(pdata->col_gpios[col]);
	}
}

static void activate_col(const struct qmk_platform_data *pdata,
			 int col, bool on)
{
	__activate_col(pdata, col, on);

	if (on && pdata->col_scan_delay_us)
		udelay(pdata->col_scan_delay_us);
}

static void activate_all_cols(const struct qmk_platform_data *pdata,
			      bool on)
{
	int col;

	for (col = 0; col < pdata->num_col_gpios; col++)
		__activate_col(pdata, col, on);
}

static bool row_asserted(const struct qmk_platform_data *pdata,
			 int row)
{
	return gpio_get_value_cansleep(pdata->row_gpios[row]) ?
			!pdata->active_low : pdata->active_low;
}

static void enable_row_irqs(struct qmk *keyboard)
{
	const struct qmk_platform_data *pdata = keyboard->pdata;
	int i;

	if (pdata->clustered_irq > 0)
		enable_irq(pdata->clustered_irq);
	else {
		for (i = 0; i < pdata->num_row_gpios; i++)
			enable_irq(gpio_to_irq(pdata->row_gpios[i]));
	}
}

static void disable_row_irqs(struct qmk *keyboard)
{
	const struct qmk_platform_data *pdata = keyboard->pdata;
	int i;

	if (pdata->clustered_irq > 0)
		disable_irq_nosync(pdata->clustered_irq);
	else {
		for (i = 0; i < pdata->num_row_gpios; i++)
			disable_irq_nosync(gpio_to_irq(pdata->row_gpios[i]));
	}
}

/*
 * This gets the keys from keyboard and reports it to input subsystem
 */
static void qmk_scan(struct work_struct *work)
{
	struct qmk *keyboard = container_of(work, struct qmk, work.work);
	struct input_dev *input_dev = keyboard->input_dev;
	const struct qmk_platform_data *pdata = keyboard->pdata;
	uint32_t new_state[MATRIX_MAX_COLS];
	int row, col;
	bool pressed = false, key_down = false;

	/* de-activate all columns for scanning */
	activate_all_cols(pdata, false);

	memset(new_state, 0, sizeof(new_state));

	/* assert each column and read the row status out */
	for (col = 0; col < pdata->num_col_gpios; col++) {

		activate_col(pdata, col, true);

		for (row = 0; row < pdata->num_row_gpios; row++)
			new_state[col] |=
				row_asserted(pdata, row) ? (1 << row) : 0;

		activate_col(pdata, col, false);
	}

	for (col = 0; col < pdata->num_col_gpios; col++) {
		uint32_t bits_changed;

		bits_changed = keyboard->last_key_state[col] ^ new_state[col];
		if (bits_changed == 0)
			continue;

		for (row = 0; row < pdata->num_row_gpios; row++) {
			if ((bits_changed & (1 << row)) == 0)
				continue;
			pressed = new_state[col] & (1 << row);
			key_down |= pressed;
			qmk_process_keycode(keyboard, row, col, pressed);
		}
	}
	input_sync(input_dev);

	memcpy(keyboard->last_key_state, new_state, sizeof(new_state));

	activate_all_cols(pdata, true);

	if (key_down) {
		schedule_delayed_work(&keyboard->work,
			msecs_to_jiffies(keyboard->pdata->debounce_ms));
	} else {
		/* Enable IRQs again */
		spin_lock_irq(&keyboard->lock);
		keyboard->scan_pending = false;
		enable_row_irqs(keyboard);
		spin_unlock_irq(&keyboard->lock);
	}
}

static irqreturn_t qmk_interrupt(int irq, void *id)
{
	struct qmk *keyboard = id;
	unsigned long flags;

	spin_lock_irqsave(&keyboard->lock, flags);

	/*
	 * See if another IRQ beaten us to it and scheduled the
	 * scan already. In that case we should not try to
	 * disable IRQs again.
	 */
	if (unlikely(keyboard->scan_pending || keyboard->stopped))
		goto out;

	disable_row_irqs(keyboard);
	keyboard->scan_pending = true;
	schedule_delayed_work(&keyboard->work,
		msecs_to_jiffies(keyboard->pdata->debounce_ms));

out:
	spin_unlock_irqrestore(&keyboard->lock, flags);
	return IRQ_HANDLED;
}

static int qmk_start(struct input_dev *dev)
{
	struct qmk *keyboard = input_get_drvdata(dev);

	keyboard->stopped = false;
	mb();

	/*
	 * Schedule an immediate key scan to capture current key state;
	 * columns will be activated and IRQs be enabled after the scan.
	 */
	schedule_delayed_work(&keyboard->work, 0);

	return 0;
}

static void qmk_stop(struct input_dev *dev)
{
	struct qmk *keyboard = input_get_drvdata(dev);

	spin_lock_irq(&keyboard->lock);
	keyboard->stopped = true;
	spin_unlock_irq(&keyboard->lock);

	flush_delayed_work(&keyboard->work);
	/*
	 * qmk_scan() will leave IRQs enabled;
	 * we should disable them now.
	 */
	disable_row_irqs(keyboard);
}

#ifdef CONFIG_PM_SLEEP
static void qmk_enable_wakeup(struct qmk *keyboard)
{
	const struct qmk_platform_data *pdata = keyboard->pdata;
	unsigned int gpio;
	int i;

	if (pdata->clustered_irq > 0) {
		if (enable_irq_wake(pdata->clustered_irq) == 0)
			keyboard->gpio_all_disabled = true;
	} else {

		for (i = 0; i < pdata->num_row_gpios; i++) {
			if (!test_bit(i, keyboard->disabled_gpios)) {
				gpio = pdata->row_gpios[i];

				if (enable_irq_wake(gpio_to_irq(gpio)) == 0)
					__set_bit(i, keyboard->disabled_gpios);
			}
		}
	}
}

static void qmk_disable_wakeup(struct qmk *keyboard)
{
	const struct qmk_platform_data *pdata = keyboard->pdata;
	unsigned int gpio;
	int i;

	if (pdata->clustered_irq > 0) {
		if (keyboard->gpio_all_disabled) {
			disable_irq_wake(pdata->clustered_irq);
			keyboard->gpio_all_disabled = false;
		}
	} else {
		for (i = 0; i < pdata->num_row_gpios; i++) {
			if (test_and_clear_bit(i, keyboard->disabled_gpios)) {
				gpio = pdata->row_gpios[i];
				disable_irq_wake(gpio_to_irq(gpio));
			}
		}
	}
}

static int qmk_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct qmk *keyboard = platform_get_drvdata(pdev);

	qmk_stop(keyboard->input_dev);

	if (device_may_wakeup(&pdev->dev))
		qmk_enable_wakeup(keyboard);

	return 0;
}

static int qmk_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct qmk *keyboard = platform_get_drvdata(pdev);

	if (device_may_wakeup(&pdev->dev))
		qmk_disable_wakeup(keyboard);

	qmk_start(keyboard->input_dev);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(qmk_pm_ops,
			 qmk_suspend, qmk_resume);



static int qmk_init_gpio(struct platform_device *pdev,
				   struct qmk *keyboard)
{
	const struct qmk_platform_data *pdata = keyboard->pdata;
	int i, err;

	/* initialized strobe lines as outputs, activated */
	for (i = 0; i < pdata->num_col_gpios; i++) {
		err = gpio_request(pdata->col_gpios[i], "matrix_kbd_col");
		if (err) {
			dev_err(&pdev->dev,
				"failed to request GPIO%d for COL%d\n",
				pdata->col_gpios[i], i);
			goto err_free_cols;
		}

		gpio_direction_output(pdata->col_gpios[i], !pdata->active_low);
	}

	for (i = 0; i < pdata->num_row_gpios; i++) {
		err = pinctrl_gpio_request(pdata->row_gpios[i]);
		if (err) {
			dev_err(&pdev->dev,
				"failed to request pinctrl for GPIO%d on ROW%d\n",
				pdata->row_gpios[i], i);
			goto err_free_rows;
		}

		pinctrl_gpio_set_config(pdata->row_gpios[i], PIN_CONFIG_BIAS_PULL_UP);
		pinctrl_gpio_direction_input(pdata->row_gpios[i]);

	}

	if (pdata->clustered_irq > 0) {
		err = request_any_context_irq(pdata->clustered_irq,
				qmk_interrupt,
				pdata->clustered_irq_flags,
				"qmk", keyboard);
		if (err < 0) {
			dev_err(&pdev->dev,
				"Unable to acquire clustered interrupt\n");
			goto err_free_rows;
		}
	} else {
		for (i = 0; i < pdata->num_row_gpios; i++) {
			err = request_any_context_irq(
					gpio_to_irq(pdata->row_gpios[i]),
					qmk_interrupt,
					IRQF_TRIGGER_RISING |
					IRQF_TRIGGER_FALLING,
					"qmk", keyboard);
			if (err < 0) {
				dev_err(&pdev->dev,
					"Unable to acquire interrupt for GPIO line %i\n",
					pdata->row_gpios[i]);
				goto err_free_irqs;
			}
		}
	}

	/* initialized as disabled - enabled by input->open */
	disable_row_irqs(keyboard);
	return 0;

err_free_irqs:
	while (--i >= 0)
		free_irq(gpio_to_irq(pdata->row_gpios[i]), keyboard);
	i = pdata->num_row_gpios;
err_free_rows:
	while (--i >= 0)
		pinctrl_gpio_free(pdata->row_gpios[i]);
	i = pdata->num_col_gpios;
err_free_cols:
	while (--i >= 0)
		gpio_free(pdata->col_gpios[i]);

	return err;
}

static void qmk_free_gpio(struct qmk *keyboard)
{
	const struct qmk_platform_data *pdata = keyboard->pdata;
	int i;

	if (pdata->clustered_irq > 0) {
		free_irq(pdata->clustered_irq, keyboard);
	} else {
		for (i = 0; i < pdata->num_row_gpios; i++)
			free_irq(gpio_to_irq(pdata->row_gpios[i]), keyboard);
	}

	for (i = 0; i < pdata->num_row_gpios; i++)
		pinctrl_gpio_free(pdata->row_gpios[i]);

	for (i = 0; i < pdata->num_col_gpios; i++)
		gpio_free(pdata->col_gpios[i]);
}

#ifdef CONFIG_OF
static struct qmk_platform_data *
qmk_parse_dt(struct device *dev)
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

	of_property_read_u32(np, "keypad,num-layers", &pdata->num_layers);

	pdata->num_row_gpios = nrow = of_gpio_named_count(np, "row-gpios");
	pdata->num_col_gpios = ncol = of_gpio_named_count(np, "col-gpios");
	if (pdata->num_layers <= 0 || nrow <= 0 || ncol <= 0) {
		dev_err(dev, "number of keyboard layers/rows/columns not specified\n");
		return ERR_PTR(-EINVAL);
	}

	if (of_get_property(np, "linux,no-autorepeat", NULL))
		pdata->no_autorepeat = true;

	pdata->wakeup = of_property_read_bool(np, "wakeup-source") ||
			of_property_read_bool(np, "linux,wakeup"); /* legacy */

	if (of_get_property(np, "gpio-activelow", NULL))
		pdata->active_low = true;

	pdata->drive_inactive_cols =
		of_property_read_bool(np, "drive-inactive-cols");

	of_property_read_u32(np, "debounce-delay-ms", &pdata->debounce_ms);
	of_property_read_u32(np, "col-scan-delay-us",
						&pdata->col_scan_delay_us);

	gpios = devm_kcalloc(dev,
			     pdata->num_row_gpios + pdata->num_col_gpios,
			     sizeof(unsigned int),
			     GFP_KERNEL);
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
	pdata->col_gpios = &gpios[pdata->num_row_gpios];

	return pdata;
}
#else
static inline struct qmk_platform_data *
qmk_parse_dt(struct device *dev)
{
	dev_err(dev, "no platform data defined\n");

	return ERR_PTR(-EINVAL);
}
#endif

static int qmk_probe(struct platform_device *pdev)
{
	const struct qmk_platform_data *pdata;
	struct qmk *keyboard;
	struct input_dev *input_dev;
	int err;

	pdata = dev_get_platdata(&pdev->dev);
	if (!pdata) {
		pdata = qmk_parse_dt(&pdev->dev);
		if (IS_ERR(pdata))
			return PTR_ERR(pdata);
	} else if (!pdata->keymap_data) {
		dev_err(&pdev->dev, "no keymap data defined\n");
		return -EINVAL;
	}

	keyboard = kzalloc(sizeof(struct qmk), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!keyboard || !input_dev) {
		err = -ENOMEM;
		goto err_free_mem;
	}

	keyboard->input_dev = input_dev;
	keyboard->pdata = pdata;
	keyboard->row_shift = get_count_order(pdata->num_col_gpios);
	keyboard->layer_shift = get_count_order(pdata->num_row_gpios 
												<< keyboard->row_shift);
	keyboard->stopped = true;
	INIT_DELAYED_WORK(&keyboard->work, qmk_scan);
	spin_lock_init(&keyboard->lock);

	input_dev->name		= pdev->name;
	input_dev->id.bustype	= BUS_HOST;
	input_dev->dev.parent	= &pdev->dev;
	input_dev->open		= qmk_start;
	input_dev->close	= qmk_stop;

	err = qmk_build_keymap(pdata->keymap_data, "qmk,keymap",
					 pdata->num_layers,
					 pdata->num_row_gpios,
					 pdata->num_col_gpios,
					 NULL, input_dev);
	if (err) {
		dev_err(&pdev->dev, "failed to build keymap\n");
		goto err_free_mem;
	}

	if (!pdata->no_autorepeat)
		__set_bit(EV_REP, input_dev->evbit);
	input_set_capability(input_dev, EV_MSC, MSC_SCAN);
	input_set_drvdata(input_dev, keyboard);

	err = qmk_init_gpio(pdev, keyboard);
	if (err)
		goto err_free_mem;

	err = input_register_device(keyboard->input_dev);
	if (err)
		goto err_free_gpio;

	device_init_wakeup(&pdev->dev, pdata->wakeup);
	platform_set_drvdata(pdev, keyboard);

	err = sysfs_create_group(&pdev->dev.kobj, &qmk_group);
    if (err) {
        dev_err(&pdev->dev, "sysfs creation failed\n");
        return err;
    }

	return 0;

err_free_gpio:
	qmk_free_gpio(keyboard);
err_free_mem:
	input_free_device(input_dev);
	kfree(keyboard);
	return err;
}

static int qmk_remove(struct platform_device *pdev)
{
	struct qmk *keyboard = platform_get_drvdata(pdev);

	qmk_free_gpio(keyboard);
	input_unregister_device(keyboard->input_dev);
	kfree(keyboard);

    sysfs_remove_group(&pdev->dev.kobj, &qmk_group);


	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id qmk_dt_match[] = {
	{ .compatible = "qmk" },
	{ }
};
MODULE_DEVICE_TABLE(of, qmk_dt_match);
#endif

static struct platform_driver qmk_driver = {
	.probe		= qmk_probe,
	.remove		= qmk_remove,
	.driver		= {
		.name	= "qmk",
		.pm	= &qmk_pm_ops,
		.of_match_table = of_match_ptr(qmk_dt_match),
	},
};
module_platform_driver(qmk_driver);

MODULE_AUTHOR("Marek Vasut <marek.vasut@gmail.com>, Jack Humbert <jack.humb@gmail.com>");
MODULE_DESCRIPTION("QMK Feature Support For GPIO Driven Keyboards");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:qmk");
