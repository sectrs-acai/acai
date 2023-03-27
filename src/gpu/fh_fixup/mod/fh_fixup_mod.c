#include <linux/fs.h>
#include <linux/fs.h>
#include <linux/kallsyms.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/mm_types.h>
#include <asm/tlbflush.h>
#include <asm/uaccess.h>
#include <asm/io.h>
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
#include <linux/mm.h>
#include <linux/slab.h>

#include "fh_fixup_mod.h"

MODULE_AUTHOR("");
MODULE_DESCRIPTION("");
MODULE_LICENSE("GPL");


#undef pr_fmt
#define pr_fmt(fmt) "[fh_fixup] " fmt

#define HERE pr_info("%s/%s: %d\n", __FILE__, __FUNCTION__, __LINE__)

static bool device_busy = false;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
#define KPROBE_KALLSYMS_LOOKUP 1

typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);

kallsyms_lookup_name_t kallsyms_lookup_name_func;
#define kallsyms_lookup_name kallsyms_lookup_name_func

static struct kprobe kp = {
        .symbol_name = "kallsyms_lookup_name"
};
#endif

void (*invalidate_tlb)(unsigned long) = NULL;

void (*flush_tlb_mm_range_func)(struct mm_struct *,
                                unsigned long,
                                unsigned long,
                                unsigned int,
                                bool) = NULL;

void (*native_write_cr4_func)(unsigned long) = NULL;

void (*flush_tlb_all)(void) = NULL;

int (*do_mprotect_pkey)(
        unsigned long start,
        size_t len,
        unsigned long prot,
        int pkey) = NULL;

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

struct protect_pid_struct
{
    struct work_struct work;
    struct mm_struct *mm;
    unsigned long addr;
    size_t len;
    unsigned long prot;
    int ret_value;
};

static int fops_open(struct inode *inode, struct file *file)
{
    pr_info("fops_open \n");
    if (device_busy == true) {
        return - EBUSY;
    }
    device_busy = true;
    return 0;
}

static int fops_release(struct inode *inode, struct file *file)
{
    pr_info("fops_release \n");
    device_busy = false;
    return 0;
}

