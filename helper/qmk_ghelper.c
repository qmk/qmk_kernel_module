// https://github.com/rricharz/Raspberry-Pi-easy-drawing/blob/master/main.c

#include <stdio.h>
#include <string.h>
#include <cairo.h>
#include <gtk/gtk.h>

#include <qmk/keycodes/strings.h>
#include "qmk_gadget.h"
#include "qmk_socket_listener.h"
#include "qmk_socket.h"

#define WINDOW_WIDTH 800 // proposed width of main window
#define WINDOW_HEIGHT 480 // proposed height of main window
#define WINDOW_NAME "Pi-Train-Controler" // name of main window

#define ASSET_FOLDER

#define TIME_INTERVAL 10 // time interval for timer function in msec
// 0 means no timer function

guint global_timeout_ref;
GtkWidget *window;

static int nls;
static bool needs_update = true;
static bool first_run = true;
static uint8_t update_row, update_col;
static uint8_t active_layer = 0;
static uint16_t layer_state = 1;
static bool usb_passthrough = false;

struct qmk_key {
	bool pressed;
	uint8_t width;
	uint8_t x;
	uint8_t y;
	char legend[16][8];
};

#define KEYBOARD_ROWS 8
#define KEYBOARD_COLS 6

#define KEY_U(i) (i * 4)

