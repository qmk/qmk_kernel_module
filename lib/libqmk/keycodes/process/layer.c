#include <qmk/keycodes/process.h>
#include <qmk/protocol.h>
#include <qmk/keycodes/quantum.h>

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
			(keyboard->layer_state) |= (1UL << layer);
		} else {
			(keyboard->layer_state) &= ~(1UL << layer);
			if (timer) {
				uint16_t time_elapsed =
					protocol.timer_elapsed(timer);
				if (time_elapsed < keyboard->mod_tap_timeout)
					*keycode = basic_keycode;
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
			(keyboard->layer_state) |= (1UL << layer);
		else
			(keyboard->layer_state) &= ~(1UL << layer);
		return true;
		break;
	default:
		return false;
		break;
	}
}