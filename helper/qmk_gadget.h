#pragma once

#include <stdlib.h>
#include <usbg/usbg.h>

struct cfg_attr {
    char *str;
    uint16_t id;
};

struct qmk_gadget_cfg {
    struct cfg_attr vendor;
    struct cfg_attr product;
    char *serial;
};

int gadget_write(unsigned char *buf);
int gadget_close(char *name);
int gadget_open(char *name, struct qmk_gadget_cfg *cfg);
