#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(void)
{
    const char *devs[] = {"/zirv/net/eth0", "/zirv/net/wlan0", "/zirv/net/bt0", NULL};

    printf("Network Interfaces:\n");
    for (int i = 0; devs[i]; i++) {
        int fd = open(devs[i], 0);
        if (fd >= 0) {
            printf("  %s: UP\n", devs[i]);
            close(fd);
        }
    }

    /* Check if any were found */
    int found = 0;
    for (int i = 0; devs[i]; i++) {
        int fd = open(devs[i], 0);
        if (fd >= 0) { found = 1; close(fd); }
    }
    if (!found)
        printf("  (no network devices found)\n");

    return found ? 0 : 1;
}
