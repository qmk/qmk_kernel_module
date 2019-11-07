/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _QMK_H
#define _QMK_H

#include <linux/types.h>
#include <linux/input.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <qmk/types.h>

#define MATRIX_MAX_LAYERS 16
#define MATRIX_MAX_ROWS 32
#define MATRIX_MAX_COLS 32

#define KEY(layer, row, col, val)                                              \
	((((layer) & (MATRIX_MAX_LAYERS 1)) << 26) |                           \
	 (((row) & (MATRIX_MAX_ROWS - 1)) << 21) |                             \
	 (((col) & (MATRIX_MAX_COLS - 1)) << 16) | ((val)&0xffff))

#define KEY_LAYER(k) (((k) >> 26) & 0xf)
#define KEY_ROW(k) (((k) >> 21) & 0x1f)
#define KEY_COL(k) (((k) >> 16) & 0x1f)
#define KEY_VAL(k) ((k)&0xffff)

#define QMK_MATRIX_SCAN_CODE(layer, row, col, layer_shift, row_shift)          \
	(((layer) << (layer_shift)) + ((row) << (row_shift)) + (col))

#define KEY_PRESSED 1
#define KEY_RELEASED 0
/**
 * struct matrix_keymap_data - keymap for matrix keyboards
 * @keymap: pointer to array of uint32 values encoded with KEY() macro
 *  representing keymap
 * @keymap_size: number of entries (initialized) in this keymap
 *
 * This structure is supposed to be used by platform code to supply
 * keymaps to drivers that implement matrix-like keypads/keyboards.
 */
struct matrix_keymap_data {
	const uint32_t *keymap;
	unsigned int keymap_size;
};

/**
 * struct qmk_module_platform_data - platform-dependent keypad data
 * @keymap_data: pointer to &matrix_keymap_data
 * @row_gpios: pointer to array of gpio numbers representing rows
 * @col_gpios: pointer to array of gpio numbers reporesenting colums
 * @num_layers: actual number of layers used by device
 * @num_row_gpios: actual number of row gpios used by device
 * @num_col_gpios: actual number of col gpios used by device
 * @col_scan_delay_us: delay, measured in microseconds, that is
 *  needed before we can keypad after activating column gpio
 * @debounce_ms: debounce interval in milliseconds
 * @clustered_irq: may be specified if interrupts of all row/column GPIOs
 *  are bundled to one single irq
 * @clustered_irq_flags: flags that are needed for the clustered irq
 * @active_low: gpio polarity
 * @wakeup: controls whether the device should be set up as wakeup
 *  source
 * @no_autorepeat: disable key autorepeat
 * @drive_inactive_cols: drive inactive columns during scan, rather than
 *  making them inputs.
 *
 * This structure represents platform-specific data that use used by
 * qmk driver to perform proper initialization.
 */
struct qmk_platform_data {
	const char *name;
	const struct matrix_keymap_data *keymap_data;

	const unsigned int *row_gpios;
	const unsigned int *col_gpios;

	unsigned int col_scan_delay_us;
	unsigned int poll_interval;

	/* key debounce interval in milli-second */
	unsigned int debounce_ms;

	unsigned int clustered_irq;
	unsigned int clustered_irq_flags;

	bool active_low;
	bool wakeup;
	bool no_autorepeat;
	bool drive_inactive_cols;
	int (*enable)(struct device *dev);
	void (*disable)(struct device *dev);

	unsigned char subclass;
	unsigned char protocol;
	unsigned short report_length;
	unsigned short report_desc_length;
	unsigned char report_desc[];
};

struct qmk_module {
	const struct qmk_platform_data *pdata;
	struct qmk_keyboard *keyboard;
	struct input_polled_dev *poll_dev;
	struct input_dev *input_dev;
	struct device *dev;
	unsigned int layer_shift;
	unsigned int row_shift;

	unsigned long layer_state;

	DECLARE_BITMAP(disabled_gpios, MATRIX_MAX_ROWS);

	uint32_t last_key_state[MATRIX_MAX_COLS];
	uint32_t current_key_state[MATRIX_MAX_COLS];
	struct delayed_work work;
	spinlock_t lock;
	bool scan_pending;
	bool stopped;
	bool gpio_all_disabled;
};

int send_socket_message_f(const char *fmt, ...);
void send_socket_message(uint8_t * msg, uint8_t msg_size);
int gadget_init(void);
void gadget_exit(void);

bool process_qkm(struct qmk_keyboard *keyboard, qmk_keycode_t *keycode,
		 bool pressed);

int qmk_init_gpio(struct platform_device *pdev, struct qmk_module *module);
void qmk_free_gpio(struct qmk_module *module);

struct attribute_group *get_qmk_group(void);
void qmk_scan(struct input_polled_dev *polled_dev);
int qmk_build_keymap(const struct matrix_keymap_data *keymap_data,
		     const char *keymap_name, unsigned int layers,
		     unsigned int rows, unsigned int cols,
		     unsigned short *keymap, struct input_dev *input_dev);
int qmk_parse_properties(struct device *dev, unsigned int *layers,
			 unsigned int *rows, unsigned int *cols);

#define qmk_parse_of_params qmk_parse_properties

#endif /* _QMK_H */
