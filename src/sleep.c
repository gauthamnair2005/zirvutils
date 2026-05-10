#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: sleep <seconds>\n");
        return 1;
    }

    long sec = atol(argv[1]);
    if (sec <= 0) return 0;

    uint64_t start = uptime();
    while (uptime() - start < (uint64_t)sec) {
        /* busy-wait — no scheduler to yield to */
    }

    return 0;
}
