#include <stdio.h>
#include <unistd.h>

int main(void)
{
    int pid = getpid();
    char cwd[256];
    printf("ZirvUtils System Info:\n");
    printf("  PID:        %d\n", pid);
    if (getcwd(cwd, sizeof(cwd)) > 0)
        printf("  CWD:        %s\n", cwd);
    printf("  Kernel:     Zirvium\n");
    printf("  Userspace:  ZirvUtils\n");
    return 0;
}
