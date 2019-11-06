#pragma once

#define HANDSHAKE 0x01
#define MSG_GENERIC 0x02
#define KEYCODE_HID 0x03
#define MATRIX_EVENT 0x04

/* Protocol family, consistent in both kernel prog and user prog. */
#define MYPROTO NETLINK_USERSOCK
/* Multicast group, consistent in both kernel prog and user prog. */
#define MYMGRP 1