#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <linux/usb/ch9.h>
#include <usbg/usbg.h>
#include <usbg/function/hid.h>
#include <usbg/function/midi.h>
#include "HIDReportData.h"

#include <stdlib.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
// #include <ncurses.h>

#define MOD_NONE 0
#define MOD_LCTRL 1 << 0
#define MOD_LSHIFT 1 << 1
#define MOD_LALT 1 << 2
#define MOD_LSUPER 1 << 3
#define MOD_RCTRL 1 << 4
#define MOD_RSHIFT 1 << 5
#define MOD_RALT 1 << 6
#define MOD_RSUPER 1 << 7

#define VENDOR 0x1d6b
#define PRODUCT 0x0104
#define HID_REPORT_SIZE 16

/* Protocol family, consistent in both kernel prog and user prog. */
#define MYPROTO NETLINK_USERSOCK
/* Multicast group, consistent in both kernel prog and user prog. */
#define MYMGRP 1

#define MSG_GENERIC 0x01
#define KEYCODE_HID 0x02

usbg_state *s;
usbg_gadget *g;
usbg_config *c;
usbg_function *f_hid;
usbg_function *f_midi;
usbg_function *f_acm0;

int hid_output;

static char keycode_to_string[256][18] = {
	"KC_NO",
	"KC_ROLL_OVER",
	"KC_POST_FAIL",
	"KC_UNDEFINED",
	"KC_A",
	"KC_B",
	"KC_C",
	"KC_D",
	"KC_E",
	"KC_F",
	"KC_G",
	"KC_H",
	"KC_I",
	"KC_J",
	"KC_K",
	"KC_L",
	"KC_M",
	"KC_N",
	"KC_O",
	"KC_P",
	"KC_Q",
	"KC_R",
	"KC_S",
	"KC_T",
	"KC_U",
	"KC_V",
	"KC_W",
	"KC_X",
	"KC_Y",
	"KC_Z",
	"KC_1",
	"KC_2",
	"KC_3",
	"KC_4",
	"KC_5",
	"KC_6",
	"KC_7",
	"KC_8",
	"KC_9",
	"KC_0",
	"KC_ENTER",
	"KC_ESCAPE",
	"KC_BSPACE",
	"KC_TAB",
	"KC_SPACE",
	"KC_MINUS",
	"KC_EQUAL",
	"KC_LBRACKET",
	"KC_RBRACKET",
	"KC_BSLASH",
	"KC_NONUS_HASH",
	"KC_SCOLON",
	"KC_QUOTE",
	"KC_GRAVE",
	"KC_COMMA",
	"KC_DOT",
	"KC_SLASH",
	"KC_CAPSLOCK",
	"KC_F1",
	"KC_F2",
	"KC_F3",
	"KC_F4",
	"KC_F5",
	"KC_F6",
	"KC_F7",
	"KC_F8",
	"KC_F9",
	"KC_F10",
	"KC_F11",
	"KC_F12",
	"KC_PSCREEN",
	"KC_SCROLLLOCK",
	"KC_PAUSE",
	"KC_INSERT",
	"KC_HOME",
	"KC_PGUP",
	"KC_DELETE",
	"KC_END",
	"KC_PGDOWN",
	"KC_RIGHT",
	"KC_LEFT",
	"KC_DOWN",
	"KC_UP",
	"KC_NUMLOCK",
	"KC_KP_SLASH",
	"KC_KP_ASTERISK",
	"KC_KP_MINUS",
	"KC_KP_PLUS",
	"KC_KP_ENTER",
	"KC_KP_1",
	"KC_KP_2",
	"KC_KP_3",
	"KC_KP_4",
	"KC_KP_5",
	"KC_KP_6",
	"KC_KP_7",
	"KC_KP_8",
	"KC_KP_9",
	"KC_KP_0",
	"KC_KP_DOT",
	"KC_NONUS_BSLASH",
	"KC_APPLICATION",
	"KC_POWER",
	"KC_KP_EQUAL",
	"KC_F13",
	"KC_F14",
	"KC_F15",
	"KC_F16",
	"KC_F17",
	"KC_F18",
	"KC_F19",
	"KC_F20",
	"KC_F21",
	"KC_F22",
	"KC_F23",
	"KC_F24",
	"KC_EXECUTE",
	"KC_HELP",
	"KC_MENU",
	"KC_SELECT",
	"KC_STOP",
	"KC_AGAIN",
	"KC_UNDO",
	"KC_CUT",
	"KC_COPY",
	"KC_PASTE",
	"KC_FIND",
	"KC__MUTE",
	"KC__VOLUP",
	"KC__VOLDOWN",
	"KC_LOCKING_CAPS",
	"KC_LOCKING_NUM",
	"KC_LOCKING_SCROLL",
	"KC_KP_COMMA",
	"KC_KP_EQUAL_AS400",
	"KC_INT1",
	"KC_INT2",
	"KC_INT3",
	"KC_INT4",
	"KC_INT5",
	"KC_INT6",
	"KC_INT7",
	"KC_INT8",
	"KC_INT9",
	"KC_LANG1",
	"KC_LANG2",
	"KC_LANG3",
	"KC_LANG4",
	"KC_LANG5",
	"KC_LANG6",
	"KC_LANG7",
	"KC_LANG8",
	"KC_LANG9",
	"KC_ALT_ERASE",
	"KC_SYSREQ",
	"KC_CANCEL",
	"KC_CLEAR",
	"KC_PRIOR",
	"KC_RETURN",
	"KC_SEPARATOR",
	"KC_OUT",
	"KC_OPER",
	"KC_CLEAR_AGAIN",
	"KC_CRSEL",
	"KC_EXSEL", // 164
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"IDK",
	"KC_LCTRL", // 224
	"KC_LSHIFT",
	"KC_LALT",
	"KC_LGUI",
	"KC_RCTRL",
	"KC_RSHIFT",
	"KC_RALT",
	"KC_RGUI",
};

