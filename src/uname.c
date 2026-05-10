#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int all = 0;
    if (argc > 1) {
        const char *opt = argv[1];
        if (opt[0] == '-' && opt[1] == 'a') all = 1;
    }

    if (all || argc == 1) {
        printf("Zirvium zirvium 0.1.0 x86_64 Zirvium\n");
    }
    return 0;
}
