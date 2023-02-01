#define _GNU_SOURCE

#include <stddef.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "fh_host_header.h"
#include "fpga_manager.h"
#include "fvp_escape_setup.h"

#define STATS_FILE "/tmp/hooks_mmap"

#define SETTING_FILE "./libhook_settings.txt"
#define MAPPING_FILE "./libhook_mapping.txt"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define STR(X) #X
#define WRITE_CTX(f, name) fprintf(f, STR(name) "=0x%lx\n", ctx->name)

#define WRITE_VAR(f, name, var) fprintf(f, STR(name) "=0x%lx\n", var)

#define READ_LINE fgets(line, sizeof(line), f)

#define READ_VAR(name, var)                                                         \
  READ_LINE;                                                                   \
  sscanf(line, STR(name) "=0x%lx", &var)

#define READ_CTX(name)                                                         \
  READ_LINE;                                                                   \
  sscanf(line, STR(name) "=0x%lx", &ctx->name)


#ifndef DOING_UNIT_TESTS

struct ctx_struct
{
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

    unsigned long checksum_vaddrs; /*checksum of vaddr mapping for marshalling */
};

static int read_stat_file(
        pid_t pid,
        unsigned long *ret_from,
        unsigned long *ret_to)
{
    assert(ret_to != NULL);
    assert(ret_from != NULL);

    FILE *f;
    char buffer[128];
    unsigned long from = 0;
    unsigned long to = 0;
    unsigned long size = 0;

    sprintf(buffer, "%s_%d", STATS_FILE, pid);
    printf("opening file %s\n", buffer);
    f = fopen(buffer, "r");

    if (f == NULL)
    {
        printf("open stat file failed: %s. "
               "Is FVP running with libc hook?\n",
               buffer);
        exit(1);
    }
    fscanf(f, "0x%lx", &from);
    fscanf(f, "-0x%lx", &to);

    printf("from: %lx, to %lx\n", from, to);
    printf("pid: %d\n", pid);
    fclose(f);

    if (from != 0 && to != 0 && to > from)
    {
        size = to - from;
    } else
    {
        printf("invalid data in %s\n", buffer);
        exit(1);
    }

    *ret_from = from;
    *ret_to = to;

    return 0;
}

static int open_mem_fd(pid_t pid)
{
    char buffer[256];
    sprintf(buffer, "/proc/%d/mem", pid);
    int fd = open(buffer, O_RDWR);
    if (fd < 0)
    {
        printf("cannot open %s. Do you have root access?\n", buffer);
        return - 1;
    }
    return fd;
}

static int init_addr_map(
        struct ctx_struct *ctx,
        unsigned long min_pfn,
        unsigned long max_pfn)
{
    unsigned long size = max_pfn - min_pfn;
    const unsigned long map_size = sizeof(unsigned long) * size;

    ctx->addr_map = mmap(NULL,
                         map_size,
                         PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                         - 1,
                         0);

    if (ctx->addr_map == MAP_FAILED)
    {
        perror("mmap failed\n");
        return - 1;
    }
    memset(ctx->addr_map, 0, map_size);

    ctx->addr_map_pfn_max = max_pfn;
    ctx->addr_map_pfn_min = min_pfn;
    ctx->addr_map_size = map_size;
    return 0;
}

static int free_addr_map(struct ctx_struct *ctx)
{
    if (ctx->addr_map != NULL)
    {
        munmap(ctx->addr_map, ctx->addr_map_size);
        ctx->addr_map = NULL;
        ctx->addr_map_pfn_min = 0;
        ctx->addr_map_pfn_max = 0;
        ctx->addr_map_size = 0;
    }
}

inline static unsigned long get_addr_map_vaddr(
        struct ctx_struct *ctx,
        unsigned long pfn)
{
    if (pfn >= ctx->addr_map_pfn_min && pfn <= ctx->addr_map_pfn_max)
    {
        return *(ctx->addr_map + (pfn - ctx->addr_map_pfn_min));
    } else
    {
        printf("BUG: get_addr_map_vaddr out of range,"
               " pfn: %ld, max: %ld, min: %ld\n",
               pfn, ctx->addr_map_pfn_max, ctx->addr_map_pfn_min);
    }
    return 0;
}

