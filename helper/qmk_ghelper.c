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

#define TIME_INTERVAL 20 // time interval for timer function in msec
// 0 means no timer function

guint global_timeout_ref;
GtkWidget *window;

static int nls;
static bool needs_update = true;
static bool first_run = true;
static uint8_t update_row, update_col;

struct qmk_key {
	bool pressed;
	uint8_t width;
	uint8_t x;
	uint8_t y;
};

#define KEYBOARD_ROWS 8
#define KEYBOARD_COLS 6

#define KEY_U(i) (i * 4)

#define QMK_KEY(width, x, y)                                                   \
	{                                                                      \
		0, KEY_U(width), KEY_U(x), KEY_U(y)                            \
	}

static struct qmk_key planck_keys[KEYBOARD_ROWS][KEYBOARD_COLS] = {
	{
		QMK_KEY(1, 0, 0),
		QMK_KEY(1, 1, 0),
		QMK_KEY(1, 2, 0),
		QMK_KEY(1, 3, 0),
		QMK_KEY(1, 4, 0),
		QMK_KEY(1, 5, 0),
	},
	{
		QMK_KEY(1, 0, 1),
		QMK_KEY(1, 1, 1),
		QMK_KEY(1, 2, 1),
		QMK_KEY(1, 3, 1),
		QMK_KEY(1, 4, 1),
		QMK_KEY(1, 5, 1),
	},
	{
		QMK_KEY(1, 0, 2),
		QMK_KEY(1, 1, 2),
		QMK_KEY(1, 2, 2),
		QMK_KEY(1, 3, 2),
		QMK_KEY(1, 4, 2),
		QMK_KEY(1, 5, 2),
	},
	{
		QMK_KEY(1, 0, 3),
		QMK_KEY(1, 1, 3),
		QMK_KEY(1, 2, 3),
		QMK_KEY(1, 9, 3),
		QMK_KEY(1, 10, 3),
		QMK_KEY(1, 11, 3),
	},
	{
		QMK_KEY(1, 6, 0),
		QMK_KEY(1, 7, 0),
		QMK_KEY(1, 8, 0),
		QMK_KEY(1, 9, 0),
		QMK_KEY(1, 10, 0),
		QMK_KEY(1, 11, 0),
	},
	{
		QMK_KEY(1, 6, 1),
		QMK_KEY(1, 7, 1),
		QMK_KEY(1, 8, 1),
		QMK_KEY(1, 9, 1),
		QMK_KEY(1, 10, 1),
		QMK_KEY(1, 11, 1),
	},
	{
		QMK_KEY(1, 6, 2),
		QMK_KEY(1, 7, 2),
		QMK_KEY(1, 8, 2),
		QMK_KEY(1, 9, 2),
		QMK_KEY(1, 10, 2),
		QMK_KEY(1, 11, 2),
	},
	{
		QMK_KEY(2, 5, 3),
		QMK_KEY(1, 7, 3),
		QMK_KEY(1, 8, 3),
		QMK_KEY(1, 3, 3),
		QMK_KEY(1, 4, 3),
	}
};

static void do_drawing(cairo_t *, GtkWidget *);

static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr,
			      gpointer user_data)
{
	do_drawing(cr, widget);
	return FALSE;
}

void handle_message(char *msg)
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
		uint8_t ch = (uint8_t)msg[0];
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
		uint8_t row = (uint8_t)msg[0] - 1;
		uint8_t col = (uint8_t)msg[1] - 1;
		bool pressed = (bool)msg[2];

		if (pressed) {
			printf("\033[1;34mMatrix event down:    (%d, %d)\033[0m\n",
			       row, col);
		} else {
			printf("\033[0;34mMatrix event up:      (%d, %d)\033[0m\n",
			       row, col);
		}

		planck_keys[row][col].pressed = pressed;
		update_row = row;
		update_col = col;
		needs_update = true;
	}
}

static gboolean on_timer_event(GtkWidget *widget)
{
	bool change = false;

	g_source_remove(
		global_timeout_ref); // stop timer in case application_on_timer_event takes too long
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

static void do_drawing(cairo_t *cr, GtkWidget *widget)
{
	int r, c, width, height;
	GtkWidget *win = gtk_widget_get_toplevel(widget);
	gtk_window_get_size(GTK_WINDOW(win), &width, &height);

	// draw

#define KEYDOWN_COLOR 92.0 / 255, 197.0 / 255, 76.0 / 255
#define KEYUP_COLOR 0.9, 0.9, 0.9

#define KEYBOARD_WIDTH_KU 12
#define KEYBOARD_HEIGHT_KU 4
#define SIDE_PADDING 10
#define KEY_PADDING 5
#define KU_SCALE                                                               \
	(((width - (SIDE_PADDING * 2)) /                         \
	  KEY_U(KEYBOARD_WIDTH_KU)))
#define TOP_OFFSET (((height) - (KEYBOARD_HEIGHT_KU * KU_SCALE)) / 2)

	// srand(15);
	if (first_run) {
		for (r = 0; r < KEYBOARD_ROWS; r++) {
			for (c = 0; c < KEYBOARD_COLS; c++) {
				if (planck_keys[r][c].width) {
					cairo_rectangle(
						cr,
						(planck_keys[r][c].x *
						 KU_SCALE) +
							SIDE_PADDING +
							KEY_PADDING,
						(planck_keys[r][c].y *
						 KU_SCALE) +
							TOP_OFFSET +
							KEY_PADDING,
						KU_SCALE * planck_keys[r][c]
									.width -
							KEY_PADDING,
						KU_SCALE * 4 - KEY_PADDING);
					if (planck_keys[r][c].pressed)
						cairo_set_source_rgb(
							cr, KEYDOWN_COLOR);
					else
						cairo_set_source_rgb(
							cr, KEYUP_COLOR);
					cairo_fill(cr);
				}
			}
		}
	} else {
		cairo_rectangle(cr,
				(planck_keys[r][c].x * KU_SCALE) +
					SIDE_PADDING + KEY_PADDING,
				(planck_keys[r][c].y * KU_SCALE) + TOP_OFFSET +
					KEY_PADDING,
				KU_SCALE * planck_keys[r][c].width -
					KEY_PADDING,
				KU_SCALE * 4 - KEY_PADDING);
		if (planck_keys[r][c].pressed)
			cairo_set_source_rgb(cr, KEYDOWN_COLOR);
		else
			cairo_set_source_rgb(cr, KEYUP_COLOR);
		cairo_fill(cr);
	}

	cairo_set_source_surface(cr, glob.usb_passthrough_enabled,
				 width - (SIDE_PADDING * 2) - 44,
				 SIDE_PADDING * 2);
	cairo_paint(cr);

	// display text "click to erase"
	// cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
	// cairo_select_font_face(cr, "Purisa",
	// 		       CAIRO_FONT_SLANT_NORMAL,
	// 		       CAIRO_FONT_WEIGHT_BOLD);
	// cairo_set_font_size(cr, 24);
	// cairo_move_to(cr, 20, 50);
	// cairo_show_text(cr, "click to erase");
	// cairo_stroke(cr);

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