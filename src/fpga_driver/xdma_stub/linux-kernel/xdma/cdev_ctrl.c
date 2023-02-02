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
    struct action_read *a = (struct action_read *) &fd_data->data;
    a->common.fd = info->fd;
    a->offset = *pos;
    a->count = count;

    fh_do_faulthook(FH_ACTION_READ);
    ret = a->common.ret;
    if (ret < 0)
    {
        pr_info("faulthook failed\n");
        return ret;
    }
    pr_info("Got %lx bytes from host read\n", a->buffer_size);

    ret = copy_to_user(buf, a->buffer, a->buffer_size);
    if (ret < 0)
    {
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

    struct action_write *a = (struct action_write *) &fd_data->data;
    a->common.fd = info->fd;
    a->offset = *pos;
    a->count = 4; /*=count;*/ // XXX: driver only writes 4 bytes

    ret = copy_to_user(a->buffer, buf, a->count);
    if (ret)
    {
        pr_info("copy to user failed\n");
        return ret;
    }
    fh_do_faulthook(FH_ACTION_WRITE);

    ret = a->common.ret;
    if (ret < 0)
    {
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
    ssize_t size;
    unsigned long pfn_start, order;
    void *virt_start;
    struct faulthook_priv_data *info = file->private_data;

    size = vma->vm_end - vma->vm_start - (vma->vm_pgoff << PAGE_SHIFT);
    pr_info("requesting size: %ld\n", size);
    if (size < 0)
    {
        return - EINVAL;
    }

    // TODO tear down logic

    order = __roundup_pow_of_two(size >> PAGE_SHIFT);
    pr_info("allocating order: %ld\n", order);
    if (order != 1)
    {
        // XXX: For now we just support 1 page
        return - EINVAL;
    }

    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    vma->vm_flags |= (VM_IO | VM_DONTEXPAND | VM_DONTDUMP);

    void *data = (void *) __get_free_pages(GFP_KERNEL, order);
    if (data == NULL)
    {
        pr_info("__get_free_pages failed\n");
        return - EINVAL;
    }
    memset(data, 0, size);

    *(((int *) data)) = 2;

    pfn_start = page_to_pfn(virt_to_page(data));
    pr_info("pfn: %lx\n", pfn_start);

    struct action_mmap_device *a = (struct action_mmap_device *) &fd_data->data;
    a->common.fd = info->fd;
    a->vm_start = vma->vm_start;
    a->vm_end = vma->vm_end;
    a->vm_pgoff = vma->vm_pgoff;
    a->vm_flags = vma->vm_flags;
    a->vm_page_prot = (unsigned long) vma->vm_page_prot.pgprot;
    a->pfn_size = 1;
    a->pfn[0] = pfn_start;

    ret = fh_do_faulthook(FH_ACTION_MMAP);
    if (ret < 0) {
        goto clean_up_and_return;
    }
    if (a->common.ret < 0)
    {
        ret = a->common.err_no;
        goto clean_up_and_return;
    }
    ret = remap_pfn_range(vma, vma->vm_start, pfn_start, size, vma->vm_page_prot);
    if (ret)
    {
        printk("remap_pfn_range failed, vm_start: 0x%lx\n", vma->vm_start);
        goto clean_up_and_return;
    }
    printk("map kernel 0x%px to user 0x%lx, size: 0x%lx\n",
           virt_start, vma->vm_start, size);

    ret = 0;
    clean_up_and_return:
    if (data != NULL)
    {
        free_pages((unsigned long) data, order);
    }
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
