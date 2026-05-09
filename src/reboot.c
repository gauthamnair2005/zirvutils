#include <unistd.h>
#include <stdio.h>

int main(void)
{
    printf("Rebooting...\n");
    reboot();
    return 0;
}
