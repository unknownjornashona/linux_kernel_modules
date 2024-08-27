#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("您的名字");
MODULE_DESCRIPTION("一个简单的网络内核模块示例");

static struct net_device *my_net_device;

static void my_netdev_setup(struct net_device *dev) {
    // 设置设备的基本参数
    dev->flags |= IFF_NOARP;  // 不使用 ARP
    dev->type = ARPHRD_NONE;   // 设置硬件类型
    dev->mtu = 1500;           // 设置最大传输单元
    dev->addr_len = 0;         // 地址长度为 0
    dev->hard_header_len = 0;  // 硬件头长度为 0
}

static int my_netdev_open(struct net_device *dev) {
    printk(KERN_INFO "我的网络设备已打开\n");
    return 0; // 返回 0 表示打开成功
}

static int my_netdev_stop(struct net_device *dev) {
    printk(KERN_INFO "我的网络设备已关闭\n");
    return 0; // 返回 0 表示关闭成功
}

static const struct net_device_ops my_netdev_ops = {
    .ndo_open = my_netdev_open,
    .ndo_stop = my_netdev_stop,
};

static int __init my_module_init(void) {
    my_net_device = alloc_netdev(0, "mydev%d", NET_NAME_PREDICTABLE, my_netdev_setup);
    if (!my_net_device) {
        printk(KERN_ERR "分配网络设备失败\n");
        return -ENOMEM;
    }

    my_net_device->netdev_ops = &my_netdev_ops;

    if (register_netdev(my_net_device)) {
        printk(KERN_ERR "注册网络设备失败\n");
        free_netdev(my_net_device);
        return -1;
    }

    printk(KERN_INFO "网络模块已加载，设备名称：%s\n", my_net_device->name);
    return 0; // 返回 0 表示初始化成功
}

static void __exit my_module_exit(void) {
    unregister_netdev(my_net_device);
    free_netdev(my_net_device);
    printk(KERN_INFO "网络模块已卸载\n");
}

module_init(my_module_init);
module_exit(my_module_exit);
