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
    long ret = 0;
    struct faulthook_priv_data *info = fp->private_data;

    fd_data->action = FH_ACTION_READ;
    struct action_read *a = (struct action_read *) &fd_data->data;
    a->common.fd = info->fd;
    a->offset = *pos;
    a->count = count;

    fh_do_faulthook();
    ret = a->common.ret;
    if (ret < 0) {
        pr_info("faulthook failed\n");
        return ret;
    }
    pr_info("Got %lx bytes from host read\n", a->buffer_size);

    ret = copy_to_user(buf, a->buffer, a->buffer_size);
    if (ret < 0) {
        pr_info("copy_to_user failed\n");
        return ret;
    }
    *pos = a->count;
    return a->buffer_size;
}

static ssize_t char_ctrl_write(struct file *file, const char __user *buf,
                               size_t count, loff_t *pos)
{

    NOT_SUPPORTED;
    struct faulthook_priv_data *info = file->private_data;
    long ret = 0;

    fd_data->action = FH_ACTION_WRITE;
    struct action_write *a = (struct action_write *) &fd_data->data;
    a->common.fd = info->fd;
    a->offset = *pos;
    a->count = 4; /*=count;*/ // XXX: driver only writes 4 bytes

    ret = copy_to_user(a->buffer, buf, a->count);
    if (ret) {
        pr_info("copy to user failed\n");
        return ret;
    }
    fh_do_faulthook();

    ret = a->common.ret;
    if (ret < 0) {
        pr_info("faulthook failed\n");
        return ret;
    }
    pr_info("wrote %lx bytes from host write\n", a->buffer_size);

    *pos += a->count;
    return a->buffer_size;
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

    #if 0
    if (fd_ctx.mmap_busy) {
        pr_err("another mmap currently mapped\n");
        return -EBUSY;
    }
    #endif

    struct faulthook_priv_data *info = file->private_data;
    size = vma->vm_end - vma->vm_start;

    if (size > 4096) {
        pr_err("We dont support larger mappings but 1 page. Got: 0x%lx\n", size);
        return -EINVAL;
    }
    if (vma->vm_pgoff > 0) {
        pr_err("We dont support vm_pgoff larger than 0\n");
        return -EINVAL;
    }

    const unsigned long mmap_offset_page = 1;
    const unsigned long mmap_offset_size = mmap_offset_page << PAGE_SHIFT;

    pfn_start = page_to_pfn(fd_ctx.page) + mmap_offset_page;
    virt_start = page_address(fd_ctx.page) + mmap_offset_size;

    #if 0
    unsigned long page_start = /* start at page boundary on host */ (PAGE_SIZE - fd_ctx.host_pg_offset)
            /* we have a page as buffer inbetween */ + PAGE_SIZE;

    pfn_start = page_to_pfn(fd_ctx.page) + page_start + (vma->vm_pgoff << PAGE_SHIFT);
    virt_start = page_address(fd_ctx.page) + page_start + (vma->vm_pgoff << PAGE_SHIFT);
    size = min(((1 << PAGE_ORDER) - vma->vm_pgoff) << PAGE_SHIFT,
               vma->vm_end - vma->vm_start);

    printk("phys_start: 0x%lx, offset: 0x%lx, vma_size: 0x%lx, map size:0x%lx\n",
           pfn_start << PAGE_SHIFT, vma->vm_pgoff << PAGE_SHIFT,
           vma->vm_end - vma->vm_start, size);
//    if (size <= 0) {
//        printk("%s: offset 0x%lx too large, max size is 0x%lx\n", __func__,
//               vma->vm_pgoff << PAGE_SHIFT, MAX_SIZE);
//        return -EINVAL;
//    }
    #endif

    fd_data->action = FH_ACTION_MMAP;
    struct action_mmap_device *a = (struct action_mmap_device *) &fd_data->data;
    a->common.fd = info->fd;
    a->vm_start = vma->vm_start;
    a->vm_end = vma->vm_end;
    a->vm_pgoff = vma->vm_pgoff;
    a->vm_flags = vma->vm_flags;
    a->vm_page_prot = (unsigned long) vma->vm_page_prot.pgprot;
    a->mmap_guest_kernel_offset = mmap_offset_size;

    fh_do_faulthook();
    if (a->common.ret < 0) {
        ret = a->common.err_no;
        goto clean_up_and_return;
    }

    pr_info("action_mmap device succeeded\n");

    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    vma->vm_flags |= (VM_IO | VM_DONTEXPAND | VM_DONTDUMP);

    ret = remap_pfn_range(vma, vma->vm_start, pfn_start, size, vma->vm_page_prot);

    if (ret) {
        printk("remap_pfn_range failed, vm_start: 0x%lx\n", vma->vm_start);
        goto clean_up_and_return;
    }
    printk("map kernel 0x%px to user 0x%lx, size: 0x%lx\n",
           virt_start, vma->vm_start, size);

    fd_ctx.mmap_page_len = mmap_offset_size;
    fd_ctx.mmap_page = (unsigned long) virt_start;

    fd_ctx.mmap_busy = 1;

    ret = 0;
    clean_up_and_return:
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
