#include <unistd.h>

int main(void)
{
    const char *yes = "y\n";
    size_t len = 2;
    for (;;) {
        write(1, yes, len);
    }
    return 0;
}
