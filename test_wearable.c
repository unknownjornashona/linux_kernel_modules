#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define DEVICE_PATH "/dev/wearable_dev"

int main() {
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    // 写入数据
    const char *data = "Hello, wearable device!";
    write(fd, data, strlen(data));

    // 读取数据
    char buffer[256];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read >= 0) {
        buffer[bytes_read] = '\0'; // Null-terminate
        printf("Read from device: %s\n", buffer);
    } else {
        perror("Failed to read from device");
    }

    close(fd);
    return 0;
}
