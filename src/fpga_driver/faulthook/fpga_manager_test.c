
#define _GNU_SOURCE

/*
 * Hacky way to have a unit testing file without an extra header file
 */
#define DOING_UNIT_TESTS 1

#include "fpga_manager.c"

/*
 * Test driver to test faulthook api without fvp
 */
#define DEVICE "/dev/xdma0_control"

#define do_fault() \
     fault->turn = FH_TURN_HOST; \
     ret = on_fault((unsigned long) addr, len, pid, target_addr); \
     printf("on_fault returned: %d\n", ret);         \
     assert(fault->turn == FH_TURN_GUEST); \
     print_status(fault->action, &a->common);        \
     if (a->common.ret < 0) {exit(1);}

static int test2(unsigned long addr,
                 unsigned long len,
                 pid_t pid,
                 unsigned long target_addr)
{
    if (ptedit_init()) {
        print_err("Error: Could not initalize PTEditor,"
                  " did you load the kernel module?\n");
        return -1;
    }
    const size_t l = 20 * 4096;
    char *map = mmap(NULL,
                     l,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                     -1,
                     0);

    char *map2 = mmap(NULL,
                      l,
                      PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                      -1,
                      0);


    for (int i = 0; i < len; i++) {
        char *a = (char *) map2 + i;
        *a = (unsigned char) i;
    }
    HERE;
    int ret = 0;
    struct faultdata_struct *fault = (struct faultdata_struct *) map;
    fault->length = 3 * 4096;
    {
        struct action_init_guest *a = (struct action_init_guest *) fault->data;


        HERE;
        fault->action = FH_ACTION_ALLOC_GUEST;
        fault->turn = FH_TURN_HOST;

        ret = on_fault((unsigned long) map, fault->length, pid, (unsigned long) map2);
        printf("on_fault returned: %d\n", ret);
        assert(fault->turn==FH_TURN_GUEST);
        print_status(fault->action, &a->common);
        if (a->common.ret < 0) {exit(1);}
    }
}


static int test(unsigned long addr,
                unsigned long len,
                pid_t pid,
                unsigned long target_addr)
{

    int ret = 0;
    int fd;
    unsigned long host_offset = 0;
    struct faultdata_struct *fault = (struct faultdata_struct *) addr;
    {
        struct action_init_guest *a = (struct action_init_guest *) fault->data;

        fault->action = FH_ACTION_ALLOC_GUEST;
        do_fault();
        host_offset = a->host_offset;
    }
    {
        struct action_openclose_device *a = (struct action_openclose_device *) fault->data;

        strcpy(a->device, DEVICE);
        a->flags = O_RDWR | O_SYNC;
        fault->action = FH_ACTION_OPEN_DEVICE;
        do_fault();
        fd = a->common.fd;
    }

    unsigned long mmap_base_offset = 4 * PAGE_SIZE;
    {
        struct action_mmap_device *a = (struct action_mmap_device *) fault->data;
        memset(a, 0, sizeof(struct action_mmap_device));

        a->common.fd = fd;
        a->mmap_guest_kernel_offset = mmap_base_offset;

        fault->action = FH_ACTION_MMAP;

        // these are within the fvp, apart from length not relevant to us
        a->vm_start = addr + a->mmap_guest_kernel_offset;
        a->vm_end = a->vm_start + 4096;

        a->vm_page_prot = PROT_WRITE | PROT_READ;
        a->vm_pgoff = 0;
        a->vm_flags = MAP_PRIVATE | MAP_ANONYMOUS;
        volatile int *ptr = (volatile int *) a->vm_start;

        printf("device before map: %x\n", *ptr);
        do_fault();

        printf("device: %x\n", *ptr);
        printf("device: %x\n", *ptr + 0x100);
        printf("device: %x\n", *ptr + 0x200);

    }

    #if 0
    {
        struct action_unmap *a = (struct action_unmap *) fault->data;
        memset(a, 0, sizeof(struct action_unmap));

        a->common.fd = fd;
        a->mmap_guest_kernel_offset = mmap_base_offset;
        fault->action = FH_ACTION_UNMAP;
        do_fault();
        volatile int *ptr = (volatile int *) addr + a->mmap_guest_kernel_offset;

        printf("device: %x\n", *ptr);
        printf("device: %x\n", *ptr + 0x100);
        printf("device: %x\n", *ptr + 0x200);
    }
    #endif
    return ret;
}

static void read_file()
{
    char file[] = "/tmp/hooks_mmap";
    int ret;
    FILE *f;
    unsigned long from = 0;
    unsigned long to = 0;
    unsigned long size = 0;
    int pid = 62335;

    f = fopen(file, "r");
    fscanf(f, "0x%lx", &from);
    fscanf(f, "-0x%lx", &to);
    // fscanf(f, ";%d", &pid);
    printf("from: %lx, to %lx\n", from, to);
    printf("pid: %d\n",pid);
    fclose(f);


    if (from!=0 && to!=0 && to > from) {
        size = to - from;
    } else {
        printf("invalid data in %s\n", file);
        exit(1);
    }

    printf("allocating shaddow buffer: %lx-%lx, size %lx\n", from, to, size);

    fh_init_pedit();

    char *map = mmap(NULL,
                     size,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS ,
                     -1,
                     0);
    if (map == MAP_FAILED) {
        perror("mmap failed\n");
        exit(1);
    }
    struct fh_mmap_region_ctx mmap_ctx;

    ret = fh_mmap_region(pid,
                   from,
                   size,
                   map,
                   &mmap_ctx);
    if (ret != 0) {
        printf("mmap region failed: %d\n", ret);
    }
}

int main(int argc, char *argv[])
{
    int ret = 0;
    const size_t magic = FAULTDATA_MAGIC;
    const unsigned long l = 10 * 4096;
    const unsigned long offset = 4 * 4096;

    char *map = mmap(NULL,
                     l,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                     -1,
                     0);


    read_file();


    #if 0
    ret = test((unsigned long) map, l, getpid(),
               (unsigned long) map);
    #endif

    HERE;
    #if 0
    ret = test2((unsigned long) map, l, getpid(),
                (unsigned long) map);

    #endif
    ret = 0;
    clean_up:
    return ret;
}
