/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2016-present,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/ioctl.h>
#include "version.h"
#include "xdma_cdev.h"
#include "cdev_ctrl.h"
#include <linux/memory.h>

#if ACCESS_OK_2_ARGS
#define xlx_access_ok(X, Y, Z) access_ok(Y, Z)
#else
#define xlx_access_ok(X, Y, Z) access_ok(X, Y, Z)
#endif

/*
 * character device file operations for control bus (through control bridge)
 */
static ssize_t char_ctrl_read(struct file *fp, char __user *buf, size_t count,
                              loff_t *pos)
{
    HERE;
    struct xdma_cdev *xcdev = (struct xdma_cdev *) fp->private_data;
    struct xdma_dev *xdev;
    void __iomem *reg;
    u32 w;
    int rv;

    rv = xcdev_check(__func__, xcdev, 0);
    if (rv < 0)
        return rv;
    xdev = xcdev->xdev;

    /* only 32-bit aligned and 32-bit multiples */
    if (*pos & 3)
        return -EPROTO;
    /* first address is BAR base plus file position offset */
    reg = xdev->bar[xcdev->bar] + *pos;
    //w = read_register(reg);
    w = ioread32(reg);
    dbg_sg("%s(@%p, count=%ld, pos=%d) value = 0x%08x\n",
           __func__, reg, (long) count, (int) *pos, w);
    rv = copy_to_user(buf, &w, 4);
    if (rv)
            dbg_sg("Copy to userspace failed but continuing\n");

    *pos += 4;
    return 4;
}

static ssize_t char_ctrl_write(struct file *file, const char __user *buf,
                               size_t count, loff_t *pos)
{
    HERE;
    struct xdma_cdev *xcdev = (struct xdma_cdev *) file->private_data;
    struct xdma_dev *xdev;
    void __iomem *reg;
    u32 w;
    int rv;

    rv = xcdev_check(__func__, xcdev, 0);
    if (rv < 0)
        return rv;
    xdev = xcdev->xdev;

    /* only 32-bit aligned and 32-bit multiples */
    if (*pos & 3)
        return -EPROTO;

    /* first address is BAR base plus file position offset */
    reg = xdev->bar[xcdev->bar] + *pos;
    rv = copy_from_user(&w, buf, 4);
    if (rv)
        pr_info("copy from user failed %d/4, but continuing.\n", rv);

    dbg_sg("%s(0x%08x @%p, count=%ld, pos=%d)\n",
           __func__, w, reg, (long) count, (int) *pos);
    //write_register(w, reg);
    iowrite32(w, reg);
    *pos += 4;
    return 4;
}

static long version_ioctl(struct xdma_cdev *xcdev, void __user *arg)
{
    HERE;
    struct xdma_ioc_info obj;
    struct xdma_dev *xdev = xcdev->xdev;
    int rv;

    rv = copy_from_user((void *) &obj, arg, sizeof(struct xdma_ioc_info));
    if (rv) {
        pr_info("copy from user failed %d/%ld.\n",
                rv, sizeof(struct xdma_ioc_info));
        return -EFAULT;
    }
    memset(&obj, 0, sizeof(obj));
    obj.vendor = xdev->pdev->vendor;
    obj.device = xdev->pdev->device;
    obj.subsystem_vendor = xdev->pdev->subsystem_vendor;
    obj.subsystem_device = xdev->pdev->subsystem_device;
    obj.feature_id = xdev->feature_id;
    obj.driver_version = DRV_MOD_VERSION_NUMBER;
    obj.domain = 0;
    obj.bus = PCI_BUS_NUM(xdev->pdev->devfn);
    obj.dev = PCI_SLOT(xdev->pdev->devfn);
    obj.func = PCI_FUNC(xdev->pdev->devfn);
    if (copy_to_user(arg, &obj, sizeof(struct xdma_ioc_info)))
        return -EFAULT;
    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
#define KPROBE_KALLSYMS_LOOKUP 1

#include <linux/kprobes.h>

typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);

kallsyms_lookup_name_t kallsyms_lookup_name_func;
#define kallsyms_lookup_name kallsyms_lookup_name_func

