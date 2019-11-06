#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <linux/usb/ch9.h>
#include <usbg/function/hid.h>
#include <usbg/function/midi.h>
#include "HIDReportData.h"
#include "qmk_gadget.h"

#define HID_REPORT_SIZE 16

usbg_state *s;
usbg_gadget *g;
usbg_config *c;
usbg_function *f_hid;

int hid_output;

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

struct usbg_gadget_attrs g_attrs = {
	.bcdUSB = 0x0111,
	.bDeviceClass = USB_CLASS_PER_INTERFACE,
	.bDeviceSubClass = 0x00,
	.bDeviceProtocol = 0x00,
	.bMaxPacketSize0 = 64, /* Max allowed ep0 packet size */
	.idVendor = 0x1d6b,
	.idProduct = 0x0104,
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

int gadget_write(unsigned char *buf) {
    hid_output = open("/dev/hidg0", O_WRONLY | O_NDELAY);
    write(hid_output, buf, HID_REPORT_SIZE);
    close(hid_output);
}

int gadget_close(char *name)
{
	int usbg_ret;

	usbg_ret = usbg_init("/sys/kernel/config", &s);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error on usbg init\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
			usbg_strerror(usbg_ret));
		goto close_out1;
	}

	g = usbg_get_gadget(s, name);
	usbg_ret = usbg_disable_gadget(g);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error disabling gadget\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
			usbg_strerror(usbg_ret));
		goto close_out2;
	}

	usbg_ret = usbg_rm_gadget(g, USBG_RM_RECURSE);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error removing gadget\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
			usbg_strerror(usbg_ret));
		goto close_out2;
	}

close_out2:
	usbg_cleanup(s);
close_out1:
	return usbg_ret;
}

int gadget_open(char *name, struct qmk_gadget_cfg *cfg)
{
	int usbg_ret;

	usbg_ret = usbg_init("/sys/kernel/config", &s);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error on usbg init\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
			usbg_strerror(usbg_ret));
		goto open_out1;
	}

    // g_attrs.idVendor = cfg->vendor.id;
    // g_attrs.idProduct = cfg->product.id;
    // strcpy(g_strs.manufacturer, cfg->vendor.str);
    // strcpy(g_strs.product, cfg->product.str);

	usbg_ret = usbg_create_gadget(s, name, &g_attrs, &g_strs, &g);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error creating gadget\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
			usbg_strerror(usbg_ret));
		goto open_out2;
	}

	usbg_ret =
		usbg_create_function(g, USBG_F_HID, "usb0", &f_attrs, &f_hid);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error creating function: USBG_F_HID\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
			usbg_strerror(usbg_ret));
		goto open_out3;
	}

	usbg_ret = usbg_create_config(g, 1, "config", &c_attrs, &c_strs, &c);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error creating config\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
			usbg_strerror(usbg_ret));
		goto open_out3;
	}

	usbg_ret = usbg_add_config_function(c, "keyboard", f_hid);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error adding function: keyboard\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
			usbg_strerror(usbg_ret));
		goto open_out3;
	}

	usbg_ret = usbg_enable_gadget(g, DEFAULT_UDC);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error enabling gadget\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
			usbg_strerror(usbg_ret));
		goto open_out3;
	}

	usbg_cleanup(s);

	return 0;
open_out3:
	usbg_rm_gadget(g, USBG_RM_RECURSE);
open_out2:
	usbg_cleanup(s);
open_out1:
	return usbg_ret;
}