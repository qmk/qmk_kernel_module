#include <linux/timer.h>
#include <qmk/protocol.h>

void * timer_init(void) {
    return NULL;
}

uint8_t timer_elapsed(void * timer) {
    return 0;
}

qmk_protocol protocol = {
    .timer_init = &timer_init,
    .timer_elapsed = &timer_elapsed,
};
