#include <qmk/types.h>

#ifndef ENCODER_RESOLUTION
#define ENCODER_RESOLUTION 4
#endif

static pin_t encoders_pad_a[] = ENCODERS_PAD_A;
static pin_t encoders_pad_b[] = ENCODERS_PAD_B;

#define NUMBER_OF_ENCODERS_AB (sizeof(encoders_pad_a) / sizeof(pin_t))
#define NUMBER_OF_ENCODERS_C (sizeof(encoders_pad_c) / sizeof(pin_t))
#define NUMBER_OF_ENCODERS (NUMBER_OF_ENCODERS_C * NUMBER_OF_ENCODERS_AB)
static pin_t encoders_pad_c[] = ENCODERS_PAD_C;

static int8_t encoder_LUT[] = { 0,  -1, 1, 0, 1, 0, 0,  -1,
				-1, 0,  0, 1, 0, 1, -1, 0 };

static uint8_t encoder_state[NUMBER_OF_ENCODERS] = { 0 };

static int8_t encoder_value[NUMBER_OF_ENCODERS] = { 0 };

void encoder_init(struct qmk_keyboard *keyboard)
{
	for (int i = 0; i < NUMBER_OF_ENCODERS_C; i++) {
		setPinOutput(encoders_pad_c[i]);
		if (i != 0)
			writePinHigh(encoders_pad_c[i]);
		else
			writePinLow(encoders_pad_c[i]);
	}

	for (int j = 0; j < NUMBER_OF_ENCODERS_C; j++) {
		writePinLow(encoders_pad_c[j]);
		wait_us(10);
		for (int i = 0; i < NUMBER_OF_ENCODERS_AB; i++) {
			setPinInputHigh(encoders_pad_a[i]);
			setPinInputHigh(encoders_pad_b[i]);

			encoder_state[j + (i * NUMBER_OF_ENCODERS_C)] =
				(readPin(encoders_pad_a[i]) << 0) |
				(readPin(encoders_pad_b[i]) << 1);
		}
		writePinHigh(encoders_pad_c[j]);
	}
	// need to disable these pins to prevent matrix activation
	for (int i = 0; i < NUMBER_OF_ENCODERS_AB; i++) {
		setPinInput(encoders_pad_a[i]);
		setPinInput(encoders_pad_b[i]);
	}
	for (int i = 0; i < NUMBER_OF_ENCODERS_C; i++) {
		setPinInputLow(encoders_pad_c[i]);
	}
}

static void encoder_update(int8_t index, uint8_t state)
{
	encoder_value[index] += encoder_LUT[state & 0xF];
	if (encoder_value[index] >= ENCODER_RESOLUTION) {
		encoder_update_kb(index, false);
	} // direction is arbitrary here, but this clockwise
	if (encoder_value[index] <= -ENCODER_RESOLUTION) {
		encoder_update_kb(index, true);
	}
	encoder_value[index] %= ENCODER_RESOLUTION;
}

void encoder_read(void)
{
	// setup row pins to act as C pins for the encoders, prep the first one
	for (int i = 0; i < NUMBER_OF_ENCODERS_C; i++) {
		setPinOutput(encoders_pad_c[i]);
		if (i != 0)
			writePinHigh(encoders_pad_c[i]);
		else
			writePinLow(encoders_pad_c[i]);
	}
	// pull these back up because we disabled them earlier
	for (int i = 0; i < NUMBER_OF_ENCODERS_AB; i++) {
		setPinInputHigh(encoders_pad_a[i]);
		setPinInputHigh(encoders_pad_b[i]);
	}
	for (int j = 0; j < NUMBER_OF_ENCODERS_C; j++) {
		writePinLow(encoders_pad_c[j]);
		wait_us(10);
		for (int i = 0; i < NUMBER_OF_ENCODERS_AB; i++) {
			encoder_state[j + (i * NUMBER_OF_ENCODERS_C)] <<= 2;
			encoder_state[j + (i * NUMBER_OF_ENCODERS_C)] |=
				(readPin(encoders_pad_a[i]) << 0) |
				(readPin(encoders_pad_b[i]) << 1);
			encoder_update(
				j + (i * NUMBER_OF_ENCODERS_C),
				encoder_state[j + (i * NUMBER_OF_ENCODERS_C)]);
		}
		writePinHigh(encoders_pad_c[j]);
	}
	// need to disable these pins again to prevent matrix activation
	for (int i = 0; i < NUMBER_OF_ENCODERS_AB; i++) {
		setPinInput(encoders_pad_a[i]);
		setPinInput(encoders_pad_b[i]);
	}
	// revert row pins back to input
	for (int i = 0; i < NUMBER_OF_ENCODERS_C; i++) {
		setPinInputLow(encoders_pad_c[i]);
	}
}
