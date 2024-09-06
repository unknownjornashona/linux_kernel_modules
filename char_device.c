#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/crypto.h>
#include <linux/init.h>
#include <linux/file.h>

#define DEVICE_NAME "my_disk_device"
#define BUFFER_SIZE 4096  // 数据缓冲区大小
#define KEY_SIZE 16       // AES-128
#define LOG_FILE_PATH "/var/log/my_disk_device.log"
#define AES_KEY "0123456789abcdef"  // 假设的安全密钥

static char buffer[BUFFER_SIZE];
static int open_count = 0;

// 日志功能
static void log_message(const char *message) {
    struct file *file;
    mm_segment_t oldfs;
    loff_t pos = 0;

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    file = filp_open(LOG_FILE_PATH, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (!IS_ERR(file)) {
        vfs_write(file, message, strlen(message), &pos);
        filp_close(file, NULL);
    }
    set_fs(oldfs);
}

static int my_open(struct inode *inode, struct file *file) {
    open_count++;
    log_message("Device opened\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file) {
    log_message("Device closed\n");
    return 0;
}

// 磁盘加密逻辑
static void disk_encrypt(void *data, size_t len) {
    struct crypto_cipher *tfm;
    int result;

    tfm = crypto_alloc_cipher("aes", 0, 0);
    if (IS_ERR(tfm)) {
        log_message("Encrypt: crypto_alloc_cipher failed\n");
        return;
    }

    result = crypto_cipher_setkey(tfm, AES_KEY, KEY_SIZE);
    if (result) {
        crypto_free_cipher(tfm);
        log_message("Encrypt: setkey failed\n");
        return;
    }

    crypto_cipher_encrypt_one(tfm, data, data);
    crypto_free_cipher(tfm);
    log_message("Data encrypted\n");
}

// 磁盘解密逻辑
static void disk_decrypt(void *data, size_t len) {
    struct crypto_cipher *tfm;
    int result;

    tfm = crypto_alloc_cipher("aes", 0, 0);
    if (IS_ERR(tfm)) {
        log_message("Decrypt: crypto_alloc_cipher failed\n");
        return;
    }

    result = crypto_cipher_setkey(tfm, AES_KEY, KEY_SIZE);
    if (result) {
        crypto_free_cipher(tfm);
        log_message("Decrypt: setkey failed\n");
        return;
    }

    crypto_cipher_decrypt_one(tfm, data, data);
    crypto_free_cipher(tfm);
    log_message("Data decrypted\n");
}

static ssize_t my_read(struct file *file, char __user *user_buffer, size_t len, loff_t *offset) {
    if (*offset >= BUFFER_SIZE) {
        return 0;  // EOF
    }
    if (*offset + len > BUFFER_SIZE) {
        len = BUFFER_SIZE - *offset;
    }

    // 解密数据
    disk_decrypt(buffer + *offset, len);

    if (copy_to_user(user_buffer, buffer + *offset, len)) {
        log_message("Read: copy_to_user failed\n");
        return -EFAULT;
    }

    *offset += len;
    return len;
}

static ssize_t my_write(struct file *file, const char __user *user_buffer, size_t len, loff_t *offset) {
    if (*offset >= BUFFER_SIZE) {
        return -ENOSPC;  
    }
    if (*offset + len > BUFFER_SIZE) {
        len = BUFFER_SIZE - *offset;  
    }

    if (copy_from_user(buffer + *offset, user_buffer, len)) {
        log_message("Write: copy_from_user failed\n");
        return -EFAULT;
    }

    // 加密数据
    disk_encrypt(buffer + *offset, len);
    *offset += len;

    log_message("Data written and encrypted\n");
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
    log_message("Character device registered\n");
    return 0;
}

static void __exit my_module_exit(void) {
    unregister_chrdev(240, DEVICE_NAME);
    log_message("Character device unregistered\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple disk encryption character device driver");
