#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/types.h>
#include <linux/aio.h>
#include <linux/wait.h>
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

static loff_t char_sgdma_llseek(struct file *file, loff_t off, int whence)
{
    NOT_SUPPORTED;
    return 0;
}


static ssize_t char_sgdma_write(struct file *file, const char __user *buf,
                                size_t count, loff_t *pos)
{
    NOT_SUPPORTED;
    return -EINVAL;
}

static ssize_t char_sgdma_read(struct file *file, char __user *buf,
                               size_t count, loff_t *pos)
{
    NOT_SUPPORTED;
    return -EINVAL;
}

static ssize_t cdev_aio_write(struct kiocb *iocb, const struct iovec *io,
                              unsigned long count, loff_t pos)
{
    NOT_SUPPORTED;
    return -EIOCBQUEUED;
}

static ssize_t cdev_aio_read(struct kiocb *iocb, const struct iovec *io,
                             unsigned long count, loff_t pos)
{
    NOT_SUPPORTED;
    return -EIOCBQUEUED;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)

static ssize_t cdev_write_iter(struct kiocb *iocb, struct iov_iter *io)
{
    NOT_SUPPORTED;
    return -EINVAL;
}

static ssize_t cdev_read_iter(struct kiocb *iocb, struct iov_iter *io)
{
    NOT_SUPPORTED;
    return -EINVAL;
}

#endif

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
    return -EINVAL;
}


static long char_sgdma_ioctl(struct file *file, unsigned int cmd,
                             unsigned long arg)
{
    int rv = 0;

    switch (cmd) {
        case IOCTL_XDMA_APERTURE_R:rv = ioctl_do_aperture_dma(file, NULL, arg, 0);
            break;
        case IOCTL_XDMA_APERTURE_W:rv = ioctl_do_aperture_dma(file, NULL, arg, 1);
            break;
        default:dbg_perf("Unsupported operation\n");
            rv = -EINVAL;
            break;
    }
    return rv;
}

static int char_sgdma_open(struct inode *inode, struct file *file)
{
    NOT_SUPPORTED;
    return -EINVAL;
}

static int char_sgdma_close(struct inode *inode, struct file *file)
{
    NOT_SUPPORTED;
    return -EINVAL;
}

static const struct file_operations sgdma_fops = {
        .owner = THIS_MODULE,
        .open = fh_char_open,
        .release = fh_char_close,
        .write = char_sgdma_write,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
        .write_iter = cdev_write_iter,
#else
        .aio_write = cdev_aio_write,
#endif
        .read = char_sgdma_read,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
        .read_iter = cdev_read_iter,
#else
        .aio_read = cdev_aio_read,
#endif
        .unlocked_ioctl = char_sgdma_ioctl,
        .llseek = char_sgdma_llseek,
};

void cdev_sgdma_init(struct xdma_cdev *xcdev)
{
    cdev_init(&xcdev->cdev, &sgdma_fops);
}
