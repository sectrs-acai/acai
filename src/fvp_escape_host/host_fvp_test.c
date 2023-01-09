#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <poll.h>

#define FILE_LINE __FILE__ ":" STR(__LINE__)
#define PTR_FMT "0x%llx"
#define HERE printf(FILE_LINE "\n")


#include "pagescan.h"

#include <pteditor/ptedit_header.h>
#include <sched.h>
#include <sched.h>

#define FAULTHOOK_DIR "/sys/kernel/debug/faulthook/"
int fd_hook;

#if 1

#define FAULTHOOK_IOCTL_MAGIC_NUMBER (long)0x3d18

#define FAULTHOOK_IOCTL_CMD_STATUS \
  _IOR(FAULTHOOK_IOCTL_MAGIC_NUMBER, 1, size_t)

#define FAULTHOOK_IOCTL_CMD_HOOK_COMPLETED \
  _IOR(FAULTHOOK_IOCTL_MAGIC_NUMBER, 2, size_t)

struct faulthook_ctrl
{
    bool active;
    pid_t pid;
    unsigned long address;
    unsigned long len;
};

#endif

#include <signal.h>

static volatile int keepRunning = 1;



static void enable_trace(void *address, unsigned long len, pid_t pid)
{
    int ret = 0;
    struct faulthook_ctrl ctrl;
    ctrl.active = 1;
    ctrl.address = (unsigned long) address;
    ctrl.pid = pid;
    ctrl.len = len;

    ret = ioctl(fd_hook, FAULTHOOK_IOCTL_CMD_STATUS, &ctrl);
    if (ret < 0)
    {
        perror("FAULTHOOK_IOCTL_CMD_STATUS failed");
        exit(EXIT_FAILURE);
    }
}

static void disable_trace()
{
    int ret = 0;
    struct faulthook_ctrl ctrl;
    ctrl.active = 0;
    ctrl.address = 0;
    ctrl.pid = 0;

    ret = ioctl(fd_hook, FAULTHOOK_IOCTL_CMD_STATUS, (size_t) &ctrl);
    if (ret < 0)
    {
        perror("FAULTHOOK_IOCTL_CMD_STATUS failed");
        exit(EXIT_FAILURE);
    }
}

static void *serve_faulthook_thread(void *vargp)
{
    printf("serve_faulthook_thread\n");

    int ret;
    struct pollfd pfd;
    short revents;

    pfd.fd = fd_hook;
    pfd.events = POLLIN;

    int served = 0;

    while (true)
    {
        ret = poll(&pfd, 1, -1);
        if (ret == -1)
        {
            perror("poll error");
            break;
        }
        revents = pfd.revents;

        if (revents & POLLIN)
        {
            printf("doing something with the data....\n");
            sleep(5);

            ret = ioctl(fd_hook, FAULTHOOK_IOCTL_CMD_HOOK_COMPLETED);
            if (ret < 0)
            {
                perror("FAULTHOOK_IOCTL_CMD_HOOK_COMPLETED failed");
                goto exit;
            }
        }
    }


    exit:
    printf("exiting serve_faulthook_thread\n");
    pthread_exit(0);
    return NULL;
}

#include <assert.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int host(int child_pid, size_t magic)
{
    printf("Hello from Parent!\n");
    char buf[32];
    sprintf(buf, PTR_FMT, magic);
    extract_region_t *regs = NULL;
    int size = 0;
    if (scan_guest_memory(child_pid,
                          buf,
                          &regs,
                          &size) != 0)
    {
        printf("scan failed\n");
        return -1;
    }
    if (size == 0)
    {
        printf("no target found\n");
        exit(-1);
    }

    extract_region_t *target = NULL;
    for (int i = 0; i < size; i++)
    {
        extract_region_t *c = regs + i;
        if (c->region_type == 0)
        {
            // misc
            target = c;
        }
        print_region(regs + i);

    }

    printf("select region\n");
    int select;
    scanf("%d", &select);
    target = regs + select;

    assert(target != NULL);
    printf("target:\n");
    print_region(target);

    pthread_t thread;
    pthread_create(&thread, NULL, serve_faulthook_thread, NULL);

    enable_trace((void *) target->address + 64, 8, child_pid);

    pthread_join(thread, NULL);

    return 0;

}
void intHandler(int dummy) {
    disable_trace();
    close(fd_hook);
}

int main(int argc, char *argv[])
{
    int ret = 0;
    signal(SIGINT, intHandler);
    fd_hook = open(FAULTHOOK_DIR "hook", O_RDWR | O_NONBLOCK);
    if (fd_hook < 0)
    {
        perror("open fd_hook");
        exit(EXIT_FAILURE);
    }

    if (argc < 1)
    {
        printf("  <pid> - PID of process to target\n");
        return -1;
    }
    pid_t pid = strtol(argv[1], NULL, 10);
    printf(" * Launching with a target PID of: %zd\n", pid);

    void *addr = (void *) strtol(argv[1], NULL, 0);
    printf(" * Launching with a target address of 0x%llx\n", addr);

    size_t magic = 0xAABBCCDDEEFF0011;
    ret = host(pid, magic);
    if (ret != 0)
    {
        goto clean_up;
    }


    ret = 0;
    clean_up:
    disable_trace();
    close(fd_hook);
    return ret;
}
