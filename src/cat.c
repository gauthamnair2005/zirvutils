#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: cat <file>\n");
        return 1;
    }
    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], 0);
        if (fd < 0) {
            printf("cat: %s: no such file\n", argv[i]);
            continue;
        }
        char buf[BUF_SIZE];
        int n;
        while ((n = read(fd, buf, BUF_SIZE)) > 0) {
            write(STDOUT_FILENO, buf, n);
        }
        close(fd);
    }
    return 0;
}
