//
// Created by abertschi on 1/12/23.
//
#define _GNU_SOURCE

#include <scanmem/scanmem.h>
#include <scanmem/commands.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ptedit_header.h"
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <poll.h>
#include "fh_host.h"

struct fh_host_context fh_ctx;

fh_host_fn void _fh_crash_handler(int code)
{
    printf("Crash handler called\n");
    fh_disable_trace();
    fh_clean_up();
}

fh_host_fn void *_fh_fault_handler(void *vargp)
{
    int ret = 0;
    short revents;
    struct pollfd pfd = {
            .events = POLL_IN,
            .fd = fh_ctx.fd_hook
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
            if (fh_ctx.listener) {
                HERE;
                ret = (*fh_ctx.listener)();
                print_progress("function returned: %d\n", ret);
                HERE;
                if (ret) {
                    goto exit;
                }
                HERE;
            }
            ret = ioctl(fh_ctx.fd_hook, FAULTHOOK_IOCTL_CMD_HOOK_COMPLETED);
            if (ret < 0) {
                perror("FAULTHOOK_IOCTL_CMD_HOOK_COMPLETED failed");
                goto exit;
            }
        }
    }

    exit:
    pthread_exit(0);
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

fh_host_fn void fh_print_region(extract_region_t *r)
{
    print_progress("[%2lu] "PTR_FMT", %2u + "PTR_FMT", %5s (%d)\n",
                   r->num,
                   r->address,
                   r->region_id,
                   r->match_off,
                   r->region_type_str,
                   r->region_type);
}

fh_host_fn int fh_scan_guest_memory(pid_t pid,
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

fh_host_fn int fh_memory_unmap(struct fh_memory_map_ctx *req)
{
    if (req->host_mem==NULL) {
        return 0;
    }
    *(req->_host_pt_entry) = req->_host_pt.pte;
    // TODO: invalidate tlb?
    return 0;
}

fh_host_fn int fh_memory_map(struct fh_memory_map_ctx *req)
{
    int ret = 0;
    char *pt;

    unsigned long addr = req->addr;
    int pid = req->pid;
    char *host_mem = req->host_mem;

    ptedit_entry_t victim = ptedit_resolve((void *) addr, pid);

    /* "target" uses the manipulated page-table entry */
    ptedit_entry_t host_entry = ptedit_resolve(host_mem, 0);

    /* "pt" should map the page table corresponding to "target" */
    size_t pt_pfn = ptedit_cast(host_entry.pmd, ptedit_pmd_t).pfn;
    pt = ptedit_pmap(pt_pfn * ptedit_get_pagesize(), ptedit_get_pagesize());

    /* "target" entry is bits 12 to 20 of "target" virtual address */
    size_t entry = (((size_t) host_mem) >> 12) & 0x1ff;
    size_t *mapped_entry = ((size_t *) pt) + entry;

    /* "mapped_entry" is a user-space-accessible pointer to the PTE of "target" */
    if (*mapped_entry!=host_entry.pte) {
        ret = -1;
        goto clean_up;
    }
    /* let "target" point to "secret" */
    *mapped_entry = ptedit_set_pfn(*mapped_entry, ptedit_get_pfn(victim.pte));

    ptedit_invalidate_tlb(host_mem);
    /*
     * host points page aligned to the target page of interest
     */
    req->_host_pt = host_entry;
    req->_host_pt_entry = mapped_entry;

    return 0;
    clean_up:
    return ret;
}

fh_host_fn int fh_enable_trace(
        unsigned long address,
        unsigned long len,
        pid_t pid)
{
    int ret = 0;
    struct faulthook_ctrl ctrl;
    ctrl.active = 1;
    ctrl.address = (unsigned long) address;
    ctrl.pid = pid;
    ctrl.len = len;

    ret = ioctl(fh_ctx.fd_hook, FAULTHOOK_IOCTL_CMD_STATUS, &ctrl);
    if (ret < 0) {
        perror("FAULTHOOK_IOCTL_CMD_STATUS failed");
    } else {
        fh_ctx.hook_enabled = 1;
        fh_ctx.victim_pid = pid;
        fh_ctx.victim_addr = address;
    }
    return ret;
}

fh_host_fn int fh_disable_trace(void)
{
    if (!fh_ctx.hook_enabled) {
        return 0;
    }
    int ret = 0;
    struct faulthook_ctrl ctrl;
    ctrl.active = 0;
    ctrl.address = 0;
    ctrl.pid = 0;

    ret = ioctl(fh_ctx.fd_hook, FAULTHOOK_IOCTL_CMD_STATUS, (size_t) &ctrl);
    if (ret < 0) {
        perror("FAULTHOOK_IOCTL_CMD_STATUS failed");
    } else {
        fh_ctx.hook_enabled = 0;
        fh_ctx.victim_pid = 0;
        fh_ctx.victim_addr = 0;
    }
    return ret;
}

fh_host_fn int fh_clean_up(void)
{
    close(fh_ctx.fd_hook);
    memset(&fh_ctx, 0, sizeof(struct fh_host_context));

    ptedit_cleanup();
    return 0;
}

fh_host_fn pthread_t *run_thread(fh_listener_fn fn)
{
    fh_ctx.listener = fn;
    pthread_create(
            &fh_ctx.listener_thread,
            NULL,
            _fh_fault_handler,
            NULL);
    return &fh_ctx.listener_thread;
}

fh_host_fn int fh_init(const char *device)
{
    if (device==NULL) {
        device = FAULTHOOK_DIR "hook";
    }
    memset(&fh_ctx, 0, sizeof(struct fh_host_context));

    /*
     * If something fails we have to make sure to clean up page table modifications.
     * On a sudden crash the system may become completely unusable!
     */
    signal(SIGINT, _fh_crash_handler);
    signal(SIGTERM, _fh_crash_handler);
    signal(SIGSEGV, _fh_crash_handler);
    signal(SIGABRT, _fh_crash_handler);

    if (ptedit_init()) {
        print_err("Error: Could not initalize PTEditor,"
                  " did you load the kernel module?\n");
        return -1;
    }

    int fd = open(device, O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        perror("open fd_hook");
        return fd;
    }
    fh_ctx.fd_hook = fd;
    return 0;
}

