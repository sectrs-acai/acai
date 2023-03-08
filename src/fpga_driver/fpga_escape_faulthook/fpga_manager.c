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
#include "fpga_manager.h"
#include "fh_host_header.h"
#include <stdint.h>
#include "../xdma/linux-kernel/xdma/cdev_ctrl.h"

static unsigned long _addr_host;
inline static void print_status(int action, struct action_common *c)
{
    print_progress("action: %s, ret: %d, err_no: %d, fd: %d",
                   fh_action_to_str(action),
                   c->ret, c->err_no, c->fd);

    print_progress("perror: \n");
}

inline static void print_action_mmap_device(struct action_mmap_device *a)
{
    printf("action_mmap_device->vm_start: 0x%lx\n", a->vm_start);
    printf("action_mmap_device->vm_end: 0x%lx\n", a->vm_end);
    printf("action_mmap_device->vm_pgoff: 0x%lx\n", a->vm_pgoff);
    printf("action_mmap_device->vm_flags: 0x%lx\n", a->vm_flags);
    printf("action_mmap_device->vm_page_prot: 0x%lx\n", a->vm_page_prot);
    printf("action_mmap_device->mmap_guest_kernel_offset: 0x%lx\n", a->mmap_guest_kernel_offset);
}

#define set_ret_and_err_no(a, _r) \
    a->common.fd = _r; \
    a->common.ret = _r; \
    a->common.err_no = _r < 0 ? errno:0;

#define MAP_MORE 0