static struct kprobe kp = {
        .symbol_name = "kallsyms_lookup_name"
};
#endif

int (*orig_do_munmap)(struct mm_struct *, unsigned long, size_t,
                      struct list_head *uf);

void (*orig_flush_tlb_all)(void);

void (*orig_unmap_mapping_page)(struct page *page);

static int kprobe_init = 0;

typedef struct {
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


static int resolve_vm(struct mm_struct *mm, size_t addr, vm_t *entry, int lock)
{

    if (!entry) return 1;
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
    if(lock) down_read(&mm->mmap_sem);
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
  if (pud_none(*(entry->pud))) {
    entry->pud = NULL;
    goto error_out;
  }
  entry->valid |= PTEDIT_VALID_MASK_PUD;
#endif


    /* Get offset of PMD (page middle directory) */
    entry->pmd = pmd_offset(entry->pud, addr);
    if (pmd_none(*(entry->pmd)) || pud_large(*(entry->pud))) {
        entry->pmd = NULL;
        goto error_out;
    }

    /* Map PTE (page table entry) */
    entry->pte = pte_offset_map(entry->pmd, addr);
    if (entry->pte==NULL || pmd_large(*(entry->pmd))) {
        entry->pte = NULL;
        goto error_out;
    }

    /* Unmap PTE, fine on x86 and ARM64 -> unmap is NOP */
    pte_unmap(entry->pte);

    /* Unlock mm */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
    if (lock) mmap_read_unlock(mm);
#else
    if(lock) up_read(&mm->mmap_sem);
#endif

    return 0;

    error_out:

    /* Unlock mm */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
    if (lock) mmap_read_unlock(mm);
#else
    if(lock) up_read(&mm->mmap_sem);
#endif

    return 1;
}


static bool low_pfn(unsigned long pfn)
{
    return true;
}

static void dump_pagetable(unsigned long address)
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
    if (!low_pfn(pmd_pfn(*pmd)) || !pmd_present(*pmd) || pmd_large(*pmd))
        goto out;

    pte = pte_offset_kernel(pmd, address);
    pr_cont("*pte = %0*Lx ", sizeof(*pte) * 2, (u64) pte_val(*pte));
    out:
    pr_cont("\n");
}

static inline struct mm_struct *get_mm(size_t pid)
{
    struct task_struct *task;
    struct pid *vpid;

    /* Find mm */
    task = current;
    if (pid!=0) {
        vpid = find_vpid(pid);
        if (!vpid)
            return NULL;
        task = pid_task(vpid, PIDTYPE_PID);
        if (!task)
            return NULL;
    }
    if (task->mm) {
        return task->mm;
    } else {
        return task->active_mm;
    }
    return NULL;
}

void (*_flush_tlb_mm_range)(struct mm_struct *mm, unsigned long start,
                            unsigned long end, unsigned int stride_shift,
                            bool freed_tables);

#define TLB_FLUSH_ALL    -1UL
#define _flush_tlb_mm(mm)                        \
        _flush_tlb_mm_range(mm, 0UL, TLB_FLUSH_ALL, 0UL, true)

static bool ensure_lookup_init = 0;

static int ensure_lookup(void)
{
    if (!ensure_lookup_init) {
        #ifdef KPROBE_KALLSYMS_LOOKUP
        ensure_lookup_init = 1;
        register_kprobe(&kp);
        kallsyms_lookup_name = (kallsyms_lookup_name_t) kp.addr;
        unregister_kprobe(&kp);

        if (!unlikely(kallsyms_lookup_name)) {
            pr_alert("Could not retrieve kallsyms_lookup_name address\n");
            return -ENXIO;
        }
        #endif

        _flush_tlb_mm_range = (void *) kallsyms_lookup_name("flush_tlb_mm_range");
        if (_flush_tlb_mm_range==NULL) {
            pr_info("lookup failed flush_tlb_mm_range\n");
            return -ENXIO;
        }
    }

    return 0;
}