inline static unsigned long set_addr_map_vaddr(
        struct ctx_struct *ctx,
        unsigned long pfn,
        unsigned long vaddr)
{
    if (pfn >= ctx->addr_map_pfn_min && pfn <= ctx->addr_map_pfn_max)
    {
        *(ctx->addr_map + (pfn - ctx->addr_map_pfn_min)) = vaddr;
    } else
    {
        printf("BUG: set_addr_map_vaddr out of range,"
               " pfn: %ld, max: %ld, min: %ld, vaddr: %ld\n",
               pfn,
               ctx->addr_map_pfn_max,
               ctx->addr_map_pfn_min,
               vaddr);
    }
    return 0;
}

static unsigned long checksum_settings(struct ctx_struct *ctx)
{
    unsigned long checksum;
    checksum += ctx->target_pid;
    checksum += ctx->target_from;
    checksum += ctx->target_to;
    checksum += ctx->escape_page_pfn;
    checksum += ctx->escape_vaddr;
    checksum += ctx->escape_page_reserve_size;
    checksum += ctx->addr_map_pfn_min;
    checksum += ctx->addr_map_pfn_max;
    checksum += ctx->addr_map_size;
    return checksum;
}

static unsigned long checksum_vaddrs(struct ctx_struct *ctx)
{
    unsigned long checksum = 0;
    for (unsigned long i = ctx->addr_map_pfn_min;
         i < ctx->addr_map_pfn_max;
         i ++)
    {
        checksum += get_addr_map_vaddr(ctx, i);
    }
    return checksum;
}

static int save_settings(struct ctx_struct *ctx)
{
    FILE *f;
    unsigned long checksum = checksum_settings(ctx);

    if ((f = fopen(SETTING_FILE, "w")) == NULL)
    {
        printf("Error! opening file: %s\n", SETTING_FILE);
        return - 1;
    }
    WRITE_CTX(f, target_pid);
    WRITE_CTX(f, target_from);
    WRITE_CTX(f, target_to);
    WRITE_CTX(f, escape_page_pfn);
    WRITE_CTX(f, escape_vaddr);
    WRITE_CTX(f, escape_page_reserve_size);
    WRITE_CTX(f, addr_map_pfn_min);
    WRITE_CTX(f, addr_map_pfn_max);
    WRITE_CTX(f, addr_map_size);
    WRITE_VAR(f, checksum, checksum);
    WRITE_VAR(f, checksum_vaddrs, checksum_vaddrs(ctx));

    fclose(f);
    printf("Writing settings to %s\n", SETTING_FILE);

    if ((f = fopen(MAPPING_FILE, "w")) == NULL)
    {
        printf("Error! opening file: %s\n", MAPPING_FILE);
        return - 1;
    }
    for (unsigned long i = ctx->addr_map_pfn_min;
         i < ctx->addr_map_pfn_max;
         i ++)
    {
        fprintf(f, "0x%lx=0x%lx\n", i, get_addr_map_vaddr(ctx, i));
    }
    fclose(f);
    printf("Writing mappings to %s\n", MAPPING_FILE);
    return 0;
}

static int read_mappings(struct ctx_struct *ctx)
{
    FILE *f;
    char line[256];
    unsigned long count = 0, total;

    printf("Reading mappings from %s\n", MAPPING_FILE);
    if ((f = fopen(MAPPING_FILE, "r")) == NULL)
    {
        printf("Error! opening file: %s\n", MAPPING_FILE);
        return - 1;
    }
    total = ctx->addr_map_pfn_max - ctx->addr_map_pfn_min;
    while (fgets(line, sizeof(line), f))
    {
        unsigned long key, val;
        sscanf(line, "0x%lx=0x%lx", &key, &val);
        set_addr_map_vaddr(ctx, key, val);
        count ++;
    }

    if (count != total)
    {
        printf("Something went wrong: count != total, count=%ld, total: %ld\n",
               count,
               total);
        return - 1;
    }

    if (ctx->checksum_vaddrs != checksum_vaddrs(ctx))
    {
        printf("checksum vaddrs failed\n");
        return - 1;
    }
    return 0;
}

