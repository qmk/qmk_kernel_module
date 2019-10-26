#include "qmk.h"
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/delay.h>

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
        gpio_direction_output(pdata->col_gpios[col], !level_on);
        // gpio_set_value(pdata->col_gpios[col], !level_on);
        // if (!pdata->drive_inactive_cols)
            // gpio_direction_input(pdata->col_gpios[col]);
    }
}

static void activate_col(const struct qmk_platform_data *pdata,
             int col, bool on)
{
    __activate_col(pdata, col, on);

    if (on && pdata->col_scan_delay_us)
        udelay(pdata->col_scan_delay_us);
}

// static void activate_all_cols(const struct qmk_platform_data *pdata, bool on)
// {
//     int col;

//     for (col = 0; col < pdata->num_col_gpios; col++)
//         __activate_col(pdata, col, on);
// }

static bool row_asserted(const struct qmk_platform_data *pdata, int row)
{
    return gpio_get_value(pdata->row_gpios[row]) ?
            !pdata->active_low : pdata->active_low;
}

/*
 * This gets the keys from keyboard and reports it to input subsystem
 */
void qmk_scan(struct input_polled_dev *polled_dev)
{
	struct qmk *keyboard = polled_dev->private;
    struct input_dev *input = keyboard->input_dev;
    const struct qmk_platform_data *pdata = keyboard->pdata;
    uint32_t new_state[MATRIX_MAX_COLS];
    int row, col;
    bool pressed;

    /* de-activate all columns for scanning */
    // activate_all_cols(pdata, false);

    memset(new_state, 0, sizeof(new_state));

    /* assert each column and read the row status out */
    for (col = 0; col < pdata->num_col_gpios; col++) {

        activate_col(pdata, col, true);

        for (row = 0; row < pdata->num_row_gpios; row++) {
            // pinctrl_gpio_set_config(pdata->row_gpios[row], 
            //     PIN_CONFIG_BIAS_PULL_DOWN);
            new_state[col] |= row_asserted(pdata, row) ? (1 << row) : 0;
        }

        activate_col(pdata, col, false);
    }

    for (col = 0; col < pdata->num_col_gpios; col++) {
        uint32_t bits_changed;

        bits_changed = keyboard->last_key_state[col] ^ new_state[col];
        if (bits_changed != 0) {
            for (row = 0; row < pdata->num_row_gpios; row++) {
                if ((bits_changed & (1 << row))) {
                    pressed = new_state[col] & (1 << row);
                    qmk_process_keycode(keyboard, row, col, pressed);
                }
            }
        }
    }
    input_sync(input);

    memcpy(keyboard->last_key_state, new_state, sizeof(new_state));

    // activate_all_cols(pdata, true);

}
