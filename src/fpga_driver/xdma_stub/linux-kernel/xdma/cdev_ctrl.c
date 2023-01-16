#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/ioctl.h>
#include "xdma_cdev.h"


/*
 * character device file operations for control bus (through control bridge)
 */
static ssize_t char_ctrl_read(struct file *fp, char __user *buf, size_t count,
                              loff_t *pos)
{
    NOT_SUPPORTED;
    return 0;
}

static ssize_t char_ctrl_write(struct file *file, const char __user *buf,
                               size_t count, loff_t *pos)
{
    NOT_SUPPORTED;
    return 0;
}

long char_ctrl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    NOT_SUPPORTED;
    return 0;
}


int bridge_mmap(struct file *file, struct vm_area_struct *vma)
{
    int ret;

    unsigned long size;
    unsigned long pfn_start;
    void *virt_start;

    size = vma->vm_end - vma->vm_start;

    #if 0
    unsigned long page_start = /* start at page boundary on host */ (PAGE_SIZE - fd_ctx.host_pg_offset)
            /* we have a page as buffer inbetween */ + PAGE_SIZE;

    pfn_start = page_to_pfn(fd_ctx.page) + page_start + (vma->vm_pgoff << PAGE_SHIFT);
    virt_start = page_address(fd_ctx.page) + page_start + (vma->vm_pgoff << PAGE_SHIFT);
    #endif

    pfn_start = page_to_pfn(fd_ctx.page) + 1;
    virt_start = page_address(fd_ctx.page) + PAGE_SIZE;


//    size = min(((1 << PAGE_ORDER) - vma->vm_pgoff) << PAGE_SHIFT,
//               vma->vm_end - vma->vm_start);

    printk("phys_start: 0x%lx, offset: 0x%lx, vma_size: 0x%lx, map size:0x%lx\n",
           pfn_start << PAGE_SHIFT, vma->vm_pgoff << PAGE_SHIFT,
           vma->vm_end - vma->vm_start, size);

//    if (size <= 0) {
//        printk("%s: offset 0x%lx too large, max size is 0x%lx\n", __func__,
//               vma->vm_pgoff << PAGE_SHIFT, MAX_SIZE);
//        return -EINVAL;
//    }


    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    vma->vm_flags |= (VM_IO | VM_DONTEXPAND | VM_DONTDUMP);

    ret = remap_pfn_range(vma, vma->vm_start, pfn_start, size, vma->vm_page_prot);

    if (ret) {
        printk("remap_pfn_range failed, vm_start: 0x%lx\n", vma->vm_start);
    } else {
        printk("map kernel 0x%px to user 0x%lx, size: 0x%lx\n",
               virt_start, vma->vm_start, size);
    }

    #if 1

    if (ret) {
        return ret;
    }

    struct faulthook_priv_data *info = file->private_data;
    fd_data->action = FH_ACTION_MMAP;
    struct action_mmap_device *a = (struct action_mmap_device *) &fd_data->data;
    a->fd = info->fd;
    a->vm_start = vma->vm_start;
    a->vm_end = vma->vm_end;
    a->vm_pgoff = vma->vm_pgoff;
    a->vm_flags = vma->vm_flags;
    a->vm_page_prot = (unsigned long) vma->vm_page_prot.pgprot;
    a->mmap_guest_kernel_offset = PAGE_SIZE;

    fh_do_faulthook();
    ret = a->err_no;

    fd_ctx.mmap_page_len = size;
    fd_ctx.mmap_page = (unsigned long) page_address(fd_ctx.page) + PAGE_SIZE;
    #endif

    return ret;
}

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
