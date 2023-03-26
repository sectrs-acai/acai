//
// Created by b on 3/22/23.
//

#ifndef FPGA_ESCAPE_LIBHOOK__USR_MANAGER_H_
#define FPGA_ESCAPE_LIBHOOK__USR_MANAGER_H_

#ifndef MODULE
#include <stdbool.h>
#endif

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

typedef struct ctx_struct *ctx_struct;

/**
 * given a pfn, get vaddress on host
 * bool notify: if 1, print error checks if pfn not found
 */
unsigned long get_addr_map_vaddr_verify(
        ctx_struct ctx, unsigned long pfn, bool notify);

unsigned long get_mapped_escape_buffer_size(ctx_struct ctx);

unsigned long get_addr_map_vaddr(
        ctx_struct ctx,
        unsigned long pfn);

int on_fault(unsigned long addr,
             unsigned long len,
             pid_t target_pid,
             unsigned long target_addr,
             ctx_struct ctx);

ssize_t copy_from_target(ctx_struct ctx,
                         unsigned long from, unsigned long size, void* target);

ssize_t copy_to_target(ctx_struct ctx,
                       void* source, unsigned long size, unsigned long dest);

unsigned long get_addr_map_dims(
        ctx_struct ctx, unsigned long *pfn_min, unsigned long *pfn_max);


#endif //FPGA_ESCAPE_LIBHOOK__USR_MANAGER_H_
