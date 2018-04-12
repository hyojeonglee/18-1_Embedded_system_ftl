#ifndef _DT_NETLINK_H
#define _DT_NETLINK_H

#define KNETLINK_UNIT			17
#define KNETLINK_GROUP			1
#define MAX_PAYLOAD 			4096

int dt_netlink_create (struct sockaddr_nl* src_addr);
void dt_netlink_send_message_to_kernel (int sock_fd, struct sockaddr_nl* dst_addr, struct iovec* iov, struct msghdr* msg, struct nlmsghdr* nlh);
void dt_netlink_recv_message_from_kernel (int sock_fd, struct msghdr* msg, struct nlmsghdr* nlh);
void dt_netlink_close (int sock_fd);

#endif
