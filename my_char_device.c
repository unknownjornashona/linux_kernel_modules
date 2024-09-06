#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>

#define DEVICE_NAME "my_char_device"
#define BUFFER_SIZE 1024

static char buffer[BUFFER_SIZE];
static int open_count = 0;

static int my_open(struct inode *inode, struct file *file) {
    open_count++;
    printk(KERN_INFO "Device opened %d times\n", open_count);
    return 0;
}

static int my_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Device closed\n");
    return 0;
}

static ssize_t my_read(struct file *file, char __user *user_buffer, size_t len, loff_t *offset) {
    if (*offset >= BUFFER_SIZE) {
        return 0;  // EOF
    }
    if (*offset + len > BUFFER_SIZE) {
        len = BUFFER_SIZE - *offset;  // Adjust length if it exceeds buffer
    }
    if (copy_to_user(user_buffer, buffer + *offset, len)) {
        return -EFAULT;
    }
    *offset += len;
    return len;
}

static ssize_t my_write(struct file *file, const char __user *user_buffer, size_t len, loff_t *offset) {
    if (*offset >= BUFFER_SIZE) {
        return -ENOSPC;  // No space left
    }
    if (*offset + len > BUFFER_SIZE) {
        len = BUFFER_SIZE - *offset;  // Adjust length if it exceeds buffer
    }
    if (copy_from_user(buffer + *offset, user_buffer, len)) {
        return -EFAULT;
    }
    *offset += len;
    return len;
}

static struct file_operations fops = {
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
};

static int __init my_module_init(void) {
    register_chrdev(240, DEVICE_NAME, &fops);
    printk(KERN_INFO "Character device registered: %s\n", DEVICE_NAME);
    return 0;
}

static void __exit my_module_exit(void) {
    unregister_chrdev(240, DEVICE_NAME);
    printk(KERN_INFO "Character device unregistered: %s\n", DEVICE_NAME);
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple character device driver");