static struct qmk_key planck_keys[KEYBOARD_ROWS][KEYBOARD_COLS] = {
	{
		{ 0, KEY_U(1), KEY_U(0), KEY_U(0), { "Tab", "", "" } },
		{ 0, KEY_U(1), KEY_U(1), KEY_U(0), { "Q", "!", "1" } },
		{ 0, KEY_U(1), KEY_U(2), KEY_U(0), { "W", "@", "2" } },
		{ 0, KEY_U(1), KEY_U(3), KEY_U(0), { "F", "#", "3" } },
		{ 0, KEY_U(1), KEY_U(4), KEY_U(0), { "P", "$", "4" } },
		{ 0, KEY_U(1), KEY_U(5), KEY_U(0), { "G", "%", "5" } },
	},
	{
		{ 0, KEY_U(1), KEY_U(0), KEY_U(1), { "Esc", "", "" } },
		{ 0, KEY_U(1), KEY_U(1), KEY_U(1), { "A", "F1", "F1" } },
		{ 0, KEY_U(1), KEY_U(2), KEY_U(1), { "R", "F2", "F2" } },
		{ 0, KEY_U(1), KEY_U(3), KEY_U(1), { "S", "F3", "F3" } },
		{ 0, KEY_U(1), KEY_U(4), KEY_U(1), { "T", "F4", "F4" } },
		{ 0, KEY_U(1), KEY_U(5), KEY_U(1), { "D", "F5", "F5" } },
	},
	{
		{ 0, KEY_U(1), KEY_U(0), KEY_U(2), { "Shift", "", "" } },
		{ 0, KEY_U(1), KEY_U(1), KEY_U(2), { "Z", "F7", "F7" } },
		{ 0, KEY_U(1), KEY_U(2), KEY_U(2), { "X", "F8", "F8" } },
		{ 0, KEY_U(1), KEY_U(3), KEY_U(2), { "C", "F9", "F9" } },
		{ 0, KEY_U(1), KEY_U(4), KEY_U(2), { "V", "F10", "F10" } },
		{ 0, KEY_U(1), KEY_U(5), KEY_U(2), { "B", "F11", "F11" } },
	},
	{
		{ 0, KEY_U(1), KEY_U(0), KEY_U(3), { "Fn", "", "" } },
		{ 0, KEY_U(1), KEY_U(1), KEY_U(3), { "Gui", "", "" } },
		{ 0, KEY_U(1), KEY_U(2), KEY_U(3), { "Alt", "", "" } },
		{ 0, KEY_U(1), KEY_U(9), KEY_U(3), { "Down", "", "" } },
		{ 0, KEY_U(1), KEY_U(10), KEY_U(3), { "Up", "", "" } },
		{ 0, KEY_U(1), KEY_U(11), KEY_U(3), { "Right", "", "" } },
	},
	{
		{ 0, KEY_U(1), KEY_U(6), KEY_U(0), { "J", "^", "6" } },
		{ 0, KEY_U(1), KEY_U(7), KEY_U(0), { "L", "&", "7" } },
		{ 0, KEY_U(1), KEY_U(8), KEY_U(0), { "U", "*", "8" } },
		{ 0, KEY_U(1), KEY_U(9), KEY_U(0), { "Y", "(", "9" } },
		{ 0, KEY_U(1), KEY_U(10), KEY_U(0), { ";", ")", "0" } },
		{ 0, KEY_U(1), KEY_U(11), KEY_U(0), { "Bksp", "", "" } },
	},
	{
		{ 0, KEY_U(1), KEY_U(6), KEY_U(1), { "H", "F6", "F6" } },
		{ 0, KEY_U(1), KEY_U(7), KEY_U(1), { "N", "_", "-" } },
		{ 0, KEY_U(1), KEY_U(8), KEY_U(1), { "E", "+", "=" } },
		{ 0, KEY_U(1), KEY_U(9), KEY_U(1), { "I", "{", "[" } },
		{ 0, KEY_U(1), KEY_U(10), KEY_U(1), { "O", "}", "]" } },
		{ 0, KEY_U(1), KEY_U(11), KEY_U(1), { "'", "|", "\\" } },
	},
	{
		{ 0, KEY_U(1), KEY_U(6), KEY_U(2), { "K", "F12", "F12" } },
		{ 0, KEY_U(1), KEY_U(7), KEY_U(2), { "M", "", "USB" } },
		{ 0, KEY_U(1), KEY_U(8), KEY_U(2), { ",", "", "" } },
		{ 0, KEY_U(1), KEY_U(9), KEY_U(2), { ".", "", "" } },
		{ 0, KEY_U(1), KEY_U(10), KEY_U(2), { "/", "", "" } },
		{ 0, KEY_U(1), KEY_U(11), KEY_U(2), { "Enter", "", "" } },
	},
	{
		{ 0,
		  KEY_U(2),
		  KEY_U(5),
		  KEY_U(3),
		  { "Space", "Space", "Space" } },
		{ 0,
		  KEY_U(1),
		  KEY_U(7),
		  KEY_U(3),
		  { "Raise", "Raise", "Raise" } },
		{ 0, KEY_U(1), KEY_U(8), KEY_U(3), { "Left", "", "" } },
		{ 0, KEY_U(1), KEY_U(3), KEY_U(3), { "Ctrl", "", "" } },
		{ 0,
		  KEY_U(1),
		  KEY_U(4),
		  KEY_U(3),
		  { "Lower", "Lower", "Lower" } },
	}
};

static void do_drawing(cairo_t *, GtkWidget *);

static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr,
			      gpointer user_data)
{
	do_drawing(cr, widget);
	return FALSE;
}

void handle_message(uint8_t *msg)
{
	int i;

	if (msg[0] == HANDSHAKE) {
		msg++;
		printf("\033[0;33m%s\033[0m\n", msg);
	} else if (msg[0] == MSG_GENERIC) {
		msg++;
		printf("\033[0;33m%s\033[0m\n", msg);
	} else if (msg[0] == KEYCODE_HID) {
		msg++;
		uint8_t ch = msg[0];
		bool pressed = (bool)msg[1];

		if (pressed) {
			printf("\033[1;32mHID keycode pressed:  (0x%.2X) %s\033[0m\n",
			       ch, keycode_to_string[ch]);
		} else {
			printf("\033[0;32mHID keycode released: (0x%.2X) %s\033[0m\n",
			       ch, keycode_to_string[ch]);
		}
	} else if (msg[0] == MATRIX_EVENT) {
		msg++;
		uint8_t row = msg[0];
		uint8_t col = msg[1];
		bool pressed = (bool)msg[2];

		if (pressed) {
			printf("\033[1;34mMatrix event down:    (%d, %d)\033[0m\n",
			       row, col);
		} else {
			printf("\033[0;34mMatrix event up:      (%d, %d)\033[0m\n",
			       row, col);
		}

		planck_keys[row][col].pressed = pressed;

		needs_update = true;
	} else if (msg[0] == ACTIVE_LAYER) {
		msg++;
		active_layer = msg[0];
		printf("\033[0;33mLayer state:              0x%.2X\033[0m\n",
		       active_layer);
		needs_update = true;
	} else if (msg[0] == LAYER_STATE) {
		msg++;
		layer_state = ((uint16_t)msg[0] << 8) | (msg[1]);
	} else if (msg[0] == USB_PASSTHROUGH) {
		msg++;
		usb_passthrough = (bool)msg[0];
	}
}