static int sig_flag = 1;
static bool keys_down[256] = { 0 };

static char report_desc[] = {
	HID_RI_USAGE_PAGE(8, 0x01), // Generic Desktop
	HID_RI_USAGE(8, 0x06), // Keyboard
	HID_RI_COLLECTION(8, 0x01), // Application

	HID_RI_REPORT_ID(8, 0x01), // REPORT_ID (1)

	// Modifiers (8 bits)

	HID_RI_USAGE_PAGE(8, 0x07), // Keyboard/Keypad
	HID_RI_USAGE_MINIMUM(8, 0xE0), // Keyboard Left Control
	HID_RI_USAGE_MAXIMUM(8, 0xE7), // Keyboard Right GUI
	HID_RI_LOGICAL_MINIMUM(8, 0x00),
	HID_RI_LOGICAL_MAXIMUM(8, 0x01),
	HID_RI_REPORT_COUNT(8, 0x08),
	HID_RI_REPORT_SIZE(8, 0x01),
	HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),

	// Reserved (1 byte)

	HID_RI_REPORT_COUNT(8, 0x01),
	HID_RI_REPORT_SIZE(8, 0x08),
	HID_RI_INPUT(8, HID_IOF_CONSTANT),

	// Keycodes (6 bytes)

	HID_RI_USAGE_PAGE(8, 0x07), // Keyboard/Keypad
	HID_RI_USAGE_MINIMUM(8, 0x00),
	HID_RI_USAGE_MAXIMUM(8, 0xFF),
	HID_RI_LOGICAL_MINIMUM(8, 0x00),
	HID_RI_LOGICAL_MAXIMUM(16, 0x00FF),
	// HID_RI_REPORT_COUNT(8, 0x06),
	HID_RI_REPORT_COUNT(8, 13), // used by gadget-hid - this made it work
	HID_RI_REPORT_SIZE(8, 0x08),
	HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_ARRAY | HID_IOF_ABSOLUTE),

	// Status LEDs (5 bits)

	HID_RI_USAGE_PAGE(8, 0x08), // LED
	HID_RI_USAGE_MINIMUM(8, 0x01), // Num Lock
	HID_RI_USAGE_MAXIMUM(8, 0x05), // Kana
	HID_RI_REPORT_COUNT(8, 0x05),
	HID_RI_REPORT_SIZE(8, 0x01),
	HID_RI_OUTPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE |
				 HID_IOF_NON_VOLATILE),

	// LED padding (3 bits)

	HID_RI_REPORT_COUNT(8, 0x01),
	HID_RI_REPORT_SIZE(8, 0x03),
	HID_RI_OUTPUT(8, HID_IOF_CONSTANT),
	HID_RI_END_COLLECTION(0),

	// Extra keys

	HID_RI_USAGE_PAGE(8, 0x01), // Generic Desktop
	HID_RI_USAGE(8, 0x80), // System Control
	HID_RI_COLLECTION(8, 0x01), // Application
	HID_RI_REPORT_ID(8, 0x03),
	HID_RI_USAGE_MINIMUM(16, 0x0081), // System Power Down
	HID_RI_USAGE_MAXIMUM(16, 0x0083), // System Wake Up
	HID_RI_LOGICAL_MINIMUM(16, 0x0001),
	HID_RI_LOGICAL_MAXIMUM(16, 0x0003),
	HID_RI_REPORT_COUNT(8, 1),
	HID_RI_REPORT_SIZE(8, 16),
	HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_ARRAY | HID_IOF_ABSOLUTE),
	HID_RI_END_COLLECTION(0),

	HID_RI_USAGE_PAGE(8, 0x0C), // Consumer
	HID_RI_USAGE(8, 0x01), // Consumer Control
	HID_RI_COLLECTION(8, 0x01), // Application
	HID_RI_REPORT_ID(8, 0x04),
	HID_RI_USAGE_MINIMUM(16, 0x0001), // Consumer Control
	HID_RI_USAGE_MAXIMUM(16, 0x029C), // AC Distribute Vertically
	HID_RI_LOGICAL_MINIMUM(16, 0x0001),
	HID_RI_LOGICAL_MAXIMUM(16, 0x029C),
	HID_RI_REPORT_COUNT(8, 1),
	HID_RI_REPORT_SIZE(8, 16),
	HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_ARRAY | HID_IOF_ABSOLUTE),
	HID_RI_END_COLLECTION(0),
};

