#pragma once

#include <stdlib.h>

int open_netlink(void);
int open_unblocked_netlink(void);
void read_message(int sock, void (*callback)());
void send_message(int sock, char *message);