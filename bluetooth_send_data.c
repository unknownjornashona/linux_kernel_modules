#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <sys/socket.h>
#include <sys/types.h>

#define AES_KEY_LENGTH 128
#define BUFFER_SIZE 1024

// AES 加密和解密
static int aes_encrypt(const unsigned char *plaintext, int plaintext_len, unsigned char *ciphertext, unsigned char *key, unsigned char *iv) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;

    if (!(ctx = EVP_CIPHER_CTX_new())) {
        return -1; // 错误处理
    }
    
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv)) {
        return -1; // 错误处理
    }

    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
        return -1; // 错误处理
    }
    ciphertext_len = len;

    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) { // 完成加密
        return -1; // 错误处理
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}

static int aes_decrypt(const unsigned char *ciphertext, int ciphertext_len, unsigned char *plaintext, unsigned char *key, unsigned char *iv) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;

    if (!(ctx = EVP_CIPHER_CTX_new())) {
        return -1; // 错误处理
    }
    
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv)) {
        return -1; // 错误处理
    }

    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
        return -1; // 错误处理
    }
    plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) { // 完成解密
        return -1; // 错误处理
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return plaintext_len;
}

// 发送文件的函数
void send_file(const char *file_path, const char *bt_addr) {
    int sock;
    struct sockaddr_rc addr = { 0 };
    unsigned char key[AES_KEY_LENGTH / 8], iv[AES_KEY_LENGTH / 8];
    unsigned char buffer[BUFFER_SIZE], cipher[BUFFER_SIZE + EVP_MAX_BLOCK_LENGTH];
    FILE *file = fopen(file_path, "rb");

    // 生成 AES 密钥和 IV
    if (!RAND_bytes(key, sizeof(key)) || !RAND_bytes(iv, sizeof(iv))) {
        perror("生成随机数据失败");
        return;
    }

    // 创建蓝牙 socket
    sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t) 1;
    str2ba(bt_addr, &addr.rc_bdaddr);

    // 连接蓝牙设备
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("连接失败");
        return;
    }

    // 读取文件并加密
    while (fread(buffer, 1, sizeof(buffer), file) > 0) {
        int cipher_len = aes_encrypt(buffer, sizeof(buffer), cipher, key, iv);
        send(sock, cipher, cipher_len, 0);
    }

    fclose(file);
    close(sock);
}

// 接收文件的函数
void receive_file(const char *file_path, const char *bt_addr) {
    int sock;
    struct sockaddr_rc addr = { 0 };
    unsigned char key[AES_KEY_LENGTH / 8], iv[AES_KEY_LENGTH / 8];
    unsigned char buffer[BUFFER_SIZE + EVP_MAX_BLOCK_LENGTH], plain[BUFFER_SIZE];
    int bytes_received = 0;

    // 创建蓝牙 socket
    sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t) 1;
    str2ba(bt_addr, &addr.rc_bdaddr);
    
    // 绑定并监听
    bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(sock, 1);
    
    // 等待连接
    int client = accept(sock, NULL, NULL);
    
    // 接收文件并解密
    FILE *file = fopen(file_path, "wb");
    while ((bytes_received = recv(client, buffer, sizeof(buffer), 0)) > 0) {
        int plaintext_len = aes_decrypt(buffer, bytes_received, plain, key, iv);
        fwrite(plain, sizeof(unsigned char), plaintext_len, file);
    }

    fclose(file);
    close(client);
    close(sock);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("用法: %s <send/receive> <文件路径> <蓝牙地址>\n", argv[0]);
        return -1;
    }

    if (strcmp(argv[1], "send") == 0) {
        send_file(argv[2], argv[3]);
    } else if (strcmp(argv[1], "receive") == 0) {
        receive_file(argv[2], argv[3]);
    } else {
        printf("无效的操作: %s\n", argv[1]);
        return -1;
    }

    return 0;
}
