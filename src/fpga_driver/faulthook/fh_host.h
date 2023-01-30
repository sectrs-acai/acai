//
// Created by b on 1/12/23.
//

#ifndef TRUSTED_PERIPH_FAULTHOOKLIB_H
#define TRUSTED_PERIPH_FAULTHOOKLIB_H

#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include "ptedit_header.h"

#define fh_host_fn

#define PTR_FMT "0x%llx"

#define HERE printf("%s/%s: %d\n", __FILE__, __FUNCTION__, __LINE__)

#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_RESET "\x1b[0m"

#define TAG_OK COLOR_GREEN "[+]" COLOR_RESET " "
#define TAG_FAIL COLOR_RED "[-]" COLOR_RESET " "
#define TAG_PROGRESS COLOR_YELLOW "[~]" COLOR_RESET " "


#define log_helper(fmt, ...) printf(fmt "\n%s", __VA_ARGS__)
#define print_progress(...) log_helper(TAG_PROGRESS " "__VA_ARGS__, "")
#define print_ok(...) log_helper(TAG_OK " " __VA_ARGS__, "")
#define print_err(...) log_helper(TAG_FAIL " " __VA_ARGS__, "")

#define PAGE_SIZE 4096
#define FAULTHOOK_DIR "/sys/kernel/debug/faulthook/"

typedef struct fh_extract_region {
    unsigned long num;
    unsigned long address;
    unsigned int region_id;
    unsigned long match_off;
    const char *region_type_str;
    int region_type;
} extract_region_t;

typedef int (*fh_listener_fn) (void*);

struct fh_host_context {
    bool signals_init;
    bool hook_enabled;
    bool hook_init;
    bool pedit_init;
    int fd_hook;
    pid_t victim_pid;
    unsigned long victim_addr;

    fh_listener_fn listener;
    pthread_t listener_thread;

    char *mmap_host;
    unsigned long mmap_num_of_pages;
    ptedit_entry_t host_pte;
    size_t *host_pte_orig;
};

extern struct fh_host_context fh_ctx;

fh_host_fn pthread_t* run_thread(fh_listener_fn fn, void* ctx);

fh_host_fn int fh_init_pedit();
fh_host_fn int fh_init(const char *device);

fh_host_fn int fh_enable_trace(unsigned long address, unsigned long len, pid_t pid);

fh_host_fn int fh_disable_trace(void);

fh_host_fn int fh_clean_up(void);

fh_host_fn void fh_print_region(extract_region_t *r);


struct fh_memory_map_ctx {
    pid_t pid;
    unsigned long addr;
    char * host_mem;
    unsigned long len;

    ptedit_entry_t _host_pt;
    size_t *_host_pt_entry;
};

struct fh_mmap_region_ctx {
    unsigned long len;
    struct fh_memory_map_ctx *entries;
};

fh_host_fn int fh_unmmap_region(struct fh_mmap_region_ctx *ctx);
fh_host_fn int fh_mmap_region(pid_t pid,
                              unsigned long target_addr,
                              unsigned long len,
                              char *host_mem,
                              struct fh_mmap_region_ctx *ret_ctx);

fh_host_fn int fh_memory_unmap(struct fh_memory_map_ctx *);

fh_host_fn int fh_memory_map(struct fh_memory_map_ctx* req);

fh_host_fn int fh_scan_guest_memory(pid_t pid,
                         unsigned long magic,
                         extract_region_t **ret_regions,
                         int *ret_regions_len);

// -------------------------------------------------------
// kernel api

#define FAULTHOOK_IOCTL_MAGIC_NUMBER (long)0x3d18

#define FAULTHOOK_IOCTL_CMD_STATUS \
  _IOR(FAULTHOOK_IOCTL_MAGIC_NUMBER, 1, size_t)

#define FAULTHOOK_IOCTL_CMD_HOOK_COMPLETED \
  _IOR(FAULTHOOK_IOCTL_MAGIC_NUMBER, 2, size_t)

struct faulthook_ctrl {
    bool active;
    pid_t pid;
    unsigned long address;
    unsigned long len;
};

// -------------------------------------------------------
// utils

static inline void hex_dump(
        const void *data,
        size_t size
)
{
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++i) {
        printf("%02X ", ((unsigned char *) data)[i]);
        if (((unsigned char *) data)[i] >= ' ' && ((unsigned char *) data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char *) data)[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i + 1) % 8==0 || i + 1==size) {
            printf(" ");
            if ((i + 1) % 16==0) {
                printf("|  %s \n", ascii);
            } else if (i + 1==size) {
                ascii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8) {
                    printf(" ");
                }
                for (j = (i + 1) % 16; j < 16; ++j) {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
}

#endif //TRUSTED_PERIPH_FAULTHOOKLIB_H
