#include <qmk/keycodes/process.h>
#include <qmk/protocol.h>
#include <qmk/keycodes/basic.h>
#include <qmk/keycodes/quantum.h>

bool process_mods(struct qmk_keyboard *keyboard, qmk_keycode_t *keycode,
		  bool pressed)
{
	switch (*keycode) {
	case QK_MODS ... QK_MODS_MAX:
		if (!pressed)
			protocol.send_keycode(keyboard, *keycode & 0xFF, pressed);
		if (*keycode & QK_RMODS_MIN) {
			if (*keycode & QK_RCTL)
				protocol.send_keycode(keyboard, KC_RCTRL,
						      pressed);
			if (*keycode & QK_RSFT)
				protocol.send_keycode(keyboard, KC_RSHIFT,
						      pressed);
			if (*keycode & QK_RALT)
				protocol.send_keycode(keyboard, KC_RALT,
						      pressed);
			if (*keycode & QK_RGUI)
				protocol.send_keycode(keyboard, KC_RGUI,
						      pressed);
		} else {
			if (*keycode & QK_LCTL)
				protocol.send_keycode(keyboard, KC_LCTRL,
						      pressed);
			if (*keycode & QK_LSFT)
				protocol.send_keycode(keyboard, KC_LSHIFT,
						      pressed);
			if (*keycode & QK_LALT)
				protocol.send_keycode(keyboard, KC_LALT,
						      pressed);
			if (*keycode & QK_LGUI)
				protocol.send_keycode(keyboard, KC_LGUI,
						      pressed);
		}
		if (pressed)
			protocol.send_keycode(keyboard, *keycode & 0xFF, pressed);
		return true;
		break;
	default:
		return false;
		break;
	}
}