#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/pem.h>

#define FILE_PATH "/path/to/your/file.txt" // 替换为实际路径
#define LOG_FILE "smack_log.txt"
#define AES_KEYLENGTH 256

void log_message(const char *message) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file) {
        fprintf(log_file, "%s\n", message);
        fclose(log_file);
    }
}

// 生成ECDH密钥对
void generate_ecdh_key_pair(EC_KEY **private_key, unsigned char **public_key, int *public_key_len) {
    *private_key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    EC_KEY_generate_key(*private_key);
    
    *public_key_len = i2o_ECPublicKey(*private_key, public_key);
}

// ECDH密钥交换
void derive_shared_key(EC_KEY *my_private_key, unsigned char *peer_public_key, int peer_public_key_len, unsigned char *shared_key) {
    EC_KEY *peer_key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    o2i_ECPublicKey(&peer_key, (const unsigned char**)&peer_public_key, peer_public_key_len);
    
    // 使用ECDH协商共享密钥
    ECDH_compute_key(shared_key, AES_KEYLENGTH / 8, EC_KEY_get0_public_key(peer_key), my_private_key, NULL);
    
    EC_KEY_free(peer_key);
}

// ECIES加密
int ecies_encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *shared_key, unsigned char *ciphertext) {
    EVP_CIPHER_CTX *cipher_ctx = EVP_CIPHER_CTX_new();
    unsigned char iv[16]; // 初始化IV
    RAND_bytes(iv, sizeof(iv)); // 生成随机IV

    if (EVP_EncryptInit_ex(cipher_ctx, EVP_aes_256_cbc(), NULL, shared_key, iv) != 1) {
        log_message("EVP_EncryptInit_ex failed.");
        EVP_CIPHER_CTX_free(cipher_ctx);
        return -1;
    }

    int len;
    EVP_EncryptUpdate(cipher_ctx, ciphertext, &len, plaintext, plaintext_len);
    int ciphertext_len = len;

    EVP_EncryptFinal_ex(cipher_ctx, ciphertext + len, &len);
    ciphertext_len += len;
    
    // 前缀IV到密文
    memcpy(ciphertext + ciphertext_len, iv, sizeof(iv));
    
    EVP_CIPHER_CTX_free(cipher_ctx);
    return ciphertext_len + sizeof(iv); // 返回IV长度
}

// ECIES解密
int ecies_decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *shared_key, unsigned char *plaintext) {
    EVP_CIPHER_CTX *cipher_ctx = EVP_CIPHER_CTX_new();
    unsigned char iv[16];

    // 提取IV
    memcpy(iv, ciphertext + ciphertext_len - sizeof(iv), sizeof(iv));
    ciphertext_len -= sizeof(iv);
    
    if (EVP_DecryptInit_ex(cipher_ctx, EVP_aes_256_cbc(), NULL, shared_key, iv) != 1) {
        log_message("EVP_DecryptInit_ex failed.");
        EVP_CIPHER_CTX_free(cipher_ctx);
        return -1;
    }

    int len;
    if (EVP_DecryptUpdate(cipher_ctx, plaintext, &len, ciphertext, ciphertext_len) != 1) {
        log_message("EVP_DecryptUpdate failed.");
        EVP_CIPHER_CTX_free(cipher_ctx);
        return -1;
    }
    int plaintext_len = len;

    if (EVP_DecryptFinal_ex(cipher_ctx, plaintext + len, &len) != 1) {
        log_message("EVP_DecryptFinal_ex failed.");
        EVP_CIPHER_CTX_free(cipher_ctx);
        return -1;
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(cipher_ctx);
    return plaintext_len; // 返回实际解密长度
}

int main() {
    // 设置进程标签（模拟）
    const char *label = "restricted"; // 假设这是必须的标签
    log_message("Setting process label to restricted.");

    // 生成ECDH密钥对
    EC_KEY *my_private_key;
    unsigned char *my_public_key = NULL;
    int my_public_key_len;
    generate_ecdh_key_pair(&my_private_key, &my_public_key, &my_public_key_len);

    // 假设平滑代码的方式获取对方的公钥，这里用随机生成的公钥做示例
    unsigned char *peer_public_key = my_public_key; // 替换为真正的对方公钥
    int peer_public_key_len = my_public_key_len; 

    // 导出共享密钥
    unsigned char shared_key[AES_KEYLENGTH / 8];
    derive_shared_key(my_private_key, peer_public_key, peer_public_key_len, shared_key);

    // 尝试读取文件并加密内容
    unsigned char ciphertext[1024];
    int fd = open(FILE_PATH, O_RDONLY);
    
    if (fd == -1) {
        log_message("Failed to open file for reading.");
        return 1; // 失败
    }

    char buffer[256];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read >= 0) {
        buffer[bytes_read] = '\0'; // Null terminate the string
        int ciphertext_len = ecies_encrypt((unsigned char *)buffer, bytes_read, shared_key, ciphertext);
        if (ciphertext_len < 0) {
            log_message("Encryption failed.");
            close(fd);
            return 1; // 失败
        } else {
            log_message("Encryption successful.");
        }
    } else {
        log_message("Read failed.");
        close(fd);
        return 1; // 失败
    }
    close(fd);

    // 尝试解密
    unsigned char decryptedtext[256];
    int decrypted_len = ecies_decrypt(ciphertext, bytes_read + sizeof(unsigned char[16]), shared_key, decryptedtext);
    if (decrypted_len >= 0) {
        decryptedtext[decrypted_len] = '\0'; // Null terminate the string
        log_message("Decrypted content: ");
        log_message((const char *)decryptedtext);
    } else {
        log_message("Decryption failed.");
    }

    // 释放EC_KEY
    EC_KEY_free(my_private_key);
    free(my_public_key);

    return 0;
}
