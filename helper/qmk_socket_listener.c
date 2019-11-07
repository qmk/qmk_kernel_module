#include <stdlib.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "qmk_socket_listener.h"
#include "qmk_socket.h"

#define MAX_PAYLOAD 1024

int open_netlink(void)
{
    int sock, status;
    struct sockaddr_nl addr;
    int group = MYMGRP;

    sock = socket(AF_NETLINK, SOCK_RAW, MYPROTO);
    if (sock < 0) {
        printf("sock < 0.\n");
        return sock;
    }

    memset((void *)&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    /* This doesn't work for some reason. See the setsockopt() below. */
    addr.nl_groups = MYMGRP;

    status = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (status < 0) {
        fprintf(stderr, "bind error occurred: %s\n", strerror(errno));
        return status;
    }

    /*
     * 270 is SOL_NETLINK. See
     * http://lxr.free-electrons.com/source/include/linux/socket.h?v=4.1#L314
     * and
     * http://stackoverflow.com/questions/17732044/
     */
    status = setsockopt(sock, 270, NETLINK_ADD_MEMBERSHIP, &group,
                sizeof(group));
    if (status < 0) {
        printf("setsockopt: %d\n", status);
        return status;
    }

    status = fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);

    if (status == -1) {
        perror("calling fcntl");
        // handle the error.  By the way, I've never seen fcntl fail in this way
    }

    return sock;
}

void read_message(int sock, void (*callback)())
{
    struct sockaddr_nl nladdr;
    struct msghdr msg;
    struct iovec iov;
    u_int8_t buffer[65536];
    int ret, i;

    iov.iov_base = (void *)buffer;
    iov.iov_len = sizeof(buffer);

    msg.msg_name = (void *)&(nladdr);
    msg.msg_namelen = sizeof(nladdr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    ret = recvmsg(sock, &msg, 0);
    if (ret >= 0) {
        int8_t *input = NLMSG_DATA((struct nlmsghdr *)&buffer);
        callback(input);
    }
}

void send_message(int sock, char *message)
{
    struct sockaddr_nl nladdr;
    struct nlmsghdr *nlh;
    struct msghdr msg;
    struct iovec iov;
    int ret, i;

    nlh = (struct nlmsghdr *) malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;
    strcpy((char *) NLMSG_DATA(nlh), message);

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;

    msg.msg_name = (void *)&(nladdr);
    msg.msg_namelen = sizeof(nladdr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    sendmsg(sock, &msg, 0);
}