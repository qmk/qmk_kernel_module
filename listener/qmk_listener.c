#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <ncurses.h>

/* Protocol family, consistent in both kernel prog and user prog. */
#define MYPROTO NETLINK_USERSOCK
/* Multicast group, consistent in both kernel prog and user prog. */
#define MYMGRP 1

static int sig_flag = 1;

int open_netlink(void)
{
    int sock;
    struct sockaddr_nl addr;
    int group = MYMGRP;

    sock = socket(AF_NETLINK, SOCK_RAW, MYPROTO);
    if (sock < 0) {
        printf("sock < 0.\n");
        return sock;
    }

    memset((void *) &addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    /* This doesn't work for some reason. See the setsockopt() below. */
     // addr.nl_groups = MYMGRP; 

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        printf("bind < 0.\n");
        return -1;
    }

    /*
     * 270 is SOL_NETLINK. See
     * http://lxr.free-electrons.com/source/include/linux/socket.h?v=4.1#L314
     * and
     * http://stackoverflow.com/questions/17732044/
     */
    if (setsockopt(sock, 270, NETLINK_ADD_MEMBERSHIP, &group, sizeof(group)) < 0) {
        printf("setsockopt < 0\n");
        return -1;
    }

    int status = fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);

    if (status == -1){
      perror("calling fcntl");
      // handle the error.  By the way, I've never seen fcntl fail in this way
    }

    return sock;
}

void read_event(int sock)
{
    struct sockaddr_nl nladdr;
    struct msghdr msg;
    struct iovec iov;
    char buffer[65536];
    int ret;

    iov.iov_base = (void *) buffer;
    iov.iov_len = sizeof(buffer);
    msg.msg_name = (void *) &(nladdr);
    msg.msg_namelen = sizeof(nladdr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    ret = recvmsg(sock, &msg, 0);
    if (ret >= 0)
        printf("%s\n", NLMSG_DATA((struct nlmsghdr *) &buffer));
}

void interrupt_signal(int sig)
{
    sig_flag = 0;
}

int main(int argc, char *argv[])
{
    int nls, c;

    nls = open_netlink();
    if (nls < 0)
        return nls;

    // initscr();
    // keypad(stdscr, TRUE);
    // noecho();

    // something to catch the interrput and handle it well?
    signal(SIGINT, interrupt_signal);

    printf("Bound, listening.\n");
    while (sig_flag) {
        read_event(nls);
    }

    close(nls);

    // endwin();

    return 0;
}