#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void run_target_program() {
    // 这里是我们要执行的目标程序
    char *args[] = {"./target_program", NULL};
    execvp(args[0], args);
}

int main() {
    pid_t child;
    long ptrace_ret;

    // 创建子进程
    child = fork();
    if (child == 0) {
        // 在子进程中运行目标程序
        run_target_program();
    } else if (child > 0) {
        // 在父进程中调用 ptrace
        wait(NULL); // 等待子进程启动

        ptrace_ret = ptrace(PTRACE_ATTACH, child, NULL, NULL); // 尝试附加到子进程
        if (ptrace_ret == -1) {
            perror("ptrace");
            printf("ptrace call failed, possibly due to Yama LSM restrictions.\n");
        } else {
            printf("ptrace call succeeded. Attached to child process: %d\n", child);
            ptrace(PTRACE_DETACH, child, NULL, NULL); // 解除附加
        }

        // 等待子进程结束
        wait(NULL);
    } else {
        perror("fork");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