int open_netlink(void)
{
	int sock;
	struct sockaddr_nl addr;
	int group = MYMGRP;

	sock = socket(AF_NETLINK, SOCK_RAW, MYPROTO);
	if (sock < 0) {
		printf("sock < 0.\n");
		return sock;
	}

	memset((void *)&addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_pid = getpid();
	/* This doesn't work for some reason. See the setsockopt() below. */
	// addr.nl_groups = MYMGRP;

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		printf("bind < 0.\n");
		return -1;
	}

	/*
     * 270 is SOL_NETLINK. See
     * http://lxr.free-electrons.com/source/include/linux/socket.h?v=4.1#L314
     * and
     * http://stackoverflow.com/questions/17732044/
     */
	if (setsockopt(sock, 270, NETLINK_ADD_MEMBERSHIP, &group,
		       sizeof(group)) < 0) {
		printf("setsockopt < 0\n");
		return -1;
	}

	int status = fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);

	if (status == -1) {
		perror("calling fcntl");
		// handle the error.  By the way, I've never seen fcntl fail in this way
	}

	return sock;
}

void read_event(int sock)
{
	struct sockaddr_nl nladdr;
	struct msghdr msg;
	struct iovec iov;
	char buffer[65536];
	int ret, i;

	iov.iov_base = (void *)buffer;
	iov.iov_len = sizeof(buffer);
	msg.msg_name = (void *)&(nladdr);
	msg.msg_namelen = sizeof(nladdr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	ret = recvmsg(sock, &msg, 0);
	if (ret >= 0) {
		char *input = NLMSG_DATA((struct nlmsghdr *)&buffer);
		if (input[0] == MSG_GENERIC) {
			input++;
			printf("\033[0;33m%s\033[0m\n", input);
		}
		if (input[0] == KEYCODE_HID) {
			input++;
			uint8_t ch = (uint8_t)input[0];
			bool pressed = (bool)input[1];

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
			// buf[3] = input[0]; // keycode
			for (x = 3; (x < 16); x++) {
				buf[x] = key_list[x - 3]; // keycodes
			}

			hid_output = open("/dev/hidg0", O_WRONLY | O_NDELAY);
			write(hid_output, buf, HID_REPORT_SIZE);
			close(hid_output);
		}
	}
}

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

	hid_output = open("/dev/hidg0", O_WRONLY | O_NDELAY);
	write(hid_output, buf, HID_REPORT_SIZE);
	buf[3] = 0;
	usleep(1000);
	write(hid_output, buf, HID_REPORT_SIZE);
	close(hid_output);
}

