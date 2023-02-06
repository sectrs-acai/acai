#include <linux/uaccess.h>
#include <linux/mmu_context.h>
#include <linux/workqueue.h>
#include <linux/mm_types.h>
#include <linux/sched/mm.h>
#include <linux/sched/task.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/kallsyms.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mman.h>

#include <linux/kprobes.h>
#include <linux/kthread.h>
#include <linux/version.h>

#include "fvp_escape.h"

#define HERE pr_info("%s/%s: %d\n", __FILE__, __FUNCTION__, __LINE__)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
#define KPROBE_KALLSYMS_LOOKUP 1

kallsyms_lookup_name_t kallsyms_lookup_name_func;
#define kallsyms_lookup_name kallsyms_lookup_name_func

static struct kprobe kp = {
        .symbol_name = "kallsyms_lookup_name"
};
#endif

void (*flush_tlb_all)(void) = NULL;

void (*flush_tlb_mm_range)(struct mm_struct *mm, unsigned long start,
                           unsigned long end, unsigned int stride_shift,
                           bool freed_tables) = NULL;

int (*do_mprotect_pkey)(
        unsigned long start,
        size_t len,
        unsigned long prot,
        int pkey) = NULL;


static bool ensure_lookup_init = 0;

int resolve_vm(struct mm_struct *mm, size_t addr, vm_t *entry, int lock)
{
    if (! entry) return 1;
    entry->pud = NULL;
    entry->pmd = NULL;
    entry->pgd = NULL;
    entry->pte = NULL;
    entry->p4d = NULL;
    entry->valid = 0;

    /* Lock mm */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
    if (lock) mmap_read_lock(mm);
#else
    if (lock) down_read(&mm->mmap_lock);
#endif

    /* Return PGD (page global directory) entry */
    entry->pgd = pgd_offset(mm, addr);
    if (pgd_none(*(entry->pgd)) || pgd_bad(*(entry->pgd)))
    {
        entry->pgd = NULL;
        goto error_out;
    }


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    /* Return p4d offset */
    entry->p4d = p4d_offset(entry->pgd, addr);
    if (p4d_none(*(entry->p4d)) || p4d_bad(*(entry->p4d)))
    {
        entry->p4d = NULL;
        goto error_out;
    }

    /* Get offset of PUD (page upper directory) */
    entry->pud = pud_offset(entry->p4d, addr);
    if (pud_none(*(entry->pud)))
    {
        entry->pud = NULL;
        goto error_out;
    }
#else
    /* Get offset of PUD (page upper directory) */
    entry->pud = pud_offset(entry->pgd, addr);
    if (pud_none(*(entry->pud)))
    {
        entry->pud = NULL;
        goto error_out;
    }
#endif


    /* Get offset of PMD (page middle directory) */
    entry->pmd = pmd_offset(entry->pud, addr);
    if (pmd_none(*(entry->pmd)) || pud_large(*(entry->pud)))
    {
        entry->pmd = NULL;
        goto error_out;
    }

    /* Map PTE (page table entry) */
    entry->pte = pte_offset_map(entry->pmd, addr);
    if (entry->pte == NULL || pmd_large(*(entry->pmd)))
    {
        entry->pte = NULL;
        goto error_out;
    }

    /* Unmap PTE, fine on x86 and ARM64 -> unmap is NOP */
    pte_unmap(entry->pte);

    /* Unlock mm */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
    if (lock) mmap_read_unlock(mm);
#else
    if (lock) up_read(&mm->mmap_lock);
#endif

    return 0;

    error_out:

    /* Unlock mm */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
    if (lock) mmap_read_unlock(mm);
#else
    if (lock) up_read(&mm->mmap_lock);
#endif

    return 1;
}


static bool low_pfn(unsigned long pfn)
{
    return true;
}

void dump_pagetable(unsigned long address)
{
    pgd_t *base = __va(read_cr3_pa());
    pgd_t *pgd = &base[pgd_index(address)];
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;

#ifdef CONFIG_X86_PAE
    pr_info("*pdpt = %016Lx ", pgd_val(*pgd));
    if (!low_pfn(pgd_val(*pgd) >> PAGE_SHIFT) || !pgd_present(*pgd))
        goto out;
#define pr_pde pr_cont
#else
#define pr_pde pr_info
#endif
    p4d = p4d_offset(pgd, address);
    pud = pud_offset(p4d, address);
    pmd = pmd_offset(pud, address);
            pr_pde("*pde = %0*Lx ", sizeof(*pmd) * 2, (u64) pmd_val(*pmd));
#undef pr_pde

    /*
     * We must not directly access the pte in the highpte
     * case if the page table is located in highmem.
     * And let's rather not kmap-atomic the pte, just in case
     * it's allocated already:
     */
    if (! low_pfn(pmd_pfn(*pmd)) || ! pmd_present(*pmd) || pmd_large(*pmd))
        goto out;

    pte = pte_offset_kernel(pmd, address);
    pr_cont("*pte = %0*Lx ", sizeof(*pte) * 2, (u64) pte_val(*pte));
    out:
    pr_cont("\n");
}

struct mm_struct *get_mm(size_t pid)
{
    struct task_struct *task;
    struct pid *vpid;

    // TODO: clean up with  put_task_struct(task); and put_pid(pid_struct);

