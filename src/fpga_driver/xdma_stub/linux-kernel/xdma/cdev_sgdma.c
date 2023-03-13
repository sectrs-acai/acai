#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/types.h>
#include <linux/aio.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)

#include <linux/uio.h>

#endif

#include <linux/pci.h>
#include <linux/dma-map-ops.h>

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

// XXX: Caller has to hold escape lock
static int transfer_chunk_to_host(
        struct file *file,
        char *data,
        unsigned long size,
        int info_ctx)
{
    int ret;
    unsigned long transfer_size = 0;
    unsigned long size_remain = 0;
    long transfer = 0;
    struct action_transfer_escape_data *escape =
            (struct action_transfer_escape_data *) &fd_data->data;
    if (size == 0)
    {
        return 0;
    }
    escape->status = action_transfer_escape_data_status_transfer_size;
    escape->handshake.total_size = size;
    escape->handshake.chunk_size = 0;
    escape->handshake.info_ctx = info_ctx;
    ret = fh_do_faulthook(FH_ACTION_TRANSFER_ESCAPE_DATA);
    if (ret != 0)
    {
        pr_info("fh_do_faulthook failed\n");
        return ret;
    }
    transfer_size = escape->handshake.chunk_size;
    pr_info("using transfer size : %lx\n", transfer_size);

    size_remain = size;
    ret = 0;
    do
    {
        transfer = min(transfer_size, size_remain);
        memcpy(escape->chunk.chunk_data, data + (size - size_remain), transfer);
        escape->chunk.chunk_data_size = transfer;
        size_remain -= transfer;
        escape->status = action_transfer_escape_data_status_transfer_data;
        pr_info("transfering %ld bytes, remain: %ld\n", transfer, size_remain);

        ret = fh_do_faulthook(FH_ACTION_TRANSFER_ESCAPE_DATA);
        if (ret != 0)
        {
            pr_info("fh_do_faulthook failed\n");
            break;
        }
    } while (escape->status == action_transfer_escape_data_status_ok && size_remain > 0);
    if (escape->status != action_transfer_escape_data_status_ok)
    {
        pr_info("transfer failed with status: %d\n", escape->status);
        ret = - 1;
    }
    return ret;
}

/*
 * XXX: This simualtes dma communction because we dont have a device yet
 * in realm
 */
static int simulate_pci_dma(struct pin_pages_struct *pinned)
{
    unsigned long i = 0;
    unsigned long num, offset, nbytes;
    struct sg_table *sgt;
    struct scatterlist *sg;
    struct page *page;

    pr_info("simulating pci dma access...\n");
    sgt = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
    if (sgt == NULL)
    {
        pr_err("sgl OOM.\n");
        return - ENOMEM;
    }
    if (sg_alloc_table(sgt, pinned->pages_nr, GFP_KERNEL))
    {
        pr_err("sgl OOM.\n");
        return - ENOMEM;
    }
    sg = sgt->sgl;
    for (i = 0; i < pinned->pages_nr; i ++, sg = sg_next(sg))
    {
        page = pfn_to_page(pinned->page_chunks[i].addr);
        offset = pinned->page_chunks[i].offset;
        nbytes = pinned->page_chunks[i].nbytes;
        #if 0
        pr_info("sg_set_page(pages[%d], %x, %x\n", i, nbytes, offset);
        #endif
        sg_set_page(sg, page, nbytes, offset);
    }

    /*
     * XXX: We dont have a device, so the code below
     * initializes dummy structes such that pci_map_sg returns with an error
     * without causing segfaults. This simulates pci_map_sg
     * and allows us to notify tfa about which regions to assign as devmem.
     */
    {
        struct dma_map_ops dma_ops;
        struct pci_dev *dummy = kmalloc(sizeof(struct pci_dev), GFP_KERNEL);
        if (dummy == NULL)
        {
            return - ENOMEM;
        }
        memset(dummy, 0, sizeof(struct pci_dev));
        memset(&dma_ops, 0, sizeof(struct dma_map_ops));
        dummy->dev.dma_ops = &dma_ops;
        /* pci_map_sg calls these functions: make sure they dont crash:
         *
         * const struct dma_map_ops *ops = get_dma_ops(dev);
         * if (WARN_ON_ONCE(!dev->dma_mask))
         *   return 0; // return here
         */
        dma_map_sg(&dummy->dev, sgt->sgl, sgt->orig_nents,
                   0 /* bidirectional */);

        kfree(dummy);
    }
    pinned->priv_data = (unsigned long) sgt;
    return 0;
}

static int simulate_pci_dma_cleanup(struct pin_pages_struct *pinned)
{

    struct sg_table *sgt;
    if (pinned->priv_data == 0)
    {
        return 0;
    }
    sgt = (struct sg_table* )  pinned->priv_data;

    /* TODO(bean): Clean up logic, for now we dont unmap */
    #if 0
    {
        struct dma_map_ops dma_ops;
        struct pci_dev *dummy = kmalloc(sizeof(struct pci_dev), GFP_KERNEL);
        if (dummy == NULL)
        {
            return - ENOMEM;
        }
        memset(dummy, 0, sizeof(struct pci_dev));
        memset(&dma_ops, 0, sizeof(struct dma_map_ops));
        dummy->dev.dma_ops = &dma_ops;

        dma_unmap_sg(&dummy->dev, sgt->sgl, sgt->orig_nents,
                   0 /* bidirectional */);

        kfree(dummy);
    }
    #endif

    sg_free_table(sgt);
    kfree(sgt);
    pinned->priv_data = 0;

    return 0;
}

