#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/types.h>
#include <linux/aio.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)

#include <linux/uio.h>

#endif

#include "xdma_cdev.h"
#include "cdev_sgdma.h"
#include "xdma_thread.h"

/* Module Parameters */
unsigned int h2c_timeout = 10;

module_param(h2c_timeout, uint, 0644);
MODULE_PARM_DESC(h2c_timeout, "H2C sgdma timeout in seconds, default is 10 sec.");

unsigned int c2h_timeout = 10;

module_param(c2h_timeout, uint, 0644);
MODULE_PARM_DESC(c2h_timeout, "C2H sgdma timeout in seconds, default is 10 sec.");

static ssize_t cdev_aio_write(struct kiocb *iocb, const struct iovec *io,
                              unsigned long count, loff_t pos)
{
    NOT_SUPPORTED;
    return - EIOCBQUEUED;
}

static ssize_t cdev_aio_read(struct kiocb *iocb, const struct iovec *io,
                             unsigned long count, loff_t pos)
{
    NOT_SUPPORTED;
    return - EIOCBQUEUED;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)

static ssize_t cdev_write_iter(struct kiocb *iocb, struct iov_iter *io)
{
    NOT_SUPPORTED;
    return - EINVAL;
}

static ssize_t cdev_read_iter(struct kiocb *iocb, struct iov_iter *io)
{
    NOT_SUPPORTED;
    return - EINVAL;
}

#endif

#if 0
static int ioctl_do_aperture_dma(struct file *file, struct xdma_engine *engine, unsigned long arg,
                                 bool write)
{
    struct xdma_aperture_ioctl io;
    struct xdma_io_cb cb;
    ssize_t res;
    int rv;
#if 0

    rv = copy_from_user(&io, (struct xdma_aperture_ioctl __user *) arg,
                        sizeof(struct xdma_aperture_ioctl));
    if (rv < 0) {
                dbg_tfr("%s failed to copy from user space 0x%lx\n",
                        "", arg);
        return -EINVAL;
    }

            dbg_tfr("%s, W %d, buf 0x%lx,%lu, ep %llu, aperture %u.\n",
                    "", write, io.buffer, io.len, io.ep_addr,
                    io.aperture);

    fd_data->action = FH_ACTION_IOCTL_DMA;
    struct action_dma *a = (struct action_dma *) &fd_data->data;
    struct faulthook_priv_data *info = file->private_data;
    a->common.fd = info->fd;
    a->io.ep_addr = io.ep_addr;
    a->io.len = io.len;
    a->io.aperture = io.aperture;
    a->io.ep_addr = io.ep_addr;
    a->write_read = write;

    pr_info("copying bytes of len: %ld\n", io.len);
    if (write) {
        rv = copy_from_user(&a->io.buffer, (void __user *) io.buffer,
                            io.len);

        if (rv < 0) {
                    dbg_tfr("%s failed to copy from user space 0x%lx\n",
                            "", arg);
            return -EINVAL;
        }
    }

    fh_do_faulthook();


    rv = copy_to_user((struct xdma_aperture_ioctl __user *) arg, &a->io,
                      sizeof(struct xdma_aperture_ioctl));
    if (rv < 0) {
                dbg_tfr("%s failed to copy to user space 0x%lx, %ld\n",
                        "", arg, res);
        return -EINVAL;
    }

    return io.error;
#endif
    return - EINVAL;
}
#endif

static long char_sgdma_ioctl(struct file *file, unsigned int cmd,
                             unsigned long arg)
{
    int rv = 0;
    NOT_SUPPORTED;
    rv = - EINVAL;
    return rv;
}