static gboolean on_timer_event(GtkWidget *widget)
{
	int i;
	bool change = false;

	g_source_remove(global_timeout_ref);

	// scan for 4 messages at a time
	for (i = 0; i < 4; i++)
		read_message(nls, handle_message);

	if (change || needs_update) {
		needs_update = false;
		gtk_widget_queue_draw(widget);
	}

	global_timeout_ref = g_timeout_add(
		TIME_INTERVAL, (GSourceFunc)on_timer_event, (gpointer)window);

	return TRUE;
}

static gboolean clicked(GtkWidget *widget, GdkEventButton *event,
			gpointer user_data)
{
	bool change = false;

	// if (application_clicked(event->button, event->x, event->y))

	if (change)
		gtk_widget_queue_draw(widget);

	return TRUE;
}

static gboolean key_press_event(GtkWidget *widget, GdkEventKey *event,
				gpointer data)
{
	if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_c)) {
		gtk_main_quit();
		return TRUE;
	}
	return FALSE;
}

static void on_quit_event()
{
	close(nls);
	gtk_main_quit();
}

struct {
	cairo_surface_t *usb_passthrough_enabled;
	cairo_surface_t *usb_passthrough_disabled;
} glob;

void cairo_rounded_rectangle(cairo_t *cr, double x, double y, double w,
			     double h, double r)
{
	double degrees = G_PI / 180.0;

	cairo_new_sub_path(cr);
	cairo_arc(cr, x + w - r, y + r, r, -90 * degrees, 0 * degrees);
	cairo_arc(cr, x + w - r, y + h - r, r, 0 * degrees, 90 * degrees);
	cairo_arc(cr, x + r, y + h - r, r, 90 * degrees, 180 * degrees);
	cairo_arc(cr, x + r, y + r, r, 180 * degrees, 270 * degrees);
	cairo_close_path(cr);
}

void cairo_circle(cairo_t *cr, double x, double y, double r)
{
	cairo_new_sub_path(cr);
	cairo_arc(cr, x, y, r, 0, 2 * G_PI);
	cairo_close_path(cr);
}

#define KEYDOWN_COLOR 92.0 / 255, 197.0 / 255, 76.0 / 255
#define KEYUP_COLOR 0.9, 0.9, 0.9

#define KEYBOARD_WIDTH_KU 12
#define KEYBOARD_HEIGHT_KU 4
#define SIDE_PADDING 10
#define KEY_PADDING 4
#define KU_SCALE (((width - (SIDE_PADDING * 2)) / KEY_U(KEYBOARD_WIDTH_KU)))
#define TOP_OFFSET                                                             \
	(((height) - (KEYBOARD_HEIGHT_KU * KU_SCALE)) / 2 - SIDE_PADDING)
#define LEGEND_SIZE 16
#define CIRCLE_RADIUS 20

