#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc > 1) {
        if (sethostname(argv[1], strlen(argv[1])) < 0) {
            printf("hostname: failed to set hostname\n");
            return 1;
        }
        return 0;
    }

    char buf[64];
    if (gethostname(buf, sizeof(buf)) == 0) {
        printf("%s\n", buf);
        return 0;
    }
    printf("hostname: failed\n");
    return 1;
}