static ssize_t char_sgdma_read_write(struct file *file,
                                     const char __user *buf,
                                     size_t count,
                                     loff_t *pos,
                                     bool do_write)
{
    int ret, i;
    struct pin_pages_struct *pinned = NULL;
    unsigned long page_chunk_size;
    if (count > PAGE_SIZE)
    {
        pr_info("We dont support larger sizes for now\n");
        return - EINVAL;
    }
    ret = fh_pin_pages(buf, count, &pinned);
    if (ret != 0)
    {
        pr_info("pin_pages failed: %d\n", ret);
        return ret;
    }
    page_chunk_size = pinned->pages_nr * sizeof(struct page_chunk);
    struct faulthook_priv_data *fh_info = file->private_data;
    struct action_dma *escape = (struct action_dma *) &fd_data->data;
    escape->do_write = do_write;
    escape->common.fd = fh_info->fd;
    escape->phy_addr = *pos;
    escape->len = pinned->len;
    escape->pages_nr = pinned->pages_nr;
    escape->user_buf = (char *) buf;
    memcpy(escape->page_chunks, pinned->page_chunks, page_chunk_size);

    for(i = 0; i < escape->pages_nr; i ++) {
        pr_info("pinned page: %lx, %lx, %lx\n",
                escape->page_chunks[i].addr,
                escape->page_chunks[i].offset,
                escape->page_chunks[i].nbytes);
    }

     ret = fh_do_faulthook(FH_ACTION_DMA);
    if (ret < 0)
    {
        pr_info("fh_do_faulthook(FH_ACTION_DMA) failed\n");
        goto clean_up;
    }
    if (escape->common.ret < 0)
    {
        ret = escape->common.err_no;
        goto clean_up;
    }
    ret = escape->common.ret;
    pr_info("escape->common.ret: %lx\n", ret);

    clean_up:
    fh_unpin_pages(pinned, 1, 1);
    return ret;
}

static ssize_t char_sgdma_write(struct file *file, const char __user *buf,
                                size_t count, loff_t *pos)
{
    return char_sgdma_read_write(file, buf, count, pos, 1);
}

static ssize_t char_sgdma_read(struct file *file, char __user *buf,
                               size_t count, loff_t *pos)
{
    return char_sgdma_read_write(file, buf, count, pos, 0);
}

static int char_sgdma_open(struct inode *inode, struct file *file)
{
    return fh_char_open(inode, file);
}

static int char_sgdma_close(struct inode *inode, struct file *file)
{
    return fh_char_close(inode, file);
}

/*
 * character device file operations for SG DMA engine
 */
static loff_t char_sgdma_llseek(struct file *file, loff_t off, int whence)
{
    /* TODO: Do we need to escape for seek? */
    #if 0
    int ret;
    struct faulthook_priv_data *fh_info = file->private_data;
    struct action_seek *escape = (struct action_seek *) &fd_data->data;
    escape->common.fd = fh_info->fd;
    escape->off = off;
    escape->whence = whence;
    ret = fh_do_faulthook(FH_ACTION_SEEK);
    if (ret < 0)
    {
        pr_info("fh_do_faulthook(FH_ACTION_DMA) failed\n");
        return ret;
    }
    if (escape->common.ret < 0)
    {
        ret = escape->common.ret;
        return ret;
    }
    #endif
    HERE;
    loff_t newpos = 0;
    switch (whence)
    {
        case 0: /* SEEK_SET */
            newpos = off;
            break;
        case 1: /* SEEK_CUR */
            newpos = file->f_pos + off;
            break;
        case 2: /* SEEK_END, @TODO should work from end of address space */
            newpos = UINT_MAX + off;
            break;
        default: /* can't happen */
            return - EINVAL;
    }
    if (newpos < 0)
        return - EINVAL;
    file->f_pos = newpos;
            dbg_fops("%s: pos=%lld\n", __func__, (signed long long) newpos);

    return newpos;
}

static const struct file_operations sgdma_fops = {
        .owner = THIS_MODULE,
        .open = char_sgdma_open,
        .release = char_sgdma_close,
        .write = char_sgdma_write,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
        .write_iter = cdev_write_iter,
#else
        // .aio_write = cdev_aio_write,
#endif
        .read = char_sgdma_read,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
        .read_iter = cdev_read_iter,
#else
        // .aio_read = cdev_aio_read,
#endif
        .unlocked_ioctl = char_sgdma_ioctl,
        .llseek = char_sgdma_llseek,
};

void cdev_sgdma_init(struct xdma_cdev *xcdev)
{
    cdev_init(&xcdev->cdev, &sgdma_fops);
}
