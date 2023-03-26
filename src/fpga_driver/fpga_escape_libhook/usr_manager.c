#define _GNU_SOURCE
/*
 * libc allocator hook based fpga user space manager
 * the fvp needs libhook preloaded
 *
 * Assumptions:
 * - We map ESCAPE_PAGE_SIZE many bytes of target address
 *   space into our own address space.
 *   This may be less than the reserved size.
 *   If the current value is not enough then increase as needed
 * - The same machine runs the faulthook enabled kernel
 * - The same machine runs the target device driver
 * - This approach needs libc allocator hooks which preallocates a fixed mmaped region.
 *   if you run the FVP with much DRAM you may increase libc hook preallocated area.
 * - Run this program with sudo rights
 * - /tmp/ and current directory must be read/writable
 * - We currently assume that the FVP memory is contiguous.
 *   - We remove some 24 bytes from fvp allocations to make the pages contiguous
 *   - This way contiguous FVP DRAM is contiguous on the host
 *   - This is relevant when accessing the escape buffer at page boundaries
 *   - If this no longer holds then escape page access turn a bit more complicated (tbd).
 */
#include <stddef.h>

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fh_host_header.h"
#include "fvp_escape_setup.h"
#include "usr_manager.h"

#define STATS_FILE "/tmp/hooks_mmap"
#define SETTING_FILE "/tmp/.temp.libhook_settings.txt"
#define MAPPING_FILE "/tmp/.temp.libhook_mapping.txt"

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


// XXX: Increase this if needed
// hard coded value in boot process of fvp
#define ESCAPE_PAGE_SIZE (1 * PAGE_SIZE)

#ifndef DOING_UNIT_TESTS

struct ctx_struct
{
    int target_mem_fd;           /* /proc/%d/mem fd */
    pid_t target_pid;            /* pid of target to intercpet */
    unsigned long target_from;   /* vaddr start of interesting target mem */
    unsigned long target_to;     /* vaddr end of interesting target mem */

    char *escape_page;               /* pointer to target's address space where escape buffer lies */
    struct fh_mmap_region_ctx escape_page_mmap_ctx;
    unsigned long escape_page_pfn;   /* pfn within fvp of escape page  */
    unsigned long escape_vaddr;      /* vaddr @target process of escape page */
    unsigned long escape_page_reserve_size;  /* escape page size */

    /* lookup array pfn -> vaddr */
    unsigned long *addr_map;
    unsigned long addr_map_pfn_min;
    unsigned long addr_map_pfn_max;
    unsigned long addr_map_size;

    unsigned long checksum_vaddrs; /*checksum of vaddr mapping for marshalling */
};

unsigned long get_mapped_escape_buffer_size(ctx_struct ctx)
{
    return ESCAPE_PAGE_SIZE;
}

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
    print_ok("opening file %s\n", buffer);
    f = fopen(buffer, "r");

    if (f == NULL)
    {
        print_err("open stat file failed: %s. "
                  "Is FVP running with libc hook?\n",
                  buffer);
        exit(1);
    }
    fscanf(f, "0x%lx", &from);
    fscanf(f, "-0x%lx", &to);

    print_ok("from: %lx, to %lx\n", from, to);
    print_ok("pid: %d\n", pid);
    fclose(f);

    if (from != 0 && to != 0 && to > from)
    {
        size = to - from;
    } else
    {
        print_err("invalid data in %s\n", buffer);
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
        print_err("cannot open %s. Do you have root access?\n", buffer);
        return - 1;
    }
    return fd;
}

/*
 * This captures an address map pfn -> vaddr
 * XXX: Note that the dts may define holes in the physical dram address space.
 *      The current data structure (array) will contain many empty entries
 *      if said holes are large. This is not problematic as long as we have enough memory.
 *      If device tree contains holes, then an empty mapping may be caused either by
 *      a reserved area in existing dram or by a hole in the
 *      memory range definition in the device tree. The assumption no mapping -> reserved area
 *      may no longer hold in this case.
 */
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
    return 0;
}

unsigned long get_addr_map_vaddr_verify(
        ctx_struct ctx,
        unsigned long pfn,
        bool complain)
{
    unsigned long ret = 0;
    if (pfn >= ctx->addr_map_pfn_min && pfn <= ctx->addr_map_pfn_max)
    {
        ret = *(ctx->addr_map + (pfn - ctx->addr_map_pfn_min));
        if (ret == 0 && complain)
        {
            print_err("BUG: get_addr_map_vaddr no mapping for %lx\n", pfn);
        }
    } else
    {
        print_err("BUG: get_addr_map_vaddr out of range,"
                  " pfn: %lx, max: %lx, min: %lx\n",
                  pfn, ctx->addr_map_pfn_max, ctx->addr_map_pfn_min);
    }
    return ret;
}

