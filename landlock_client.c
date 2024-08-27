#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define FILE_PATH "/path/to/sensitive/file" // 替换为实际限制的路径

int main() {
    int fd;
    char buffer[256];
    ssize_t bytes_read;

    // 尝试写入受限文件
    fd = open(FILE_PATH, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("Failed to open file for writing");
    } else {
        const char *msg = "Trying to write to restricted file!\n";
        write(fd, msg, strlen(msg));
        close(fd);
        printf("Successfully wrote to the file. This should not happen if Landlock is working.\n");
    }

    // 尝试读取受限文件
    fd = open(FILE_PATH, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open file for reading");
    } else {
        bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read >= 0) {
            buffer[bytes_read] = '\0'; // Null terminate the string
            printf("Read from file: %s\n", buffer);
        }
        close(fd);
    }

    return 0;
}
