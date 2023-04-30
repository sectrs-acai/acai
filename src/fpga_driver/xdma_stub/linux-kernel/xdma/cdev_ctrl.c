#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include "xdma_cdev.h"

int char_open(struct inode *inode, struct file *file) {
    NOT_SUPPORTED;
    CCA_MARKER_DRIVER_FOP;
    return -EINVAL;
}
int char_close(struct inode *inode, struct file *file) {
    NOT_SUPPORTED;
    CCA_MARKER_DRIVER_FOP;
    return -EINVAL;
}

int bridge_mmap(struct file *file, struct vm_area_struct *vma) {
    NOT_SUPPORTED;
    CCA_MARKER_DRIVER_FOP;
    return -EINVAL;
}

static const struct file_operations ctrl_fops = {
        .owner = THIS_MODULE,
        .open = fh_char_open,
        .release = fh_char_close,
        .read = fh_char_ctrl_read,
        .write = fh_char_ctrl_write,
        .mmap = fh_bridge_mmap,
        .unlocked_ioctl = fh_char_ctrl_ioctl,
};

void cdev_ctrl_init(struct xdma_cdev *xcdev)
{
    cdev_init(&xcdev->cdev, &ctrl_fops);
}
