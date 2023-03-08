#include <stdio.h>
#include <string.h>

#include <time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint-gcc.h>

#define STRINGIZING(x) #x
#define STR(x) STRINGIZING(x)
#define FILE_LINE __FILE__ ":" STR(__LINE__)

#define HERE printf(FILE_LINE "\n")

static inline void hex_dump(const void *data, size_t size)
{
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++i)
    {
        printf("%02X ", ((unsigned char *) data)[i]);
        if (((unsigned char *) data)[i] >= ' ' && ((unsigned char *) data)[i] <= '~')
        {
            ascii[i % 16] = ((unsigned char *) data)[i];
        } else
        {
            ascii[i % 16] = '.';
        }
        if ((i + 1) % 8 == 0 || i + 1 == size)
        {
            printf(" ");
            if ((i + 1) % 16 == 0)
            {
                printf("|  %s \n", ascii);
            } else if (i + 1 == size)
            {
                ascii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8)
                {
                    printf(" ");
                }
                for (j = (i + 1) % 16; j < 16; ++j)
                {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
}

#if 0
static void test0()
{
    volatile size_t *_ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    if (_ptr == NULL)
    {
        printf("error\n");
        return -1;
    }
    memset((char *) _ptr, 0, 4096);
    pid_t pid = getpid();


    while(1) {
        printf("wrinting value at page boundary: %p , pid: %d\n", _ptr, pid);
        *((volatile char*) _ptr) = 0xFF;
        sleep(5);
    }


    munmap(_ptr, 4096);
}

#endif

static void test1() {
    size_t phy_addr = 0x2fdf5c000;

    int fd;
    HERE;

    if ((fd = open("/dev/mem", O_RDWR | O_SYNC )) < 0)
    {
        return;
    }
    HERE;

    void *map_base  = mmap(NULL,
                           4096,
                           PROT_READ | PROT_WRITE,
                           MAP_SHARED,
                                 fd,
                           phy_addr);

    HERE;
    if (map_base == MAP_FAILED) {
        perror("map_based mmap failed\n");
        return;
    }
    if (!map_base)
    {
        printf("map_base failed\n");
        return;
    }
    HERE;
    printf("%p \n", map_base);

    char c = *((volatile char*) map_base);
    printf("%d", c);

    HERE;

}
int main(int argc, char *argv[])
{
    // test0();

    test1();

    return 0;
}
