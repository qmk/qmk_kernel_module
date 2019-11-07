#include "qmk.h"
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/input-polldev.h>
#include <linux/input.h>
#include <linux/pinctrl/consumer.h>
#include <qmk/keycodes/process.h>
#include <qmk/protocol.h>
#include <qmk/types.h>
#include "qmk_socket.h"

int qmk_init_gpio(struct platform_device *pdev, struct qmk_module *module)
{
	const struct qmk_platform_data *pdata = module->pdata;
	struct qmk_keyboard *keyboard = module->keyboard;
	int i, err;

	/* initialized strobe lines as outputs, activated */
	for (i = 0; i < keyboard->cols; i++) {
		err = gpio_request(pdata->col_gpios[i], "matrix_kbd_col");
		if (err) {
			dev_err(&pdev->dev,
				"failed to request GPIO%d for COL%d\n",
				pdata->col_gpios[i], i);
			goto err_free_cols;
		}

		gpio_direction_output(pdata->col_gpios[i], !pdata->active_low);
	}

	for (i = 0; i < keyboard->rows; i++) {
		err = pinctrl_gpio_request(pdata->row_gpios[i]);
		if (err) {
			dev_err(&pdev->dev,
				"failed to request pinctrl for GPIO%d on ROW%d\n",
				pdata->row_gpios[i], i);
			goto err_free_rows;
		}

		pinctrl_gpio_set_config(pdata->row_gpios[i],
					PIN_CONFIG_BIAS_PULL_DOWN);
		pinctrl_gpio_direction_input(pdata->row_gpios[i]);
	}

	return 0;

err_free_rows:
	while (--i >= 0)
		pinctrl_gpio_free(pdata->row_gpios[i]);
	i = keyboard->cols;
err_free_cols:
	while (--i >= 0)
		gpio_free(pdata->col_gpios[i]);

	return err;
}

void qmk_free_gpio(struct qmk_module *module)
{
	const struct qmk_platform_data *pdata = module->pdata;
	struct qmk_keyboard *keyboard = module->keyboard;
	int i;

	for (i = 0; i < keyboard->rows; i++)
		pinctrl_gpio_free(pdata->row_gpios[i]);

	for (i = 0; i < keyboard->cols; i++)
		gpio_free(pdata->col_gpios[i]);
}

/*
 * NOTE: If drive_inactive_cols is false, then the GPIO has to be put into
 * HiZ when de-activated to cause minmal side effect when scanning other
 * columns. In that case it is configured here to be input, otherwise it is
 * driven with the inactive value.
 */
static void activate_col(const struct qmk_platform_data *pdata, int col,
			 bool on)
{
	bool level_on = !pdata->active_low;

	if (on) {
		gpio_direction_output(pdata->col_gpios[col], level_on);
		if (pdata->col_scan_delay_us)
			udelay(pdata->col_scan_delay_us);
	} else {
		// gpio_direction_output(pdata->col_gpios[col], !level_on);
		gpio_set_value_cansleep(pdata->col_gpios[col], !level_on);
	}
}

static bool row_asserted(const struct qmk_platform_data *pdata, int row)
{
	return gpio_get_value(pdata->row_gpios[row]) ? !pdata->active_low :
						       pdata->active_low;
}

void qmk_analyze_state(struct qmk_module *module);

/*
 * This gets the keys from keyboard and reports it to input subsystem
 */
void qmk_scan(struct input_polled_dev *polled_dev)
{
	struct qmk_module *module = polled_dev->private;
	struct qmk_keyboard *keyboard = module->keyboard;
	const struct qmk_platform_data *pdata = module->pdata;
	int row, col;

	memset(module->current_key_state, 0, sizeof(module->current_key_state));

	/* assert each column and read the row status out */
	for (col = 0; col < keyboard->cols; col++) {
		activate_col(pdata, col, true);

		for (row = 0; row < keyboard->rows; row++) {
			module->current_key_state[col] |=
				row_asserted(pdata, row) ? (1 << row) : 0;
		}

		activate_col(pdata, col, false);
	}

	qmk_analyze_state(module);
}

void qmk_analyze_state(struct qmk_module *module)
{
	struct input_dev *input = module->input_dev;
	struct qmk_keyboard *keyboard = module->keyboard;
	int row, col;
	struct qmk_matrix_event *event;
	qmk_keycode_t keycode = 0;
	bool pressed, handled;

	uint8_t starting_layer = keyboard->active_layer;
	uint8_t starting_state = keyboard->layer_state;

	event = devm_kzalloc(&input->dev, sizeof(*event), GFP_KERNEL);

	for (col = 0; col < keyboard->cols; col++) {
		uint32_t bits_changed;

		bits_changed = module->last_key_state[col] ^
			       module->current_key_state[col];
		if (bits_changed != 0) {
			for (row = 0; row < keyboard->rows; row++) {
				if ((bits_changed & (1 << row))) {
					pressed =
						module->current_key_state[col] &
						(1 << row);
					event->row = row;
					event->col = col;
					event->pressed = pressed;
					send_socket_message((uint8_t[]){ MATRIX_EVENT, row, col, pressed }, 4);
					handled =
						process_keycode(keyboard, event,
								&keycode) ||
						process_qkm(keyboard, &keycode,
							    pressed);

					if (!handled) {
						dev_warn(
							&input->dev,
							"unhandled keycode: 0x%x",
							keycode);
					}
				}
			}
		}
	}
	input_sync(input);

	if (starting_layer != keyboard->active_layer)
		send_socket_message((uint8_t[]){ ACTIVE_LAYER, keyboard->active_layer }, 2);
	if (starting_state != keyboard->layer_state)
		send_socket_message((uint8_t[]){ LAYER_STATE, ((keyboard->layer_state >> 8) & 0xFF), (keyboard->layer_state & 0xFF) }, 3);

	memcpy(module->last_key_state, module->current_key_state,
	       sizeof(module->current_key_state));

	devm_kfree(&input->dev, event);
}
