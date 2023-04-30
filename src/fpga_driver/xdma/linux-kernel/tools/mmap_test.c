#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <byteswap.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "../xdma/cdev_ctrl.h"

/* ltoh: little endian to host */
/* htol: host to little endian */
#if __BYTE_ORDER==__LITTLE_ENDIAN
#define ltohl(x)       (x)
#define ltohs(x)       (x)
#define htoll(x)       (x)
#define htols(x)       (x)
#elif __BYTE_ORDER==__BIG_ENDIAN
#define ltohl(x)     __bswap_32(x)
#define ltohs(x)     __bswap_16(x)
#define htoll(x)     __bswap_32(x)
#define htols(x)     __bswap_16(x)
#endif

int main(int argc, char **argv)
{

    int err = 0;
    void *map;
    const unsigned long l = 10 * 4096;
    const unsigned long offset = 4 * 4096;
    map = mmap(NULL, l, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
               -1,
               0);

    unsigned long map_offset = (unsigned long) (map + offset);
    if (map==MAP_FAILED) {
        perror("error with mmap\n");
        printf("error");
        exit(1);
    }

    uint8_t read_result = *(((int *) map_offset));
    printf("result: 0x%lx\n", read_result);

    err = mprotect((void *) map_offset, 4096, PROT_EXEC | PROT_READ | PROT_WRITE);
    if (err < 0) {
        perror("mprotect failed");
    }

    int fd = open("/dev/xdma0_control", O_RDWR);
    if (fd < 0) {
        perror("error opening fd\n");
        exit(1);
    }
    struct xdma_ioc_faulthook_mmap io = {0};

    io.addr = (unsigned long) map_offset;
    io.size = 4096;
    int rc = ioctl(fd, XDMA_IOCMMAP, &io);
    if (rc < 0) {
        perror("error ioctl fd\n");
        fprintf(stdout,
                "error: %d\n", rc);
        exit(1);
    }

    printf("starting loop\n");
//    while(1) {
//
//    }
    int  i= 0;
    while (1) {
        i ++;
        int read_result = *(((int *) map_offset));
        printf("result: 0x%lx\n", read_result);
        *(((int *) map_offset)) = i;
        sleep(1);
    }


    munmap(map, l);
    return err;
}
