#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include "fh_fixup_header.h"
#include <assert.h>

#define PAGE_SIZE 4096

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int child(void) {
    const long buf1_num = 20;
    char *buf1 = mmap(0, buf1_num * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if (buf1 == MAP_FAILED) {
        printf("MAP_FAILED for buf1\n");
        return -1;
    }
    for (unsigned long i = 0; i < buf1_num * PAGE_SIZE; i ++) {
        buf1[i] = 0x11;
    }

    printf("child pid: %d\n", getpid());
    printf("child: buf: 0x%lx\n", (void*)buf1);
    printf("child num: %d\n", buf1_num);

    volatile int n = 0;
    while(n < 5) {
        printf("child buf[0 page]: %lx ", buf1[0]);
        printf("child buf[1 page]: %lx ", buf1[PAGE_SIZE]);
        printf("child buf[2 page]: %lx ", buf1[PAGE_SIZE * 2]);
        printf("child buf[3 page]: %lx\n", buf1[PAGE_SIZE * 3]);
        sleep(5);
        n++;
    }
}

int parent(int child_pid) {
    int ret = 0;
    const long buf0_num = 50;
    char *buf0 = mmap(0, buf0_num * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if (buf0 == MAP_FAILED) {
        printf("MAP_FAILED for buf0\n");
        return -1;
    }
    for (unsigned long i = 0; i < buf0_num * PAGE_SIZE; i ++) {
        buf0[i] = 0x22;
    }

    ret = fh_fixup_init();
    if (ret < 0) {
        printf("fh_fixup_init failed\n");
        return ret;
    }

    long buf1_num = 0;

    char input[100];
    unsigned long hex_num; // variable to store the parsed hexadecimal number

    while(1) {
        printf("Enter buf: ");
        fgets(input, 100, stdin); // read user input from the console
        hex_num = strtoll(input, NULL, 16);

        if (hex_num != 0) {
            printf("Parsed hexadecimal number: 0x%x\n", hex_num);
            break;
        } else {
            printf("Invalid input!\n");

        }
    }

    unsigned long target_buf[1];
    target_buf[0] = hex_num;
    fh_fixup_map_ctx_t *map_ctx;
    ret = fh_fixup_map(buf0, 1, child_pid, (void **) &target_buf, &map_ctx);
    if (ret < 0) {
        printf("fh_fixup_map returned: %d\n", ret);
    }
    sleep(5);

    ret = fh_fixup_unmap(map_ctx);
    if (ret < 0) {
        printf("fh_fixup_unmap returned: %d\n", ret);
    }


    fh_fixup_cleanup();
    return 0;
}

int main() {
    pid_t pid;
    int x = 1;

    pid = fork();

    if (pid == 0) {
        child();
    } else if (pid > 0) {
        parent(pid);
    } else {
        // fork failed
        printf("Fork failed!\n");
        return 1;
    }
    return 0;
}