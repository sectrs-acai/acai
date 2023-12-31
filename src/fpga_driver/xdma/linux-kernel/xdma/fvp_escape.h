#ifndef FVP_ESCAPE_H_
#define FVP_ESCAPE_H_
#include <stdbool.h>
#include <linux/types.h>
#include "fpga_escape_libhook/fpga_usr_manager.h"

#ifdef XDMA_DO_TRACE
#define xdma_trace(...) pr_info(__VA_ARGS__)
#else

#define xdma_trace(...)
#endif

#define HERE xdma_trace("%s/%s: %d\n", __FILE__, __FUNCTION__, __LINE__)
#define xdma_error(...) pr_err(__VA_ARGS__)
#define xdma_info(...) pr_info(__VA_ARGS__)


int fvp_escape_init(void);

typedef struct
{
    size_t pid;
    pgd_t *pgd;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    p4d_t *p4d;
#else
    size_t *p4d;
#endif
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    size_t valid;
} vm_t;

typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);

extern void (*flush_tlb_all)(void);

extern void (*flush_tlb_mm_range)(struct mm_struct *mm, unsigned long start,
                                  unsigned long end, unsigned int stride_shift,
                                  bool freed_tables);

extern int (*do_mprotect_pkey)(
        unsigned long start,
        size_t len,
        unsigned long prot,
        int pkey);


int resolve_vm(struct mm_struct *mm, size_t addr, vm_t *entry, int lock);

void dump_pagetable(unsigned long address);

struct mm_struct *get_mm(size_t pid);

#define TLB_FLUSH_ALL    -1UL
#define flush_tlb_mm(mm)                        \
        flush_tlb_mm_range(mm, 0UL, TLB_FLUSH_ALL, 0UL, true)

struct protect_pid_struct
{
    struct work_struct work;
    struct mm_struct *mm;
    unsigned long addr;
    size_t len;
    unsigned long prot;
    int ret_value;
};

long protect_pid(
        pid_t pid,
        unsigned long addr,
        size_t len,
        unsigned long prot
);


struct mmap_remote_struct
{

};

int prepare_mmap_remote(pid_t target_pid,
                        unsigned long vm_pgoff,
                        unsigned long vm_page_prot,
                        unsigned long vaddr_pages_size,
                        unsigned long *vaddr_pages,
                        struct mmap_remote_struct *ret_ctx,
                        struct vm_area_struct **ret_vma);

int release_mmap_remote(struct mmap_remote_struct *ctx,
                        struct vm_area_struct *ret_vma);


long fh_handle_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

#endif
