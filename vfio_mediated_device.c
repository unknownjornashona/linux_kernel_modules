#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/vfio.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "vfio_mediated_device"
#define LOG_FILE_PATH "/var/log/vfio_mediated_device.log"

// 设备结构体
struct vfio_mediated_device {
    struct vfio_device *vdev;
    // 其他硬件相关数据
};

static struct vfio_mediated_device *mdev;

// 日志记录函数
void log_message(const char *message) {
    struct file *file;
    mm_segment_t old_fs;

    // 打开日志文件
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    file = filp_open(LOG_FILE_PATH, O_WRONLY | O_APPEND | O_CREAT, 0644);
    set_fs(old_fs);

    if (IS_ERR(file)) {
        printk(KERN_ERR "Failed to open log file: %ld\n", PTR_ERR(file));
        return;
    }

    // 写入日志内容
    kernel_write(file, message, strlen(message), &file->f_pos);
    filp_close(file, NULL);
}

// 抛出异常的函数
static void throw_exception(const char *function_name) {
    char log_entry[256];
    snprintf(log_entry, sizeof(log_entry), "Exception occurred in %s\n", function_name);
    log_message(log_entry);
    printk(KERN_ERR "%s: Exception occurred\n", function_name);
}

// 设备的操作结构体
static const struct vfio_device_ops mdev_ops = {
    .get_info = /* Implement your get_info function */,
    .get_region = /* Implement your get_region function */,
};

// 设备注册函数
static int vfio_mediated_device_register(void) {
    int ret;

    // 分配 VFIO 媒介设备结构体
    mdev = kzalloc(sizeof(*mdev), GFP_KERNEL);
    if (!mdev) {
        throw_exception(__func__);
        return -ENOMEM;
    }

    // 注册 VFIO 设备
    mdev->vdev = vfio_register_device(DRIVER_NAME, &mdev_ops);
    if (IS_ERR(mdev->vdev)) {
        ret = PTR_ERR(mdev->vdev);
        kfree(mdev);
        throw_exception(__func__);
        return ret;
    }

    return 0;
}

// 设备注销函数
static void vfio_mediated_device_unregister(void) {
    if (mdev && mdev->vdev) {
        vfio_unregister_device(mdev->vdev);
    }
    kfree(mdev);
}

// 模块初始化
static int __init vfio_mediated_device_init(void) {
    int ret;

    ret = vfio_mediated_device_register();
    if (ret < 0) {
        throw_exception(__func__);
    }
    return ret;
}

// 模块退出
static void __exit vfio_mediated_device_exit(void) {
    vfio_mediated_device_unregister();
}

module_init(vfio_mediated_device_init);
module_exit(vfio_mediated_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("VFIO Mediated Device with Logging");
