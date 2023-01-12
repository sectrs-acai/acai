//
// Created by abertschi on 1/12/23.
//
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
#include <scanmem/scanmem.h>
#include <scanmem/commands.h>
#include <assert.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ptedit_header.h"
#include "faulthooklib.h"
#include "fpga_manager.h"

#define PAGE_SIZE 4096

struct context_struct {
    bool hook_enabled;
    bool hook_init;
    int fd_hook;
    pid_t victim_pid;
    unsigned long victim_addr;

    char *mmap_host;
    unsigned long mmap_num_of_pages;
    ptedit_entry_t host_pte;
    size_t *host_pte_orig;
};
struct context_struct ctx;

#define log_helper(fmt, ...) printf(fmt "\n%s", __VA_ARGS__)
#define print_progress(...) log_helper(TAG_PROGRESS " "__VA_ARGS__, "")
#define print_ok(...) log_helper(TAG_OK " " __VA_ARGS__, "")
#define print_err(...) log_helper(TAG_FAIL " " __VA_ARGS__, "")


void print_region(extract_region_t *r)
{
    print_progress("[%2lu] "POINTER_FMT", %2u + "POINTER_FMT", %5s (%d)\n",
                   r->num, r->address, r->region_id,
                   r->match_off, r->region_type_str, r->region_type);
}

static extract_region_t *extract_regions(globals_t *vars)
{
    assert(vars->num_matches > 0);

    const char *region_names[] = REGION_TYPE_NAMES;
    unsigned long num = 0;
    element_t *np = NULL;

    extract_region_t *res = calloc(vars->num_matches,
                                   sizeof(extract_region_t));
    if (!res) {
        printf("malloc failed\n");
        exit(-1);
    }

    np = vars->regions->head;
    matches_and_old_values_swath *reading_swath_index = vars->matches->swaths;
    size_t reading_iterator = 0;

    while (reading_swath_index->first_byte_in_child) {
        match_flags flags = reading_swath_index->data[reading_iterator].match_info;
        if (flags!=flags_empty) {
            void *address = remote_address_of_nth_element(reading_swath_index,
                                                          reading_iterator);
            unsigned long address_ul = (unsigned long) address;
            unsigned int region_id = 99;
            unsigned long match_off = 0;
            const char *region_type_str = "??";
            int region_type = -1;
            while (np) {
                region_t *region = np->data;
                unsigned long region_start = (unsigned long) region->start;
                if (address_ul < region_start + region->size &&
                        address_ul >= region_start) {
                    region_id = region->id;
                    match_off = address_ul - region->load_addr;
                    region_type_str = region_names[region->type];
                    region_type = region->type;
                    break;
                }
                np = np->next;
            }

            extract_region_t *curr = &res[num];
            curr->num = num;
            curr->address = address_ul;
            curr->region_id = region_id;
            curr->match_off = match_off;
            curr->region_type_str = region_type_str;
            curr->region_type = region_type;
            num++;
        }
        ++reading_iterator;
        if (reading_iterator >= reading_swath_index->number_of_bytes) {
            reading_swath_index = local_address_beyond_last_element(reading_swath_index);
            reading_iterator = 0;
        }
    }
    assert(vars->num_matches==num);
    return res;
}

int fh_scan_guest_memory(pid_t pid,
                         unsigned long magic,
                         extract_region_t **ret_regions,
                         int *ret_regions_len
)
{
    char magic_buf[64];
    globals_t *vars = &sm_globals;
    vars->target = pid;
    sprintf(magic_buf, "0x%x", magic);

    if (!sm_init()) {
        printf("failed to init\n");
        return -1;
    }
    if (getuid()!=0) {
        printf("We need sudo rights to scan all regions\n");
    }
    if (sm_execcommand(vars, "reset")==false) {
        vars->target = 0;
    }
    if (sm_execcommand(vars, magic_buf)==false) {
        printf("Magic %s not found in address space\n", magic_buf);
        return -1;
    }
    *ret_regions_len = vars->num_matches;
    *ret_regions = extract_regions(vars);
    sm_cleanup();

    return 0;
}