static int read_settings(struct ctx_struct *ctx)
{
    FILE *f;
    char line[256];
    unsigned long check_settings = 0;
    unsigned long check_settings_ctrl = 0;

    printf("Reading settings file from %s\n", SETTING_FILE);
    if ((f = fopen(SETTING_FILE, "r")) == NULL)
    {
        printf("Error opening file: %s\n", SETTING_FILE);
        return - 1;
    }

    READ_CTX(target_pid);
    READ_CTX(target_from);
    READ_CTX(target_to);
    READ_CTX(escape_page_pfn);
    READ_CTX(escape_vaddr);
    READ_CTX(escape_page_reserve_size);
    READ_CTX(addr_map_pfn_min);
    READ_CTX(addr_map_pfn_max);
    READ_CTX(addr_map_size);
    READ_VAR(checksum, check_settings);
    READ_VAR(checksum_vaddrs, ctx->checksum_vaddrs);

    WRITE_CTX(stdout, target_pid);
    WRITE_CTX(stdout, target_from);
    WRITE_CTX(stdout, target_to);
    WRITE_CTX(stdout, escape_page_pfn);
    WRITE_CTX(stdout, escape_vaddr);
    WRITE_CTX(stdout, escape_page_reserve_size);
    WRITE_CTX(stdout, addr_map_pfn_min);
    WRITE_CTX(stdout, addr_map_pfn_max);
    WRITE_CTX(stdout, addr_map_size);
    WRITE_VAR(stdout, checksum, check_settings);

    fclose(f);
    check_settings_ctrl = checksum_settings(ctx);

    if (check_settings_ctrl != check_settings)
    {
        printf("checksum verify failed\n");
        printf("checksum: %lx, verify: %lx\n", check_settings, check_settings_ctrl);
        return - 1;
    } else
    {
        printf("checksum is ok\n");
    }
    return 0;
}

