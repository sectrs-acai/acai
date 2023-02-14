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

#define set_ret_and_err_no_direct(a, _r) \
    a->common.fd = _r; \
    a->common.ret = _r; \
    a->common.err_no = _r


inline static void print_status(int action, struct action_common *c)
{
    print_progress("ret: %ld, err_no: %d, fd: %d \n",
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

static int do_dma(struct faultdata_struct *fault,
                  pid_t pid,
                  ctx_struct ctx)
{
    int ret = 0;
    struct action_dma *a = (struct action_dma *) fault->data;
    struct page_chunk *chunks = NULL;
    struct page_chunk *chunk = NULL;
    unsigned long chunk_size;
    unsigned long pfn;

    chunk_size = a->pages_nr * sizeof(struct page_chunk);
    printf("chunk_size=%ld\n", chunk_size);
    chunks = malloc(chunk_size);
    if (chunks == NULL)
    {
        print_err("addrs is null\n");
        return - 1;
    }
    memcpy(chunks, a->page_chunks, chunk_size);
    print_progress("transfering %d pages\n", a->pages_nr);
    for (int i = 0; i < a->pages_nr; i ++)
    {
        chunk = chunks + i;
        pfn = chunk->addr;
        chunk->addr = get_addr_map_vaddr(ctx, chunk->addr);
        if (chunk->addr == 0) {
            print_err("no mapping for %lx\n", pfn);
            ret = -1;
            goto dma_clean;
        }

        #if 0
        print_progress("dma %lx->%lx, %lx %lx\n",
                       pfn,
                       chunk->addr,
                       chunk->nbytes,
                       chunk->offset);
        #endif
    }

    struct fh_host_ioctl_dma dma = {
            .pid = pid,
            .phy_addr = a->phy_addr,
            .do_write = a->do_write,
            .user_buf = a->user_buf,
            .do_aperture = a->do_aperture,
            .chunks_nr = a->pages_nr,
            .len = a->len,
            .chunks = chunks
    };
    HERE;
    ret = ioctl(a->common.fd, FH_HOST_IOCTL_DMA, &dma);
    if (ret < 0)
    {
        perror("FH_ACTION_DMA failed\n");
        print_progress("FH_HOST_IOCTL_DMA res: %d\n", ret);
        goto dma_clean;
    }
    HERE;

    dma_clean:
set_ret_and_err_no(a, ret);
    free(chunks);

    return ret;
}
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))


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
        case FH_ACTION_PING:
        {
            struct action_ping *a = (struct action_ping *) fault->data;
            a->ping ++;
            break;
        }
        case FH_ACTION_GET_EMPTY_MAPPINGS:
        {
            unsigned long pfn_min, pfn_max, pfn, i;
            struct action_empty_mappings *a = (struct action_empty_mappings *) fault->data;
            unsigned long max_mappings = 4096 -
                    sizeof(struct faultdata_struct) - sizeof(struct action_empty_mappings);
            max_mappings /= 8;
            max_mappings = MIN(max_mappings, a->pfn_max_nr_guest);

            get_addr_map_dims(ctx, &pfn_min, &pfn_max);
            if (a->last_pfn == 0)
            {
                pfn = pfn_min;
            } else
            {
                pfn = a->last_pfn;
            }
            for (i = 0; pfn < pfn_max && i < max_mappings; pfn ++)
            {
                if (get_addr_map_vaddr(ctx, pfn) == 0)
                {
                    a->pfn[i ++] = pfn;
                }
            }
            if (i == max_mappings && pfn < pfn_max)
            {
                a->last_pfn = pfn;
            } else
            {
                a->last_pfn = 0;
            }
            a->pfn_nr = i;
            print_progress("i = %ld, pfn=%lx, max_mappings=%ld, a->last_pf: %lx\n",
                           i,
                           pfn,
                           max_mappings,
                           a->last_pfn);
            set_ret_and_err_no_direct(a, 0);
            break;
        }
        case FH_ACTION_VERIFY_MAPPING:
        {
            struct action_verify_mappping *a = (struct action_verify_mappping *) fault->data;
            unsigned long vaddr = get_addr_map_vaddr(ctx, a->pfn);
            if (vaddr != 0)
            {
                a->status = action_verify_mappping_success;
            } else
            {
                print_err("no mapping found for pfn: %lx\n", a->pfn);
                a->status = action_verify_mappping_fail;
            }
            break;
        }
        case FH_ACTION_OPEN_DEVICE:
        {
            struct action_openclose_device *a = (struct action_openclose_device *) fault->data;
            int fd = open(a->device, a->flags);
            a->common.fd = fd;
            set_ret_and_err_no(a, fd);


            #if 0
            print_status(fault->action, &a->common);
            print_progress("device: %s\n", a->device);
            print_progress("flags: %d\n", a->flags);
            #endif
            break;
        }
        case FH_ACTION_CLOSE_DEVICE:
        {
            struct action_openclose_device *a = (struct action_openclose_device *) fault->data;
            int ret = close(a->common.fd);
            set_ret_and_err_no(a, ret);

            #if 0
            print_status(fault->action, &a->common);
            #endif
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
            // print_progress("xdma_ioc_faulthook_mmap with pid: %d \n", pid);
            unsigned long *addrs = malloc(a->pfn_size * sizeof(unsigned long));
            if (addrs == NULL)
            {
                print_err("addrs is null\n");
                return - 1;
            }
            for (int i = 0; i < a->pfn_size; i ++)
            {
                addrs[i] = get_addr_map_vaddr(ctx, a->pfn[i]);
            }
            struct xdma_ioc_faulthook_mmap ioc = {
                    .pid = pid,
                    .vm_pgoff = a->vm_pgoff,
                    .vm_page_prot = a->vm_page_prot,
                    .vm_flags = a->vm_flags,
                    .addr = addrs,
                    .addr_size = a->pfn_size
            };
            ret = ioctl(a->common.fd, XDMA_IOCMMAP, &ioc);
            if (ret < 0)
            {
                perror("XDMA_IOCMMAP failed\n");
                goto map_clean;
            }
            map_clean:
            free(addrs);
            set_ret_and_err_no(a, ret);
            #if 0
            print_status(fault->action, &a->common);
            #endif
            break;
        }
        case FH_ACTION_UNMAP:
        {
            int ret = 0;
            struct action_unmap *a = (struct action_unmap *) fault->data;
            unsigned long *addrs = malloc(a->pfn_size * sizeof(unsigned long));
            if (addrs == NULL)
            {
                print_err("addrs is null\n");
                return - 1;
            }
            for (int i = 0; i < a->pfn_size; i ++)
            {
                addrs[i] = get_addr_map_vaddr(ctx, a->pfn[i]);
            }
            struct xdma_ioc_faulthook_unmmap ioc = {
                    .pid = pid,
                    .addr = addrs,
                    .addr_size = a->pfn_size
            };
            ret = ioctl(a->common.fd, XDMA_IOCUNMMAP, &ioc);
            if (ret < 0)
            {
                perror("XDMA_IOCUNMMAP failed\n");
                goto unmap_clean;
            }
            unmap_clean:
            free(addrs);
            set_ret_and_err_no(a, ret);
            print_status(fault->action, &a->common);
        }
        case FH_ACTION_DMA:
        {
            do_dma(fault, pid, ctx);
            break;
        }
        default:
        {
            print_err("unknown action code: %ld\n", fault->action);
        }
    }
    fault->turn = FH_TURN_GUEST;
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