static long char_unmap_ioctl(struct file *filp, struct xdma_ioc_faulthook_mmap *io)
{
    int ret = 0;
    unsigned long addr = io->addr;
    unsigned long size = io->size;

    #if 0
    /*
     * We dont care about clean up
     * and keep the PCI memory mapped but overwrite the pte to point to original memory
     * Kernel will clean up the rest once the process dies
     */
    ret = ensure_lookup();
    if (ret!=0) {
        pr_info("lookup failed\n");
        return ret;
    }

    struct mm_struct *mm = current->mm;
    if (io->pid!=0) {
        pr_info("using other process pid\n");
        mm = get_mm(io->pid);
        if (mm==NULL) {
            pr_info("invalid pid\n");
            return -EFAULT;
        }
    }
    struct vm_area_struct *vma = find_vma(mm, addr);
    if (vma==NULL) {
        pr_info("addr not found in vma\n");
        return -EFAULT;
    }
    if (addr >= vma->vm_end) {
        pr_info("addr not found in vma\n");
        return -EFAULT;
    }
    vm_t vm;
    ret = resolve_vm(mm, addr, &vm, 1);
    if (ret!=0) {
        pr_info("pte lookup failed\n");
        return -ENXIO;
    }

    pr_info("page table before unmapping\n");
    dump_pagetable(addr);
    /* clear mapping, we dont care about restoring for now */
    vm.pte->pte = 0;

    pr_info("page table after mapping\n");
    dump_pagetable(addr);
    _flush_tlb_mm(mm);
    #endif
    return ret;
}

static long char_mmap_ioctl(struct file *filp, struct xdma_ioc_faulthook_mmap *io)
{
    int ret = 0;
    unsigned long addr = io->addr;
    unsigned long size = io->size;

    ret = ensure_lookup();
    if (ret!=0) {
        pr_info("lookup failed\n");
        return ret;
    }

    struct mm_struct *mm = current->mm;
    if (io->pid!=0) {
        pr_info("using other process pid\n");
        mm = get_mm(io->pid);
        if (mm==NULL) {
            pr_info("invalid pid\n");
            return -EFAULT;
        }
    }
    struct vm_area_struct *vma = find_vma(mm, addr);
    if (vma==NULL) {
        pr_info("addr not found in vma\n");
        return -EFAULT;
    }
    if (addr >= vma->vm_end) {
        pr_info("addr not found in vma\n");
        return -EFAULT;
    }

    vm_t vm;
    ret = resolve_vm(mm, addr, &vm, 1);
    if (ret!=0) {
        pr_info("pte lookup failed\n");
        return -ENXIO;
    }


    // TODO: is cache flush needed? Test it
    // flush_cache_mm(mm);

    pr_info("page table before mapping\n");
    dump_pagetable(addr);
    /* clear mapping, we dont care about restoring for now */
    vm.pte->pte = 0;
    vma->vm_pgoff = io->vm_pgoff;

    // TODO: flags and protection from user

    _flush_tlb_mm(mm);

    ret = bridge_mmap(filp, vma);
    pr_info("page table after mapping\n");
    dump_pagetable(addr);

    return ret;
}