int main(int argc, char *argv[])
{
	int ret = -EINVAL;
	int usbg_ret;
	int nls;

	if (argc == 2 && strcmp(argv[1], "test") == 0) {
		send_test();
		return ret;
	}

	if ((argc == 2) &&
	    (strcmp(argv[1], "open") == 0 || strcmp(argv[1], "close") == 0)) {
		struct usbg_gadget_attrs g_attrs = {
			.bcdUSB = 0x0111,
			.bDeviceClass = USB_CLASS_PER_INTERFACE,
			.bDeviceSubClass = 0x00,
			.bDeviceProtocol = 0x00,
			.bMaxPacketSize0 = 64, /* Max allowed ep0 packet size */
			.idVendor = VENDOR,
			.idProduct = PRODUCT,
			.bcdDevice = 0x0001, /* Verson of device */
		};

		struct usbg_gadget_strs g_strs = {
			.serial = "234879", /* Serial number */
			.manufacturer = "OLKB", /* Manufacturer */
			.product = "PlanckenPi" /* Product string */
		};

		// https://www.beyondlogic.org/usbnutshell/usb5.shtml
		struct usbg_config_attrs c_attrs = {
			.bmAttributes = 0b10000000,
			.bMaxPower = 500,
		};

		struct usbg_config_strs c_strs = { .configuration = "1xHID" };

		struct usbg_f_midi_attrs midi_attrs = { .index = 1,
							.id = "usb1",
							.buflen = 128,
							.qlen = 16,
							.in_ports = 1,
							.out_ports = 1 };

		struct usbg_f_hid_attrs f_attrs = {
			.protocol = 1,
			.report_desc =
				{
					.desc = report_desc,
					.len = sizeof(report_desc),
				},
			.report_length = 16,
			.subclass = 0,
		};

		usbg_ret = usbg_init("/sys/kernel/config", &s);
		if (usbg_ret != USBG_SUCCESS) {
			fprintf(stderr, "Error on usbg init\n");
			fprintf(stderr, "Error: %s : %s\n",
				usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
			goto out1;
		}

		if (argc == 2 && strcmp(argv[1], "close") == 0) {
			g = usbg_get_gadget(s, "g1");
			usbg_ret = usbg_disable_gadget(g);
			if (usbg_ret != USBG_SUCCESS) {
				fprintf(stderr, "Error disabling gadget\n");
				fprintf(stderr, "Error: %s : %s\n",
					usbg_error_name(usbg_ret),
					usbg_strerror(usbg_ret));
				goto out2;
			}
			usbg_ret = usbg_rm_gadget(g, USBG_RM_RECURSE);
			if (usbg_ret != USBG_SUCCESS) {
				fprintf(stderr, "Error on removing gadget\n");
				fprintf(stderr, "Error: %s : %s\n",
					usbg_error_name(usbg_ret),
					usbg_strerror(usbg_ret));
				goto out2;
			}
			ret = 0;
			goto out2;
		}

		usbg_ret = usbg_create_gadget(s, "g1", &g_attrs, &g_strs, &g);
		if (usbg_ret != USBG_SUCCESS) {
			fprintf(stderr, "Error creating gadget\n");
			fprintf(stderr, "Error: %s : %s\n",
				usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
			goto out2;
		}

		// usbg_ret = usbg_create_function(g, USBG_F_ACM, "usb0", NULL, &f_acm0);
		// if (usbg_ret != USBG_SUCCESS) {
		//     fprintf(stderr, "Error creating function\n");
		//     fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
		//             usbg_strerror(usbg_ret));
		//     goto out3;
		// }

		usbg_ret = usbg_create_function(g, USBG_F_HID, "usb0", &f_attrs,
						&f_hid);
		if (usbg_ret != USBG_SUCCESS) {
			fprintf(stderr,
				"Error creating function: USBG_F_HID\n");
			fprintf(stderr, "Error: %s : %s\n",
				usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
			goto out3;
		}

		// usbg_ret = usbg_create_function(g, USBG_F_MIDI, "usb0", &midi_attrs, &f_midi);
		// if (usbg_ret != USBG_SUCCESS) {
		//     fprintf(stderr, "Error creating function: USBG_F_MIDI\n");
		//     fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
		//             usbg_strerror(usbg_ret));
		//     goto out3;
		// }

		usbg_ret = usbg_create_config(g, 1, "config", &c_attrs, &c_strs,
					      &c);
		// usbg_ret = usbg_create_config(g, 1, "config", NULL, &c_strs, &c);
		if (usbg_ret != USBG_SUCCESS) {
			fprintf(stderr, "Error creating config\n");
			fprintf(stderr, "Error: %s : %s\n",
				usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
			goto out3;
		}

		// usbg_ret = usbg_add_config_function(c, "acm.GS0", f_acm0);
		// if (usbg_ret != USBG_SUCCESS) {
		//     fprintf(stderr, "Error adding function ecm.GS0\n");
		//     fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
		//             usbg_strerror(usbg_ret));
		//     goto out3;
		// }

		usbg_ret = usbg_add_config_function(c, "keyboard", f_hid);
		if (usbg_ret != USBG_SUCCESS) {
			fprintf(stderr, "Error adding function: keyboard\n");
			fprintf(stderr, "Error: %s : %s\n",
				usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
			goto out3;
		}

		// usbg_ret = usbg_add_config_function(c, "midi", f_midi);
		// if (usbg_ret != USBG_SUCCESS) {
		//     fprintf(stderr, "Error adding function: midi\n");
		//     fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
		//             usbg_strerror(usbg_ret));
		//     goto out3;
		// }

		usbg_ret = usbg_enable_gadget(g, DEFAULT_UDC);
		if (usbg_ret != USBG_SUCCESS) {
			fprintf(stderr, "Error enabling gadget\n");
			fprintf(stderr, "Error: %s : %s\n",
				usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
			goto out3;
		}

		usbg_cleanup(s);

		return 0;
	out3:
		usbg_rm_gadget(g, USBG_RM_RECURSE);
	out2:
		usbg_cleanup(s);
	out1:
		return ret;

	} else {
		nls = open_netlink();
		if (nls < 0) {
			return nls;
		}

		signal(SIGINT, interrupt_signal);

		printf("Bound, listening.\n");
		while (sig_flag) {
			read_event(nls);
		}

		close(nls);

		return 0;
	}
}