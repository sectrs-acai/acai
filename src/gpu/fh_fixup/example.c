#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include "fh_fixup_header.h"

#define PAGE_SIZE 4096

int main(int argc, char *argv[])
{
    int ret = 0;
    ret = fh_fixup_init();
    if (ret < 0) {
        printf("fh_fixup_init failed\n");
        return ret;
    }
    const long buf0_size = PAGE_SIZE * 10;
    char *buf0 = mmap(0, buf0_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if (buf0 == MAP_FAILED) {
        printf("MAP_FAILED for buf0\n");
        return -1;
    }
    for (unsigned long i = 0; i < buf0_size; i ++) {
        buf0[i] = 0x22;
    }
    const long buf1_size = PAGE_SIZE * 5;
    char *buf1 = mmap(0, buf1_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if (buf1 == MAP_FAILED) {
        printf("MAP_FAILED for buf1\n");
        return -1;
    }
    for (unsigned long i = 0; i < buf1_size; i ++) {
        buf1[i] = 0x11;
    }
    printf("src_buf: %lx, target_buf: %lx\n", buf0, buf1);
    pid_t pid = getpid();
    size_t src_buf[1];
    src_buf[0] = (unsigned long) buf0;
    size_t target_buf[1];
    target_buf[0] = (unsigned long) buf1;

    fh_fixup_map_ctx_t *map_ctx;
    ret = fh_fixup_map((void **) src_buf, 1, pid, (void **) target_buf, &map_ctx);
    if (ret < 0) {
        printf("fh_fixup_map returned: %d\n", ret);
    }
    if (buf1[0] == 0x22) {
        for(int i = 0; i < PAGE_SIZE; i ++){
            if (buf1[i] != buf0[i]) {
                printf("buf1 != buf0 at i %d: buf1=%x, buf0=%x\n", i, buf1[i], buf0[i]);
            }
        }
        printf ("success: buf1 mapped into buf0\n");
    }
    else if (buf1[0] == 0x11) {
        printf ("fail: buf1 mapping unchanged\n");
    } else {
        printf("fail: buf1 mapping trash: %x\n", buf1[1]);
    }

    ret = fh_fixup_unmap(map_ctx);
    if (ret < 0) {
        printf("fh_fixup_unmap returned: %d\n", ret);
    }
    fh_fixup_cleanup();
    return 0;
}