static void do_drawing(cairo_t *cr, GtkWidget *widget)
{
	int r, c, l, width, height;
	char digit_str[2];
	char legend_str[8];
	cairo_text_extents_t extents;
	GtkWidget *win = gtk_widget_get_toplevel(widget);
	gtk_window_get_size(GTK_WINDOW(win), &width, &height);

	// draw

	for (l = 0; l < 16; l++) {
		sprintf(digit_str, "%d", l + 1);
		cairo_circle(cr,
			     l * ((width - (SIDE_PADDING * 2)) / (16)) +
				     (SIDE_PADDING * 2) + CIRCLE_RADIUS,
			     160, CIRCLE_RADIUS);
		if (layer_state & (1UL << l))
			cairo_set_source_rgb(cr, KEYDOWN_COLOR);
		else
			cairo_set_source_rgb(cr, KEYUP_COLOR);
		cairo_fill(cr);

		if (layer_state & (1UL << l))
			cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
		else
			cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
		cairo_select_font_face(cr, "Purisa", CAIRO_FONT_SLANT_NORMAL,
				       CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cr, LEGEND_SIZE);
		cairo_text_extents(cr, digit_str, &extents);
		cairo_move_to(cr,
			      l * ((width - (SIDE_PADDING * 2)) / (16)) +
				      (SIDE_PADDING * 2) + CIRCLE_RADIUS -
				      (extents.width / 2 + extents.x_bearing),
			      160 + (LEGEND_SIZE / 2) - 1);
		cairo_show_text(cr, digit_str);
		cairo_stroke(cr);
	}

	// srand(15);
	// if (first_run) {
	first_run = false;
	for (r = 0; r < KEYBOARD_ROWS; r++) {
		for (c = 0; c < KEYBOARD_COLS; c++) {
			if (planck_keys[r][c].width) {
				cairo_rectangle(
					cr,
					(planck_keys[r][c].x * KU_SCALE) +
						SIDE_PADDING +
						(KEY_PADDING * 2),
					(planck_keys[r][c].y * KU_SCALE) +
						TOP_OFFSET + (KEY_PADDING * 2),
					KU_SCALE * planck_keys[r][c].width -
						KEY_PADDING,
					KU_SCALE * 4 - KEY_PADDING);
				if (planck_keys[r][c].pressed)
					cairo_set_source_rgb(cr, KEYDOWN_COLOR);
				else
					cairo_set_source_rgb(cr, KEYUP_COLOR);
				cairo_fill(cr);

				if (strcmp(planck_keys[r][c]
						   .legend[active_layer],
					   "") == 0) {
					strcpy(legend_str,
					       planck_keys[r][c].legend[0]);
					if (planck_keys[r][c].pressed) {
						cairo_set_source_rgb(cr, 1.0,
								     1.0, 1.0);
					} else {
						cairo_set_source_rgb(cr, 0.8,
								     0.8, 0.8);
					}
				} else {
					strcpy(legend_str,
					       planck_keys[r][c]
						       .legend[active_layer]);
					if (planck_keys[r][c].pressed) {
						cairo_set_source_rgb(cr, 1.0,
								     1.0, 1.0);
					} else {
						cairo_set_source_rgb(cr, 0.2,
								     0.2, 0.2);
					}
				}
				cairo_select_font_face(
					cr, "Purisa", CAIRO_FONT_SLANT_NORMAL,
					CAIRO_FONT_WEIGHT_NORMAL);
				cairo_set_font_size(cr, LEGEND_SIZE);
				cairo_text_extents(cr, legend_str, &extents);
				cairo_move_to(cr,
					      (planck_keys[r][c].x * KU_SCALE) +
						      SIDE_PADDING +
						      (KEY_PADDING * 1.5) +
						      (KU_SCALE *
						       planck_keys[r][c].width /
						       2) -
						      (extents.width / 2 +
						       extents.x_bearing),
					      (planck_keys[r][c].y * KU_SCALE) +
						      TOP_OFFSET +
						      (KEY_PADDING * 1.5) +
						      (KU_SCALE * 2) +
						      (LEGEND_SIZE / 2) - 1);
				cairo_show_text(cr, legend_str);
				cairo_stroke(cr);
			}
		}
	}
	// } else {
	// 	cairo_rectangle(cr,
	// 			(planck_keys[update_row][update_col].x * KU_SCALE) +
	// 				SIDE_PADDING + (KEY_PADDING * 2),
	// 			(planck_keys[update_row][update_col].y * KU_SCALE) + TOP_OFFSET +
	// 				(KEY_PADDING * 2),
	// 			KU_SCALE * planck_keys[update_row][update_col].width -
	// 				KEY_PADDING,
	// 			KU_SCALE * 4 - KEY_PADDING);
	// 	if (planck_keys[update_row][update_col].pressed)
	// 		cairo_set_source_rgb(cr, KEYDOWN_COLOR);
	// 	else
	// 		cairo_set_source_rgb(cr, KEYUP_COLOR);
	// 	cairo_fill(cr);
	// }
	if (usb_passthrough)
		cairo_set_source_surface(cr, glob.usb_passthrough_enabled,
					 width - (SIDE_PADDING * 2) - 44,
					 SIDE_PADDING * 2);
	else
		cairo_set_source_surface(cr, glob.usb_passthrough_disabled,
					 width - (SIDE_PADDING * 2) - 44,
					 SIDE_PADDING * 2);
	cairo_paint(cr);

	// display text "click to erase"
	cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
	cairo_select_font_face(cr, "Purisa", CAIRO_FONT_SLANT_NORMAL,
			       CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 44);
	cairo_move_to(cr, SIDE_PADDING + KEY_PADDING, SIDE_PADDING + 44);
	cairo_show_text(cr, "PlanckenPi");
	cairo_stroke(cr);

	// display counter at random position
	// cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	// cairo_select_font_face(cr, "Purisa", CAIRO_FONT_SLANT_NORMAL,
	// 		       CAIRO_FONT_WEIGHT_BOLD);
	// cairo_set_font_size(cr, 24);
	// cairo_move_to(cr, (rand() % (width - 80)) + 25,
	// 	      (rand() % (height - 80)) + 50);
	// char s[16];
	// sprintf(s, "%d", counter);
	// cairo_show_text(cr, s);

	// cairo_stroke(cr);
}

