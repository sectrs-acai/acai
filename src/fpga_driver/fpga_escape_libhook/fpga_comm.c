//
// Created by abertschi on 1/12/23.
//
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include <errno.h>
#include <stddef.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "usr_manager.h"
#include "fpga_usr_manager.h"
#include "../xdma/linux-kernel/xdma/cdev_ctrl.h"
#include "bench/fpga_bench.h"

#define HERE printf("%s/%s: %d\n", __FILE__, __FUNCTION__, __LINE__)
#define MIN(a, b) (((a)<(b))?(a):(b))
#define MAX(a, b) (((a)>(b))?(a):(b))

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

#define BENCH_START() bench_time_t _bench_start = bench__start()
#define BENCH_STOP(type) bench__stop(_bench_start, type)

#define set_ret_and_err_no(a, _r) \
    a->common.fd = _r; \
    a->common.ret = _r; \
    a->common.err_no = _r < 0 ? errno:0;

#define set_ret_and_err_no_direct(a, _r) \
    a->common.fd = _r; \
    a->common.ret = _r; \
    a->common.err_no = _r

static char *action_transfer_data_ptr = NULL;
static unsigned long action_transfer_data_size = 0;
static unsigned long action_transfer_data_info_ctx = 0;
static unsigned long action_transfer_data_chunk = 0;

#define TRACE_FILE_DIR "/tmp/armcca_fpga/"
inline static FILE *__get_bench_file()
{
    char filename[64];
    struct tm *timenow;
    FILE *f;
    time_t now = time(NULL);
    struct stat st = {0};
    timenow = localtime(&now);

    strftime(filename, sizeof(filename), TRACE_FILE_DIR "fpga_trace_%Y-%m-%d_%H-%M-%S.txt",
             timenow);
    print_ok("opening trace file %s\n", filename);

    if (stat(TRACE_FILE_DIR, &st) == -1) {
        print_ok("creating dir %s\n", TRACE_FILE_DIR);
        mkdir(TRACE_FILE_DIR, 0777);
    }

    f = fopen(filename, "w");
    return f;
}

int do_action_transfer_escape_data_free(ctx_struct ctx) {
    free(action_transfer_data_ptr);
    action_transfer_data_ptr = NULL;
    action_transfer_data_chunk = 0;
    action_transfer_data_info_ctx = 0;
    action_transfer_data_size = 0;
    return 0;
}

int do_action_transfer_escape_data(struct faultdata_struct *fault,
                                    pid_t pid,
                                    ctx_struct ctx)
{
    struct action_transfer_escape_data *a = (struct action_transfer_escape_data *) fault->data;
    const unsigned long transfer_max =
            get_mapped_escape_buffer_size(ctx) - sizeof (struct action_transfer_escape_data)
                    - sizeof(struct faultdata_struct);

    print_ok("max transfer_size %ld\n", transfer_max);

    if (a->status == action_transfer_escape_data_status_transfer_size) {
        if (action_transfer_data_ptr != NULL) {
            print_err("do_action_transfer_escape_data without a free. memory leak\n");
        }
        a->handshake.chunk_size = transfer_max;
        action_transfer_data_size = a->handshake.total_size;
        action_transfer_data_chunk = 0;
        action_transfer_data_info_ctx = a->handshake.info_ctx;
        action_transfer_data_ptr = malloc(action_transfer_data_size);

        if (action_transfer_data_ptr == NULL) {
            print_err("oom issue\n");
            a->status = action_transfer_escape_data_status_error;
            return -1;
        }
        a->status = action_transfer_escape_data_status_ok;

    } else if (a->status == action_transfer_escape_data_status_transfer_data) {
        unsigned long transfer = MIN(a->chunk.chunk_data_size, transfer_max);
        print_progress("transfer: %ld\n", transfer);

        if (action_transfer_data_chunk + transfer > action_transfer_data_size) {
            print_err("Program error. out of bounds memory transfer!\n");
            return -1;
        }

        memcpy(action_transfer_data_ptr + action_transfer_data_chunk,
               a->chunk.chunk_data,
               transfer);

        action_transfer_data_chunk += transfer;
        a->status = action_transfer_escape_data_status_ok;
    } else {
        print_err("unknown status: %d\n", a->status);
    }
    return 0;
}

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