    /* Find mm */
    task = current;
    if (pid != 0)
    {
        vpid = find_vpid(pid);
        if (! vpid)
            return NULL;
        task = pid_task(vpid, PIDTYPE_PID);
        if (! task)
            return NULL;
    }
    if (task->mm)
    {
        return task->mm;
    } else
    {
        return task->active_mm;
    }
    return NULL;
}


int fvp_escape_init(void)
{
    if (! ensure_lookup_init)
    {
        ensure_lookup_init = 1;
        #ifdef KPROBE_KALLSYMS_LOOKUP
        register_kprobe(&kp);
        kallsyms_lookup_name = (kallsyms_lookup_name_t) kp.addr;
        unregister_kprobe(&kp);

        if (! unlikely(kallsyms_lookup_name))
        {
            pr_alert("Could not retrieve kallsyms_lookup_name address\n");
            return - ENXIO;
        }
        #endif

        flush_tlb_mm_range = (void *) kallsyms_lookup_name("flush_tlb_mm_range");
        if (flush_tlb_mm_range == NULL)
        {
            pr_info("lookup failed flush_tlb_mm_range\n");
            return - ENXIO;
        }

        do_mprotect_pkey = (void *) kallsyms_lookup_name("do_mprotect_pkey");
        if (do_mprotect_pkey == NULL)
        {
            pr_info("lookup failed orig_mprotect_pkey\n");
            return - ENXIO;
        }
    }

    return 0;
}


static void protect_pid_worker(struct work_struct *work)
{
    HERE;
    struct protect_pid_struct *data

            = container_of(work,
                           struct protect_pid_struct, work);

    HERE;
    kthread_use_mm(data->mm);
    pr_info("addr: %lx, len: %lx, prot: %x\n", data->addr, data->len, data->prot);

    fvp_escape_init();
    data->ret_value = do_mprotect_pkey(data->addr,
                                       data->len,
                                       data->prot, - 1);
    HERE;
    kthread_unuse_mm(data->mm);
}


/*
 * spawn kernel worker, assign it the mm of the requested pid and change its vma
 * to reflect given protection flags
 */
long protect_pid(
        pid_t pid,
        unsigned long addr,
        size_t len,
        unsigned long prot
)
{
    long ret;
    struct protect_pid_struct work;
    struct mm_struct *mm = NULL;
    fvp_escape_init();

    if (pid == current->pid)
    {
        return do_mprotect_pkey(addr, len, prot, - 1);
    } else
    {
        mm = get_mm(pid);
        if (mm == NULL)
        {
            pr_info("invalid pid\n");
            ret = - EFAULT;
            goto clean_up;
        }

        pr_info("protect_pid: %d, %ld, %ld, %x\n", pid, addr, len, prot);

        HERE;
        INIT_WORK(&work.work, protect_pid_worker);
        work.mm = mm;
        work.addr = addr;
        work.len = len;
        work.prot = prot;

        HERE;
        (void) schedule_work(&work.work);
        flush_work(&work.work);
        ret = work.ret_value;
        HERE;
    }

    clean_up:
//    if (mm) {
//        mmput(mm);
//    }
    return ret;
}

int prepare_mmap_remote(pid_t target_pid,
                        unsigned long vm_pgoff,
                        unsigned long vm_page_prot,
                        unsigned long vaddr_pages_size,
                        unsigned long *vaddr_pages,
                        struct mmap_remote_struct *ret_ctx,
                        struct vm_area_struct **ret_vma)
{
    // TODO mmap_remote_struct

    int ret = 0;
    unsigned long addr;
    unsigned long size = PAGE_SIZE;
    vm_t vm;

    if (vaddr_pages_size != 1)
    {
        pr_info("currently only supporting 1 page\n");
        return - EINVAL;
    }

    addr = vaddr_pages[0];
    pr_info("addr: %lx, size: %lx\n", addr, size);

    struct mm_struct *mm = current->mm;
    if (target_pid != 0)
    {
        pr_info("using other process pid\n");
        mm = get_mm(target_pid);
        if (mm == NULL)
        {
            pr_info("invalid pid\n");
            return - EFAULT;
        }
    }
    struct vm_area_struct *vma = find_vma(mm, addr);
    if (vma == NULL)
    {
        pr_info("addr not found in vma\n");
        return - EFAULT;
    }
    if (addr >= vma->vm_end)
    {
        pr_info("addr not found in vma\n");
        return - EFAULT;
    }

    ret = resolve_vm(mm, addr, &vm, 1);
    if (ret != 0)
    {
        pr_info("pte lookup failed\n");
        return - ENXIO;
    }
    ret = protect_pid(
            target_pid,
            addr,
            size,
            (PROT_EXEC | PROT_READ | PROT_WRITE)
    );
    if (ret != 0)
    {
        pr_info("protect failed for addr %ld\n", addr);
        return - EFAULT;
    }

    pr_info("page table before mapping\n");
    dump_pagetable(addr);

    /* TODO clear mapping, we dont care about restoring for now */
    vm.pte->pte = 0;
    vma->vm_pgoff = vm_pgoff;

    // TODO: flags and protection from user
    flush_tlb_mm(mm);
    *ret_vma = vma;

    return ret;
}