#if 0
// experiments
static long char_mmap_ioctl(struct file *filp, struct xdma_ioc_faulthook_mmap *io)
{
    int ret = 0;
    unsigned long addr = io->addr;
    unsigned long size = io->size;
    pr_info("ok addr: 0x%lx, size: 0x%lx\n", addr, size);
    struct vm_area_struct *vma = find_vma(current->mm, addr);
    if (vma==NULL) {
        return -EFAULT;
    }
    if (addr >= vma->vm_end) {
        return -EFAULT;
    }
    pr_info("vma start: 0x%lx: \n", vma->vm_start);
    pr_info("vma end: 0x%lx: \n", vma->vm_end);
    /*
     * we have to clear backing before we can
     * remap pf range
     */

#ifdef KPROBE_KALLSYMS_LOOKUP
    if (!kprobe_init) {
        kprobe_init = 1;
        register_kprobe(&kp);
        kallsyms_lookup_name = (kallsyms_lookup_name_t) kp.addr;
        unregister_kprobe(&kp);

        if (!unlikely(kallsyms_lookup_name)) {
            pr_alert("Could not retrieve kallsyms_lookup_name address\n");
            return -ENXIO;
        }
    }
#endif
#if 0
    unsigned long pfn = 0;
    ret = follow_pfn(vma, addr, &pfn);
    if (ret != 0) {
        pr_info("lookup follow_pfn failed: 0x%x\n", ret);
        return -ENXIO;
    }
    struct page *page = pfn_to_page(pfn);

#endif
    struct task_struct *task = current;
    vm_t vm;
    ret = resolve_vm(task->mm, addr, &vm, 1);
    if (ret!=0) {
        pr_info("pte lookup failed\n");
        return -ENXIO;
    }


#if 0
        down_read(&task->mm->mmap_lock);
        HERE;
        struct page *page;
        struct vm_area_struct *_vma;
        ret = get_user_pages_fast(addr, 1, (FOLL_GET | FOLL_WRITE), &page);
        up_read(&task->mm->mmap_lock);
        HERE;
        if (ret < 0) {
            pr_info("lookup failed get_user_pages\n");
            return -ENXIO;
        }

        pr_info("page: %lx\n", page);
        unsigned long pfn = page_to_pfn(page);
#endif

    HERE;
#if 0
    orig_unmap_mapping_page = (void *) kallsyms_lookup_name("unmap_mapping_page");
    if (orig_unmap_mapping_page == NULL) {
        pr_info("lookup failed munmap\n");
        return -ENXIO;
    }
    orig_unmap_mapping_page(page);

#endif

    HERE;
#if 0
    orig_do_munmap = (void *) kallsyms_lookup_name("do_munmap");
    if (orig_do_munmap == NULL) {
        pr_info("lookup failed munmap\n");
        return -ENXIO;
    }

    ret = orig_do_munmap(vma->vm_mm, vma->vm_start, size, NULL);
    if (ret < 0) {
        pr_info("munmap failed: %d\n", ret);
    }
#endif


    orig_flush_tlb_all = (void *) kallsyms_lookup_name("flush_tlb_all");
    if (orig_flush_tlb_all==NULL) {
        pr_info("lookup failed munmap\n");
        return -ENXIO;
    }
    pr_info("flushing tlb\n");


    dump_pagetable(addr);
    HERE;
    unsigned long old_pte = (vm.pte->pte);
    vm.pte->pte = 0;

    vma->vm_pgoff = 0;


    orig_flush_tlb_all();
    dump_pagetable(addr);

    ret = bridge_mmap(filp, vma);
    orig_flush_tlb_all();
    dump_pagetable(addr);

    HERE;
    pr_info("mmap ioctl reeturns: %d\n", ret);
    return ret;
}
#endif

