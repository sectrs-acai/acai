#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include<unistd.h>
#define DATA_SIZE 4032

struct __attribute__((__packed__))  data_t
{
    volatile unsigned long magic;
    volatile unsigned long turn;
    char padding[48];
    char data[DATA_SIZE];
};


struct data_t *data;
int main(int argc, char *argv[])
{

    volatile size_t *_ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    if (_ptr == NULL)
    {
        printf("error\n");
        return -1;
    }
    memset((char *) _ptr, 0, 4096);
    data = (struct data_t *) _ptr;
    data->magic = 0xAABBCCDDEEFF0011;

    __builtin___clear_cache((char *) _ptr, (char *) 4096);
    asm volatile("dmb sy");

    printf("Pin all the pages in memory.\n");
    if(mlockall(MCL_CURRENT | MCL_FUTURE) < 0) {
        perror("mlockall");
        return 1;
    }

    printf("init");
    sleep(2);
    size_t i = 0;
    for(;;)
    {
        printf("before write %ld\n", i);
        __builtin___clear_cache((char *) data, (char *) data + 4096);
        *((volatile size_t *) data->data) = i;
        __builtin___clear_cache((char *) data, (char *) data + 4096);
        asm volatile("dmb sy");

        printf("after write %ld \n", i);

        if (i % 10 == 0) {
             sleep(5);
        }
        i ++;
    }

}

