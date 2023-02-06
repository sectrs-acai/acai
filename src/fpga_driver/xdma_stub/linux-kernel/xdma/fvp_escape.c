#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include "fvp_escape.h"
#include <linux/ioctl.h>
#include <linux/mm.h>
#include <linux/slab.h>

struct faultdata_driver_struct fd_ctx;

int fh_char_open(struct inode *inode, struct file *file)
{
    struct faulthook_priv_data *info = kmalloc(sizeof(struct faulthook_priv_data), GFP_KERNEL);
    file->private_data = info;

    struct action_openclose_device *a = (struct action_openclose_device *) &fd_data->data;

    strcpy(a->device, "/dev/");
    strcpy(a->device + strlen(a->device), file->f_path.dentry->d_iname);
    a->flags = file->f_flags;

    fh_do_faulthook(FH_ACTION_OPEN_DEVICE);
    /*
     * We use fd as key to query device on host
     */
    info->fd = a->common.fd;
    return a->common.err_no;
}

/*
 * Called when the device goes from used to unused.
 */
int fh_char_close(struct inode *inode, struct file *file)
{
    int ret;

    struct faulthook_priv_data *info = file->private_data;
    struct action_openclose_device *a = (struct action_openclose_device *) &fd_data->data;
    a->common.fd = info->fd;

    fh_do_faulthook(FH_ACTION_CLOSE_DEVICE);
    ret = a->common.err_no;

    kfree(file->private_data);
    return ret;
}


int faulthook_init(void)
{
    pr_info("faulthook page: %lx+%lx\n", (unsigned long) fvp_escape_page, fvp_escape_size);
    memset(&fd_ctx, 0, sizeof(struct faultdata_driver_struct));
    memset(fvp_escape_page, 0, fvp_escape_size);
    fd_data = (struct faultdata_struct *) fvp_escape_page;
    fh_do_faulthook(FH_ACTION_SETUP);
    return 0;
}


int fh_do_faulthook(int action)
{
    unsigned long nonce = ++ fd_ctx.fh_nonce;
    fd_data->turn = FH_TURN_HOST;
    fd_data->action = action;

    #if defined(__x86_64__) || defined(_M_X64)
    #else
    asm volatile("dmb sy");
    #endif
    fd_data->nonce = nonce; /* escape to other world */
    if (fd_data->turn != FH_TURN_GUEST)
    {
        pr_err("Host did not reply to request. Nonce: 0x%lx. Is host listening?", nonce);
        return -ENXIO;
    }
    return 0;
}

int faulthook_cleanup(void)
{
    fh_do_faulthook(FH_ACTION_TEARDOWN);
    memset(fvp_escape_page, 0, fvp_escape_size);
    return 0;
}

/*
 * character device file operations for control bus (through control bridge)
 */
