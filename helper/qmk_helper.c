#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <unistd.h>
// #include <ncurses.h>

#include <qmk/keycodes/strings.h>
#include <qmk/keycodes/basic.h>
#include "qmk_gadget.h"
#include "qmk_socket.h"
#include "qmk_socket_listener.h"

#define MOD_NONE 0
#define MOD_LCTRL 1 << 0
#define MOD_LSHIFT 1 << 1
#define MOD_LALT 1 << 2
#define MOD_LSUPER 1 << 3
#define MOD_RCTRL 1 << 4
#define MOD_RSHIFT 1 << 5
#define MOD_RALT 1 << 6
#define MOD_RSUPER 1 << 7

static int sig_flag = 1;
static bool keys_down[REPORT_ID_MAX][256] = { 0 };
static uint8_t mods = 0;

void interrupt_signal(int sig)
{
	sig_flag = 0;
}

void send_test()
{
	int x;
	unsigned char buf[16];
	buf[0] = 1; // report id
	buf[1] = 0; // mods
	buf[2] = 0; // padding
	buf[3] = 4; // keycode
	for (x = 4; x < 16; x++) {
		buf[x] = 0;
	}
	gadget_write_u8(buf);

	buf[3] = 0;
	usleep(1000);
	gadget_write_u8(buf);
}

void handle_message(uint8_t *msg)
{
	int i;
    uint8_t ch, row, col;
    bool pressed;

	if (msg[0] == HANDSHAKE) {
		msg++;
		printf("\033[0;33m%s\033[0m\n", msg);
	} else if (msg[0] == MSG_GENERIC) {
		msg++;
		printf("\033[0;33m%s\033[0m\n", msg);
	} else if (msg[0] == KEYCODE_HID) {
		msg++;
		uint8_t ch = (uint8_t)msg[0];
		bool pressed = (bool)msg[1];

		if (pressed) {
			printf("\033[1;32mHID keycode pressed:  (0x%.2X) %s\033[0m\n",
			       ch, keycode_to_string[ch]);
		} else {
			printf("\033[0;32mHID keycode released: (0x%.2X) %s\033[0m\n",
			       ch, keycode_to_string[ch]);
		}
	}

    uint8_t *end = msg + msg[0];
    msg++;

    while (msg < end) {
        switch (msg[0]) {
        case MATRIX_EVENT:
            row = msg[1];
            col = msg[2];
            pressed = (bool)msg[3];

            if (pressed) {
                printf("\033[1;34mMatrix event down:    (%d, %d)\033[0m\n",
                       row, col);
            } else {
                printf("\033[0;34mMatrix event up:      (%d, %d)\033[0m\n",
                       row, col);
            }
            msg += 4;
            break;
        case LAYER_STATE:
            msg += 3;
            break;
        case ACTIVE_LAYER:
            msg += 2;
            break;
        case USB_PASSTHROUGH:
            if (msg[1])
                printf("\033[0;33mUSD Passthrough Enabled\033[0m\n");
            else
                printf("\033[0;33mUSD Passthrough Disabled\033[0m\n");
            msg += 2;
            break;
        case KEYCODE_HID:
            ch = msg[1];
            pressed = (bool)msg[2];

            if (pressed) {
                printf("\033[1;32mHID keycode pressed:  (0x%.2X) %s\033[0m\n",
                       ch, keycode_to_string[ch]);
            } else {
                printf("\033[0;32mHID keycode released: (0x%.2X) %s\033[0m\n",
                       ch, keycode_to_string[ch]);
            }
            msg += 3;
            break;
        default:
            msg++;
            break;
        }
    }

}

