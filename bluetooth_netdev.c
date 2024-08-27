#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/init.h>
#include <linux/etherdevice.h>
#include <net/bluetooth.h>
#include <net/sock.h>
#include <crypto/ecdh.h>
#include <crypto/aes.h>
#include <crypto/skcipher.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("您的名字");
MODULE_DESCRIPTION("蓝牙网络设备驱动，集成 ECDH、ECIES 和文件加密传输功能");

static struct net_device *bluetooth_netdev;
static struct bt_sock *bt_sk;

// 密钥和加密相关参数
struct crypto_ecdh *ecdh_key;
struct crypto_skcipher *tfm;
static u8 local_pubkey[64]; // ECDH 公钥
static u8 shared_secret[32]; // 共享密钥

// ECDH 和加密初始化
static int crypto_init(void) {
    ecdh_key = crypto_alloc_ecdh("secp256r1", 0, 0);
    if (IS_ERR(ecdh_key)) {
        printk(KERN_ERR "ECDH 初始化失败\n");
        return PTR_ERR(ecdh_key);
    }
    
    tfm = crypto_alloc_skcipher("aes128-cbc", 0, 0);
    if (IS_ERR(tfm)) {
        printk(KERN_ERR "加密算法初始化失败\n");
        return PTR_ERR(tfm);
    }

    return 0;
}

// 生成 ECDH 密钥
static int generate_ecdh_keys(void) {
    if (crypto_ecdh_keypair(ecdh_key, local_pubkey, shared_secret) < 0) {
        printk(KERN_ERR "生成 ECDH 密钥对失败\n");
        return -1;
    }
    return 0;
}

// 加密函数
static int encrypt_data(const u8 *input, size_t len, u8 *output) {
    struct scatterlist sg[2];
    struct skcipher_request *req;
    struct crypto_skcipher *tfm;
    u8 iv[16] = {0}; // IV 应根据需求进行设置

    tfm = crypto_alloc_skcipher("aes128-cbc", 0, 0);
    req = skcipher_request_alloc(tfm, GFP_KERNEL);
    if (!req) {
        return -ENOMEM;
    }

    // 设置输入和输出
    sg_init_one(&sg[0], input, len);
    sg_init_one(&sg[1], output, len);

    // 设置加密请求
    skcipher_request_set_crypt(req, &sg[0], &sg[1], len, iv);
    skcipher_request_set_callback(req, 0, NULL, NULL);

    int err = skcipher_encrypt(req);
    skcipher_request_free(req);
    crypto_free_skcipher(tfm);

    return err; // 0 for success
}

// 解密函数
static int decrypt_data(const u8 *input, size_t len, u8 *output) {
    struct scatterlist sg[2];
    struct skcipher_request *req;
    struct crypto_skcipher *tfm;
    u8 iv[16] = {0}; // IV 应根据需求进行设置

    tfm = crypto_alloc_skcipher("aes128-cbc", 0, 0);
    req = skcipher_request_alloc(tfm, GFP_KERNEL);
    if (!req) {
        return -ENOMEM;
    }

    // 设置输入和输出
    sg_init_one(&sg[0], input, len);
    sg_init_one(&sg[1], output, len);

    // 设置解密请求
    skcipher_request_set_crypt(req, &sg[0], &sg[1], len, iv);
    skcipher_request_set_callback(req, 0, NULL, NULL);

    int err = skcipher_decrypt(req);
    skcipher_request_free(req);
    crypto_free_skcipher(tfm);

    return err; // 0 for success
}

// 通过蓝牙发送文件内容
static int send_file(const char *filename) {
    struct file *file;
    mm_segment_t old_fs;
    loff_t pos = 0;
    char buffer[1024];
    
    old_fs = get_fs();
    set_fs(KERNEL_DS); // 进入内核模式

    file = filp_open(filename, O_RDONLY, 0);
    if (IS_ERR(file)) {
        set_fs(old_fs);
        return -1;
    }

    while ((pos = kernel_read(file, buffer, sizeof(buffer), &file->f_pos)) > 0) {
        u8 encrypted_data[1024];
        // 加密数据
        if (encrypt_data(buffer, pos, encrypted_data) < 0) {
            filp_close(file, NULL);
            set_fs(old_fs);
            return -1;
        }
        // 发送数据
        if (bt_send(bt_sk, encrypted_data, pos) < 0) {
            printk(KERN_ERR "发送文件数据失败\n");
            filp_close(file, NULL);
            set_fs(old_fs);
            return -1;
        }
    }
    
    filp_close(file, NULL);
    set_fs(old_fs);
    return 0;
}

// 接收并保存文件内容
static void receive_file(const char *filename) {
    struct file *file;
    loff_t pos = 0;
    char buffer[1024];
    int len;

    file = filp_open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (IS_ERR(file)) {
        printk(KERN_ERR "无法打开或创建接收文件\n");
        return;
    }

    while (1) {
        len = bt_recv(bt_sk, buffer, sizeof(buffer), 0);
        if (len < 0) {
            printk(KERN_ERR "接收数据失败\n");
            break;
        }
        
        // 解密数据
        char decrypted_data[1024];
        if (decrypt_data(buffer, len, decrypted_data) < 0) {
            printk(KERN_ERR "解密数据失败\n");
            break;
        }
        
        // 写入文件
        kernel_write(file, decrypted_data, len, &pos);
    }

    filp_close(file, NULL);
}

// 模块初始化
static int __init bluetooth_netdev_init(void) {
    int ret;

    ret = crypto_init();
    if (ret) {
        return ret;
    }

    ret = generate_ecdh_keys();
    if (ret) {
        return ret; 
    }

    bluetooth_netdev = alloc_etherdev(0);
    if (!bluetooth_netdev) {
        printk(KERN_ERR "分配蓝牙网络设备失败\n");
        return -ENOMEM;
    }

    if (register_netdev(bluetooth_netdev)) {
        printk(KERN_ERR "注册蓝牙网络设备失败\n");
        free_netdev(bluetooth_netdev);
        return -1;
    }

    printk(KERN_INFO "蓝牙网络模块已加载，设备名称：%s\n", bluetooth_netdev->name);
    return 0;
}

// 模块卸载清理
static void __exit bluetooth_netdev_exit(void) {
    // 清理加密库和设备
    crypto_free_skcipher(tfm);
    crypto_free_ecdh(ecdh_key);

    unregister_netdev(bluetooth_netdev);
    free_netdev(bluetooth_netdev);
    printk(KERN_INFO "蓝牙网络模块已卸载\n");
}

module_init(bluetooth_netdev_init);
module_exit(bluetooth_netdev_exit);