static int fh_memory_unmap(void)
{
    if (ctx.mmap_host==NULL) return 0;

    assert(ctx.host_pte_orig!=NULL);
    *ctx.host_pte_orig = ctx.host_pte.pte;
    ptedit_cleanup();
    munmap(ctx.mmap_host, ctx.mmap_num_of_pages * PAGE_SIZE);
    ctx.mmap_host = NULL;
    ctx.mmap_num_of_pages = 0;
    ctx.host_pte_orig = NULL;

    return 0;
}

static int fh_memory_map(pid_t pid, unsigned long addr, unsigned long num_of_pages)
{
    int ret = 0;
    char *pt;
    assert(addr!=0);
    assert(num_of_pages==1);
    const size_t tot_len = 4096 * num_of_pages;

    char *host = mmap(0,
                      tot_len,
                      PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS,
                      -1,
                      0);
    if (!host) {
        printf(TAG_FAIL "ERROR: could not mmap shared buf\n");
        ret = -1;
        goto clean_up;
    }
    memset(host, 0, tot_len);

    if (ptedit_init()) {
        printf(TAG_FAIL "Error: Could not initalize PTEditor,"
               " did you load the kernel module?\n");
        ret = -1;
        goto clean_up;
    }

    ptedit_entry_t victim = ptedit_resolve((void *) addr, pid);

    /* "target" uses the manipulated page-table entry */
    ptedit_entry_t host_entry = ptedit_resolve(host, 0);

    /* "pt" should map the page table corresponding to "target" */
    size_t pt_pfn = ptedit_cast(host_entry.pmd, ptedit_pmd_t).pfn;
    pt = ptedit_pmap(pt_pfn * ptedit_get_pagesize(), ptedit_get_pagesize());

    /* "target" entry is bits 12 to 20 of "target" virtual address */
    size_t entry = (((size_t) host) >> 12) & 0x1ff;
    size_t *mapped_entry = ((size_t *) pt) + entry;

    /* "mapped_entry" is a user-space-accessible pointer to the PTE of "target" */
    if (*mapped_entry!=host_entry.pte) {
        ret = -1;
        goto clean_up;
    }
    /* let "target" point to "secret" */
    *mapped_entry = ptedit_set_pfn(*mapped_entry, ptedit_get_pfn(victim.pte));

    ptedit_invalidate_tlb(host);
    /*
     * host points page aligned to the target page of interest
     */
    ctx.mmap_host = host;
    ctx.mmap_num_of_pages = num_of_pages;
    ctx.host_pte = host_entry;
    ctx.host_pte_orig = mapped_entry;

    return 0;
    clean_up:
    if (host) {
        free(host);
    }
    return ret;
}

static int fh_enable_trace(unsigned long address, unsigned long len, pid_t pid)
{
    int ret = 0;
    struct faulthook_ctrl ctrl;
    ctrl.active = 1;
    ctrl.address = (unsigned long) address;
    ctrl.pid = pid;
    ctrl.len = len;

    ret = ioctl(ctx.fd_hook, FAULTHOOK_IOCTL_CMD_STATUS, &ctrl);
    if (ret < 0) {
        perror("FAULTHOOK_IOCTL_CMD_STATUS failed");
    } else {
        ctx.hook_enabled = 1;
        ctx.victim_pid = pid;
        ctx.victim_addr = address;
    }
    return ret;
}

static int fh_disable_trace()
{
    print_progress("disabling trace\n");
    if (!ctx.hook_enabled) {
        return 0;
    }
    int ret = 0;
    struct faulthook_ctrl ctrl;
    ctrl.active = 0;
    ctrl.address = 0;
    ctrl.pid = 0;

    ret = ioctl(ctx.fd_hook, FAULTHOOK_IOCTL_CMD_STATUS, (size_t) &ctrl);
    if (ret < 0) {
        perror("FAULTHOOK_IOCTL_CMD_STATUS failed");
    } else {
        ctx.hook_enabled = 0;
        ctx.victim_pid = 0;
        ctx.victim_addr = 0;
    }
    return ret;
}