static ssize_t char_sgdma_read_write(struct file *file,
                                     const char __user *buf,
                                     size_t count,
                                     loff_t *phy_addr,
                                     bool do_write,
                                     bool do_aperture)
{
    int ret, i;
    struct pin_pages_struct *pinned = NULL;
    unsigned long page_chunk_size;

    ret = fh_pin_pages(buf, count, &pinned);
    if (ret != 0)
    {
        pr_info("pin_pages failed: %d\n", ret);
        return ret;
    }
    ret = simulate_pci_dma(pinned);
    if (ret < 0)
    {
        pr_info("simulate_pci_dma failed with: %d. continue\n", ret);
    }

    page_chunk_size = pinned->pages_nr * sizeof(struct page_chunk);
    if (sizeof(struct action_dma) + page_chunk_size > fvp_escape_size)
    {
        pr_info("escape page too small\n");
        return - ENOMEM;
    }

    fd_data_lock();
    ret = transfer_chunk_to_host(file,
                                 (void *) pinned->page_chunks,
                                 page_chunk_size,
                                 FH_ACTION_DMA);
    if (ret < 0)
    {
        pr_info("transfer_chunk_to_host failed\n");
        goto clean_up;
    }

    for (i = 0; i < pinned->pages_nr; i ++)
    {
        #if 0
        pr_info("pinned page: %lx, %lx, %lx\n",
                pinned->page_chunks[i].addr,
                pinned->page_chunks[i].offset,
                pinned->page_chunks[i].nbytes);
        #endif
    }


    struct faulthook_priv_data *fh_info = file->private_data;
    struct action_dma *escape = (struct action_dma *) &fd_data->data;
    escape->do_write = do_write;
    escape->common.fd = fh_info->fd;
    escape->phy_addr = *phy_addr;
    escape->len = pinned->len;
    escape->do_aperture = do_aperture;
    escape->pages_nr = pinned->pages_nr;
    escape->user_buf = (char *) buf;

    pr_info("ret = fh_do_faulthook(FH_ACTION_DMA): size: %lx\n", page_chunk_size);
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

    #if 0
    pr_info("escape->common.ret: %lx\n", ret);
    #endif

    clean_up:
    fd_data_unlock();
    simulate_pci_dma_cleanup(pinned);
    fh_unpin_pages(pinned, 1, 1);
    return ret;
}

static int ioctl_do_aperture_dma(struct file *file, unsigned long arg,
                                 bool write)
{
    struct xdma_aperture_ioctl io;
    struct xdma_io_cb cb;
    ssize_t rv;

    rv = copy_from_user(&io,
                        (struct xdma_aperture_ioctl __user *) arg,
                        sizeof(struct xdma_aperture_ioctl));
    if (rv < 0)
    {
        pr_info("failed to copy from userspace\n");
        return - EINVAL;
    }

    memset(&cb, 0, sizeof(struct xdma_io_cb));
    cb.buf = (char __user *) io.buffer;
    cb.len = io.len;
    cb.ep_addr = io.ep_addr;
    cb.write = write;
    rv = char_sgdma_read_write(file,
                               cb.buf,
                               cb.count,
                               &cb.ep_addr,
                               cb.write,
                               1);
    if (rv < 0)
    {
        pr_info("char_sgdma_read_write failed with error: %d\n", rv);
    }
    io.error = rv;
    if (rv > 0)
    {
        io.done = rv;
    }
    rv = copy_to_user((struct xdma_aperture_ioctl __user *) arg,
                      &io,
                      sizeof(struct xdma_aperture_ioctl));
    if (rv < 0)
    {
        pr_info("copy_to_user failed\n");
        return - EINVAL;
    }
    return io.error;
}


static ssize_t char_sgdma_write(struct file *file, const char __user *buf,
                                size_t count, loff_t *pos)
{
    return char_sgdma_read_write(file, buf, count, pos, 1, 0);
}

static ssize_t char_sgdma_read(struct file *file, char __user *buf,
                               size_t count, loff_t *pos)
{
    return char_sgdma_read_write(file, buf, count, pos, 0, 0);
}

static int char_sgdma_open(struct inode *inode, struct file *file)
{
    return fh_char_open(inode, file);
}

static int char_sgdma_close(struct inode *inode, struct file *file)
{
    return fh_char_close(inode, file);
}

static loff_t char_sgdma_llseek(struct file *file, loff_t off, int whence)
{
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
    {
        return - EINVAL;
    }
    file->f_pos = newpos;
            dbg_fops("%s: pos=%lld\n", __func__, (signed long long) newpos);
    return newpos;
}

static long char_sgdma_ioctl(struct file *file, unsigned int cmd,
                             unsigned long arg)
{
    int rv = 0;
    switch (cmd)
    {
        case IOCTL_XDMA_APERTURE_R:
        {
            rv = ioctl_do_aperture_dma(file, arg, 0);
            break;
        }
        case IOCTL_XDMA_APERTURE_W:
        {
            rv = ioctl_do_aperture_dma(file, arg, 1);
            break;
        }
        default:
        {
            pr_info("unknown ioctl: %d\n", cmd);
            rv = - EINVAL;
            break;
        }
    }
    return rv;
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