int on_fault(unsigned long addr,
             unsigned long len,
             pid_t pid,
             unsigned long target_addr)
{
    struct faultdata_struct *fault = (struct faultdata_struct *) addr;
    if (fault->turn!=FH_TURN_HOST) {
        print_err("not turn host\n");
        /* fault was caused unintentionally */
        return 0;
    }

    print_progress("length: %ld", fault->length);

    #if MAP_MORE
    struct fh_memory_map_ctx mappings[20];
    unsigned long i;
    for (i = 1; i < fault->length / 4096; i += 1) {
        struct fh_memory_map_ctx *req = &mappings[i - 1];
        req->addr = (target_addr& ~0xFFF) + (i * 4096);
        req->len = 4096;
        req->pid = pid;
        req->host_mem = (char *) _addr_host + i * 4096;

        print_progress("mapping addr %d %lx\n", i, req->addr);
        int ret = fh_memory_map(req);
        print_progress("mapping addr ok %lx\n", req->addr);
        if (ret!=0) {
            printf("error: %d, ret: %d\n", ret, i);
            return 1;
        }
    }
    int mapping_count = i - 1;

    hex_dump((char*)_addr_host, fault->length);
    #endif

    print_progress("action: %s", fh_action_to_str(fault->action));

    switch (fault->action) {
        case FH_ACTION_ALLOC_GUEST: {
            struct action_init_guest *a = (struct action_init_guest *) fault->data;
            fault->action = FH_ACTION_GUEST_CONTINUE;
            a->host_offset = addr & 0xFFF;
            set_ret_and_err_no(a, 0);
            break;
        }
        case FH_ACTION_OPEN_DEVICE: {
            struct action_openclose_device *a = (struct action_openclose_device *) fault->data;
            int fd = open(a->device, a->flags);
            printf("opening: fd: %d\n", fd);
            a->common.fd = fd;
            set_ret_and_err_no(a, fd);
            print_status(fault->action, &a->common);
            print_progress("device: %s", a->device);
            print_progress("flags: %d", a->flags);
            break;
        }
        case FH_ACTION_CLOSE_DEVICE: {
            struct action_openclose_device *a = (struct action_openclose_device *) fault->data;
            int ret = close(a->common.fd);
            set_ret_and_err_no(a, ret);
            print_status(fault->action, &a->common);
            break;
        }
        case FH_ACTION_MMAP: {
            struct action_mmap_device *a = (struct action_mmap_device *) fault->data;
            int ret = 0;
            const long allowed_size = 4096;
            const long size = a->vm_end - a->vm_start;

            if (size > allowed_size) {
                print_err("Cannot map more than 0x%lx \n", allowed_size);
                ret = -EINVAL;
                errno = -EINVAL;
                goto clean_up;
            }

            void *p = (void *) addr + a->mmap_guest_kernel_offset;
            print_progress("mprotect at addr 0x%lx and len: %lx", p, allowed_size);
            print_action_mmap_device(a);

            #if 0
            // we cant mprotect here since target is not this process
            ret = mprotect(p,
                           allowed_size,
                           PROT_EXEC | PROT_READ | PROT_WRITE);
            if (ret < 0) {
                perror("mprotect failed");
                goto clean_up;
            }
            #endif

            /*
             * XXX: Unlike other calls we have to forward this
             * to a custom ioctl as we dont mmap into the calling process
             */
            print_progress("xdma_ioc_faulthook_mmap with pid: %d", pid);
            struct xdma_ioc_faulthook_mmap ioc = {
                    .map_type = 0 /*map */,
                    .addr = (unsigned long) p,
                    .size = allowed_size,
                    .pid = pid,
                    .vm_pgoff = a->vm_pgoff,
                    .vm_page_prot = a->vm_page_prot,
                    .vm_flags = a->vm_flags,
            };
            ret = ioctl(a->common.fd, XDMA_IOCMMAP, &ioc);
            if (ret < 0) {
                perror("error ioctl fd\n");
                goto clean_up;
            }

            clean_up:
            a->common.fd = ret;
            set_ret_and_err_no(a, ret);
            print_status(fault->action, &a->common);
            break;
        }
        case FH_ACTION_UNMAP: {
            int ret = 0;
            struct action_unmap *a = (struct action_unmap *) fault->data;
            struct xdma_ioc_faulthook_mmap ioc = {0};
            ioc.map_type = 1 /*unmap */;
            ioc.addr = (unsigned long) a->mmap_guest_kernel_offset;
            ret = ioctl(a->common.fd, XDMA_IOCMMAP, &ioc);
            if (ret < 0) {
                perror("error ioctl for unmap\n");
            }
            a->common.fd = ret;
            print_status(fault->action, &a->common);
            set_ret_and_err_no(a, ret);
            break;
        }
        case FH_ACTION_READ: {
            struct action_read *a = (struct action_read *) fault->data;
            ssize_t ret = read(a->common.fd, a->buffer, a->count);
            a->count = ret;
            set_ret_and_err_no(a, ret);
            print_status(fault->action, &a->common);
            break;
        }
        case FH_ACTION_WRITE: {
            struct action_write *a = (struct action_write *) fault->data;
            ssize_t ret = write(a->common.fd, a->buffer, a->buffer_size);
            a->count = ret;
            // XXX: We ignore offset for now
            print_status(fault->action, &a->common);
            set_ret_and_err_no(a, ret);
            break;
        }
        case FH_ACTION_PING: {
            char *a = (char*) fault->data;
            for(int i = 0; i < PING_LEN; i ++) {
                *(a + i) += 1;
            }
            print_progress("done\n");
            break;
        }
        default: {

        }
    }

    #if MAP_MORE
    for (int j = 0; i < mapping_count; j ++) {
        struct fh_memory_map_ctx *req = &mappings[j];
        int ret = fh_memory_unmap(req);
        if (ret!=0) {
            printf("unmap error: %d, ret: %d\n", ret, j);
            return 1;
        }
    }
    #endif

    fault->turn = FH_TURN_GUEST;
    invalid:

    return 0;
}

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


static char *host = NULL;
static unsigned host_len = 20 * PAGE_SIZE;
static bool init_map = 0;

static int _on_fault(void* arg)
{
    unsigned long victim_offset = fh_ctx.victim_addr & 0xFFF;
    unsigned long len = PAGE_SIZE - victim_offset;
    unsigned long victim_addr = fh_ctx.victim_addr & ~(0xFFF);

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

    if (!init_map) {
        init_map = 1;
        int ret = fh_memory_map(&req);
        if (ret!=0) {
            print_err("fh_memory_map error");
            return ret;
        }
    }

    print_ok("Mapped target 0x%lx@pid:%d into host memory %lx", req.addr, req.pid, host);
    unsigned long addr = ((unsigned long) host) + victim_offset - /* magic field */ 8;

    _addr_host = ((unsigned long) host);
    int ret = on_fault(addr,
                   len,
                   fh_ctx.victim_pid,
                   fh_ctx.victim_addr);


    // fh_memory_unmap(&req);
    // print_ok("Unmapping target 0x%lx@pid:%d", req.addr, req.pid);

    return ret;
}


#ifndef DOING_UNIT_TESTS

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

    pthread_t *th = run_thread((fh_listener_fn) _on_fault, NULL);

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

#endif
