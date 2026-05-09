#include <unistd.h>
#include <stdio.h>

int main(void)
{
    printf("Shutting down...\n");
    shutdown();
    return 0;
}
