#define pr_fmt(fmt)	KBUILD_MODNAME ":%s: " fmt, __func__
#include "xdma_cdev.h"

static int copy_desc_data(struct xdma_transfer *transfer, char __user *buf,
		size_t *buf_offset, size_t buf_size)
{
    HERE;
	return 0;
}

static ssize_t char_bypass_read(struct file *file, char __user *buf,
		size_t count, loff_t *pos)
{
HERE;
		return 0;
}

static ssize_t char_bypass_write(struct file *file, const char __user *buf,
		size_t count, loff_t *pos)
{
HERE;
	return 0;
}


/*
 * character device file operations for bypass operation
 */

static const struct file_operations bypass_fops = {
	.owner = THIS_MODULE,
	.open = char_open,
	.release = char_close,
	.read = char_bypass_read,
	.write = char_bypass_write,
	.mmap = bridge_mmap,
};

void cdev_bypass_init(struct xdma_cdev *xcdev)
{
	cdev_init(&xcdev->cdev, &bypass_fops);
}
