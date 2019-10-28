#pragma once

#include <qmk/types.h>

typedef struct qmk_protocol {
    void *		(*timer_init)		(void);
    uint8_t 	(*timer_elapsed)	(void *);
} qmk_protocol;

qmk_protocol protocol;