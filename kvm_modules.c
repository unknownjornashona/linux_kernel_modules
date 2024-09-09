#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kvm_host.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/jiffies.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("KVM Coalesced MMIO Example");
MODULE_VERSION("0.2");

#define COALESCED_MMIO_SIZE  1024
#define COALESCE_TIMEOUT      msecs_to_jiffies(100)

struct mmio_request {
    uint64_t address;
    uint64_t data;
    struct list_head list;
};

static struct list_head mmio_requests;
static unsigned long last_request_time = 0;
static void *coalesced_mmio_area;

// MMIO 处理函数
void handle_coalesced_mmio(uint64_t addr, uint64_t data) {
    struct mmio_request *req;

    // 创建新请求
    req = kmalloc(sizeof(*req), GFP_KERNEL);
    if (!req) return;

    req->address = addr;
    req->data = data;
    list_add_tail(&req->list, &mmio_requests);

    // 检查是否需要触发批处理
    if (jiffies - last_request_time > COALESCE_TIMEOUT) {
        // 批处理请求
        struct mmio_request *request;
        list_for_each_entry(request, &mmio_requests, list) {
            printk(KERN_INFO "Coalesced MMIO access at address: 0x%lx, data: 0x%lx\n", request->address, request->data);
        }
        // 清空请求列表
        list_flush(&mmio_requests);
    }
    last_request_time = jiffies;
}

// 模块初始化
static int __init kvm_coalesced_mmio_init(void) {
    INIT_LIST_HEAD(&mmio_requests);
    coalesced_mmio_area = kmalloc(COALESCED_MMIO_SIZE, GFP_KERNEL);
    if (!coalesced_mmio_area) return -ENOMEM;

    printk(KERN_INFO "KVM Coalesced MMIO Module Loaded\n");
    return 0;
}

// 模块退出
static void __exit kvm_coalesced_mmio_exit(void) {
    struct mmio_request *req, *tmp;

    // 释放请求列表
    list_for_each_entry_safe(req, tmp, &mmio_requests, list) {
        list_del(&req->list);
        kfree(req);
    }
    
    kfree(coalesced_mmio_area);
    printk(KERN_INFO "KVM Coalesced MMIO Module Unloaded\n");
}

module_init(kvm_coalesced_mmio_init);
module_exit(kvm_coalesced_mmio_exit);
