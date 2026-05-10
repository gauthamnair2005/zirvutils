#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: ping <path>\n");
        printf("Check if a VFS path or network device is reachable.\n");
        return 1;
    }

    const char *target = argv[1];

    int fd = open(target, 0);
    if (fd >= 0) {
        printf("PING %s: 1 targets, 1 reachable\n", target);
        printf("--- %s ping statistics ---\n", target);
        printf("1 packets transmitted, 1 packets received, 0%% packet loss\n");
        close(fd);
        return 0;
    }

    printf("ping: %s: no route to target\n", target);
    return 1;
}