long char_ctrl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    HERE;
    struct xdma_cdev *xcdev = (struct xdma_cdev *) filp->private_data;
    struct xdma_dev *xdev;
    struct xdma_ioc_base ioctl_obj;
    long result = 0;
    int rv;

    rv = xcdev_check(__func__, xcdev, 0);
    if (rv < 0)
        return rv;

    xdev = xcdev->xdev;
    if (!xdev) {
        pr_info("cmd %u, xdev NULL.\n", cmd);
        return -EINVAL;
    }
    pr_info("cmd 0x%x, xdev 0x%p, pdev 0x%p.\n", cmd, xdev, xdev->pdev);

    if (_IOC_TYPE(cmd)!=XDMA_IOC_MAGIC) {
        pr_err("cmd %u, bad magic 0x%x/0x%x.\n",
               cmd, _IOC_TYPE(cmd), XDMA_IOC_MAGIC);
        return -ENOTTY;
    }

    if (_IOC_DIR(cmd) & _IOC_READ)
        result = !xlx_access_ok(VERIFY_WRITE, (void __user *)arg,
                                _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        result = !xlx_access_ok(VERIFY_READ, (void __user *)arg,
                                _IOC_SIZE(cmd));

    if (result) {
        pr_err("bad access %ld.\n", result);
        return -EFAULT;
    }

    switch (cmd) {
        case XDMA_IOCINFO:
            if (copy_from_user((void *) &ioctl_obj, (void __user *) arg,
                               sizeof(struct xdma_ioc_base))) {
                pr_err("copy_from_user failed.\n");
                return -EFAULT;
            }

            if (ioctl_obj.magic!=XDMA_XCL_MAGIC) {
                pr_err("magic 0x%x !=  XDMA_XCL_MAGIC (0x%x).\n",
                       ioctl_obj.magic, XDMA_XCL_MAGIC);
                return -ENOTTY;
            }
            return version_ioctl(xcdev, (void __user *) arg);
        case XDMA_IOCOFFLINE: xdma_device_offline(xdev->pdev, xdev);
            break;
        case XDMA_IOCONLINE: xdma_device_online(xdev->pdev, xdev);
            break;
        case XDMA_IOCMMAP: {
            struct xdma_ioc_faulthook_mmap obj;
            rv = copy_from_user((void *) &obj,
                                (void __user *) arg,
                                sizeof(struct xdma_ioc_faulthook_mmap));
            if (rv) {
                pr_info("copy from user failed %d/%ld.\n",
                        rv, sizeof(struct xdma_ioc_faulthook_mmap));
                return -EFAULT;
            }
            pr_info("FAULTHOOK_IOMMAP");
            if (obj.map_type==0) {
                rv = char_mmap_ioctl(filp, &obj);
            } else {
                rv = char_unmap_ioctl(filp, &obj);
            }
            if (rv) {
                return rv;
            }
            break;
        }
        default: pr_err("UNKNOWN ioctl cmd 0x%x.\n", cmd);
            return -ENOTTY;
    }
    return 0;
}

/* maps the PCIe BAR into user space for memory-like access using mmap() */
int bridge_mmap(struct file *file, struct vm_area_struct *vma)
{
    HERE;
    #define dbg_sg pr_info
    struct xdma_dev *xdev;
    struct xdma_cdev *xcdev = (struct xdma_cdev *) file->private_data;
    unsigned long off;
    unsigned long phys;
    unsigned long vsize;
    unsigned long psize;
    int rv;

    rv = xcdev_check(__func__, xcdev, 0);
    if (rv < 0)
        return rv;
    xdev = xcdev->xdev;

    off = vma->vm_pgoff << PAGE_SHIFT;
    /* BAR physical address */
    phys = pci_resource_start(xdev->pdev, xcdev->bar) + off;
    vsize = vma->vm_end - vma->vm_start;
    /* complete resource */
    psize = pci_resource_end(xdev->pdev, xcdev->bar) -
            pci_resource_start(xdev->pdev, xcdev->bar) + 1 - off;

            dbg_sg("mmap(): xcdev = 0x%08lx\n", (unsigned long) xcdev);
            dbg_sg("mmap(): cdev->bar = %d\n", xcdev->bar);
            dbg_sg("mmap(): xdev = 0x%p\n", xdev);
            dbg_sg("mmap(): pci_dev = 0x%08lx\n", (unsigned long) xdev->pdev);
            dbg_sg("off = 0x%lx, vsize 0x%lx, psize 0x%lx.\n", off, vsize, psize);
            dbg_sg("start = 0x%llx\n",
                   (unsigned long long) pci_resource_start(xdev->pdev,
                                                           xcdev->bar));
            dbg_sg("phys = 0x%lx\n", phys);

    if (vsize > psize) {
        HERE;
        return -EINVAL;
    }



    /*
     * pages must not be cached as this would result in cache line sized
     * accesses to the end point
     */
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    /*
     * prevent touching the pages (byte access) for swap-in,
     * and prevent the pages from being swapped out
     */
    vma->vm_flags |= VMEM_FLAGS;
    /* make MMIO accessible to user space */
    HERE;
    pr_info("vma->vma_start: %lx\n", vma->vm_start);
    pr_info("vma->page prot: %lx\n", vma->vm_page_prot);
    pr_info("vma->page pgoff: %lx\n", vma->vm_pgoff);
    pr_info("vma->page flags: %lx\n", vma->vm_flags);


    rv = io_remap_pfn_range(vma, vma->vm_start, phys >> PAGE_SHIFT,
                            vsize, vma->vm_page_prot);
    HERE;
            dbg_sg("vma=0x%p, vma->vm_start=0x%lx, phys=0x%lx, size=%lu = %d\n",
                   vma, vma->vm_start, phys >> PAGE_SHIFT, vsize, rv);

    if (rv)
        return -EAGAIN;
    return 0;
}