unsigned long get_addr_map_vaddr(
        ctx_struct ctx, unsigned long pfn)
{
    return get_addr_map_vaddr_verify(ctx, pfn, 1);
}

unsigned long get_addr_map_dims(
        ctx_struct ctx, unsigned long *pfn_min, unsigned long *pfn_max)
{
    if (pfn_min)
    {
        *pfn_min = ctx->addr_map_pfn_min;
    }
    if (pfn_max)
    {
        *pfn_max = ctx->addr_map_pfn_max;
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
        print_err("BUG: set_addr_map_vaddr out of range,"
                  " pfn: %lx, max: %lx, min: %lx, vaddr: %lx\n",
                  pfn,
                  ctx->addr_map_pfn_max,
                  ctx->addr_map_pfn_min,
                  vaddr);
    }
    return 0;
}

static unsigned long checksum_settings(struct ctx_struct *ctx)
{
    unsigned long checksum = 0;
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
        checksum += get_addr_map_vaddr_verify(ctx, i, 0);
    }
    return checksum;
}

static int save_settings(struct ctx_struct *ctx)
{
    FILE *f;
    unsigned long checksum = checksum_settings(ctx);
    unsigned long mapping = 0;

    if ((f = fopen(SETTING_FILE, "w")) == NULL)
    {
        print_err("Error! opening file: %s\n", SETTING_FILE);
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
    print_progress("Writing settings to %s\n", SETTING_FILE);

    if ((f = fopen(MAPPING_FILE, "w")) == NULL)
    {
        print_err("Error! opening file: %s\n", MAPPING_FILE);
        return - 1;
    }
    for (unsigned long i = ctx->addr_map_pfn_min;
         i < ctx->addr_map_pfn_max;
         i ++)
    {
        mapping = get_addr_map_vaddr_verify(ctx, i, 0);
        /*
         * We only dump mapping entry which resolve to a value
         * The address space may have holes so dumping everything
         * leads to unnecessary large files!
         */
        if (mapping != 0)
        {
            fprintf(f, "0x%lx=0x%lx\n", i, mapping);
        }
    }
    fclose(f);
    print_progress("Writing mappings to %s\n", MAPPING_FILE);
    return 0;
}

static int read_mappings(struct ctx_struct *ctx)
{
    FILE *f;
    char line[256];
    unsigned long count = 0;

    print_progress("Reading mappings from %s\n", MAPPING_FILE);
    if ((f = fopen(MAPPING_FILE, "r")) == NULL)
    {
        print_err("Error! opening file: %s\n", MAPPING_FILE);
        return - 1;
    }
    while (fgets(line, sizeof(line), f))
    {
        unsigned long key, val;
        sscanf(line, "0x%lx=0x%lx", &key, &val);
        set_addr_map_vaddr(ctx, key, val);
        count ++;
    }

    if (ctx->checksum_vaddrs != checksum_vaddrs(ctx))
    {
        print_err("checksum vaddrs failed\n");
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

    print_progress("Reading settings file from %s\n", SETTING_FILE);
    if ((f = fopen(SETTING_FILE, "r")) == NULL)
    {
        print_err("Error opening file: %s\n", SETTING_FILE);
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

    print_progress("printing context: \n");
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
        print_err("checksum verify failed\n");
        print_err("checksum: %lx, verify: %lx\n", check_settings, check_settings_ctrl);
        return - 1;
    } else
    {
        print_ok("checksum is ok\n");
    }
    return 0;
}

ssize_t copy_from_target(ctx_struct ctx,
                  unsigned long from, unsigned long size, void* dest) {
    int ret;
    int mem_fd = ctx->target_mem_fd;
    print_progress("reading.. %lx+%lx to %lx\n", from, size, dest);

    lseek(mem_fd, from, SEEK_SET);
    return read(mem_fd,
               dest,
               size);
}

ssize_t copy_to_target(ctx_struct ctx,
                         void* source, unsigned long size, unsigned long dest) {
    int ret;
    int mem_fd = ctx->target_mem_fd;
    print_progress("writing.. %lx+%lx to %lx\n", (unsigned long) source, size, dest);

    lseek(mem_fd, dest, SEEK_SET);
    return write(mem_fd,
                source,
                size);
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
        print_progress("Searching addresses 0x%lx-0x%lx in pid %d...\n",
                       from,
                       to,
                       ctx->target_pid);
        for (unsigned long addr = from; addr < to; addr += PAGE_SIZE)
        {
            lseek(mem_fd, addr, SEEK_SET);
            ret = read(mem_fd,
                       &escape,
                       sizeof(struct fvp_escape_scan_struct));
            if (ret < 0)
            {
                print_err("Error while reading escape memory: %lx, addr: %lx\n",
                          ret,
                          addr);
                continue;
            }

            if (escape.ctrl_magic == FVP_CONTROL_MAGIC
                    && escape.escape_magic == FVP_ESCAPE_MAGIC)
            {
                print_err("found escape page: %p=%ld\n", addr, escape.addr_tag);

                escape_page_vaddr = addr;
                *ret_escape_pfn = escape.addr_tag;
                break;
            }
        }
    }
    *ret_escape_vaddr = escape_page_vaddr;
    return 0;
}

static long fvpmem_find_dimensions(
        struct ctx_struct *ctx,
        unsigned long *ret_min,
        unsigned long *ret_max)
{
    long ret;
    struct fvp_escape_scan_struct escape;
    const unsigned long from = ctx->target_from;
    const unsigned long to = ctx->target_to;
    const int mem_fd = ctx->target_mem_fd;

    unsigned long min_pfn = INT64_MAX;
    unsigned long max_pfn = 0;

    for (unsigned long addr = from; addr < to; addr += PAGE_SIZE)
    {

        lseek(mem_fd, addr, SEEK_SET);
        ret = read(mem_fd, &escape, sizeof(struct fvp_escape_scan_struct));
        if (ret < 0)
        {
            print_err("Error while reading escape memory: %ld, addr: %ld\n", ret, addr);
            continue;
        }
        if (escape.ctrl_magic == FVP_CONTROL_MAGIC)
        {
            min_pfn = MIN(min_pfn, escape.addr_tag);
            max_pfn = MAX(max_pfn, escape.addr_tag);
        }
    }
    if (ret_min)
    {
        *ret_min = min_pfn;
    }
    if (ret_max)
    {
        *ret_max = max_pfn;
    }
    return 0;
}

static long fvpmem_scan_addresses(struct ctx_struct *ctx)
{
    long ret;
    struct fvp_escape_scan_struct escape;
    const unsigned long from = ctx->target_from;
    const unsigned long to = ctx->target_to;
    const int mem_fd = ctx->target_mem_fd;

    for (unsigned long addr = from; addr < to; addr += PAGE_SIZE)
    {
        lseek(mem_fd, addr, SEEK_SET);
        ret = read(mem_fd, &escape, sizeof(struct fvp_escape_scan_struct));
        if (ret < 0)
        {
            print_err("Error while reading escape memory: %ld, addr: %ld\n", ret, addr);
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
                ESCAPE_PAGE_SIZE,
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
        print_err("restoring context from file failed: %ld\n", ret);
        return ret;
    }
    if (ctx->target_pid != pid)
    {
        print_err("Pid changed. Cant load from file. Old=%d, new=%d\n",
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
    print_progress("mapping vaddr 0x%lx into host\n", ctx->escape_vaddr);
    ret = fh_mmap_region(
            ctx->target_pid,
            ctx->escape_vaddr,
            ESCAPE_PAGE_SIZE,
            (char *) ctx->escape_page,
            &ctx->escape_page_mmap_ctx);
    if (ret != 0)
    {
        print_err("fh_mmap_region failed: %ld\n", ret);
        return ret;
    }
    ret = init_addr_map(ctx, ctx->addr_map_pfn_min, ctx->addr_map_pfn_max);
    if (ret != 0)
    {
        print_err("init_addr_map failed: %ld\n", ret);
        return ret;
    }
    ret = read_mappings(ctx);
    if (ret != 0)
    {
        print_err("read_mappings failed: %d\n", ret);
        return ret;
    }
    return 0;
}

static long map_memory_fvp_boot(struct ctx_struct *ctx)
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

    print_progress("mapping vaddr 0x%lx into host\n", ctx->escape_vaddr);
    ret = fh_mmap_region(
            ctx->target_pid,
            ctx->escape_vaddr,
            ESCAPE_PAGE_SIZE,
            (char *) ctx->escape_page,
            &ctx->escape_page_mmap_ctx);
    if (ret != 0)
    {
        print_err("fh_mmap_region failed: %ld\n", ret);
        return ret;
    }

    ctx->escape_page_reserve_size = *((unsigned long *) escape->data);

    print_ok("guest reserved 0x%lx bytes for escape\n",
             ctx->escape_page_reserve_size);

    /* Make fvp wait until further notice */
    escape->action = fvp_escape_setup_action_wait_guest;

    ret = fvpmem_find_dimensions(ctx,
                                 &ctx->addr_map_pfn_min,
                                 &ctx->addr_map_pfn_max);
    if (ret != 0)
    {
        print_err("fvpmem_find_dimensions failed: %ld\n", ret);
        return ret;
    }

    print_ok("init_addr_map with pfn: 0x%lx-0x%lx (%ld)\n",
             ctx->addr_map_pfn_min,
             ctx->addr_map_pfn_max,
             ctx->addr_map_pfn_max - ctx->addr_map_pfn_min);

    ret = init_addr_map(
            ctx,
            ctx->addr_map_pfn_min,
            ctx->addr_map_pfn_max);
    if (ret != 0)
    {
        print_err("init_addr_map failed: %ld\n", ret);
        return ret;
    }

    print_progress("fvpmem_scan_addresses...\n");
    ret = fvpmem_scan_addresses(ctx);
    if (ret != 0)
    {
        print_err("fvpmem_scan_addresses failed: %ld\n", ret);
        return ret;
    }

    print_ok("escape_page: 0x%lx\n", escape_page_vaddr);
    hex_dump(ctx->escape_page, 64);

    return 0;
}

static void unmap_fvp_memory(struct ctx_struct *ctx)
{
    print_progress("unmap_fvp_memory\n");
    fh_unmmap_region(&ctx->escape_page_mmap_ctx);
    print_ok("fh_unmmap_region ok\n");
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
                       ctx->escape_vaddr,
                       ctx);
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
        print_err("invalid pid\n");
        exit(- 1);
    }
    return pid;
}

static int verify_mappings(struct ctx_struct *ctx)
{
    unsigned long vaddr, vaddr_prev;
    int null = 0;
    int mapped = 0;
    int not_contiguous = 0;
    print_progress("verifying pfn-vaddr mappings\n");
    for (unsigned long i = ctx->addr_map_pfn_min + 1; i < ctx->addr_map_pfn_max; i += 1)
    {
        vaddr_prev = get_addr_map_vaddr_verify(ctx, i - 1, 0);
        vaddr = get_addr_map_vaddr_verify(ctx, i, 0);
        if (vaddr_prev == 0 && vaddr == 0)
        {
            null ++;
            continue;
        } else {
            mapped ++;
        }
        if (vaddr_prev + PAGE_SIZE != vaddr)
        {
            print_progress("non-contiguous addresses : 0x%lx, 0x%lx\n", vaddr_prev, vaddr);
        }
    }
    print_ok("number of unmapped pfn: %d\n", null);
    print_ok("number of mapped pfn: %d\n", mapped);
    print_progress("verifying escape_page_pfn\n");

    not_contiguous = 0;
    for (unsigned long i = ctx->escape_page_pfn + 1;
         i < ctx->escape_page_pfn + (ctx->escape_page_reserve_size >> 12); i += 1)
    {
        vaddr_prev = get_addr_map_vaddr_verify(ctx, i - 1, 0);
        vaddr = get_addr_map_vaddr_verify(ctx, i, 0);
        if (vaddr_prev + PAGE_SIZE != vaddr)
        {
            print_err("vaddr : 0x%lx, 0x%lx\n", vaddr_prev, vaddr);
            not_contiguous ++;

        }
    }
    if (not_contiguous != 0)
    {
        print_err("escape_page_pfn is not contiguous on host\n");
    }
    return 0;
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
    print_ok("Using pid %d \n", pid);

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
    print_ok("escape page size: %lx bytes\n", ESCAPE_PAGE_SIZE);

    ret = map_memory_from_file(&ctx, pid);
    if (ret != 0)
    {
        print_err("map memory from file failed. "
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

        ret = map_memory_fvp_boot(&ctx);
        if (ret != 0)
        {
            goto clean_up;
        }

        /*
         * We store settings such that we can
         * restart manager without rebooting fvp.
         */
        save_settings(&ctx);

        print_ok("continuing guest\n");
        ((struct fvp_escape_setup_struct *) ctx.escape_page)
                ->escape_turn = fvp_escape_setup_turn_guest;

        ((struct fvp_escape_setup_struct *) ctx.escape_page)
                ->action = fvp_escape_setup_action_addr_mapping_success;
    }

    if (ctx.escape_page_reserve_size < ESCAPE_PAGE_SIZE)
    {
        print_err("escape reserve size (%lx) < ESCAPE_PAGE_SIZE (%lx)\n",
                  ctx.escape_page_reserve_size,
                  ESCAPE_PAGE_SIZE);
    }

    verify_mappings(&ctx);

    if (has_faulthook)
    {
        unsigned long vaddr_target = ctx.escape_vaddr;
        pthread_t *th = run_thread((fh_listener_fn) _on_fault, &ctx);
        ret = fh_enable_trace(
                (unsigned long) vaddr_target,
                8,
                pid);
        if (ret != 0)
        {
            print_err("fh_enable_trace failed: %d\n", ret);
            goto clean_up;
        }
        pthread_join(*th, NULL);

    } else
    {
        print_err("no faulthook present on system\n");
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