/*
 * precondition: dma, requires chunk transfer with
 * action_transfer_data
 */
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

    if (action_transfer_data_info_ctx != FH_ACTION_DMA)
    {
        print_err("No action_transfer_data done beforehand\n");
        set_ret_and_err_no(a, - 1);
        return - 1;
    }

    #if 0
    chunk_size = a->pages_nr * sizeof(struct page_chunk);
    print_progress("chunk_size=%ld\n", chunk_size);
    chunks = action_transfer_data_ptr;
    if (chunks == NULL)
    {
        print_err("addrs is null\n");
        set_ret_and_err_no(a, -1);
        return - 1;
    }
    memcpy(chunks, a->page_chunks, chunk_size);
    #endif

    chunks = (struct page_chunk *) action_transfer_data_ptr;
    chunk_size = action_transfer_data_size;

    #if 0
    for (int i = 0; i < a->pages_nr; i ++) {
        chunk = chunks + i;
        print_ok("%lx, %lx, %lx\n", chunk->addr, chunk->nbytes, chunk->offset);
    }
    #endif

    print_progress("transfering %d pages\n", a->pages_nr);
    for (int i = 0; i < a->pages_nr; i ++)
    {
        chunk = chunks + i;
        pfn = chunk->addr;
        chunk->addr = get_addr_map_vaddr_verify(ctx, chunk->addr, 1);
        if (chunk->addr == 0)
        {
            print_err("no mapping for %lx\n", pfn);
            ret = - 1;
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

    BENCH_START();
    ret = ioctl(a->common.fd, FH_HOST_IOCTL_DMA, &dma);
    BENCH_STOP(BENCH_FPGA_DMA);

    if (ret < 0)
    {
        perror("FH_ACTION_DMA failed\n");
        print_progress("FH_HOST_IOCTL_DMA res: %d\n", ret);
        goto dma_clean;
    }
    dma_clean:
set_ret_and_err_no(a, ret);
    do_action_transfer_escape_data_free(ctx);

    return ret;
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
        /*
         * fault was caused unintentionally
         */
        #if 0
        print_err("turn is not FH_TURN_HOST\n");
        #endif
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
            unsigned long max_mappings = get_mapped_escape_buffer_size(ctx) -
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
                if (get_addr_map_vaddr_verify(ctx, pfn, 0) == 0)
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
            set_ret_and_err_no_direct(a, 0);
            break;
        }
        case FH_ACTION_VERIFY_MAPPING:
        {
            struct action_verify_mappping *a = (struct action_verify_mappping *) fault->data;
            unsigned long vaddr = get_addr_map_vaddr_verify(ctx, a->pfn, 0);
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
            bench__init();

            BENCH_START();
            int fd = open(a->device, a->flags);
            BENCH_STOP(BENCH_FPGA_OPEN);

            a->common.fd = fd;
            set_ret_and_err_no(a, fd);
            break;
        }
        case FH_ACTION_CLOSE_DEVICE:
        {
            FILE *trace_file = NULL;
            struct action_openclose_device *a = (struct action_openclose_device *) fault->data;

            BENCH_START();
            int ret = close(a->common.fd);
            BENCH_STOP(BENCH_FPGA_CLOSE);

            /*
             * writing benchmark results
             */
            trace_file = __get_bench_file();
            if (trace_file == NULL) {
                print_err("trace file is NULL!\n");
            }
            bench__results(trace_file);
            if (trace_file != NULL) {
                fclose(trace_file);
            }

            set_ret_and_err_no(a, ret);
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

            BENCH_START();
            ret = ioctl(a->common.fd, XDMA_IOCMMAP, &ioc);
            BENCH_STOP(BENCH_FPGA_MMAP);

            if (ret < 0)
            {
                perror("XDMA_IOCMMAP failed\n");
                goto map_clean;
            }
            map_clean:
            free(addrs);
            set_ret_and_err_no(a, ret);
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

            BENCH_START();
            ret = ioctl(a->common.fd, XDMA_IOCUNMMAP, &ioc);
            BENCH_STOP(BENCH_FPGA_UNMMAP);
            
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
            int ret = do_dma(fault, pid, ctx);
            if (ret <  0) {
                return ret;
            }
            break;
        }
        case FH_ACTION_TRANSFER_ESCAPE_DATA:
        {
            int ret = do_action_transfer_escape_data(fault, pid, ctx);
            if (ret <  0) {
                return ret;
            }
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