int main(int argc, char *argv[])
{
	GtkWidget *darea;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	darea = gtk_drawing_area_new();

	// figure out how to do relative paths
	glob.usb_passthrough_enabled = cairo_image_surface_create_from_png(
		"/home/pi/qmk_kernel_module/helper/assets/usb-passthrough-enabled.png");
	glob.usb_passthrough_disabled = cairo_image_surface_create_from_png(
		"/home/pi/qmk_kernel_module/helper/assets/usb-passthrough-disabled.png");

	gtk_container_add(GTK_CONTAINER(window), darea);
	gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK);

	g_signal_connect(G_OBJECT(darea), "draw", G_CALLBACK(on_draw_event),
			 NULL);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_quit_event),
			 NULL);
	g_signal_connect(G_OBJECT(window), "button-press-event",
			 G_CALLBACK(clicked), NULL);
	g_signal_connect(G_OBJECT(window), "key_press_event",
			 G_CALLBACK(key_press_event), NULL);

	if (TIME_INTERVAL > 0) {
		// Add timer event
		// Register the timer and set time in mS.
		// The timer_event() function is called repeatedly until it returns FALSE.
		global_timeout_ref = g_timeout_add(TIME_INTERVAL,
						   (GSourceFunc)on_timer_event,
						   (gpointer)window);
	}

	gtk_window_set_default_size(GTK_WINDOW(window), WINDOW_WIDTH,
				    WINDOW_HEIGHT);
	gtk_window_set_title(GTK_WINDOW(window), WINDOW_NAME);

	// if (strlen(ICON_NAME) > 0) {
	// 	gtk_window_set_icon_from_file(GTK_WINDOW(window), ICON_NAME,
	// 				      NULL);
	// }

	// init

	nls = open_netlink();
	send_message(nls, "hi!");

	gtk_widget_show_all(window);

	gtk_main();

	cairo_surface_destroy(glob.usb_passthrough_enabled);
	cairo_surface_destroy(glob.usb_passthrough_disabled);

	return 0;
}