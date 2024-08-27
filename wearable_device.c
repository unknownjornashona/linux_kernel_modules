#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DRIVER_NAME "wearable_device"
#define DEVICE_NAME "wearable_dev"
#define BUFFER_SIZE 256

static int major_number;
static char *device_buffer;
static struct class *device_class = NULL;
static struct device *device_device = NULL;

static int wearable_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Wearable device opened\n");
    return 0;
}

static ssize_t wearable_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    unsigned long not_copied;
    if (*offset >= BUFFER_SIZE) {
        return 0;  // EOF
    }

    if (*offset + len > BUFFER_SIZE) {
        len = BUFFER_SIZE - *offset;  // Adjust read length
    }

    not_copied = copy_to_user(buf, device_buffer + *offset, len);
    if (not_copied) {
        return -EFAULT;  // Error copying data
    }

    *offset += len;
    printk(KERN_INFO "Read %zu bytes from wearable device\n", len);
    return len;  // Return number of bytes read
}

static ssize_t wearable_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    unsigned long not_copied;
    if (*offset >= BUFFER_SIZE) {
        return -ENOSPC;  // No space left on device
    }

    if (*offset + len > BUFFER_SIZE) {
        len = BUFFER_SIZE - *offset;  // Adjust write length
    }

    not_copied = copy_from_user(device_buffer + *offset, buf, len);
    if (not_copied) {
        return -EFAULT;  // Error copying data
    }

    *offset += len;
    printk(KERN_INFO "Wrote %zu bytes to wearable device\n", len);
    return len;  // Return number of bytes written
}

static int wearable_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Wearable device closed\n");
    return 0;
}

static struct file_operations fops = {
    .open = wearable_open,
    .read = wearable_read,
    .write = wearable_write,
    .release = wearable_release,
};

static int __init wearable_init(void) {
    printk(KERN_INFO "Initializing wearable device driver\n");

    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "Failed to register a major number\n");
        return major_number;
    }

    device_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!device_buffer) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to allocate memory for device buffer\n");
        return -ENOMEM;
    }

    device_class = class_create(THIS_MODULE, "wearable");
    if (IS_ERR(device_class)) {
        kfree(device_buffer);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create device class\n");
        return PTR_ERR(device_class);
    }

    device_device = device_create(device_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(device_device)) {
        class_destroy(device_class);
        kfree(device_buffer);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create device\n");
        return PTR_ERR(device_device);
    }

    return 0;
}

static void __exit wearable_exit(void) {
    device_destroy(device_class, MKDEV(major_number, 0));
    class_unregister(device_class);
    class_destroy(device_class);
    kfree(device_buffer);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "Wearable device driver exited\n");
}

module_init(wearable_init);
module_exit(wearable_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple wearable device driver");
MODULE_VERSION("0.1");
