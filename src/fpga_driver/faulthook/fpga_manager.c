//
// Created by abertschi on 1/12/23.
//

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>

#include "fh_host_header.h"
#include "fpga_manager.h"

static void *find_magic_region(
        pid_t pid,
        unsigned long magic)
{
    // TODO: CLean up
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
    if (regions_len==0) {
        free(regions);
        return NULL;
    }

    extract_region_t *magic_region = NULL;

    for (int i = 0; i < regions_len; i++) {
        extract_region_t *r = regions + i;

        // region must be on heap
        if (r->region_type==0) {
            magic_region = r;
        }
        fh_print_region(regions + i);
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

static int on_fault(void)
{
    unsigned long victim_offset = fh_ctx.victim_addr & 0xFFF;
    unsigned long len = PAGE_SIZE - victim_offset;
    unsigned long victim_addr = fh_ctx.victim_addr & ~(0xFFF);
    unsigned long addr = ((unsigned long) fh_ctx.mmap_host) + victim_offset - /* magic field */ 8;

    print_progress("on_fault");
    hex_dump((void *) addr, 64);

    struct faultdata_struct *fault = (struct faultdata_struct *) addr;

    switch (fault->action) {
        case FH_ACTION_ALLOC_GUEST: {

            break;
            default:break;
        }
    }
    fault->action = FH_ACTION_GUEST_CONTINUE;
    return 0;
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

    if ((ret = fh_init(NULL))!=0) {
        print_err("fh_init failed: %d\n", ret);
        return -1;
    };
    const size_t magic = FAULTDATA_MAGIC;

    print_progress("Searching for magic 0x%lx in pid %d", magic, pid);
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

    pthread_t *th = run_thread((int *(*)(void)) on_fault);

    /*
     * We enable faulthook on 8 bytes starting at index 8
     */
    fh_enable_trace((unsigned long) addr + 8, 8, pid);

    pthread_join(*th, NULL);

    ret = 0;
    clean_up:
    fh_disable_trace();
    fh_memory_unmap();
    fh_clean_up();
    return ret;
}