ssize_t fh_char_ctrl_read(struct file *fp,
                          char __user *buf,
                          size_t count,
                          loff_t *pos)
{
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

ssize_t fh_char_ctrl_write(struct file *file, const char __user *buf,
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

long fh_char_ctrl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct faulthook_priv_data *fh_info = filp->private_data;
    struct action_ioctl *escape = (struct action_ioctl *) &fd_data->data;
    escape->common.fd = fh_info->fd;
    escape->cmd = cmd;
    escape->arg = arg;

    NOT_SUPPORTED;
    return 0;
}

static void vm_close(struct vm_area_struct *vma)
{
    /*
     * Issue:
     * We cannot unfree kmalloc region because refcount may still be >0
     * What other ways are there to hook into release of mmap when last
     * handle to memory is closed?
     * For now we dont inform host about unmaps and leave mappings alive
     */
    struct faulthook_priv_data *fh_info = vma->vm_file->private_data;
    struct mmap_info *mmap_info = (struct mmap_info *) vma->vm_private_data;
    struct action_unmap *escape = (struct action_unmap *) &fd_data->data;
    unsigned long i;
    unsigned long pfn_start;
    int ret;
    pr_info("vm_close\n");

    memset(escape, 0, sizeof(struct action_unmap));
    escape->common.fd = fh_info->fd;
    escape->vm_start = vma->vm_start;
    escape->vm_end = vma->vm_end;
    escape->pfn_size = mmap_info->data_size >> PAGE_SHIFT;
    pfn_start = page_to_pfn(virt_to_page((char *) mmap_info->data));
    for (i = 0; i < escape->pfn_size; i ++)
    {
        escape->pfn[i] = pfn_start + i * PAGE_SIZE;
    }
    ret = fh_do_faulthook(FH_ACTION_UNMAP);
    if (ret < 0)
    {
        pr_info("host not online");
        goto clean_up;
    }
    if (escape->common.ret < 0)
    {
        pr_info("escape failed: %ld\n", escape->common.ret);
        goto clean_up;
    }
    pr_info("fh_do_faulthook(FH_ACTION_UNMAP); is ok\n");

    clean_up:
    if (mmap_info->data != NULL)
    {
        free_pages((unsigned long) mmap_info->data, mmap_info->order);
        mmap_info->data = NULL;
    }
    if (mmap_info != NULL)
    {
        kfree(mmap_info);
        mmap_info = NULL;
    }
}

static vm_fault_t vm_fault(struct vm_fault *vmf)
{
    pr_info("vm_fault\n");
    return 0;
}

static void vm_open(struct vm_area_struct *vma)
{
    pr_info("vm_open\n");
}

static struct vm_operations_struct mmap_ops = {
        .close = vm_close,
        .fault = vm_fault,
        .open = vm_open,
};

int fh_bridge_mmap(struct file *file, struct vm_area_struct *vma)
{
    int ret = 0;
    unsigned long pfn, pfn_start, i;
    ssize_t size, order = 0;
    struct faulthook_priv_data *fh_info = file->private_data;
    struct mmap_info *mmap_info = NULL;
    struct action_mmap_device *escape = (struct action_mmap_device *) &fd_data->data;

    pr_info("end: %lx, start: %lx, pgoff: %lx\n", vma->vm_end, vma->vm_start, vma->vm_pgoff);

    mmap_info = kmalloc(GFP_KERNEL, sizeof(struct mmap_info));
    if (mmap_info == NULL)
    {
        return - ENOMEM;
    }
    memset(mmap_info, 0, sizeof(struct mmap_info));

    size = vma->vm_end - vma->vm_start;
    if (size <= 0)
    {
        ret = - EINVAL;
        pr_info("Invalid size: %ld\n", size);
        goto err_cleanup;
    }

    mmap_info->order = __roundup_pow_of_two(size >> PAGE_SHIFT);
    mmap_info->data = (void *) __get_free_pages(GFP_KERNEL, order);
    mmap_info->data_size = size;
    pr_info("Allocating %lx (order %ld)\n", mmap_info->data_size, mmap_info->order);

    if (mmap_info->data == NULL)
    {
        ret = - ENOMEM;
        goto err_cleanup;
    }
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    vma->vm_flags |= (VM_IO | VM_DONTEXPAND | VM_DONTDUMP);

    /* escape */
    memset(escape, 0, sizeof(struct action_mmap_device));
    escape->common.fd = fh_info->fd;
    escape->vm_start = vma->vm_start;
    escape->vm_end = vma->vm_end;
    escape->vm_pgoff = vma->vm_pgoff;
    escape->vm_flags = vma->vm_flags;
    escape->vm_page_prot = (unsigned long) vma->vm_page_prot.pgprot;
    escape->pfn_size = size >> PAGE_SHIFT;

    pr_info("number of entries (escape->pfn_size): %ld\n", escape->pfn_size);

    pfn_start = page_to_pfn(virt_to_page((char *) mmap_info->data));
    for (i = 0; i < escape->pfn_size; i ++)
    {
        pfn = pfn_start + i * PAGE_SIZE;
        pr_info("pfn: %lx\n", pfn);
        escape->pfn[i] = pfn;
    }
    ret = fh_do_faulthook(FH_ACTION_MMAP);
    if (ret < 0)
    {
        pr_info("host not online");
        goto err_cleanup;
    }
    if (escape->common.ret < 0)
    {
        pr_info("escape failed");
        ret = escape->common.err_no;
        goto err_cleanup;
    }
    pr_info("fh_do_faulthook(FH_ACTION_MMAP) is ok\n");

    ret = remap_pfn_range(vma, vma->vm_start, pfn_start, size, vma->vm_page_prot);
    if (ret)
    {
        pr_info("remap_pfn_range failed");
        goto err_cleanup;
    }
    pr_info("map kernel 0x%px to user 0x%lx, size: 0x%lx\n",
            mmap_info->data, vma->vm_start, size);

    vma->vm_private_data = mmap_info;

    /*
     * We dont unmap for now
     */
    // vma->vm_ops = &mmap_ops;
    // vm_open(vma);

    ret = 0;
    return ret;

    err_cleanup:
    if (mmap_info->data != NULL)
    {
        free_pages((unsigned long) mmap_info->data, mmap_info->order);
        mmap_info->data = NULL;
    }
    if (mmap_info != NULL)
    {
        kfree(mmap_info);
        mmap_info = NULL;
    }
    return ret;
}