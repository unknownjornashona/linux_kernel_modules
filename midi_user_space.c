#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#define DEVICE_PATH "/dev/midiman_device"
#define LOG_FILE "midi_user_space.log"

void log_message(const char *message) {
    FILE *logfile = fopen(LOG_FILE, "a");
    if (logfile) {
        fprintf(logfile, "%s\n", message);
        fclose(logfile);
    } else {
        perror("Failed to open log file");
    }
}

void handle_error(const char *context) {
    char error_message[256];
    sprintf(error_message, "%s: %s", context, strerror(errno));
    log_message(error_message);
    fprintf(stderr, "%s\n", error_message);
}

int main() {
    int fd;
    char buffer[256];
    ssize_t bytes_read, bytes_written;
    
    // 打开设备
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        handle_error("Failed to open device");
        return EXIT_FAILURE;
    }
    
    log_message("Device opened successfully");

    // 写入 MIDI 数据（示例数据）
    const char *midi_data = "\x90\x40\x7F"; // Note On message
    bytes_written = write(fd, midi_data, strlen(midi_data));
    if (bytes_written < 0) {
        handle_error("Failed to write to device");
        close(fd);
        return EXIT_FAILURE;
    }
    
    log_message("MIDI data written successfully");

    // 读取 MIDI 数据（示例）
    memset(buffer, 0, sizeof(buffer));
    bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        handle_error("Failed to read from device");
        close(fd);
        return EXIT_FAILURE;
    }
    
    buffer[bytes_read] = '\0'; // Null-terminate the string
    log_message("MIDI data read successfully");
    printf("Read MIDI data: %s\n", buffer);

    // 关闭设备
    if (close(fd) < 0) {
        handle_error("Failed to close device");
        return EXIT_FAILURE;
    }
    
    log_message("Device closed successfully");
    return EXIT_SUCCESS;
}