void handle_daemon_message(uint8_t *msg)
{
	int i, report, x;
    uint8_t buf_u8[16] = { 0 };
    uint16_t buf_u16[16] = { 0 };
	bool key_change[REPORT_ID_MAX] = { false };
	uint8_t key_list[REPORT_ID_MAX][256] = { { 0 } };
	uint8_t key_count[REPORT_ID_MAX] = { 0 };

	uint8_t *end = msg + msg[0];
	msg++;

	while (msg < end) {
		switch (msg[0]) {
		case MATRIX_EVENT:
			msg += 1;
		case LAYER_STATE:
			msg += 1;
		case ACTIVE_LAYER:
		case USB_PASSTHROUGH:
			msg += 2;
			break;
		case KEYCODE_HID:
			msg++;
			uint8_t ch = msg[0];
			bool pressed = (bool)msg[1];
			msg += 2;

			// remap mods for jack's keyboard
			// would be nice to have this configurable somehow
			// maybe a mapping of keycodes sent to the host?
			switch (ch) {
			case KC_LCTL:
				ch = KC_LGUI;
				break;
			case KC_LGUI:
				ch = KC_LCTL;
				break;
			default:
				break;
			}

			switch (ch) {
			case 0xE0 ... 0xE7:
				key_change[REPORT_ID_KEYBOARD] = true;
				if (pressed)
					mods |= (1 << (ch & 0x7));
				else
					mods &= ~(1 << (ch & 0x7));
				break;
			case KC_PWR ... KC_WAKE:
				key_change[REPORT_ID_SYSTEM] = true;
				keys_down[REPORT_ID_SYSTEM][KEYCODE2SYSTEM(ch)] =
					pressed;
				break;
			case KC_MUTE ... KC_BRID:
				key_change[REPORT_ID_CONSUMER] = true;
				keys_down[REPORT_ID_CONSUMER]
					 [KEYCODE2CONSUMER(ch)] = pressed;
				break;
			default:
				key_change[REPORT_ID_KEYBOARD] = true;
				keys_down[REPORT_ID_KEYBOARD][ch] = pressed;
				break;
			}
			break;
		default:
			msg++;
			break;
		}
	}

	// a lot of this logic could be moved to libqmk

	for (report = 0; report < REPORT_ID_MAX; report++) {
		if (key_change[report]) {
			for (i = 0; i < 256; i++) {
				if (keys_down[report][i]) {
					key_list[report][key_count[report]++] =
						i;
				}
			}

			switch (report) {
			case REPORT_ID_KEYBOARD:
				buf_u8[0] = REPORT_ID_KEYBOARD; // report id
				buf_u8[1] = mods; // mods
				// start at 3 since 2 is padding
				for (x = 3; x < 16; x++) {
					buf_u8[x] = key_list[report]
							 [x - 3]; // keycodes
				}
                gadget_write_u8(buf_u8);
				break;
			case REPORT_ID_SYSTEM:
			case REPORT_ID_CONSUMER:
				for (x = 0; x < 16; x++) {
					buf_u16[x] =
						key_list[report][x]; // keycodes
				}
				break;
                gadget_write_u16(buf_u16);
			}
			// don't print this out unless you're debugging - it'll be logged
			// printf("%.16X\n", buf_u8);
			// printf("Mod state: %.2X\n", mods);
		}
	}
}

struct qmk_gadget_cfg cfg = {
	.vendor =
		{
			.id = 0x0838,
			.str = "OLKB",
		},
	.product =
		{
			.id = 0x0737,
			.str = "PlanckenPi",
		},
};

static void __attribute__((noreturn)) usage(char *name)
{
	fprintf(stderr, "Usage: %s [-hdoct]\n", name);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int nls, c;
	// signal(SIGINT, interrupt_signal);

	while ((c = getopt(argc, argv, "hdoct:")) != EOF) {
		switch (c) {
		case 'h':
			usage(argv[0]);
			break;
		case 'd':
			gadget_open("g1", &cfg);

			nls = open_netlink();
			while (sig_flag) {
				read_message(nls, handle_daemon_message);
			}
			close(nls);
			exit(EXIT_SUCCESS);
			break;
		case 'o':
			exit(gadget_open("g1", &cfg));
			break;
		case 'c':
			exit(gadget_close("g1"));
			break;
		case 't':
			send_test();
			exit(EXIT_SUCCESS);
			break;
		default:
			usage(argv[0]);
			break;
		}
	}

	nls = open_netlink();
	send_message(nls, "hi!");
	while (sig_flag) {
		read_message(nls, handle_message);
		// refresh();
	}
	close(nls);
	// endwin();

	return EXIT_SUCCESS;
}