static long fvpmem_find_escape_page(struct ctx_struct *ctx,
                                    unsigned long *ret_escape_vaddr,
                                    unsigned long *ret_escape_pfn)
{
    unsigned long from = ctx->target_from;
    unsigned long to = ctx->target_to;
    int mem_fd = ctx->target_mem_fd;
    struct fvp_escape_scan_struct escape;
    unsigned long escape_page_vaddr;
    ssize_t ret = 0;

    escape_page_vaddr = 0;
    while (escape_page_vaddr == 0)
    {
        printf("Searching addresses 0x%lx-0x%lx in pid %d...\n",
               from,
               to,
               ctx->target_pid);
        for (unsigned long addr = from; addr < to; addr += 4096)
        {
            lseek(mem_fd, addr, SEEK_SET);
            ret = read(mem_fd,
                       &escape,
                       sizeof(struct fvp_escape_scan_struct));
            if (ret < 0)
            {
                printf("Error while reading escape memory: %ld, addr: %ld\n",
                       ret,
                       addr);
                continue;
            }

            if (escape.ctrl_magic == FVP_CONTROL_MAGIC
                    && escape.escape_magic == FVP_ESCAPE_MAGIC)
            {
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

static long fvpmem_find_dimensions(struct ctx_struct *ctx)
{
    long ret;
    struct fvp_escape_scan_struct escape;
    const unsigned long from = ctx->target_from;
    const unsigned long to = ctx->target_to;
    const int mem_fd = ctx->target_mem_fd;

    unsigned long min_pfn = INT64_MAX;
    unsigned long max_pfn = 0;

    for (unsigned long addr = from; addr < to; addr += 4096)
    {

        lseek(mem_fd, addr, SEEK_SET);
        ret = read(mem_fd, &escape, sizeof(struct fvp_escape_scan_struct));
        if (ret < 0)
        {
            printf("Error while reading escape memory: %ld, addr: %ld\n", ret, addr);
            continue;
        }
        if (escape.ctrl_magic == FVP_CONTROL_MAGIC)
        {
            min_pfn = MIN(min_pfn, escape.addr_tag);
            max_pfn = MAX(max_pfn, escape.addr_tag);
        }
    }
    ctx->addr_map_pfn_min = min_pfn;
    ctx->addr_map_pfn_max = max_pfn;
    return 0;
}

static long fvpmem_scan_addresses(struct ctx_struct *ctx)
{
    long ret;
    struct fvp_escape_scan_struct escape;
    const unsigned long from = ctx->target_from;
    const unsigned long to = ctx->target_to;
    const int mem_fd = ctx->target_mem_fd;

    for (unsigned long addr = from; addr < to; addr += 4096)
    {
        lseek(mem_fd, addr, SEEK_SET);
        ret = read(mem_fd, &escape, sizeof(struct fvp_escape_scan_struct));
        if (ret < 0)
        {
            printf("Error while reading escape memory: %ld, addr: %ld\n", ret, addr);
            continue;
        }
        if (escape.ctrl_magic == FVP_CONTROL_MAGIC)
        {
            set_addr_map_vaddr(ctx, escape.addr_tag, addr);
        }
    }
    return 0;
}

static void *create_escape_page(void)
{
    return mmap(NULL,
                2 * 4096,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                - 1,
                0);
}

static long map_memory_from_file(struct ctx_struct *ctx, int pid)
{
    long ret;
    ret = read_settings(ctx);
    if (ret != 0)
    {
        printf("restoring context from file failed: %ld\n", ret);
        return ret;
    }
    if (ctx->target_pid != pid)
    {
        printf("Pid changed. Cant load from file. Old=%ld, new=%ld\n",
               ctx->target_pid, pid);
        return - 1;
    }
    ctx->escape_page = create_escape_page();
    if (ctx->escape_page == MAP_FAILED)
    {
        perror("mmap failed\n");
        return - 1;
    }

    // TODO: can we share more code with file appraoch?
    printf("mapping vaddr 0x%lx into host\n", ctx->escape_vaddr);
    ret = fh_mmap_region(
            ctx->target_pid,
            ctx->escape_vaddr,
            4096,
            (char *) ctx->escape_page,
            &ctx->escape_page_mmap_ctx);
    if (ret != 0)
    {
        printf("fh_mmap_region failed: %ld\n", ret);
        return ret;
    }
    ret = init_addr_map(ctx, ctx->addr_map_pfn_min, ctx->addr_map_pfn_max);
    if (ret != 0)
    {
        printf("init_addr_map failed: %ld\n", ret);
        return ret;
    }
    ret = read_mappings(ctx);
    if (ret != 0)
    {
        printf("read_mappings failed: %d\n", ret);
        return ret;
    }
    return 0;
}

static long map_fvp_memory(struct ctx_struct *ctx)
{
    long ret;
    unsigned long escape_page_vaddr;
    unsigned long escape_page_pfn;
    struct fvp_escape_setup_struct *escape;

    ret = fvpmem_find_escape_page(
            ctx,
            &escape_page_vaddr,
            &escape_page_pfn);
    if (ret != 0)
    {
        return ret;
    }
    ctx->escape_page_pfn = escape_page_pfn;
    ctx->escape_vaddr = escape_page_vaddr;
    ctx->escape_page = create_escape_page();
    if (ctx->escape_page == MAP_FAILED)
    {
        perror("mmap failed\n");
        return - 1;
    }

    escape = (struct fvp_escape_setup_struct *) ctx->escape_page;

    printf("mapping vaddr 0x%lx into host\n", ctx->escape_vaddr);
    ret = fh_mmap_region(
            ctx->target_pid,
            ctx->escape_vaddr,
            4096,
            (char *) ctx->escape_page,
            &ctx->escape_page_mmap_ctx);
    if (ret != 0)
    {
        printf("fh_mmap_region failed: %ld\n", ret);
        return ret;
    }

    ctx->escape_page_reserve_size = *((unsigned long *) escape->data);

    printf("guest reserved 0x%lx bytes for escape\n",
           ctx->escape_page_reserve_size);

    /* Make fvp wait until further notice */
    escape->action = fvp_escape_setup_action_wait_guest;

    ret = fvpmem_find_dimensions(ctx);
    if (ret != 0)
    {
        printf("fvpmem_find_dimensions failed: %ld\n", ret);
        return ret;
    }

    printf("init_addr_map with pfn: 0x%lx-0x%lx\n",
           ctx->addr_map_pfn_min,
           ctx->addr_map_pfn_max);

    ret = init_addr_map(
            ctx,
            ctx->addr_map_pfn_min,

            ctx->addr_map_pfn_max);
    if (ret != 0)
    {
        printf("init_addr_map failed: %ld\n", ret);
        return ret;
    }

    printf("fvpmem_scan_addresses\n");
    ret = fvpmem_scan_addresses(ctx);
    if (ret != 0)
    {
        printf("fvpmem_scan_addresses failed: %ld\n", ret);
        return ret;
    }

    printf("escape_page: %ld\n", escape_page_vaddr);
    hex_dump(ctx->escape_page, 64);

    return 0;
}

static void unmap_fvp_memory(struct ctx_struct *ctx)
{
    printf("unmap_fvp_memory\n");
    fh_unmmap_region(&ctx->escape_page_mmap_ctx);
    printf("fh_unmmap_region ok\n");
    free_addr_map(ctx);
    close(ctx->target_mem_fd);
}

static void _unmap_fvp_memory(void *ctx)
{
    unmap_fvp_memory((struct ctx_struct *) ctx);
}

#if 0
int on_fault(unsigned long addr,
             unsigned long len,
             pid_t pid,
             unsigned long target_addr)
{
    struct faultdata_struct *fault = (struct faultdata_struct *) addr;
    if (fault->turn != FH_TURN_HOST)
    {
        print_err("not turn host\n");
        /* fault was caused unintentionally */
        return 0;
    }

    print_progress("length: %ld", fault->length);
    return 0;
}
#endif

static int _on_fault(void *arg)
{
    struct ctx_struct *ctx = (struct ctx_struct *) arg;

    int ret = on_fault((unsigned long) ctx->escape_page,
                       MIN(ctx->escape_page_reserve_size, PAGE_SIZE),
                       ctx->target_pid,
                       ctx->escape_vaddr);
    return ret;
}


static pid_t get_pid(int argc, char *argv[])
{
    pid_t pid;
    if (argc < 2)
    {
        print_err("Need pid as argument\n");
        exit(- 1);
    }
    pid = strtol(argv[1], NULL, 10);
    if (pid == 0)
    {
        print_err("invalid pid");
        exit(- 1);
    }
    return pid;
}

int main(int argc, char *argv[])
{
    ssize_t ret = 0;
    unsigned long addr_from, addr_to;
    pid_t pid;
    bool has_faulthook = 0;

    struct ctx_struct ctx;
    memset(&ctx, 0, sizeof(struct ctx_struct));
    pid = get_pid(argc, argv);
    print_progress("Using pid %d", pid);

    if ((ret = fh_init(NULL)) != 0)
    {
        print_err("fh_init failed: %ld\n", ret);
    } else
    {
        has_faulthook = 1;
    }
    fh_clean_up_hook(_unmap_fvp_memory, &ctx);
    ret = read_stat_file(pid, &addr_from, &addr_to);
    if (ret != 0)
    {
        goto clean_up;
    }
    int mem_fd = open_mem_fd(pid);
    if (mem_fd < 0)
    {
        goto clean_up;
    }
    ctx.target_mem_fd = mem_fd;
    ctx.target_pid = pid;
    ctx.target_from = addr_from;
    ctx.target_to = addr_to;

    ret = map_memory_from_file(&ctx, pid);
    if (ret != 0)
    {
        printf("map memory from file failed. "
               "Restarting without file: %ld\n", ret);
        /*
         * Something went wrong. Either we running a new FVP instance or
         * there was an error while writing state to disk.
         * We restart and scan without loading old state.
         */

        memset(&ctx, 0, sizeof(struct ctx_struct));
        ctx.target_pid = pid;
        ctx.target_from = addr_from;
        ctx.target_to = addr_to;
        ctx.target_mem_fd = mem_fd;

        ret = map_fvp_memory(&ctx);
        if (ret != 0)
        {
            goto clean_up;
        }

        /*
         * We store settings such that we can
         * restart manager without rebooting fvp.
         */
        save_settings(&ctx);

        printf("continuing guest\n");
        ((struct fvp_escape_setup_struct *) ctx.escape_page)
                ->escape_turn = fvp_escape_setup_turn_guest;

        ((struct fvp_escape_setup_struct *) ctx.escape_page)
                ->action = fvp_escape_setup_action_addr_mapping_success;
    }

    if (has_faulthook)
    {
        unsigned long vaddr_target = ctx.escape_vaddr;
        pthread_t *th = run_thread((fh_listener_fn) _on_fault, &ctx);
        ret = fh_enable_trace(
                (unsigned long) vaddr_target + /*offset in struct to fault */ 8,
                8,
                pid);
        if (ret != 0)
        {
            printf("fh_enable_trace failed: %d\n", ret);
            goto clean_up;
        }
        pthread_join(*th, NULL);
    } else
    {
        printf("no faulthook present on system\n");
        while (1)
        {
            hex_dump(ctx.escape_page, 64);
            sleep(5);
        }
    }

    clean_up:
    unmap_fvp_memory(&ctx);
    fh_disable_trace();
    fh_clean_up();
    return ret;
}

#endif
