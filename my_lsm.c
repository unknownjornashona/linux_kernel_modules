#include <linux/init.h>
#include <linux/module.h>
#include <linux/lsm_hooks.h>
#include <linux/security.h>
#include <linux/cred.h>

// 定义模块信息
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple Yama LSM module to check ptrace calls.");

// 最大安全级别
#define MAX_SECURITY_LEVEL 2

// 安全策略等级变量
static int security_level = 0;

// 审计 ptrace 调用的钩子函数
static int my_ptrace(struct task_struct *target, struct task_struct *task, unsigned long request)
{
    // 检查安全策略等级
    if (security_level < MAX_SECURITY_LEVEL) { // 假设安全等级 2 是禁止 ptrace
        pr_info("Yama LSM: ptrace call denied for PID %d\n", target->pid);
        return -EPERM; // 返回权限拒绝
    }

    return 0; // 允许调用
}

// LSM 初始化
static struct security_hook_list my_hooks[] __lsm_ro_after_init = {
    LSM_HOOK_INIT(ptrace, my_ptrace),
};

// LSM 模块初始化
static int __init my_lsm_init(void)
{
    // 注册 LSM 钩子
    if (!security_add_hooks(my_hooks, ARRAY_SIZE(my_hooks), "my_lsm")) {
        pr_info("Yama LSM: Module loaded with security level %d\n", security_level);
        return 0;
    } else {
        pr_err("Yama LSM: Failed to register hooks\n");
        return -EINVAL; // 注册失败，返回错误
    }
}

// LSM 模块退出
static void __exit my_lsm_exit(void)
{
    pr_info("Yama LSM: Module unloaded\n");
}

// 显示安全等级
static ssize_t security_level_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", security_level);
}

// 设置安全等级
static ssize_t security_level_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int temp_level;

    // 读取输入并解析
    if (sscanf(buf, "%d", &temp_level) != 1) {
        return -EINVAL; // 输入无效
    }

    // 验证输入的安全等级
    if (temp_level < 0 || temp_level > MAX_SECURITY_LEVEL) {
        return -EINVAL; // 超出范围
    }

    security_level = temp_level; // 更新安全等级
    return count;
}

// 创建属性文件以管理安全等级
static struct kobj_attribute security_level_attribute = __ATTR(security_level, 0664, security_level_show, security_level_store);

// 注册内核参数
static int __init my_sysfs_init(void) {
    // 创建 sysfs 文件
    pr_info("Creating sysfs entry for security level\n");
    return sysfs_create_file(kernel_kobj, &security_level_attribute.attr);
}

static void __exit my_sysfs_exit(void) {
    // 清理 sysfs 文件
    pr_info("Removing sysfs entry for security level\n");
    sysfs_remove_file(kernel_kobj, &security_level_attribute.attr);
}

// 模块入口和出口
module_init(my_lsm_init);
module_exit(my_lsm_exit);