#if 0
/* maps the PCIe BAR into user space for memory-like access using mmap() */
static int bridge_mmap2(
        struct file *file,
        pid_t pid,

)
{
    unsigned long start = 0;
    unsigned long len = 0;
    struct task_struct *task = pid_task(find_vpid(pid), PIDTYPE_PID);
    struct page *pages;
    struct vm_area_struct *vmas;


    down_read(&task->mm->mmap_lock);
    if (task->mm) {
        get_user_pages_remote(task->mm,
                              start,
                              1,
                              FOLL_FORCE,
                              &pages,
                              &vmas,
                              NULL);

        struct vm_area_struct *vma = vmas;

    }


    up_read(&task->mm->mmap_lock);


    HERE;
    struct xdma_dev *xdev;
    struct xdma_cdev *xcdev = (struct xdma_cdev *) file->private_data;
    unsigned long off;
    unsigned long phys;
    unsigned long vsize;
    unsigned long psize;
    int rv;

    rv = xcdev_check(__func__, xcdev, 0);
    if (rv < 0)
        return rv;
    xdev = xcdev->xdev;

    off = vma->vm_pgoff << PAGE_SHIFT;
    /* BAR physical address */
    phys = pci_resource_start(xdev->pdev, xcdev->bar) + off;
    vsize = vma->vm_end - vma->vm_start;
    /* complete resource */
    psize = pci_resource_end(xdev->pdev, xcdev->bar) -
            pci_resource_start(xdev->pdev, xcdev->bar) + 1 - off;

    dbg_sg("mmap(): xcdev = 0x%08lx\n", (unsigned long) xcdev);
    dbg_sg("mmap(): cdev->bar = %d\n", xcdev->bar);
    dbg_sg("mmap(): xdev = 0x%p\n", xdev);
    dbg_sg("mmap(): pci_dev = 0x%08lx\n", (unsigned long) xdev->pdev);
    dbg_sg("off = 0x%lx, vsize 0x%lu, psize 0x%lu.\n", off, vsize, psize);
    dbg_sg("start = 0x%llx\n",
           (unsigned long long) pci_resource_start(xdev->pdev,
                                                   xcdev->bar));
    dbg_sg("phys = 0x%lx\n", phys);

    if (vsize > psize)
        return -EINVAL;
    /*
     * pages must not be cached as this would result in cache line sized
     * accesses to the end point
     */
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    /*
     * prevent touching the pages (byte access) for swap-in,
     * and prevent the pages from being swapped out
     */
    vma->vm_flags |= VMEM_FLAGS;
    /* make MMIO accessible to user space */
    rv = io_remap_pfn_range(vma, vma->vm_start, phys >> PAGE_SHIFT,
                            vsize, vma->vm_page_prot);
    dbg_sg("vma=0x%p, vma->vm_start=0x%lx, phys=0x%lx, size=%lu = %d\n",
           vma, vma->vm_start, phys >> PAGE_SHIFT, vsize, rv);

    if (rv)
        return -EAGAIN;
    return 0;
}
#endif

/*
 * character device file operations for control bus (through control bridge)
 */
static const struct file_operations ctrl_fops = {
        .owner = THIS_MODULE,
        .open = char_open,
        .release = char_close,
        .read = char_ctrl_read,
        .write = char_ctrl_write,
        .mmap = bridge_mmap,
        .unlocked_ioctl = char_ctrl_ioctl,
};

void cdev_ctrl_init(struct xdma_cdev *xcdev)
{
    cdev_init(&xcdev->cdev, &ctrl_fops);
}
