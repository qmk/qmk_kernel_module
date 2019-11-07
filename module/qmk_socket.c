#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netlink.h>
#include <net/netlink.h>
#include <net/net_namespace.h>
#include "qmk_socket.h"

static struct sock *nl_sk = NULL;

void nl_input(struct sk_buff *skb)
{
    pr_info("data ready");

    struct nlmsghdr *nlh;
    pid_t pid;
    char *message = "\x01Hello from kernel unicast";
    size_t message_size = strlen(message) + 1;

    nlh = (struct nlmsghdr *) skb->data;
    pid = nlh->nlmsg_pid; // pid of the sending process

    struct sk_buff *skb_out = nlmsg_new(message_size, GFP_KERNEL);
    if (!skb_out) {
        printk(KERN_ERR "Failed to allocate a new skb\n");
        return;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, message_size, 0);
    NETLINK_CB(skb_out).dst_group = 0;
    strncpy(nlmsg_data(nlh), message, message_size);

    int result = nlmsg_unicast(nl_sk, skb_out, pid);
}

int nl_bind(struct net *net, int group) {
    pr_info("bind");
    return 0;
}

void send_socket_message(uint8_t * msg, uint8_t msg_size)
{
    struct sk_buff *skb;
    struct nlmsghdr *nlh;
    // int msg_size = strlen(msg) + 1;

    int res;

    skb = nlmsg_new(NLMSG_ALIGN(msg_size + 1), GFP_KERNEL);
    if (!skb) {
        pr_err("Allocation failure.\n");
        return;
    }

    nlh = nlmsg_put(skb, 0, 1, NLMSG_DONE, msg_size + 1, 0);
    // strcpy(nlmsg_data(nlh), msg);
    memcpy(nlmsg_data(nlh), msg, msg_size);

    res = nlmsg_multicast(nl_sk, skb, 0, MYMGRP, GFP_KERNEL);
    if (res < 0 && res != -3)
        pr_info("nlmsg_multicast() error: %d\n", res);

}

int send_socket_message_f(const char *fmt, ...)
{
    va_list args;
    int i;
    char *buf = kzalloc(200*sizeof(char), GFP_KERNEL);

    va_start(args, fmt);
    i=vsprintf(buf,fmt,args);
    send_socket_message(buf, strlen(buf) + 1);
    va_end(args);
    return i;
}

static struct netlink_kernel_cfg nl_cfg = {
    .input = &nl_input,
    .bind = &nl_bind,
    .flags = NL_CFG_F_NONROOT_RECV | NL_CFG_F_NONROOT_SEND,
    .groups = MYMGRP,
};

int gadget_init(void)
{
    nl_sk = netlink_kernel_create(&init_net, MYPROTO, &nl_cfg);

    if (!nl_sk) {
        pr_err("Error creating socket.\n");
        return -10;
    }

    return 0;
}

void gadget_exit(void)
{
    netlink_kernel_release(nl_sk);
    // pr_info("Exiting hello module.\n");
}