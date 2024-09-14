#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEVICE "/dev/jcore_pit"
#define IOCTL_SET_PERIODIC  _IOW('q', 1, unsigned long)
#define IOCTL_SET_ONESHOT   _IOW('q', 2, unsigned long)
#define LOG_FILE "log.txt"

void log_message(const char *level, const char *message) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        fprintf(log_file, "[%s] %04d-%02d-%02d %02d:%02d:%02d: %s\n", 
                level, 
                t->tm_year + 1900, 
                t->tm_mon + 1, 
                t->tm_mday, 
                t->tm_hour, 
                t->tm_min, 
                t->tm_sec, 
                message);
        fclose(log_file);
    }
}

int main() {
    int fd;
    unsigned long period;

    log_message("INFO", "Starting user_space_app.");

    // 打开设备文件
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        log_message("ERROR", "Failed to open the device.");
        return -1;
    }
    log_message("INFO", "Successfully opened device /dev/jcore_pit.");

    // 设置周期性定时器
    period = 100000; // 100ms
    if (ioctl(fd, IOCTL_SET_PERIODIC, &period) < 0) {
        log_message("ERROR", "Failed to set periodic timer.");
        close(fd);
        return -1;
    }
    printf("Periodic timer set to %lu microseconds\n", period);
    log_message("INFO", "Periodic timer set successfully.");

    // 设置单次定时器
    if (ioctl(fd, IOCTL_SET_ONESHOT, &period) < 0) {
        log_message("ERROR", "Failed to set oneshot timer.");
        close(fd);
        return -1;
    }
    printf("Oneshot timer set to %lu microseconds\n", period);
    log_message("INFO", "Oneshot timer set successfully.");

    // 关闭设备文件
    close(fd);
    log_message("INFO", "Closed device /dev/jcore_pit.");
    return 0;
}
