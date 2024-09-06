#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SYSFS_PATH "/sys/kernel/vfio_mediated_device/device_id"
#define LOG_FILE_PATH "/var/log/vfio_mediated_device_user.log"

class VFIODevice {
public:
    VFIODevice() {
        // 打开设备文件
        device_fd = open(SYSFS_PATH, O_RDWR);
        if (device_fd < 0) {
            log_message("Failed to open device file.");
            throw std::runtime_error("Failed to open device file.");
        }
    }

    ~VFIODevice() {
        if (device_fd >= 0) {
            close(device_fd);
        }
    }

    void set_device_id(int id) {
        std::string id_str = std::to_string(id) + "\n";
        if (write(device_fd, id_str.c_str(), id_str.size()) < 0) {
            log_message("Failed to set device ID.");
            throw std::runtime_error("Failed to set device ID.");
        }
        log_message("Device ID updated successfully.");
    }

    int get_device_id() {
        char buffer[32];
        if (read(device_fd, buffer, sizeof(buffer) - 1) < 0) {
            log_message("Failed to get device ID.");
            throw std::runtime_error("Failed to get device ID.");
        }
        buffer[31] = '\0';  // 确保字符串结束
        return std::stoi(buffer);
    }

private:
    int device_fd;

    void log_message(const std::string& message) {
        std::ofstream log_file(LOG_FILE_PATH, std::ios_base::app);
        if (log_file.is_open()) {
            log_file << message << std::endl;
        } else {
            std::cerr << "Failed to open log file: " << LOG_FILE_PATH << std::endl;
        }
    }
};

int main() {
    try {
        VFIODevice device;

        // 获取当前设备ID并输出
        int current_id = device.get_device_id();
        std::cout << "Current Device ID: " << current_id << std::endl;

        // 设置新的设备ID
        device.set_device_id(current_id + 1);
        std::cout << "Device ID updated." << std::endl;

        // 验证更新后的设备ID
        current_id = device.get_device_id();
        std::cout << "Updated Device ID: " << current_id << std::endl;
    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
