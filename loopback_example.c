#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// 模拟 loopback_cable 结构体
struct loopback_cable {
    unsigned int valid;
    unsigned int running;
    unsigned int pause;
};

// 记录日志到文件
void log_to_file(const char *message) {
    FILE *log_file = fopen("loopback_log.txt", "a");
    if (log_file) {
        fprintf(log_file, "%s\n", message);
        fclose(log_file);
    } else {
        perror("Failed to open log file");
    }
}

// 输出 CSV 数据文件
void output_csv(struct loopback_cable *cable) {
    FILE *csv_file = fopen("loopback_data.csv", "w");
    if (!csv_file) {
        log_to_file("Error: Failed to open CSV file");
        perror("Failed to open CSV file");
        exit(EXIT_FAILURE);
    }

    // 写入表头
    fprintf(csv_file, "valid,running,pause\n");
    // 写入数据
    fprintf(csv_file, "%u,%u,%u\n", cable->valid, cable->running, cable->pause);
    fclose(csv_file);
}

// 主函数
int main() {
    struct loopback_cable cable;
    memset(&cable, 0, sizeof(cable)); // 初始化结构体

    // 模拟一些操作
    cable.valid = 1;
    cable.running = 1;
    cable.pause = 0;

    // 输出 CSV 文件
    output_csv(&cable);
    log_to_file("CSV file created successfully.");

    // 模拟异常
    if (cable.valid == 0) {
        log_to_file("Error: loopback_cable is not valid.");
        fprintf(stderr, "Exception: loopback_cable is not valid, exiting.\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}