static int fh_clean_up()
{
    print_progress("cleaning up data\n");
    if (ctx.mmap_host) {
        fh_memory_unmap();
    }
    close(ctx.fd_hook);
    return 0;
}


static void crash_handler(int code)
{
    fh_disable_trace();
    fh_clean_up();
}

static int fh_init()
{
    memset(&ctx, 0, sizeof(struct context_struct));

    signal(SIGINT, crash_handler);
    signal(SIGTERM, crash_handler);
    signal(SIGSEGV, crash_handler);

    int fd = open(FAULTHOOK_DIR "hook", O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        perror("open fd_hook");
        return fd;
    }
    ctx.fd_hook = fd;
    return 0;
}


static void on_fault(void)
{
    unsigned long victim_offset = ctx.victim_addr & 0xFFF;
    unsigned long len = PAGE_SIZE - victim_offset;
    unsigned long victim_addr = ctx.victim_addr & ~(0xFFF);
    unsigned long addr = ((unsigned long) ctx.mmap_host) + victim_offset - /* magic field */ 8;

    print_progress("on_fault");
    hex_dump((void *) addr, 64);

    struct faultdata_struct* fault = (struct faultdata_struct*) addr;
    fault->action = FH_ACTION_GUEST_CONTINUE;

}

static void *fh_handler(void *vargp)
{
    int ret = 0;
    short revents;
    struct pollfd pfd = {
            .events = POLL_IN,
            .fd = ctx.fd_hook
    };
    print_progress("Listening for faulthooks...\n");

    while (true) {
        ret = poll(&pfd, 1, -1);
        if (ret==-1) {
            perror("poll error");
            break;
        }
        revents = pfd.revents;

        if (revents & POLLIN) {
            on_fault();
            ret = ioctl(ctx.fd_hook, FAULTHOOK_IOCTL_CMD_HOOK_COMPLETED);
            if (ret < 0) {
                perror("FAULTHOOK_IOCTL_CMD_HOOK_COMPLETED failed");
                goto exit;
            }
        }
    }

    exit:
    pthread_exit(0);
}

static void *find_magic_region(
        pid_t pid,
        unsigned long magic)
{
    int ret = 0;
    extract_region_t *regions;
    int regions_len = 0;
    if (fh_scan_guest_memory(
            pid,
            magic,
            &regions,
            &regions_len)) {
        return NULL;
    }
    assert(regions_len > 0);

    extract_region_t *magic_region = NULL;

    for (int i = 0; i < regions_len; i++) {
        extract_region_t *r = regions + i;

        // region must be on heap
        if (r->region_type==0) {
            magic_region = r;
        }
        print_region(regions + i);
    }

    if (regions_len==1) {
        return (void *) regions->address;
    } else if (regions_len > 0) {
        print_progress("select region by index: ");

        int select;
        scanf("%d", &select);
        magic_region = regions + select;
        return (void *) magic_region->address;
    }
    print_err("No regions found on heap\n");
    return NULL;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    if (argc < 2) {
        print_err("Need pid as argument\n");
        return -1;
    }
    pid_t pid = strtol(argv[1], NULL, 10);
    print_progress("pid: %d", pid);

    if ((ret = fh_init())!=0) {
        print_err("fh_init failed: %d\n", ret);
        return -1;
    };
    const size_t magic = FAULTDATA_MAGIC;
    void *addr = find_magic_region(pid, magic);
    if (!addr) {
        ret = -1;
        goto clean_up;
    }
    print_ok("identified target address: 0x%xl in pid %d", addr, pid);

    if (fh_memory_map(pid, (unsigned long) addr, 1)) {
        print_err("fh_memory_map failed\n");
        ret = -1;
        goto clean_up;
    }
    print_ok("Mapped target 0x%lx into address space of host", addr);

    pthread_t fh_thread;
    pthread_create(&fh_thread, NULL, fh_handler, NULL);

    /*
     * We enable faulthook on 8 bytes starting at index 8
     */
    fh_enable_trace((unsigned long) addr + 8, 8, pid);
    pthread_join(fh_thread, NULL);

    ret = 0;
    clean_up:
    fh_disable_trace();
    fh_memory_unmap();
    fh_clean_up();
    return ret;
}
