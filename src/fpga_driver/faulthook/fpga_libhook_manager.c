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
#include "fvp_escape_setup.h"

#define STATS_FILE "/tmp/hooks_mmap"


#define HAS_FAULTHOOK 0

#ifndef DOING_UNIT_TESTS

static int read_stat_file(pid_t pid, unsigned long *ret_from, unsigned long *ret_to)
{
    assert(ret_to!=NULL);
    assert(ret_from!=NULL);

    int ret = 0;
    FILE *f;
    char buffer[128];

    unsigned long from = 0;
    unsigned long to = 0;
    unsigned long size = 0;

    sprintf(buffer, "%s_%d", STATS_FILE, pid);
    printf("opening file %s\n", buffer);
    f = fopen(buffer, "r");

    if (f==NULL) {
        printf("open stat file failed: %s. Is FVP running with libc hook?\n", buffer);
        exit(1);
    }
    fscanf(f, "0x%lx", &from);
    fscanf(f, "-0x%lx", &to);

    printf("from: %lx, to %lx\n", from, to);
    printf("pid: %d\n", pid);
    fclose(f);


    if (from!=0 && to!=0 && to > from) {
        size = to - from;
    } else {
        printf("invalid data in %s\n", buffer);
        exit(1);
    }

    *ret_from = from;
    *ret_to = to;

    return 0;
}

struct ctx_struct {
    int target_mem_fd;
    pid_t target_pid;
    unsigned long target_from;
    unsigned long target_to;

    char *escape_page;
    struct fh_mmap_region_ctx escape_page_mmap_ctx;
    unsigned long escape_page_pfn;
    unsigned long escape_vaddr;
    unsigned long escape_page_reserve_size;

    unsigned long *addr_map;
    unsigned long addr_map_pfn_min;
    unsigned long addr_map_pfn_max;
    unsigned long addr_map_size;

};

static int open_mem_fd(pid_t pid)
{
    char buffer[256];
    sprintf(buffer, "/proc/%d/mem", pid);
    int fd = open(buffer, O_RDWR);
    if (fd < 0) {
        printf("cannot open %s. Do you have root access?\n", buffer);
        return -1;
    }
    return fd;
}

#define MIN(a, b) (((a)<(b))?(a):(b))
#define MAX(a, b) (((a)>(b))?(a):(b))

static int init_addr_map(struct ctx_struct *ctx, unsigned long min_pfn, unsigned long max_pfn)
{
    unsigned long size = max_pfn - min_pfn;
    const unsigned long map_size = sizeof(unsigned long) * size;

    ctx->addr_map = mmap(NULL,
                         map_size,
                         PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                         -1,
                         0);
    if (ctx->addr_map==MAP_FAILED) {
        perror("mmap failed\n");
        return -1;
    }
    memset(ctx->addr_map, 0, map_size);

    ctx->addr_map_pfn_max = max_pfn;
    ctx->addr_map_pfn_min = min_pfn;
    ctx->addr_map_size = map_size;
    return 0;
}

static int free_addr_map(struct ctx_struct *ctx)
{
    if (ctx->addr_map!=NULL) {
        free(ctx->addr_map);
        ctx->addr_map = NULL;
        ctx->addr_map_pfn_min = 0;
        ctx->addr_map_pfn_max = 0;
        ctx->addr_map_size = 0;
    }
}

inline static unsigned long get_addr_map_vaddr(struct ctx_struct *ctx, unsigned long pfn)
{
    if (pfn >= ctx->addr_map_pfn_min && pfn <= ctx->addr_map_pfn_max) {
        return *(ctx->addr_map + (pfn - ctx->addr_map_pfn_min));
    } else {
        printf("BUG: get_addr_map_vaddr out of range,"
               " pfn: %ld, max: %ld, min: %ld\n",
               pfn, ctx->addr_map_pfn_max, ctx->addr_map_pfn_min);
    }
    return 0;
}

inline static unsigned long set_addr_map_vaddr(struct ctx_struct *ctx,
                                               unsigned long pfn,
                                               unsigned long vaddr)
{
    if (pfn >= ctx->addr_map_pfn_min && pfn <= ctx->addr_map_pfn_max) {
        *(ctx->addr_map + (pfn - ctx->addr_map_pfn_min)) = vaddr;
    } else {
        printf("BUG: set_addr_map_vaddr out of range,"
               " pfn: %ld, max: %ld, min: %ld, vaddr: %ld\n",
               pfn, ctx->addr_map_pfn_max, ctx->addr_map_pfn_min, vaddr);
    }
    return 0;
}


