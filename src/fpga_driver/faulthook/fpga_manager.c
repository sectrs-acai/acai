//
// Created by abertschi on 1/12/23.
//

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include "fh_host_header.h"
#include "fpga_manager.h"
#include "fh_host.h"

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

static int on_fault(unsigned long addr, unsigned long len)
{
    print_progress("on_fault");
    // hex_dump((void *) addr, len);

    struct faultdata_struct *fault = (struct faultdata_struct *) addr;

    switch (fault->action) {
        case FH_ACTION_ALLOC_GUEST: {
            print_progress("FH_ACTION_ALLOC_GUEST");
            HERE;
            fault->action = FH_ACTION_GUEST_CONTINUE;
            HERE;
            break;
        }
        case FH_ACTION_OPEN_DEVICE: {
            print_progress("FH_ACTION_OPEN_DEVICE");
            struct action_openclose_device *a = (struct action_openclose_device *) fault->data;
            print_progress("device: %s\n", a->device);
            print_progress("flags: %d\n", a->flags);
            int fd = open(a->device, a->flags);
            print_progress("return: %d\n", fd);
            print_progress("errno: %d\n", errno);
            a->ret = fd;
            a->fd = fd;
            a->err_no = errno;
            HERE;
            break;
        }
        case FH_ACTION_CLOSE_DEVICE: {
            print_progress("FH_ACTION_CLOSE_DEVICE");
            struct action_openclose_device *a = (struct action_openclose_device *) fault->data;
            print_progress("close fd: %d\n", a->fd);
            a->ret = close(a->fd);
            print_progress("close return: %d\n", a->ret);
            a->err_no = errno;
            print_progress("close errno: %d\n", errno);
            HERE;
        }
        default:print_err("unknown action: %d", fault->action);
            break;
    }
    HERE;
    return 0;
}

static char *host = NULL;
static unsigned host_len = 10 * PAGE_SIZE;

static int _on_fault(void)
{
    unsigned long victim_offset = fh_ctx.victim_addr & 0xFFF;
    unsigned long len = PAGE_SIZE - victim_offset;
    unsigned long victim_addr = fh_ctx.victim_addr & ~(0xFFF);
    // unsigned long addr = ((unsigned long) fh_ctx.mmap_host) + victim_offset - /* magic field */ 8;

    print_progress("victim_offset: %lx", victim_offset);
    print_progress("len: %lx", len);
    print_progress("victim_addr: %lx", fh_ctx.victim_addr);
    print_progress("victim_addr aligned: %lx", victim_addr);
    // print_progress("addr: %lx", addr);
    // return on_fault(addr, len);

    if (host==NULL) {
        host = mmap(0,
                    host_len,
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    -1,
                    0);
        if (!host) {
            print_err("ERROR: could not mmap shared buf");
            return -1;
        }
        memset(host, 0, host_len);
    }

    struct fh_memory_map_ctx req;
    req.addr = fh_ctx.victim_addr;
    req.len = 4096;
    req.pid = fh_ctx.victim_pid;
    req.host_mem = host;

    int  ret = fh_memory_map(&req);
    if (ret != 0) {
        print_err("fh_memory_map error");
        return ret;
    }
    unsigned long addr = ((unsigned long) host) + victim_offset - /* magic field */ 8;
    ret = on_fault(addr, len);
    fh_memory_unmap(&req);

    return ret;
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

    pthread_t *th = run_thread((fh_listener_fn) _on_fault);

    /*
     * We enable faulthook on 8 bytes starting at index 8
     */
    fh_enable_trace((unsigned long) addr + 8, 8, pid);

    pthread_join(*th, NULL);

    ret = 0;
    clean_up:
    fh_disable_trace();
    fh_clean_up();
    return ret;
}
