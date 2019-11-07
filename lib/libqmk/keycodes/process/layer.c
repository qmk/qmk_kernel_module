#include <qmk/keycodes/process.h>
#include <qmk/protocol.h>
#include <qmk/keycodes/quantum.h>

void update_active_layer(struct qmk_keyboard *keyboard) {
	int i;
	uint8_t layer = 0;
	for (i = keyboard->layers - 1; i >= 0; i--) {
		if ((keyboard->layer_state >> i) & 1) {
			layer = i;
			break;
		}
	}
	keyboard->active_layer = layer;
}

void layer_and_equal(struct qmk_keyboard *keyboard, uint16_t value) {
	keyboard->layer_state &= value;
	update_active_layer(keyboard);
}

void layer_or_equal(struct qmk_keyboard *keyboard, uint16_t value) {
	keyboard->layer_state |= value;
	update_active_layer(keyboard);
}

bool process_layer(struct qmk_keyboard *keyboard, qmk_keycode_t *keycode,
		   bool pressed)
{
	qmk_layer layer;
	qmk_keycode_t basic_keycode;
	void *timer = NULL;

	switch (*keycode) {
	case QK_LAYER_TAP ... QK_LAYER_TAP_MAX:
		layer = (*keycode >> 0x8) & 0xF;
		basic_keycode = *keycode & 0xFF;
		if (pressed) {
			timer = protocol.timer_init();
			layer_or_equal(keyboard, (1UL << layer));
		} else {
			layer_and_equal(keyboard, ~(1UL << layer));
			if (timer) {
				uint16_t time_elapsed =
					protocol.timer_elapsed(timer);
				if (time_elapsed < keyboard->mod_tap_timeout) {
					protocol.send_keycode(keyboard, basic_keycode, true);
					protocol.send_keycode(keyboard, basic_keycode, false);
				}
			}
		}
		return true;
		break;
	case QK_TO ... QK_TO_MAX:;
		// Layer set "GOTO"
		// when = (keycode >> 0x4) & 0x3;
		// action_layer = keycode & 0xF;
		// action.code = ACTION_LAYER_SET(action_layer, when);
		return true;
		break;
	case QK_MOMENTARY ... QK_MOMENTARY_MAX:;
		// Momentary action_layer
		layer = *keycode & 0xFF;
		if (pressed)
			layer_or_equal(keyboard, (1UL << layer));
		else
			layer_and_equal(keyboard, ~(1UL << layer));
		return true;
		break;
	default:
		return false;
		break;
	}
}