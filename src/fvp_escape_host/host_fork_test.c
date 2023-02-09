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

#include "ptedit_header.h"
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
            sleep(2);

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

static void print_affinity() {
    cpu_set_t mask;
    long nproc, i;

    if (sched_getaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
        perror("sched_getaffinity");
        assert(false);
    }
    nproc = sysconf(_SC_NPROCESSORS_ONLN);
    printf("sched_getaffinity = ");
    for (i = 0; i < nproc; i++) {
        printf("%d ", CPU_ISSET(i, &mask));
    }
    printf("\n");
}


static void child(size_t magic)
{
    volatile size_t *_ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    if (_ptr == NULL)
    {
        printf("error\n");
        exit(-1);
    }
    memset((char *) _ptr, 0, 4096);
    *_ptr = magic;

    pid_t pid = getpid();
      sleep(5);
     printf("wrinting value at page boundary: %p , pid: %d\n", _ptr, pid);
    for(;;)
    {
        int cpu;
        int node;
        // print_affinity();
        printf("before write\n");
        printf("%c\n", *((volatile char *) _ptr));
        printf("after write\n");
        printf("some other random access\n");
        *((volatile char *) _ptr + 10);
        printf("some other random access done\n");
         sleep(1);
    }
    printf("done child\n");
}


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
    assert(target != NULL);
    printf("target:\n");
    print_region(target);

    pthread_t thread;
    pthread_create(&thread, NULL, serve_faulthook_thread, NULL);

    enable_trace((void *) target->address, 8, child_pid);

    pthread_join(thread, NULL);

    return 0;

}

static int scan()
{
    int ret = 0;
    fd_hook = open(FAULTHOOK_DIR "hook", O_RDWR | O_NONBLOCK);
    if (fd_hook < 0)
    {
        perror("open fd_hook");
        exit(EXIT_FAILURE);
    }

    void *addr = NULL;
    pid_t pid = 0;

    size_t magic = 0xAABBCCDDEEFF0011;
    int child_pid;
    if ((child_pid = fork()) == 0)
    {
        child(magic);
        return 0;
    } else
    {
        ret = host(child_pid, magic);
        if (ret != 0)
        {
            goto clean_up;
        }
    }


    ret = 0;
    clean_up:
    disable_trace();
    close(fd_hook);
    return ret;

}

static void simple()
{
    int ret = 0;
    fd_hook = open(FAULTHOOK_DIR "hook", O_RDWR | O_NONBLOCK);
    if (fd_hook < 0)
    {
        perror("open fd_hook");
        exit(EXIT_FAILURE);
    }

    volatile size_t *_ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    if (_ptr == NULL)
    {
        printf("error\n");
        exit(-1);
    }
    memset((char *) _ptr, 0, 4096);

    enable_trace((void *) _ptr, 4096, getpid());
    *_ptr = 1;
    close(fd_hook);
}

int main(int argc, char *argv[])
{
    scan();
}
