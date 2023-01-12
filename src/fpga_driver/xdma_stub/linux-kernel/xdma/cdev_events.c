#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include "xdma_cdev.h"

static ssize_t char_events_read(struct file *file, char __user *buf,
		size_t count, loff_t *pos)
{
    HERE;
	return 4;
}

static unsigned int char_events_poll(struct file *file, poll_table *wait)
{
    HERE;
	return 0;
}

/*
 * character device file operations for the irq events
 */
static const struct file_operations events_fops = {
	.owner = THIS_MODULE,
	.open = char_open,
	.release = char_close,
	.read = char_events_read,
	.poll = char_events_poll,
};

void cdev_event_init(struct xdma_cdev *xcdev)
{
	cdev_init(&xcdev->cdev, &events_fops);
}
