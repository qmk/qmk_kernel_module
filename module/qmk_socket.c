#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netlink.h>
#include <net/netlink.h>
#include <net/net_namespace.h>

/* Protocol family, consistent in both kernel prog and user prog. */
#define MYPROTO NETLINK_USERSOCK
/* Multicast group, consistent in both kernel prog and user prog. */
#define MYGRP 1

static struct sock *nl_sk = NULL;

void nl_input(struct sk_buff *skb)
{
    pr_info("data ready");
}

int nl_bind(struct net *net, int group) {
    pr_info("bind");
    return 0;
}

void send_socket_message(char * msg)
{
    struct sk_buff *skb;
    struct nlmsghdr *nlh;
    int msg_size = strlen(msg) + 1;
    int res;

    skb = nlmsg_new(NLMSG_ALIGN(msg_size + 1), GFP_KERNEL);
    if (!skb) {
        pr_err("Allocation failure.\n");
        return;
    }

    nlh = nlmsg_put(skb, 0, 1, NLMSG_DONE, msg_size + 1, 0);
    strcpy(nlmsg_data(nlh), msg);

    res = nlmsg_multicast(nl_sk, skb, 0, MYGRP, GFP_KERNEL);
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
    send_socket_message(buf);
    va_end(args);
    return i;
}

static struct netlink_kernel_cfg nl_cfg = {
    .input = &nl_input,
    .bind = &nl_bind
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