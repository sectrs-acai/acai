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
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "fpga_manager.h"
#include "../xdma/linux-kernel/xdma/cdev_ctrl.h"

#define HERE printf("%s/%s: %d\n", __FILE__, __FUNCTION__, __LINE__)

#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_RESET "\x1b[0m"

#define TAG_OK COLOR_GREEN "[+]" COLOR_RESET " "
#define TAG_FAIL COLOR_RED "[-]" COLOR_RESET " "
#define TAG_PROGRESS COLOR_YELLOW "[~]" COLOR_RESET " "
#define log_helper(fmt, ...) printf(fmt "%s", __VA_ARGS__)
#define print_progress(...) log_helper(TAG_PROGRESS " "__VA_ARGS__, "")
#define print_ok(...) log_helper(TAG_OK " " __VA_ARGS__, "")
#define print_err(...) log_helper(TAG_FAIL " " __VA_ARGS__, "")


#define set_ret_and_err_no(a, _r) \
    a->common.fd = _r; \
    a->common.ret = _r; \
    a->common.err_no = _r < 0 ? errno:0;


inline static void print_status(int action, struct action_common *c)
{
    print_progress("action: %s, ret: %ld, err_no: %d, fd: %d \n",
                   fh_action_to_str(action),
                   c->ret, c->err_no, c->fd);
}

static inline void hex_dump(
        const void *data,
        size_t size
)
{
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++ i)
    {
        printf("%02X ", ((unsigned char *) data)[i]);
        if (((unsigned char *) data)[i] >= ' ' && ((unsigned char *) data)[i] <= '~')
        {
            ascii[i % 16] = ((unsigned char *) data)[i];
        } else
        {
            ascii[i % 16] = '.';
        }
        if ((i + 1) % 8 == 0 || i + 1 == size)
        {
            printf(" ");
            if ((i + 1) % 16 == 0)
            {
                printf("|  %s \n", ascii);
            } else if (i + 1 == size)
            {
                ascii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8)
                {
                    printf(" ");
                }
                for (j = (i + 1) % 16; j < 16; ++ j)
                {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
}


int on_fault(unsigned long addr,
             unsigned long len,
             pid_t pid,
             unsigned long target_addr,
             ctx_struct ctx)
{
    struct faultdata_struct *fault = (struct faultdata_struct *) addr;
    if (fault->turn != FH_TURN_HOST)
    {
        // print_err("turn is not FH_TURN_HOST\n");
        /* fault was caused unintentionally */
        return 0;
    }
    print_progress("page dump before: \n");
    hex_dump((void *) addr, 64);


    print_progress("action: %s \n", fh_action_to_str(fault->action));
    switch (fault->action)
    {
        case FH_ACTION_SETUP:
        {
            break;
        }
        case FH_ACTION_TEARDOWN:
        {
            break;
        }
        case FH_ACTION_OPEN_DEVICE:
        {
            struct action_openclose_device *a = (struct action_openclose_device *) fault->data;
            int fd = open(a->device, a->flags);
            a->common.fd = fd;
            set_ret_and_err_no(a, fd);

            print_status(fault->action, &a->common);
            print_progress("device: %s\n", a->device);
            print_progress("flags: %d\n", a->flags);
            break;
        }
        case FH_ACTION_CLOSE_DEVICE:
        {
            struct action_openclose_device *a = (struct action_openclose_device *) fault->data;
            int ret = close(a->common.fd);
            set_ret_and_err_no(a, ret);

            print_status(fault->action, &a->common);
            break;
        }
        case FH_ACTION_MMAP:
        {
            struct action_mmap_device *a = (struct action_mmap_device *) fault->data;
            int ret = 0;
            /*
             * XXX: Unlike other calls we have to forward this
             * to a custom ioctl as we dont mmap into the calling process
             */
            print_progress("xdma_ioc_faulthook_mmap with pid: %d", pid);

            for(int i = 0; i< a->pfn_size;  i++) {
                a->pfn[i] = get_addr_map_vaddr(ctx, a->pfn[i]);
            }

            struct xdma_ioc_faulthook_mmap ioc = {
                    .map_type = 0 /*map */,
                    .pid = pid,
                    .vm_pgoff = a->vm_pgoff,
                    .vm_page_prot = a->vm_page_prot,
                    .vm_flags = a->vm_flags,
                    .addr = a->pfn,
                    .addr_size = a->pfn_size
            };
            ret = ioctl(a->common.fd, XDMA_IOCMMAP, &ioc);
            if (ret < 0)
            {
                perror("error ioctl fd\n");
                goto clean_up;
            }

            clean_up:
            a->common.fd = ret;
            set_ret_and_err_no(a, ret);
            print_status(fault->action, &a->common);
            break;
        }
        default:
        {
            print_err("unknown action code: %ld\n", fault->action);
        }
    }
    fault->turn = FH_TURN_GUEST;
    print_progress("page dump after: \n");
    hex_dump((void *) addr, 64);

    return 0;
}

#if 0

case FH_ACTION_ALLOC_GUEST:
        {
            struct action_init_guest *a = (struct action_init_guest *) fault->data;
            fault->action = FH_ACTION_GUEST_CONTINUE;
            a->host_offset = addr & 0xFFF;
            set_ret_and_err_no(a, 0);
            break;
        }

        case FH_ACTION_MMAP:
        {
            struct action_mmap_device *a = (struct action_mmap_device *) fault->data;
            int ret = 0;
            const long allowed_size = 4096;
            const long size = a->vm_end - a->vm_start;

            if (size > allowed_size)
            {
                print_err("Cannot map more than 0x%lx \n", allowed_size);
                ret = - EINVAL;
                errno = - EINVAL;
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
            if (ret < 0)
            {
                perror("error ioctl fd\n");
                goto clean_up;
            }

            clean_up:
            a->common.fd = ret;
            set_ret_and_err_no(a, ret);
            print_status(fault->action, &a->common);
            break;
        }
        case FH_ACTION_UNMAP:
        {
            int ret = 0;
            struct action_unmap *a = (struct action_unmap *) fault->data;
            struct xdma_ioc_faulthook_mmap ioc = {0};
            ioc.map_type = 1 /*unmap */;
            ioc.addr = (unsigned long) a->mmap_guest_kernel_offset;
            ret = ioctl(a->common.fd, XDMA_IOCMMAP, &ioc);
            if (ret < 0)
            {
                perror("error ioctl for unmap\n");
            }
            a->common.fd = ret;
            print_status(fault->action, &a->common);
            set_ret_and_err_no(a, ret);
            break;
        }
        case FH_ACTION_READ:
        {
            struct action_read *a = (struct action_read *) fault->data;
            ssize_t ret = read(a->common.fd, a->buffer, a->count);
            a->count = ret;
            set_ret_and_err_no(a, ret);
            print_status(fault->action, &a->common);
            break;
        }
        case FH_ACTION_WRITE:
        {
            struct action_write *a = (struct action_write *) fault->data;
            ssize_t ret = write(a->common.fd, a->buffer, a->buffer_size);
            a->count = ret;
            // XXX: We ignore offset for now
            print_status(fault->action, &a->common);
            set_ret_and_err_no(a, ret);
            break;
        }
        case FH_ACTION_PING:
        {
            struct action_ping *a = (struct action_ping *) fault->data;
            a->ping ++;
            break;
        }
        case FH_ACTION_VERIFY_MAPPING:
        {
            struct action_ping *a = (struct action_ping *) fault->data;
            a->ping ++;
            break;
        }
#endif