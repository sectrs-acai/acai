#include <stdio.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include<unistd.h>

int main(int argc, char *argv[])
{

	printf("Testing\n");

    unsigned long *map = mmap(NULL,
               5000,
               PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS,
               -1,
               0);

    *map = 0xAABBCCDDAABBCCDD;
    int i = 0;
    while(1) {
        sleep(1);
        printf("i: %d\n", i);
        *(map + 1) +=1;
    }

    return 0;
}
