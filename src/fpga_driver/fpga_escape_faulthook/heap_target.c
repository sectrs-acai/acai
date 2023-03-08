#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    const int P = 4096;
    char *p = malloc(40 * P);
    memset(p, 0, P * 40);

    while (1) {
        printf("%p: %x\n", p, *p);
        sleep(1);
    }
    return 0;
}
