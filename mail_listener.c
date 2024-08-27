#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/netlink.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stddef.h>
#include <libmnl/libmnl.h>
#include <syslog.h>

#define NETLINK_USER 31
#define MAX_PAYLOAD 1024  // 最大负载大小

// 发送邮件的简单函数
void send_email(const char *message) {
    // 这里您可以使用系统命令或库函数发送邮件
    FILE *mail_pipe = popen("/usr/sbin/sendmail you@example.com", "w");
    if (mail_pipe) {
        fprintf(mail_pipe, "Subject: 捕获到 UDP 数据包\n");
        fprintf(mail_pipe, "%s", message);
        pclose(mail_pipe);
    } else {
        perror("无法打开邮件管道");
    }
}

void nl_recv_msg() {
    struct sockaddr_nl addr;
    struct nlmsghdr *nlh;
    int sock_fd;
    char buffer[MAX_PAYLOAD];

    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_USER);
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr));

    while (1) {
        nlh = (struct nlmsghdr *)buffer;
        recv(sock_fd, nlh, MAX_PAYLOAD, 0);
        send_email(NLMSG_DATA(nlh));
    }
    
    close(sock_fd);
}

int main() {
    nl_recv_msg();
    return 0;
}
