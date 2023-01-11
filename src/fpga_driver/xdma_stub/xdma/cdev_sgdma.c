#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__
#include <linux/types.h>
#include <asm/cacheflush.h>
#include <linux/slab.h>
#include <linux/aio.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
#include <linux/uio.h>
#endif
// #include "libxdma_api.h"
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

extern struct kmem_cache *cdev_cache;

static loff_t char_sgdma_llseek(struct file *file, loff_t off, int whence)
{
    HERE;
	loff_t newpos = 0;
	return newpos;
}

static ssize_t char_sgdma_read_write(struct file *file, const char __user *buf,
		size_t count, loff_t *pos, bool write)
{
    HERE;
	return 0;
}

static ssize_t char_sgdma_write(struct file *file, const char __user *buf,
		size_t count, loff_t *pos)
{
    HERE;
	return char_sgdma_read_write(file, buf, count, pos, 1);
}

static ssize_t char_sgdma_read(struct file *file, char __user *buf,
				size_t count, loff_t *pos)
{
    HERE;
	return char_sgdma_read_write(file, buf, count, pos, 0);
}

static ssize_t cdev_aio_write(struct kiocb *iocb, const struct iovec *io,
				unsigned long count, loff_t pos)
{
	return -EIOCBQUEUED;
}

static ssize_t cdev_aio_read(struct kiocb *iocb, const struct iovec *io,
				unsigned long count, loff_t pos)
{
	return -EIOCBQUEUED;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
static ssize_t cdev_write_iter(struct kiocb *iocb, struct iov_iter *io)
{
    HERE;
	return 0;
}

static ssize_t cdev_read_iter(struct kiocb *iocb, struct iov_iter *io)
{
    HERE;
	return 0;
}
#endif

static long char_sgdma_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
    HERE;
	return 0;
}

static int char_sgdma_open(struct inode *inode, struct file *file)
{
    HERE;
	return 0;
}

static int char_sgdma_close(struct inode *inode, struct file *file)
{
    HERE;
	return 0;
}
static const struct file_operations sgdma_fops = {
	.owner = THIS_MODULE,
	.open = char_sgdma_open,
	.release = char_sgdma_close,
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
