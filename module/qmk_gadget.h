#include <linux/skbuff.h> 

void hello_nl_recv_msg(struct sk_buff *skb);
int gadget_init(void);
void gadget_exit(void);