static int resolve_vm(struct mm_struct *mm, size_t addr, vm_t *entry, int lock)
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
    if (pgd_none(*(entry->pgd)) || pgd_bad(*(entry->pgd))) {
        entry->pgd = NULL;
        goto error_out;
    }


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    /* Return p4d offset */
    entry->p4d = p4d_offset(entry->pgd, addr);
    if (p4d_none(*(entry->p4d)) || p4d_bad(*(entry->p4d))) {
        entry->p4d = NULL;
        goto error_out;
    }

    /* Get offset of PUD (page upper directory) */
    entry->pud = pud_offset(entry->p4d, addr);
    if (pud_none(*(entry->pud))) {
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
    if (pmd_none(*(entry->pmd)) || pud_large(*(entry->pud))) {
        entry->pmd = NULL;
        goto error_out;
    }

    /* Map PTE (page table entry) */
    entry->pte = pte_offset_map(entry->pmd, addr);
    if (entry->pte == NULL || pmd_large(*(entry->pmd))) {
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

static void dump_pagetable(unsigned long address)
{
    pgd_t * base = __va(read_cr3_pa());
    pgd_t * pgd = &base[pgd_index(address)];
    p4d_t * p4d;
    pud_t * pud;
    pmd_t * pmd;
    pte_t * pte;

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

static struct mm_struct *get_mm(size_t pid)
{
    struct task_struct *task;
    struct pid *vpid;
    task = current;
    if (pid != task_pid_nr(current)) {
        vpid = find_vpid(pid);
        if (! vpid) {
            return NULL;
        }
        task = pid_task(vpid, PIDTYPE_PID);
        if (! task)
            return NULL;
    }
    if (task->mm) {
        return task->mm;
    } else {
        return task->active_mm;
    }
    return NULL;
}

static void protect_pid_worker(struct work_struct *work)
{
    struct protect_pid_struct *data
            = container_of(work,
                           struct protect_pid_struct, work);
    kthread_use_mm(data->mm);
    pr_info("addr: %lx, len: %lx, prot: %x\n", data->addr, data->len, data->prot);
    data->ret_value = do_mprotect_pkey(data->addr,
                                       data->len,
                                       data->prot, - 1);
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

    HERE;
    if (pid == current->pid) {
        HERE;
        return do_mprotect_pkey(addr, len, prot, - 1);
    } else {
        mm = get_mm(pid);
        if (mm == NULL) {
            pr_info("invalid pid\n");
            ret = - EFAULT;
            goto clean_up;
        }

        pr_info("protect_pid: %d, %ld, %ld, %x\n", pid, addr, len, prot);
        INIT_WORK(&work.work, protect_pid_worker);
        work.mm = mm;
        work.addr = addr;
        work.len = len;
        work.prot = prot;

        (void) schedule_work(&work.work);
        flush_work(&work.work);
        ret = work.ret_value;
        clean_up:
        if (mm) {
            mmput(mm);
        }
    }

    return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
#define read_lock_mm(mm) mmap_read_lock(mm)
#define read_unlock_mm(mm) mmap_read_unlock(mm)
#else
#define read_lock_mm(mm) down_read(&mm->mmap_lock)
#define read_unlock_mm(mm) up_read(&mm->mmap_lock)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
#define write_lock_mm(mm) mmap_write_lock(mm)
#define write_unlock_mm(mm) mmap_write_unlock(mm)
#else
#define write_lock_mm(mm) down_write(&mm->mmap_lock)
#define write_unlock_mm(mm) up_write(&mm->mmap_lock)
#endif
static vm_t vm_target;
static vm_t vm_source;
unsigned long target_orig_vma_flags;
unsigned long target_orig_pte;

static long fops_ioctl_do_map(struct file *file,
                              unsigned int ioctl_num,
                              unsigned long arg,
                              int do_map)
{
    long ret = 0;
    fh_fixup_map_t fixup_map;
    unsigned long target_addr, src_addr;
    pid_t target_pid;
    unsigned long target_size;
    unsigned long *target_addr_buf;
    unsigned long *src_addr_buf;
    unsigned long buf_size;

    if (copy_from_user(&fixup_map, (void __user *) arg, sizeof(fixup_map))) {
        return - EFAULT;
    }
    HERE;
    if (fixup_map.target_pages == NULL
            || fixup_map.pages_num == 0
            || fixup_map.src_pages == NULL) {
        return - EINVAL;
    }
    buf_size = fixup_map.pages_num * sizeof(size_t);
    target_addr_buf = kmalloc(buf_size, GFP_KERNEL);
    if (target_addr_buf == NULL) {
        return - ENOMEM;
    }
    src_addr_buf = kmalloc(buf_size, GFP_KERNEL);
    if (src_addr_buf == NULL) {
        return - ENOMEM;
    }

    if (copy_from_user(src_addr_buf, (void __user *) fixup_map.src_pages, buf_size)) {
        return - EFAULT;
    }
    if (copy_from_user(target_addr_buf, (void __user *) fixup_map.target_pages, buf_size)) {
        return - EFAULT;
    }

    target_addr = (unsigned long) target_addr_buf[0];
    src_addr = (unsigned long) src_addr_buf[0];
    target_pid = fixup_map.target_pid;
    target_size = PAGE_SIZE;
    HERE;
    pr_info("target_addr: %lx\n", target_addr);
    pr_info("src_addr: %lx\n", src_addr);
    pr_info("buf_size: %d\n", buf_size);
    pr_info("target_pid: %d\n", target_pid);
    pr_info("target_size: %d\n", target_size);

    struct mm_struct *mm = current->mm;
    if (target_pid != task_pid_nr(current)) {
        pr_info("using other process pid\n");
        mm = get_mm(target_pid);
        if (mm == NULL) {
            pr_info("invalid pid\n");
            return - EFAULT;
        }
    }
    {
        HERE;
        read_lock_mm(mm);
        struct vm_area_struct *vma = find_vma(mm, target_addr);
        if (vma == NULL
                || target_addr >= vma->vm_end
                || target_addr < vma->vm_start) {
            pr_info("target_addr %lx not found in vma\n", target_addr);
            return - EFAULT;
        }
        read_unlock_mm(mm);
    }


    HERE;
    ret = resolve_vm(mm, target_addr, &vm_target, 1);
    if (ret != 0) {
        pr_info("pte lookup failed\n");
        return - ENXIO;
    }
    HERE;

    struct vm_area_struct *target_vma;
    struct vm_area_struct *src_vma;
    if (do_map) {
        #if 1
        ret = protect_pid(
                target_pid,
                target_addr,
                target_size,
                (PROT_EXEC | PROT_READ | PROT_WRITE)
        );
        if (ret != 0) {
            pr_info("protect failed for addr %ld: %d\n", target_addr, ret);
            return - EFAULT;
        }
        #endif
#if 1

        read_lock_mm(mm);
        read_lock_mm(current->mm);
        target_vma = find_vma(mm, target_addr);
        if (target_vma == NULL) {
            pr_info("target_addr %lx not found in vma\n", target_addr);
            return - EFAULT;
        }
        src_vma = find_vma(current->mm, src_addr);
        if (src_vma == NULL) {
            pr_info("src_vma %lx not found in vma\n", src_addr);
            return - EFAULT;
        }

        read_unlock_mm(current->mm);
        read_unlock_mm(mm);

////        target_orig_vma_flags = (unsigned long)target_vma->vm_flags;
////        target_vma->vm_flags = src_vma->vm_flags;
        #endif
    }

    HERE;
    pr_info("page table before mapping\n");
    dump_pagetable(target_addr);

    ret = resolve_vm(current->mm, src_addr, &vm_source, 1);
    if (ret != 0) {
        pr_info("pte lookup failed for src\n");
        return - ENXIO;
    }

    pr_info("page table of srouce\n");
    dump_pagetable(src_addr);

    if (do_map) {
        /* TODO clear mapping, we dont care about restoring for now */
        target_orig_pte = vm_target.pte->pte;
        vm_target.pte->pte = 0;
        // vm_target.pte->pte = vm_source.pte->pte;
        ret = remap_pfn_range(target_vma, target_addr, vm_source.pte->pte >> PAGE_SHIFT,
                        PAGE_SIZE,
                        src_vma->vm_page_prot);
        if (ret < 0) {
            pr_info("remap fn renage failed with %d\n", ret);
            return ret;
        }

    } else {
        // vm_target.pte->pte = target_orig_pte;
    }



    // TODO: flags and protection from user
    flush_tlb_mm_range_func(mm,
                            target_addr, target_addr + PAGE_SIZE, PAGE_SHIFT, false);

    HERE;
    pr_info("page table after mapping\n");
    dump_pagetable(target_addr);
    return ret;
}

static long fops_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
    long ret = 0;
    pr_info("fops_ioctl cmd: %ld\n", ioctl_num);
    switch (ioctl_num) {
        case FH_FIXUP_IOCTL_MAP: {
            ret = fops_ioctl_do_map(file, ioctl_num, ioctl_param, 1);
            break;
        }
        case FH_FIXUP_IOCTL_UNMAP: {
            HERE;
            // ] BUG: Bad rss-counter state mm:000000001bc4a6a2 type:MM_ANONPAGES val:1
            ret = fops_ioctl_do_map(file, ioctl_num, ioctl_param, 0);
            break;
        }
        default: {
            ret = - EINVAL;
            break;
        }
    }
    return ret;
}

static struct file_operations fops = {
        .owner = THIS_MODULE,
        .unlocked_ioctl = fops_ioctl,
        .open = fops_open,
        .release = fops_release};

static struct miscdevice misc_dev = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = FH_FIXUP_DEVICE_NAME,
        .fops = &fops,
        .mode = S_IRWXUGO,
};


static int __init fh_fixup_init(void)
{
    int ret;

    #ifdef KPROBE_KALLSYMS_LOOKUP
    register_kprobe(&kp);
    kallsyms_lookup_name = (kallsyms_lookup_name_t) kp.addr;
    unregister_kprobe(&kp);

    if (! unlikely(kallsyms_lookup_name)) {
        pr_alert("Failed to lookup kallsyms_lookup_name_t\n");
        return - ENXIO;
    }
    #endif


    ret = misc_register(&misc_dev);
    if (ret != 0) {
        pr_alert("misc_register failede with %d\n", ret);
        return - ENXIO;
    }

    flush_tlb_mm_range_func = (void *) kallsyms_lookup_name("flush_tlb_mm_range");
    if (! flush_tlb_mm_range_func) {
        pr_alert("Could not retrieve flush_tlb_mm_range function\n");
        return - ENXIO;
    }
    if (! cpu_feature_enabled(X86_FEATURE_INVPCID_SINGLE)) {
        native_write_cr4_func = (void *) kallsyms_lookup_name("native_write_cr4");
        if (! native_write_cr4_func) {
            pr_alert("Could not retrieve native_write_cr4 function\n");
            return - ENXIO;
        }
    }
    do_mprotect_pkey = (void *) kallsyms_lookup_name("do_mprotect_pkey");
    if (do_mprotect_pkey == NULL) {
        pr_info("lookup failed orig_mprotect_pkey\n");
        return - ENXIO;
    }

    pr_info("Loaded.\n");
    return 0;
}

static void __exit fh_fixup_exit(void)
{
    misc_deregister(&misc_dev);
    pr_info("fh_fixup_exit.\n");
}

module_init(fh_fixup_init);
module_exit(fh_fixup_exit);