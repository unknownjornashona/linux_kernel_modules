#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/bpf.h>
#include <linux/bpf_helpers.h>
#include <linux/ptrace.h>
#include <net/netlink.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("您的名字");
MODULE_DESCRIPTION("扩展的 BPF 内核模块 示例");

// Netlink 套接字
struct sock *nl_sock;

// Netlink 接收标识符
#define NETLINK_USER 31

// BPF 程序示例
SEC("filter/bpf_prog")
int bpf_prog(struct __sk_buff *skb) {
    struct ethhdr *eth = bpf_hdr_pointer(skb);
    struct iphdr *ip;
    struct udphdr *udp;
    char msg[256];

    // 检查以太网头部
    if (eth->h_proto == htons(ETH_P_IP)) {
        ip = (struct iphdr *)(skb->data + sizeof(struct ethhdr));

        // 检查 IP 协议
        if (ip->protocol == IPPROTO_UDP) {
            udp = (struct udphdr *)((__u8 *)ip + (ip->ihl * 4));

            // 发送数据包信息到用户空间
            snprintf(msg, sizeof(msg), "捕获到 UDP 数据包: 源地址: %pI4 目标地址: %pI4\n", &ip->saddr, &ip->daddr);
            struct nlmsghdr *nlh;
            nlh = nlmsg_put(nl_sock, 0, 0, NLMSG_DONE, msg, strlen(msg));
            netlink_unicast(nl_sock, nlh, 0, MSG_DONTWAIT);

            return XDP_PASS; // 允许包通过
        }
    }

    return XDP_DROP; // 丢弃包
}

// Netlink 初始化
static int nl_sock_init(void) {
    struct netlink_kernel_cfg cfg = {
        .input = NULL,
    };
    nl_sock = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if (!nl_sock) {
        printk(KERN_ALERT "创建 Netlink 套接字失败\n");
        return -ENOMEM;
    }
    return 0;
}

// Netlink 销毁
static void nl_sock_destroy(void) {
    netlink_kernel_release(nl_sock);
}

// 注册 BPF 程序
static int __init bpf_module_init(void) {
    printk(KERN_INFO "加载 BPF 内核模块...\n");
    nl_sock_init();
    return 0;
}

// 卸载 BPF 程序
static void __exit bpf_module_exit(void) {
    nl_sock_destroy();
    printk(KERN_INFO "卸载 BPF 内核模块\n");
}

module_init(bpf_module_init);
module_exit(bpf_module_exit);