static long map_fvp_memory_find_magic_page(struct ctx_struct *ctx,
                                           unsigned long *ret_escape_vaddr,
                                           unsigned long *ret_escape_pfn)
{
    struct fvp_escape_scan_struct escape;
    ssize_t ret = 0;
    unsigned long escape_page_vaddr;
    unsigned long from = ctx->target_from;
    unsigned long to = ctx->target_to;
    int mem_fd = ctx->target_mem_fd;

    escape_page_vaddr = 0;
    while (escape_page_vaddr==0) {
        printf("Searching addresses 0x%lx-0x%lx in pid %d...\n", from, to, ctx->target_pid);
        for (unsigned long addr = from; addr < to; addr += 4096) {

            lseek(mem_fd, addr, SEEK_SET);
            ret = read(mem_fd, &escape, sizeof(struct fvp_escape_scan_struct));
            if (ret < 0) {
                printf("Error while reading escape memory: %ld, addr: %ld\n", ret, addr);
                continue;
            }

            if (escape.ctrl_magic==FVP_CONTROL_MAGIC
                    && escape.escape_magic==FVP_ESCAPE_MAGIC) {
                printf("found escape page: %p=%ld\n", addr, escape.addr_tag);

                escape_page_vaddr = addr;
                *ret_escape_pfn = escape.addr_tag;
                break;
            }
        }
    }
    *ret_escape_vaddr = escape_page_vaddr;
    return 0;
}

static long map_fvp_memory_scan_memory_dims(struct ctx_struct *ctx)
{
    long ret;
    struct fvp_escape_scan_struct escape;
    unsigned long from = ctx->target_from;
    unsigned long to = ctx->target_to;
    int mem_fd = ctx->target_mem_fd;

    unsigned long min_pfn = INT64_MAX;
    unsigned long max_pfn = 0;

    for (unsigned long addr = from; addr < to; addr += 4096) {

        lseek(mem_fd, addr, SEEK_SET);
        ret = read(mem_fd, &escape, sizeof(struct fvp_escape_scan_struct));
        if (ret < 0) {
            printf("Error while reading escape memory: %ld, addr: %ld\n", ret, addr);
            continue;
        }
        if (escape.ctrl_magic==FVP_CONTROL_MAGIC) {
            min_pfn = MIN(min_pfn, escape.addr_tag);
            max_pfn = MAX(max_pfn, escape.addr_tag);
        }
    }

    ctx->addr_map_pfn_min = min_pfn;
    ctx->addr_map_pfn_max = max_pfn;
    return 0;
}

static long map_fvp_memory_tag_addrs(struct ctx_struct *ctx)
{
    long ret;
    struct fvp_escape_scan_struct escape;
    unsigned long from = ctx->target_from;
    unsigned long to = ctx->target_to;
    int mem_fd = ctx->target_mem_fd;

    for (unsigned long addr = from; addr < to; addr += 4096) {
        lseek(mem_fd, addr, SEEK_SET);
        ret = read(mem_fd, &escape, sizeof(struct fvp_escape_scan_struct));
        if (ret < 0) {
            printf("Error while reading escape memory: %ld, addr: %ld\n", ret, addr);
            continue;
        }
        if (escape.ctrl_magic==FVP_CONTROL_MAGIC) {
            set_addr_map_vaddr(ctx, escape.addr_tag, addr);
        }
    }
    return 0;
}

