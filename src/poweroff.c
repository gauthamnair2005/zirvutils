#include <unistd.h>
#include <stdio.h>

int main(void)
{
    printf("Power off...\n");
    shutdown();
    return 0;
}
