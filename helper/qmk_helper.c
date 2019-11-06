#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <unistd.h>

#include <qmk/keycodes/strings.h>
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
static bool keys_down[256] = { 0 };

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
	gadget_write(buf);

	buf[3] = 0;
	usleep(1000);
	gadget_write(buf);
}

void handle_message(char * msg) {
    int i;

    if (msg[0] == HANDSHAKE) {
        msg++;
        printf("\033[0;33m%s\033[0m\n", msg);
    }
    if (msg[0] == MSG_GENERIC) {
        msg++;
        printf("\033[0;33m%s\033[0m\n", msg);
    }
    if (msg[0] == KEYCODE_HID) {
        msg++;
        uint8_t ch = (uint8_t)msg[0];
        bool pressed = (bool)msg[1];

        if (pressed) {
            printf("\033[0;32mHID keycode pressed:  (0x%.2X) %s\033[0m\n",
                   ch, keycode_to_string[ch]);
        } else {
            printf("\033[0;34mHID keycode released: (0x%.2X) %s\033[0m\n",
                   ch, keycode_to_string[ch]);
        }

        keys_down[ch] = pressed;

        uint8_t key_list[256] = { 0 };
        uint8_t key_count = 0;

        for (i = 0; i < 256; i++) {
            if (keys_down[i]) {
                key_list[key_count++] = i;
            }
        }

        int x;
        unsigned char buf[16];
        buf[0] = 1; // report id
        buf[1] = 0; // mods
        buf[2] = 0; // padding
        for (x = 3; (x < 16); x++) {
            buf[x] = key_list[x - 3]; // keycodes
        }

        gadget_write(buf);
    }
}

struct qmk_gadget_cfg cfg = {
    .vendor = {
        .id = 0x0838,
        .str = "OLKB",
    },
    .product = {
        .id = 0x0737,
        .str = "PlanckenPi",
    },
};

int main(int argc, char *argv[])
{
	int ret = -EINVAL;
	int nls;

	if (argc == 2) {
        if (strcmp(argv[1], "test") == 0) {
    		send_test();
    		return ret;
        }

    	if (strcmp(argv[1], "open") == 0) {
            gadget_open("g1", &cfg);
        }

        if (strcmp(argv[1], "close") == 0) {
            gadget_close("g1");
    	}
    }

	nls = open_netlink();
	if (nls < 0) {
		return nls;
	}

    send_message(nls, "hi!");

	signal(SIGINT, interrupt_signal);

	printf("Bound, listening.\n");
	while (sig_flag) {
		read_message(nls, handle_message);
	}

	close(nls);

	return 0;
}