static long map_fvp_memory(struct ctx_struct *ctx)
{
    long ret;
    unsigned long escape_page_vaddr;
    unsigned long escape_page_pfn;
    struct fvp_escape_setup_struct *escape;

    ret = map_fvp_memory_find_magic_page(ctx,
                                         &escape_page_vaddr,
                                         &escape_page_pfn);
    if (ret!=0) {
        return ret;
    }
    ctx->escape_page_pfn = escape_page_pfn;
    ctx->escape_vaddr = escape_page_vaddr;

    ctx->escape_page = mmap(NULL,
                            2 * 4096,
                            PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                            -1,
                            0);

    if (ctx->escape_page==MAP_FAILED) {
        perror("mmap failed\n");
        return -1;
    }
    escape = (struct fvp_escape_setup_struct *) ctx->escape_page;

    ret = fh_mmap_region(
            ctx->target_pid,
            escape_page_vaddr,
            4096,
            (char *) escape,
            &ctx->escape_page_mmap_ctx);
    if (ret!=0) {
        printf("fh_mmap_region failed: %ld\n", ret);
        return ret;
    }


    ctx->escape_page_reserve_size = *((unsigned long *) escape->data);
    printf("guest reserved 0x%lx bytes for escape\n", ctx->escape_page_reserve_size);

    /* Make fvp wait until further notice */
    escape->action = fvp_escape_setup_action_wait_guest;


    ret = map_fvp_memory_scan_memory_dims(ctx);
    if (ret!=0) {
        printf("map_fvp_memory_scan_memory_dims failed: %ld\n", ret);
        return ret;
    }

    printf("init_addr_map with pfn: 0x%lx-0x%lx\n", ctx->addr_map_pfn_min, ctx->addr_map_pfn_max);
    ret = init_addr_map(ctx, ctx->addr_map_pfn_min, ctx->addr_map_pfn_max);
    if (ret!=0) {
        printf("init_addr_map failed: %ld\n", ret);
        return ret;
    }

    printf("map_fvp_memory_tag_addrs\n");
    ret = map_fvp_memory_tag_addrs(ctx);
    if (ret!=0) {
        printf("map_fvp_memory_tag_addrs failed: %ld\n", ret);
        return ret;
    }

    printf("escape_page: %ld\n", escape_page_vaddr);
    hex_dump(ctx->escape_page, 64);

    return 0;
}

static long unmap_fvp_memory(struct ctx_struct *ctx)
{
    fh_unmmap_region(&ctx->escape_page_mmap_ctx);
    free_addr_map(ctx);
    close(ctx->target_mem_fd);
}


static int _on_fault(void *arg)
{
    struct ctx_struct *ctx = (struct ctx_struct *) arg;
    unsigned long victim_offset = fh_ctx.victim_addr & 0xFFF;
    unsigned long len = PAGE_SIZE - victim_offset;
    unsigned long victim_addr = fh_ctx.victim_addr & ~(0xFFF);


    int ret = on_fault((unsigned long) ctx->escape_page,
                       MIN(ctx->escape_page_reserve_size, PAGE_SIZE),
                       ctx->target_pid,
                       ctx->escape_vaddr);
    return ret;
}


int main(int argc, char *argv[])
{
    ssize_t ret = 0;
    unsigned long addr_from, addr_to;

    struct ctx_struct ctx;
    memset(&ctx, 0, sizeof(struct ctx_struct));

    if (argc < 2) {
        print_err("Need pid as argument\n");
        return -1;
    }
    pid_t pid = strtol(argv[1], NULL, 10);
    print_progress("pid: %d", pid);

    if ((ret = fh_init(NULL))!=0) {
        print_err("fh_init failed: %d\n", ret);
    };

    ret = read_stat_file(pid, &addr_from, &addr_to);
    if (ret!=0) {
        goto clean_up;
    }
    ctx.target_pid = pid;
    ctx.target_from = addr_from;
    ctx.target_to = addr_to;

    ret = open_mem_fd(ctx.target_pid);
    if (ret < 0) {
        goto clean_up;
    }
    ctx.target_mem_fd = ret;

    ret = map_fvp_memory(&ctx);
    if (ret!=0) {
        goto clean_up;
    }

    printf("continuing guest\n");
    ((struct fvp_escape_setup_struct *) ctx.escape_page)->escape_turn = fvp_escape_setup_turn_guest;
    ((struct fvp_escape_setup_struct *) ctx.escape_page)->action
            = fvp_escape_setup_action_addr_mapping_success;

    if (HAS_FAULTHOOK || 1) {
        pthread_t *th = run_thread((fh_listener_fn) _on_fault, &ctx);
        ret = fh_enable_trace((unsigned long) /*offset in struct to fault */ 8, 8, pid);
        if (ret!=0) {
            printf("fh_enable_trace failed: %d\n", ret);
            goto clean_up;
        }
        pthread_join(*th, NULL);
    }

#if 0
    while (1) {
        hex_dump(ctx.escape_page, 64);
        sleep(5);
    }
#endif

    clean_up:
    unmap_fvp_memory(&ctx);
    fh_disable_trace();
    fh_clean_up();
    return ret;
}

#endif
